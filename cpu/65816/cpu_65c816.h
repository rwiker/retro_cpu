#ifndef CPU_65C816_H_
#define CPU_65C816_H_

#include "cpu.h"
#include "jit.h"

// Execute for |cycles| and return number of milliseconds taken.
uint64_t WDC65C816PerfTest(uint64_t cycles);

class WDC65C816 : public EmulatedCpu, public JittableCpu, public Disassembler
{
public:
	WDC65C816(SystemBus *sys);

	static constexpr uint32_t kPageSizeBits = 12;

	CpuState* GetCpuState() override;
	void PowerOn() override;
	void Reset() override;
	uint32_t GetAddressBusBits() override { return 24; }
	uint32_t GetPCBits() override { return 16; }
	const ExecInfo* GetExecInfo() override;

	const JitOperation* GetJit(JitCore *core, cpuaddr_t addr) override;

	Disassembler* GetDisassembler() override { return this; }
	Assembler* GetAssembler() override;
	bool DisassembleOneInstruction(uint32_t& canonical_address, CpuInstruction& insn) override;
	bool GetDebugRegState(std::vector<DebugReg>& regs) override;
	bool SetRegister(const char *reg, uint64_t value) override;

	static void EmulateInstruction(void *context);
	static void Interrupt(void *context, uint32_t param);

	uint32_t GetMemoryCycleCount(cpuaddr_t addr);

	void SetNZ(uint8_t v);
	void SetNZ(uint16_t v);

	uint8_t GetStatusRegister();
	void SetStatusRegister(uint8_t v);

	void WAI();
	void STP();

	uint32_t program_address() const { return cpu_state.GetCanonicalAddress(); }
	uint16_t a() const { return cpu_state.regs.a.u16; }
	uint16_t x() const { return cpu_state.regs.x.u16; }
	uint16_t y() const { return cpu_state.regs.y.u16; }

	void InternalOp(uint32_t n)
	{
		cpu_state.cycle += internal_cycle_timing * n;
	}
	void InternalOp()
	{
		cpu_state.cycle += internal_cycle_timing;
	}

	enum InterruptType {
		BRK, COP, IRQ, NMI
	};
	void DoInterrupt(InterruptType type);

	void PeekU8(uint32_t addr, uint8_t& v)
	{
		sys->ReadByte(addr, &v);
	}
	uint8_t PeekU8(uint32_t addr)
	{
		uint8_t v;
		sys->ReadByte(addr, &v);
		return v;
	}
	uint16_t PeekU16(uint32_t addr)
	{
		uint8_t u8;
		sys->ReadByte(addr, &u8);
		uint16_t v = u8;
		sys->ReadByte(addr + 1, &u8);
		return v | (((uint16_t)u8) << 8);
	}
	void ReadU8(uint32_t addr, uint8_t& v)
	{
		cpu_state.cycle += sys->ReadByte(addr, &v);
	}
	void WriteU8(uint32_t addr, uint8_t v)
	{
		cpu_state.cycle += sys->WriteByte(addr, v);
	}
	void ReadU16NoCrossBank(uint32_t base, uint32_t addr, uint16_t& v)
	{
		uint8_t low, high;
		ReadU8(base | (addr & 0xFFFF), low);
		ReadU8(base | ((addr + 1) & 0xFFFF), high);
		v = low | ((uint16_t)high << 8);
	}

	void ReadPBR(uint32_t addr, uint8_t& v)
	{
		ReadU8(cpu_state.code_segment_base | (addr & 0xFFFF), v);
	}
	void ReadPBR(uint32_t addr, uint16_t& v)
	{
		ReadRaw(cpu_state.code_segment_base | (addr & 0xFFFF), v);
	}
	void ReadDBR(uint32_t addr, uint8_t& v)
	{
		ReadU8((addr & 0xFFFF) | cpu_state.data_segment_base, v);
	}
	void ReadDBR(uint32_t addr, uint16_t& v)
	{
		ReadU16NoCrossBank(cpu_state.data_segment_base, addr, v);
	}
	void WriteDBR(uint32_t addr, uint8_t v)
	{
		WriteU8((addr & 0xFFFF) | cpu_state.data_segment_base, v);
	}
	void WriteDBR(uint32_t addr, uint16_t v)
	{
		WriteDBR((addr & 0xFFFF) | cpu_state.data_segment_base, (uint8_t)v);
		WriteDBR(((addr + 1) & 0xFFFF) | cpu_state.data_segment_base, (uint8_t)(v>>8));
	}
	void ReadZero(uint32_t addr, uint8_t& v)
	{
		ReadU8(addr & 0xFFFF, v);
	}
	void ReadZero(uint32_t addr, uint16_t& v)
	{
		ReadU16NoCrossBank(0, addr, v);
	}
	void WriteZero(uint32_t addr, uint8_t v)
	{
		WriteU8(addr & 0xFFFF, v);
	}
	void WriteZero(uint32_t addr, uint16_t v)
	{
		WriteZero(addr, (uint8_t)v);
		WriteZero(addr + 1, (uint8_t)(v>>8));
	}
	void ReadRaw(uint32_t addr, uint8_t& v)
	{
		ReadU8(addr, v);
	}
	void ReadRaw(uint32_t addr, uint16_t& v)
	{
		uint8_t low, high;
		ReadU8(addr, low);
		ReadU8(addr + 1, high);
		v = low | ((uint16_t)high << 8);
	}
	void WriteRaw(uint32_t addr, uint8_t v)
	{
		WriteU8(addr, v);
	}
	void WriteRaw(uint32_t addr, uint16_t v)
	{
		WriteRaw(addr, (uint8_t)v);
		WriteRaw(addr + 1, (uint8_t)(v>>8));
	}

