#define _CRT_SECURE_NO_WARNINGS // Shut MSVC up about sprintf

#pragma GCC diagnostic ignored "-Wmissing-field-initializers" // Shut GCC up

#include "cpu_65c816.h"

#include "cpu_65c816_instructions.inl"

#include <map>

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

constexpr const char *formats[6][256] {
#define OP(...) __VA_ARGS__::kParamFormat,
#include "cpu_65c816_ops.inl"
#undef OP
};

constexpr uint8_t sizes[6][256] {
#define OP(...) __VA_ARGS__::kBytes,
#include "cpu_65c816_ops.inl"
#undef OP
};

typedef void (*DisassembleFn)(WDC65C816 *cpu, uint32_t& addr, const char **str, char *formatted_str);
constexpr DisassembleFn disassemble_fns[6][256] = {
#define OP(...) &__VA_ARGS__::Disassemble,
#include "cpu_65c816_ops.inl"
#undef OP
};

class WDC65816Assembler : public Assembler
{
public:
	static WDC65816Assembler* Get()
	{
		static WDC65816Assembler *inst = new WDC65816Assembler();
		return inst;
	}

	WDC65816Assembler();

	bool Assemble(const char*& start, std::string& error, std::vector<uint8_t>& bytes) override;

	struct Instruction
	{
		uint8_t opcode;
		uint8_t bytes;
		const char *format;
	};
	struct InstructionGroup
	{
		std::vector<Instruction> instructions;
	};

	struct InstructionList
	{
		std::map<std::string, InstructionGroup*> instructions_by_mnemonic;
	};

	InstructionList instructions_for_mode[WDC65C816::kNumModes];
	std::vector<std::unique_ptr<InstructionGroup>> groups;

	bool long_a = false;
	bool long_xy = false;
	uint32_t current_mode = WDC65C816::kEmulation;
};

WDC65816Assembler::WDC65816Assembler()
{
	for(uint32_t i = 0; i < WDC65C816::kNumModes; i++) {
		auto& mode = instructions_for_mode[i];
		for(uint32_t j = 0; j < 256; j++) {
			const char *mnemonic = mnemonics[i][j];
			if(!mnemonic)
				continue;
			const char *format = formats[i][j];
			uint8_t size = sizes[i][j];

			auto& group = mode.instructions_by_mnemonic[mnemonic];
			if(!group) {
				groups.emplace_back(new InstructionGroup());
				group = groups.back().get();
			}
			group->instructions.emplace_back(Instruction{(uint8_t)j, size, format});
		}
	}
}

void SkipWhitespace(const char*& p)
{
	while(isspace(*p))p++;
}
void SkipToEndOfLine(const char*& p)
{
	while(*p && *p != '\n')p++;
}
bool SkipNumber(const char*& p, uint32_t& number)
{
	bool hex = false;
	bool ok = false;
	if(*p == '$' || (p[0] == '0' && p[1] == 'x')) {
		hex = true;
		p += (p[0] == '0') ? 2 : 1;
	}
	while(*p) {
		if(*p >= '0' && *p <= '9') {
			number *= hex ? 16 : 10;
			number += *p - '0';
			ok = true;
		} else if(hex && *p >= 'A' && *p <= 'F') {
			number *= 16;
			number += 10 + *p - 'A';
			ok = true;
		} else if(hex && *p >= 'a' && *p <= 'f') {
			number *= 16;
			number += 10 + *p - 'a';
			ok = true;
		} else {
			break;
		}
		p++;
	}
	return ok;
}
void GetToken(std::string& out, const char *& p)
{
	while(isalnum(*p) || *p == '.') {
		out.push_back(toupper(*p++));
	}
}

bool TryMatch(const char*& in, const char *format, uint32_t *number = nullptr)
{
	while(*format) {
		while(isspace(*format))format++;
		if(!*format)
			break;
		while(isspace(*in))in++;

		if(*format == '%') {
			format++;
			if(*format != 'X')
				return false;
			format++;
			uint32_t v = 0;
			if(!SkipNumber(in, v))
				return false;
			if(number)
				*number = v;
		} else {
			if(toupper(*in++) != toupper(*format++))
				return false;
		}
	}
	return !!isspace(*in);
}

