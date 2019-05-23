#include "c256.h"
#include <string.h>

namespace {

static void MathCoproUMul(uint8_t *bytes)
{
	uint16_t a = bytes[0] + bytes[1] * 256;
	uint16_t b = bytes[2] + bytes[3] * 256;
	uint32_t result = a * b;
	bytes[4] = result;
	bytes[5] = result >> 8;
	bytes[6] = result >> 16;
	bytes[7] = result >> 24;
}
static void MathCoproSMul(uint8_t *bytes)
{
	uint16_t a = bytes[0] + bytes[1] * 256;
	uint16_t b = bytes[2] + bytes[3] * 256;
	uint32_t result = static_cast<int>(a) * static_cast<int>(b);
	bytes[4] = result;
	bytes[5] = result >> 8;
	bytes[6] = result >> 16;
	bytes[7] = result >> 24;
}
static void MathCoproUDiv(uint8_t *bytes)
{
	uint16_t a = bytes[0] + bytes[1] * 256;
	uint16_t b = bytes[2] + bytes[3] * 256;
	uint16_t result, rem;
	if(b != 0) {
		result = a / b;
		rem = a % b;
	} else {
		result = rem = 0;
	}
	bytes[4] = result & 0xFF;
	bytes[5] = result >> 8;
	bytes[6] = rem & 0xFF;
	bytes[7] = rem >> 8;
}
static void MathCoproSDiv(uint8_t *bytes)
{
	uint16_t a = bytes[0] + bytes[1] * 256;
	uint16_t b = bytes[2] + bytes[3] * 256;
	int16_t result, rem;
	if(b != 0) {
		int sa = static_cast<int>(a);
		int sb = static_cast<int>(b);
		result = sa / sb;
		rem = sa % sb;
	} else {
		result = rem = 0;
	}
	bytes[4] = result & 0xFF;
	bytes[5] = result >> 8;
	bytes[6] = rem & 0xFF;
	bytes[7] = rem >> 8;
}
static void MathCoproAdd(uint8_t *bytes)
{
	uint32_t sum;
	sum = (uint32_t)bytes[0];
	sum += (uint32_t)bytes[1] << 8;
	sum += (uint32_t)bytes[2] << 16;
	sum += (uint32_t)bytes[3] << 24;
	sum += (uint32_t)bytes[4];
	sum += (uint32_t)bytes[5] << 8;
	sum += (uint32_t)bytes[6] << 16;
	sum += (uint32_t)bytes[7] << 24;
	bytes[8] = sum;
	bytes[9] = sum >> 8;
	bytes[10] = sum >> 16;
	bytes[11] = sum >> 24;
}



}

C256::C256(uint32_t ram_size, const std::string& sysflash_path, const std::string& userflash_path) : cpu(&sys)
{
	ram = NativeMemory::Create(ram_size);
	vram = NativeMemory::Create(0x400000); // 4 MB VRAM
	sysflash = NativeMemory::Create(0x80000); // 512 kB flashes
	userflash = NativeMemory::Create(0x80000);
	io = NativeMemory::Create(0x10000);

	sys.memory.pages = pages;
	sys.memory.page_size = 0x1000;

	// Init memory map
	for(uint32_t i = 0; i < 256; i++) {
		constexpr uint32_t kPagesPer64k = 1U << (16 - WDC65C816::kPageSizeBits);
		for(uint32_t j = 0; j < kPagesPer64k; j++) {
			auto& p = pages[i*kPagesPer64k + j];
			p.ptr = 0;
			p.flags = 0;
			p.io_mask = 0;
			p.io_eq = 1;
			p.cycles_per_access = 1;

			// IO is mapped at 00:01xx and AF:xxxx 
			if(i == 0xAF) {
				p.io_mask = 0;
				p.io_eq = 0;
			}
			if(i == 0 && j == 0) {
				p.io_mask = 0xFF00;
				p.io_eq = 0x100;
			}
			if(i >= 0xF0)
				p.flags = Page::kReadOnly;
		}
	}

	// Map the various regions
	Map(ram.get(), 0);
	Map(vram.get(), 0xB00000);
	Map(sysflash.get(), 0xF00000);
	Map(userflash.get(), 0xF80000);
	Map(io.get(), 0xAF0000);

	sys.io_devices.context = this;
	sys.io_devices.read = &IoRead;
	sys.io_devices.write = &IoWrite;
	sys.io_devices.is_io_device_address = &IsIoDeviceAddress;

	sys.Init(WDC65C816::kPageSizeBits, cpu.GetAddressBusBits(), pages);
}