	void Push(uint8_t v)
	{
		WriteZero(cpu_state.regs.sp.u16, v);
		if(mode_emulation)
			cpu_state.regs.sp.u8[0]--;
		else
			cpu_state.regs.sp.u16--;
	}
	void Push(uint16_t v)
	{
		Push((uint8_t)(v >> 8));
		Push((uint8_t)(v & 0xFF));
	}
	// "Old" pop behaviour wraps in emulation mode on pages
	void PopOld(uint8_t& v)
	{
		if(mode_emulation)
			cpu_state.regs.sp.u8[0]++;
		else
			cpu_state.regs.sp.u16++;
		ReadZero(cpu_state.regs.sp.u16, v);
	}
	void PopOld(uint16_t& v)
	{
		Pop(v);
	}
	void Pop(uint8_t& v)
	{
		// "New" pop behaviour never wraps on pages
		ReadZero(cpu_state.regs.sp.u16 + 1, v);
		if(mode_emulation)
			cpu_state.regs.sp.u8[0]++;
		else
			cpu_state.regs.sp.u16++;
	}
	void Pop(uint16_t& v)
	{
		uint8_t hi, lo;
		Pop(lo);
		Pop(hi);
		v = ((uint16_t)hi << 8) | lo;
	}

	void FastBlockMove(uint32_t src_bank, uint32_t dest_bank, uint16_t increment)
	{
		//uint32_t nbytes = 1U + cpu_state.regs.a.u16;
		src_bank <<= 16;
		dest_bank <<= 16;
		while(cpu_state.regs.a.u16 != 0xFFFF) {
			uint32_t src = src_bank | cpu_state.regs.x.u16;
			uint32_t dst = dest_bank | cpu_state.regs.y.u16;
			cpu_state.regs.x.u16 += increment;
			cpu_state.regs.y.u16 += increment;
			cpu_state.regs.a.u16--;

			uint8_t byte;
			ReadU8(src, byte);
			WriteU8(dst, byte);
		}
		cpu_state.data_segment_base = dest_bank;
	}

	bool BlockMove(uint32_t src_bank, uint32_t dest_bank, uint16_t increment)
	{
		if(cpu_state.regs.a.u16 == 0xFFFF)
			return true;
		if(fast_block_moves) {
			FastBlockMove(src_bank, dest_bank, increment);
			return true;
		}
		src_bank <<= 16;
		dest_bank <<= 16;
		uint32_t src = src_bank | cpu_state.regs.x.u16;
		uint32_t dst = dest_bank | cpu_state.regs.y.u16;
		cpu_state.regs.x.u16 += increment;
		cpu_state.regs.y.u16 += increment;
		cpu_state.regs.a.u16--;

		uint8_t byte;
		ReadU8(src, byte);
		WriteU8(dst, byte);
		cpu_state.data_segment_base = dest_bank;

		return false;
	}

	void OnUpdateMode();

	enum Mode : uint32_t
	{
		kNative8A8XY,
		kNative16A8XY,
		kNative8A16XY,
		kNative16A16XY,
		// Emulate a 6502 as the 65816 did.
		kEmulation,
		// Emulate a 6502 exactly, including 6502-exclusive illegal instructions.
		kNative6502,

		kNumModes,
	};

	struct CpuStateImpl : public CpuState
	{
		struct Reg
		{
			union {
				uint64_t padding;
				uint16_t u16;
				uint8_t u8[2];
			};

			inline void get(uint8_t& v) { v = u8[0]; }
			inline void get(uint16_t& v) { v = u16; }

			inline void set(uint8_t v) { u8[0] = v; }
			inline void set(uint16_t v) { u16 = v; }
		};
		struct Registers {
			union {
				struct {
					Reg a, x, y, d, sp;
				};
				Reg reglist[5];
			};
		};
		Registers regs;
		uint32_t data_segment_base;

		bool is_overflow() const { return (other_flags & 0x40) != 0; }
		bool is_decimal() const { return (other_flags & 0x8) != 0; }
	};
	SystemBus *sys;
	CpuStateImpl cpu_state;

	bool mode_native_6502;
	bool mode_emulation;
	bool mode_long_a;
	bool mode_long_xy;

	bool supports_decimal = true;
	bool fast_block_moves = false;

	typedef void (*instruction_exec_fn)(WDC65C816 *cpu);
	const instruction_exec_fn *current_instruction_set;
	ExecInfo exec_info;

	uint64_t num_emulated_instructions = 0;

	uint32_t internal_cycle_timing = 1;
};


#endif
