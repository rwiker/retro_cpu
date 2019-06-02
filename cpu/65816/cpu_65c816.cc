#define _CRT_SECURE_NO_WARNINGS // Shut MSVC up about sprintf

#include "cpu_65c816.h"

#include "cpu_65c816_instructions.inl"

namespace {

typedef void (*exec_fn)(WDC65C816 *cpu);
constexpr exec_fn exec_ops[6][256] {
#define OP(...) &__VA_ARGS__::Exec,
#include "cpu_65c816_ops.inl"
#undef OP
};
constexpr const JitOperation *jit_ops[6][256] {
#define OP(...) __VA_ARGS__::ops,
#include "cpu_65c816_ops.inl"
#undef OP
};
constexpr const char *mnemonics[6][256] {
#define OP(...) __VA_ARGS__::kMnemonic,
#include "cpu_65c816_ops.inl"
#undef OP
};

typedef void (*DisassembleFn)(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str);
constexpr DisassembleFn disassemble_fns[6][256] = {
#define OP(...) &__VA_ARGS__::Disassemble,
#include "cpu_65c816_ops.inl"
#undef OP
};

}

WDC65C816::WDC65C816(SystemBus *sys) : EmulatedCpu(sys), sys(sys)
{
	mode_native_6502 = false;
	mode_emulation = true;
	mode_long_a = false;
	mode_long_xy = false;

	cpu_state.ip_mask = 0xFFFF;
	cpu_state.cycle_stop = ~0ULL;
	cpu_state.data_segments = &cpu_state.data_segment_base;

	cpu_state.registers = &cpu_state.regs.reglist[0].padding;
	cpu_state.reg_size_bytes = 2;
	cpu_state.reg_count = 5;

	exec_info.emu = &EmulateInstruction;
	exec_info.emu_context = this;
	exec_info.interrupt = &Interrupt;
	exec_info.interrupt_context = this;

	internal_cycle_timing = 1;
	OnUpdateMode();
}

CpuState* WDC65C816::GetCpuState()
{
	return &cpu_state;
}

const EmulatedCpu::ExecInfo* WDC65C816::GetExecInfo()
{
	return &exec_info;
}


void WDC65C816::SetNZ(uint8_t v)
{
	cpu_state.zero = v;
	cpu_state.negative = v >> 7;
}
void WDC65C816::SetNZ(uint16_t v)
{
	cpu_state.zero = v;
	cpu_state.negative = v >> 15;
}

uint8_t WDC65C816::GetStatusRegister()
{
	uint8_t v = cpu_state.other_flags & 0x48;
	if(!cpu_state.zero)
		v |= 2;
	if(!cpu_state.interrupts)
		v |= 4;
	if(cpu_state.carry & 2)
		v |= 1;
	if(cpu_state.negative & 1)
		v |= 0x80;
	if(mode_emulation) {
		v |= 0x30;
	} else {
		if(!mode_long_a)
			v |= 0x20;
		if(!mode_long_xy)
			v |= 0x10;
	}
	return v;
}

void WDC65C816::SetStatusRegister(uint8_t v)
{
	cpu_state.other_flags = v;
	cpu_state.zero = 2 - (v & 2);
	cpu_state.carry = (v & 1) << 1;
	cpu_state.interrupts = 1 - ((v >> 2) & 1);
	cpu_state.negative = v >> 7;

	bool changed_mode = false;
	if(mode_long_a != !(v & 0x20)) {
		changed_mode = true;
		mode_long_a = !(v & 0x20);
	}
	if(mode_long_xy != !(v & 0x10)) {
		changed_mode = true;
		mode_long_xy = !(v & 0x10);
	}
	if(changed_mode)
		OnUpdateMode();
}

void WDC65C816::WAI()
{
}
void WDC65C816::STP()
{
}

void WDC65C816::DoInterrupt(InterruptType type)
{
	if(!mode_emulation) {
		uint8_t bank = cpu_state.code_segment_base >> 16;
		Push(bank);
	}
	uint16_t pc = cpu_state.ip;
	Push(pc);

	uint8_t status = GetStatusRegister();
	if(mode_emulation) {
		if(type == IRQ || type == BRK || type == NMI)
			status = (status & 0xCF) | ((type == IRQ) ? 0x10 : 0x30);
		else
			status = (status & 0xCF) | 0x20;
	}
	Push(status);
	uint16_t vector;
	switch(type) {
	case BRK:
		vector = (mode_emulation ? 0xFFFE : 0xFFE6);
		break;
	case COP:
		vector = (mode_emulation ? 0xFFF4 : 0xFFE4);
		break;
	case IRQ:
		vector = (mode_emulation ? 0xFFFE : 0xFFEE);
		break;
	case NMI:
		vector = (mode_emulation ? 0xFFFA : 0xFFEA);
		break;
	}
	ReadRaw(vector, pc);
	cpu_state.ip = pc;
	cpu_state.code_segment_base = 0;

	cpu_state.interrupts = 0;
	if(!mode_native_6502)
		cpu_state.other_flags &= 0xFFF7;

	if(type == IRQ || type == NMI)
		sys->io_devices.irq_taken(sys->io_devices.context, (type == IRQ) ? 1 : 2);
}

