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

	bool interrupts_enabled() const { return interrupts != 0; }
	bool is_zero() const { return zero == 0; }
	bool is_negative() const { return negative & 1; }
	bool is_carry() const { return carry & 2; }
};

struct CpuInstruction
{
	uint32_t canonical_address;
	uint32_t length;
	uint8_t bytes[8];
	std::string asm_string;
};

class Disassembler
{
public:
	struct Config
	{
		uint32_t max_instruction_count = 32;
		bool include_addr = true;
		bool include_bytes = true;
	};
	virtual ~Disassembler() {}

	std::vector<CpuInstruction> Disassemble(const Config& config, uint32_t& canonical_address);
	std::vector<CpuInstruction> Disassemble(const Config& config, const CpuState *state);

protected:
	virtual bool DisassembleOneInstruction(const Config& config, uint32_t& canonical_address, CpuInstruction& insn) = 0;
};

class Assembler
{
public:
	virtual ~Assembler() {}
	virtual bool Assemble(const char*& start, std::string& error, std::vector<uint8_t>& bytes) = 0;
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
	bool open_bus_is_data = true;
	uint8_t open_bus = 0;

	void Map(cpuaddr_t addr, uint8_t *ptr, uint32_t len, bool readonly = false);

	uint32_t ReadByte(cpuaddr_t addr, uint8_t *data);
	uint32_t ReadByteNoIo(cpuaddr_t addr, uint8_t *data);
	uint32_t WriteByte(cpuaddr_t addr, uint8_t v);
	uint32_t WriteByteNoIo(cpuaddr_t addr, uint8_t v);
	bool QueryIo(cpuaddr_t addr);
	void Init(uint32_t size_shift, uint32_t addr_bus_bits, Page *pages);

	uint8_t ReadByte(cpuaddr_t addr)
	{
		uint8_t v;
		ReadByte(addr, &v);
		return v;
	}
	uint16_t PeekU16LE(cpuaddr_t addr)
	{
		uint16_t lo = ReadByte(addr);
		uint16_t hi = ReadByte(addr + 1);
		return lo | (hi << 8);
	}
	uint32_t PeekU32LE(cpuaddr_t addr)
	{
		uint32_t b0 = ReadByte(addr);
		uint32_t b1 = ReadByte(addr + 1);
		uint32_t b2 = ReadByte(addr + 2);
		uint32_t b3 = ReadByte(addr + 3);
		return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
	}
	void PokeU16LE(cpuaddr_t addr, uint16_t value)
	{
		WriteByte(addr, value & 0xFF);
		WriteByte(addr + 1, value >> 8);
	}
	void PokeU32LE(cpuaddr_t addr, uint32_t value)
	{
		WriteByte(addr, value & 0xFF);
		WriteByte(addr + 1, value >> 8);
		WriteByte(addr + 2, value >> 16);
		WriteByte(addr + 3, value >> 24);
	}
};

// This is a shortcut for when a bus that maps to a single linear address space
// is desired.
template<uint32_t AddressBusBits>
class SimpleSystemBus : public SystemBus
{
public:
	static constexpr size_t kMemSize = 1U << AddressBusBits;
	SimpleSystemBus()
	{
		page.ptr = mem;
		page.flags = 0;
		page.io_mask = 0;
		page.io_eq = 1;
		page.cycles_per_access = 1;
		Init(AddressBusBits, AddressBusBits, &page);

		io_devices.context = this;
		io_devices.read = &IoRead;
		io_devices.write = &IoWrite;
	}

	void SetIo(std::function<uint8_t(uint32_t addr)> read,
		std::function<void(uint32_t addr, uint8_t val)> write, uint32_t mask, uint32_t eq)
	{
		read_fn = std::move(read);
		write_fn = std::move(write);
		page.io_mask = mask;
		page.io_eq = eq;
	}

private:
	static void IoRead(void *context, cpuaddr_t addr, uint8_t *data, uint32_t size)
	{
		*data = reinterpret_cast<SimpleSystemBus*>(context)->read_fn(addr);
	}
	static void IoWrite(void *context, cpuaddr_t addr, const uint8_t *data, uint32_t size)
	{
		reinterpret_cast<SimpleSystemBus*>(context)->write_fn(addr, *data);
	}

	std::function<uint8_t(uint32_t addr)> read_fn;
	std::function<void(uint32_t addr, uint8_t val)> write_fn;
	Page page;
	uint8_t mem[kMemSize];
};


struct CpuTrace
{
	// A simple circular buffer. Values accessed backwards from |write|
	// record the instruction pointer of executed instructions
	std::vector<cpuaddr_t> addrs;
	uint32_t write = 0;
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

	virtual bool SaveState(std::vector<uint8_t> *out_data) { return false; }
	virtual bool LoadState(const uint8_t **in_data, const uint8_t *end) { return false; }
	virtual Disassembler* GetDisassembler() { return nullptr; }
	virtual Assembler* GetAssembler() { return nullptr; }
	struct DebugReg
	{
		const char *name;
		uint64_t value;
		uint32_t size;
		uint32_t flags;
	};
	virtual bool GetDebugRegState(std::vector<DebugReg>& regs) { return false; }
	virtual bool SetRegister(const char *reg, uint64_t value) { return false; }
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
		has_breakpoints = true;
		return breakpoints.emplace(std::make_pair(addr, std::move(fn))).second;
	}
	void RemoveBreakpoint(cpuaddr_t addr)
	{
		breakpoints.erase(addr);
		has_breakpoints = !breakpoints.empty();
	}

	bool has_breakpoints = false;
	std::unordered_map<cpuaddr_t, std::function<void(EmulatedCpu*)>> breakpoints;
};


#endif
