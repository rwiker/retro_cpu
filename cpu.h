#ifndef CPU_H_
#define CPU_H_

#include <stddef.h>
#include <stdint.h>

#include <atomic>

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

	// source should be > 0
	void SetInterruptSource(uint32_t source)
	{
		pending_interrupts.fetch_or(1U << source, std::memory_order_acq_rel);
	}

	bool is_zero() const { return zero == 0; }
	bool is_negative() const { return negative & 1; }
	bool is_carry() const { return carry & 2; }
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
		void *context;
	} io_devices;
	MemoryMap memory;
	EmulatedCpu *cpu;
	uint32_t mem_mask;

	void Map(cpuaddr_t addr, uint8_t *ptr, uint32_t len);

	uint32_t ReadByte(cpuaddr_t addr, uint8_t *data);
	uint32_t WriteByte(uint32_t addr, uint8_t v);
	void Init(uint32_t size_shift, uint32_t addr_bus_bits, Page *pages);
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

	void Emulate();

};


#endif