bool WDC65816Assembler::Assemble(const char*& p, std::string& error, std::vector<uint8_t>& bytes)
{
	while(*p) {
		SkipWhitespace(p);
		if(*p == ';') {
			SkipToEndOfLine(p);
			continue;
		}
		std::string token;
		GetToken(token, p);
		if(token[0] == '.') {
			// Directive
			if(token == ".LONGI") {
				token.resize(0);
				GetToken(token, p);
				if(token == "ON") long_xy = true;
				else if(token == "OFF") long_xy = false;
				else {
					error = ".LONGI directive expected ON or OFF not " + token;
					return false;
				}
				if(current_mode != WDC65C816::kEmulation && current_mode != WDC65C816::kNative6502)
					current_mode = (long_a ? 1 : 0) + (long_xy ? 2 : 0);
			}
			if(token == ".LONGA") {
				token.resize(0);
				GetToken(token, p);
				if(token == "ON") long_a = true;
				else if(token == "OFF") long_a = false;
				else {
					error = ".LONGA directive expected ON or OFF not " + token;
					return false;
				}
				if(current_mode != WDC65C816::kEmulation && current_mode != WDC65C816::kNative6502)
					current_mode = (long_a ? 1 : 0) + (long_xy ? 2 : 0);
			}
			if(token == ".6502") {
				current_mode = WDC65C816::kEmulation;
			}
			if(token == ".NES") {
				current_mode = WDC65C816::kNative6502;
			}
			if(token == ".65816") {
				current_mode = (long_a ? 1 : 0) + (long_xy ? 2 : 0);
			}
			continue;
		}
		if(isalpha(token[0])) {
			// Assume this is an instruction
			auto it = instructions_for_mode[current_mode].instructions_by_mnemonic.find(token);
			if(it != instructions_for_mode[current_mode].instructions_by_mnemonic.end()) {
				std::vector<Instruction*> candidates;
				for(auto& e: it->second->instructions) {
					const char *tmp = p;
					if(TryMatch(tmp, e.format))
						candidates.push_back(&e);
				}

				Instruction *best = nullptr;
				if(!candidates.empty()) {
					for(auto candidate : candidates) {
						uint32_t arg = 0;
						const char *tmp = p;
						TryMatch(tmp, candidate->format, &arg);

						if(candidate->bytes > 1 && arg >= (1U << (8 *(candidate->bytes - 1))))
							continue;
						if(!best || best->bytes > candidate->bytes)
							best = candidate;
					}
				}

				if(!best) {
					error = "Unknown instruction format for " + token;
					return false;
				}

				bytes.push_back(best->opcode);
				uint32_t v = 0;
				TryMatch(p, best->format, &v);
				for(uint8_t i = 1; i < best->bytes; i++) {
					bytes.push_back(v & 0xFF);
					v >>= 8;
				}
				if(v > 0) {
					error = "Constant too large";
					return false;
				}
			} else {
				error = "Unexpected token " + token;
				return false;
			}
		}
	}

	return true;
}

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
	uint16_t vector = 0;
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

void WDC65C816::EmulateInstruction(void *context)
{
	WDC65C816 *self = (WDC65C816*)context;
	uint8_t instruction;
	self->ReadPBR(self->cpu_state.ip, instruction);
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

bool WDC65C816::SetRegister(const char *reg, uint64_t value)
{
	if(!_stricmp(reg, "ip") || !_stricmp(reg, "pc")) {
		cpu_state.ip = (cpuaddr_t)value & 0xFFFF;
		if(value > 0xFFFF)
			cpu_state.code_segment_base = (cpuaddr_t)(value & 0xFF0000);
	} else if(!_stricmp(reg, "a")) {
		if(mode_long_a)
			cpu_state.regs.a.u16 = (uint16_t)value;
		else
			cpu_state.regs.a.u8[0] = (uint8_t)value;
	} else if(!_stricmp(reg, "x")) {
		if(mode_long_xy)
			cpu_state.regs.x.u16 = (uint16_t)value;
		else
			cpu_state.regs.x.u8[0] = (uint8_t)value;
	} else if(!_stricmp(reg, "y")) {
		if(mode_long_xy)
			cpu_state.regs.y.u16 = (uint16_t)value;
		else
			cpu_state.regs.y.u8[0] = (uint8_t)value;
	} else if(!mode_native_6502 && !_stricmp(reg, "c")) {
		cpu_state.regs.a.u16 = (uint16_t)value;
	} else if(!mode_native_6502 && !_stricmp(reg, "d")) {
		cpu_state.regs.d.u16 = (uint16_t)value;
	} else if(!_stricmp(reg, "sp")) {
		if(mode_emulation)
			cpu_state.regs.sp.u16 = 0x100 | (uint8_t)(value & 0xFF);
		else
			cpu_state.regs.sp.u16 = (uint16_t)value;
	} else if(!_stricmp(reg, "p")) {
		SetStatusRegister((uint8_t)value);
	} else if(!mode_native_6502 && !_stricmp(reg, "pb")) {
		cpu_state.code_segment_base = (cpuaddr_t)(value << 16);
	} else if(!mode_native_6502 && !_stricmp(reg, "db")) {
		cpu_state.data_segment_base = (cpuaddr_t)(value << 16);
	} else {
		return false;
	}
	return true;
}

bool WDC65C816::GetDebugRegState(std::vector<DebugReg>& regs)
{
	regs.emplace_back(DebugReg{"A", cpu_state.regs.a.u16, mode_long_a ? 2U : 1U, 0});
	regs.emplace_back(DebugReg{"X", cpu_state.regs.x.u16, mode_long_xy ? 2U : 1U, 0});
	regs.emplace_back(DebugReg{"Y", cpu_state.regs.y.u16, mode_long_xy ? 2U : 1U, 0});
	regs.emplace_back(DebugReg{"SP", cpu_state.regs.sp.u16, mode_emulation ? 1U : 2U, 0});
	regs.emplace_back(DebugReg{"P", GetStatusRegister(), 1U, 0});
	if(!mode_native_6502) {
		regs.emplace_back(DebugReg{"D", cpu_state.regs.d.u16, 2U, 0});
		regs.emplace_back(DebugReg{"PB", cpu_state.code_segment_base >> 16, 1U, 0});
		regs.emplace_back(DebugReg{"DB", cpu_state.data_segment_base >> 16, 1U, 0});
	}
	return true;
}

Assembler* WDC65C816::GetAssembler()
{
	return WDC65816Assembler::Get();
}
