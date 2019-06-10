#include "cpu_65c816.h"

#include <string.h>

const char *source = R"(
.65816
CLC
XCE
INA
DEX
TAX
LDX $1234, Y
LDY $1234, X
LDA $1234
ADC #1
STA $1234
INY
SBC #2
LDA ($3), Y
JMP $0
)";

uint64_t WDC65C816PerfTest(uint64_t cycles)
{
	std::string error;
	std::vector<uint8_t> bytes;

	SystemBus bus;
	WDC65C816 cpu(&bus);
	cpu.GetAssembler()->Assemble(source, error, bytes);
	Page p;
	std::vector<uint8_t> ram;
	ram.resize(16*1024*1024);
	p.io_eq = 1;
	p.io_mask = 0;
	p.ptr = ram.data();
	memcpy(ram.data(), bytes.data(), bytes.size());
	ram[0xFFFC] = 0;
	ram[0xFFFD] = 0;
	bus.Init(24, 24, &p);
	p.cycles_per_access = 1;
	cpu.cpu_state.cycle = 0;
	cpu.PowerOn();

	EventQueue events;
	auto t = std::chrono::high_resolution_clock::now();
	cpu.cpu_state.cycle_stop = cycles;
	cpu.Emulate(&events);
	auto diff = std::chrono::high_resolution_clock::now() - t;

	return std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
}