//FILE *f = fopen("f:/log.txt", "wt");
void WDC65C816::EmulateInstruction(void *context)
{
	WDC65C816 *self = (WDC65C816*)context;
	uint8_t instruction;
	self->ReadPBR(self->cpu_state.ip, instruction);
	//fprintf(f,"A:%02X X:%02X Y:%02X %04X:%02X %s\n",
	//	self->cpu_state.regs.a.u8[0], self->cpu_state.regs.x.u8[0], self->cpu_state.regs.y.u8[0],
	//	self->cpu_state.ip, instruction, mnemonics[self->cpu_state.mode][instruction]);
	//fflush(f);
	self->current_instruction_set[instruction](self);
	self->num_emulated_instructions++;
}

void WDC65C816::Interrupt(void *context, uint32_t param)
{
	WDC65C816 *self = (WDC65C816*)context;
	if(!param)
		self->DoInterrupt(IRQ);
	else
		self->DoInterrupt(NMI);
}

void WDC65C816::PowerOn()
{
	cpu_state.regs.a.u16 = 0;
	cpu_state.regs.x.u16 = 0;
	cpu_state.regs.y.u16 = 0;
	cpu_state.regs.d.u16 = 0;
	cpu_state.regs.sp.u16 = 0x100;
	cpu_state.zero = 1;
	Reset();
}

void WDC65C816::Reset()
{
	cpu_state.data_segment_base = 0;
	cpu_state.code_segment_base = 0;
	cpu_state.regs.sp.u8[0] -= 3;
	// Read from 00FFFC/D
	uint8_t ip_low, ip_high;
	ReadU8(0xFFFC, ip_low);
	ReadU8(0xFFFD, ip_high);
	cpu_state.cycle += internal_cycle_timing * 4;

	cpu_state.ip = ip_low | ((uint16_t)ip_high << 8);
	cpu_state.data_segment_base = 0;
	cpu_state.code_segment_base = 0;

	cpu_state.interrupts = 0;

	mode_emulation = true;
	mode_long_a = false;
	mode_long_xy = false;
	OnUpdateMode();
}

void WDC65C816::OnUpdateMode()
{
	if(mode_native_6502) {
		cpu_state.mode = kNative6502;
		mode_emulation = true;
		mode_long_a = false;
		mode_long_xy = false;
	} else if(mode_emulation) {
		cpu_state.mode = kEmulation;
		mode_long_a = false;
		mode_long_xy = false;
	} else {
		// This must match the order of includes in cpu_65c816_ops.inl
		cpu_state.mode = (mode_long_a ? 1 : 0) + (mode_long_xy ? 2 : 0);
	}
	if(!mode_long_xy) {
		cpu_state.regs.x.u8[1] = 0;
		cpu_state.regs.y.u8[1] = 0;
	}
	current_instruction_set = exec_ops[cpu_state.mode];
}

const JitOperation* WDC65C816::GetJit(JitCore *core, cpuaddr_t addr)
{
	uint8_t opcode;
	PeekU8(addr, opcode);
	return jit_ops[cpu_state.mode][opcode];
}

bool WDC65C816::DisassembleOneInstruction(uint32_t& canonical_address, CpuInstruction& insn)
{
	uint8_t opcode;
	PeekU8(canonical_address, opcode);

	uint32_t start = canonical_address;

	const char *fixed_str = nullptr;
	char formatted_str[256];
	disassemble_fns[cpu_state.mode][opcode](this, canonical_address, &fixed_str, formatted_str);

	uint32_t length  = canonical_address - start;

	insn.canonical_address = canonical_address;
	insn.length = length;

	insn.asm_string.resize(256);
	int n = sprintf(&insn.asm_string[0], "%06X: %02X %02X %02X %02X %s",
		start, opcode, PeekU8(start + 1), PeekU8(start + 2), PeekU8(start + 3),
		fixed_str ? fixed_str : formatted_str);
	for(uint32_t i = length * 3; i < 12; i++)
		insn.asm_string[i + 8] = ' ';
	insn.asm_string.resize(n);

	return true;
}
