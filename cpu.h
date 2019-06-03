#ifndef CPU_H_
#define CPU_H_

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <mutex>
#include <functional>
#include <algorithm>
#include <string>
#include <unordered_map>

#include "host_system.h"

void DoPanic(const char *file, int line, const char *function);
#define panic() DoPanic(__FILE__, __LINE__, PRETTY_FUNCTION);

typedef uint32_t cpuaddr_t;

class EmulatedCpu;

// State that is common to any CPU
struct CpuState
{
	CpuState() : pending_interrupts(0) {}

	uint64_t *registers;
	uint32_t reg_size_bytes;
	uint32_t reg_count;

	uint64_t cycle = 0;
	uint64_t cycle_stop = 0;
	uint64_t event_cycle;

	cpuaddr_t code_segment_base;
	cpuaddr_t ip_mask;
	// Current instruction pointer
	cpuaddr_t ip;

	// An opaque mode value
	uint32_t mode;

	cpuaddr_t *data_segments;

	// Flags
	uint32_t zero = 0; // Tested with == 0
	uint32_t negative = 0; // Tested with & 0x1
	// Bit 0 is interrupts allowed
	uint32_t interrupts = 0;
	// Bit 1 is IRQ pending, bit 2 is NMI pending, etc. Checking
	// for pending interrupts is done with interrupts + pending_interrupts >= 3
	std::atomic<uint32_t> pending_interrupts;
	uint32_t carry = 0; // Tested with & 0x2
	uint32_t other_flags = 0; // Everything else

	uint32_t GetCanonicalAddress() const { return code_segment_base + (ip & ip_mask); }

	// source should be > 0
	void SetInterruptSource(uint32_t source)
	{
		pending_interrupts.fetch_or(1U << source, std::memory_order_acq_rel);
	}
	void ClearInterruptSource(uint32_t source)
	{
		pending_interrupts.fetch_and(~(1U << source), std::memory_order_acq_rel);
	}

	bool is_zero() const { return zero == 0; }
	bool is_negative() const { return negative & 1; }
	bool is_carry() const { return carry & 2; }
};

struct CpuInstruction
{
	uint32_t canonical_address;
	uint32_t length;
	std::string asm_string;
};

class Disassembler
{
public:
	struct Config
	{
		uint32_t max_instruction_count = 32;
	};
	virtual ~Disassembler() {}

	std::vector<CpuInstruction> Disassemble(const Config& config, uint32_t& canonical_address);
	std::vector<CpuInstruction> Disassemble(const Config& config, const CpuState *state);

protected:
	virtual bool DisassembleOneInstruction(uint32_t& canonical_address, CpuInstruction& insn) = 0;
};

struct Page
{
	static constexpr uint32_t kReadOnly = 1;

	uint8_t *ptr;
	uint32_t flags;
	cpuaddr_t io_mask;
	cpuaddr_t io_eq;
	uint32_t cycles_per_access;
};

struct MemoryMap
{
	uint32_t page_size;
	uint32_t page_shift;
	uint32_t page_mask;
	Page *pages;
};

struct SystemBus
{
	struct IODevices
	{
		bool (*is_io_device_address)(void *context, cpuaddr_t addr);
		void (*read)(void *context, cpuaddr_t addr, uint8_t *data, uint32_t size);
		void (*write)(void *context, cpuaddr_t addr, const uint8_t *data, uint32_t size);
		// The system should determine whether to deassert the IRQ lines
		void (*irq_taken)(void *context, uint32_t type);
		void *context;
	} io_devices;
	MemoryMap memory;
	EmulatedCpu *cpu;
	uint32_t mem_mask;

	void Map(cpuaddr_t addr, uint8_t *ptr, uint32_t len, bool readonly = false);

	uint32_t ReadByte(cpuaddr_t addr, uint8_t *data);
	uint32_t ReadByteNoIo(cpuaddr_t addr, uint8_t *data);
	uint32_t WriteByte(cpuaddr_t addr, uint8_t v);
	uint32_t WriteByteNoIo(cpuaddr_t addr, uint8_t v);
	void Init(uint32_t size_shift, uint32_t addr_bus_bits, Page *pages);
};

struct CpuTrace
{
	cpuaddr_t *addrs;
	uint32_t max, write;
};

class EventQueue
{
public:
	void Schedule(uint64_t t, std::function<void()> f);
	void ScheduleNoLock(uint64_t t, std::function<void()> f);

	void Expire(uint64_t t);

	uint64_t next() const
	{
		if(entries.empty())
			return ~0ULL;
		return entries.front().t;
	}

	void Start(uint64_t *event_cycle, uint64_t stop_cycle);

private:
	struct Entry
	{
		uint64_t t;
		std::function<void()> f;

		operator uint64_t() const { return t; }
	};
	std::mutex lock;
	std::vector<Entry> entries;
	uint64_t *cycle;
	uint64_t stop;
};

class EmulatedCpu
{
public:
	EmulatedCpu(SystemBus *sys)
	{
		sys->cpu = this;
	}
	virtual ~EmulatedCpu() {}

	virtual CpuState* GetCpuState() = 0;

	virtual uint32_t GetAddressBusBits() = 0;
	virtual uint32_t GetPCBits() = 0;

	virtual Disassembler* GetDisassembler() { return nullptr; }
	struct DebugReg
	{
		const char *name;
		uint64_t value;
		uint32_t size;
		uint32_t flags;
	};
	virtual bool GetDebugRegState(std::vector<DebugReg>& regs) { return false; }
	virtual CpuTrace* GetDebugTraceState() { return nullptr; }

	struct ExecInfo
	{
		// Emulate an instruction
		void (*emu)(void *context);
		void *emu_context;

		// Param will be 0 if called from a normal interrupt, 1 for NMI.
		void (*interrupt)(void *context, uint32_t param);
		void *interrupt_context;
	};
	virtual const ExecInfo* GetExecInfo() = 0;

	virtual void PowerOn() = 0;
	virtual void Reset() = 0;

	void Emulate(EventQueue *events = nullptr);

	void SingleStep(EventQueue *events = nullptr);

	template<typename U>
	void EmulateWithCycleProcessing(U& context, EventQueue *events = nullptr)
	{
		auto exec = GetExecInfo();
		auto state = GetCpuState();
		uint64_t event_cycle = state->cycle_stop;
		if(events)
			events->Start(&event_cycle, state->cycle_stop);
		do {
			while(state->cycle < event_cycle) {
				state->ip &= state->ip_mask;
				uint32_t pending_interrupts = state->pending_interrupts.load(std::memory_order_acquire);
				if(pending_interrupts + state->interrupts >= 3) {
					uint32_t type;
					if(pending_interrupts & 4) {
						type = 1;
					} else {
						type = 0;
					}
					exec->interrupt(exec->interrupt_context, type);
					continue;
				}
				context.PreCpuCycle();
				if(breakpoints.find(state->GetCanonicalAddress()) != breakpoints.end()) {
					breakpoints.find(state->GetCanonicalAddress())->second(this);
				}
				exec->emu(exec->emu_context);
				context.PostCpuCycle();
			}
			if(events)
				events->Expire(state->cycle);
		} while(state->cycle < state->cycle_stop);
	}

	bool AddBreakpoint(cpuaddr_t addr, std::function<void(EmulatedCpu*)> fn)
	{
		return breakpoints.emplace(std::make_pair(addr, std::move(fn))).second;
	}
	void RemoveBreakpoint(cpuaddr_t addr)
	{
		breakpoints.erase(addr);
	}

	std::unordered_map<cpuaddr_t, std::function<void(EmulatedCpu*)>> breakpoints;
};


#endif