void C256::PowerOn()
{
	// C256 startup
	// sysflash is coped to RAM and then the first 64k to first 64k
	//memcpy(ram->Pointer() + 0x180000, sysflash->Pointer(), 0x80000);
	memcpy(ram->Pointer(), ram->Pointer() + 0x180000, 0x10000);
	cpu.PowerOn();
}
void C256::Reset()
{
	cpu.Reset();
}

void C256::Map(NativeMemory *mem, uint32_t addr)
{
	sys.Map(addr, mem->Pointer(), mem->GetSize());
}

bool C256::IsIoDeviceAddress(void *context, cpuaddr_t addr)
{
	return ((addr & 0xFF0000) == 0xAF0000) || (addr >= 0x100 && addr < 0x1A0);
}
void C256::IoRead(void *context, cpuaddr_t addr, uint8_t *data, uint32_t size)
{
	C256 *self = (C256*)context;
	if((addr & 0xFF0000) == 0xAF0000) {
		self->AfIoRead(addr & 0xFFFF, data, size);
	} else if(addr >= 0x100 && addr < 0x1A0) {
		self->GavinIoRead(addr - 0x100, data, size);
	} else {
		// Memory access that got trapped
		memcpy(data, self->ram->Pointer() + addr, size);
	}
}
void C256::IoWrite(void *context, cpuaddr_t addr, const uint8_t *data, uint32_t size)
{
	C256 *self = (C256*)context;
	if((addr & 0xFF0000) == 0xAF0000) {
		self->AfIoWrite(addr & 0xFFFF, data, size);
	} else if(addr >= 0x100 && addr < 0x1A0) {
		self->GavinIoWrite(addr - 0x100, data, size);
	} else {
		// Memory access that got trapped
		memcpy(self->ram->Pointer() + addr, data, size);
	}
}

void C256::GavinIoRead(uint32_t reg, uint8_t *data, uint32_t size)
{
	// 100-12F: Math coprocessor
	// 16x16 Unsigned & signed multiplier, 16/16 unsigned & signed divider, 32x32bit adder
	// 130-13F: ???
	// 140-15F: Interrupt controller
	// 160-17F: Timers
	// 180-19F: SDMA
	// 1A0-1FF: ???
	*data = gavin_low_regs[reg];
	if(size == 2) {
		data[1] = gavin_low_regs[reg + 1];
	}
}

void C256::GavinIoWrite(uint32_t reg, const uint8_t *data, uint32_t size)
{
	gavin_low_regs[reg] = *data;
	switch(reg >> 3) {
	case 0:
		MathCoproUMul(gavin_low_regs + 0);
		break;
	case 1:
		MathCoproSMul(gavin_low_regs + 8);
		break;
	case 2:
		MathCoproUDiv(gavin_low_regs + 0x10);
		break;
	case 3:
		MathCoproSDiv(gavin_low_regs + 0x18);
		break;
	case 4:
	case 5:
		MathCoproAdd(gavin_low_regs + 0x20);
		break;
	default:
		break;
	}
}

void C256::AfIoRead(uint32_t reg, uint8_t *data, uint32_t size)
{
	*data = 0;
}

void C256::AfIoWrite(uint32_t reg, const uint8_t *data, uint32_t size)
{
	io->Pointer()[reg] = *data;
}

void C256::Emulate()
{
	cpu.Emulate();
}
