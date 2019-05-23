#ifndef SYSTEM_C256_H_
#define SYSTEM_C256_H_

#include "cpu/65816/cpu_65c816.h"
#include "host_system.h"

#include <string>

class C256
{
public:
	C256(uint32_t ram_size, const std::string& sysflash_path, const std::string& userflash_path);

	void PowerOn();
	void Reset();

	void Emulate();

	uint8_t* rambase() { return ram->Pointer(); }

	SystemBus sys;
	WDC65C816 cpu;

private:
	static bool IsIoDeviceAddress(void *context, cpuaddr_t addr);
	static void IoRead(void *context, cpuaddr_t addr, uint8_t *data, uint32_t size);
	static void IoWrite(void *context, cpuaddr_t addr, const uint8_t *data, uint32_t size);

	void GavinIoRead(uint32_t reg, uint8_t *data, uint32_t size);
	void GavinIoWrite(uint32_t reg, const uint8_t *data, uint32_t size);
	void AfIoRead(uint32_t reg, uint8_t *data, uint32_t size);
	void AfIoWrite(uint32_t reg, const uint8_t *data, uint32_t size);

	void Map(NativeMemory *mem, uint32_t addr);

	std::unique_ptr<NativeMemory> ram;
	std::unique_ptr<NativeMemory> vram;
	std::unique_ptr<NativeMemory> io;
	std::unique_ptr<NativeMemory> sysflash;
	std::unique_ptr<NativeMemory> userflash;

	uint8_t gavin_low_regs[256];

	// 16 MB
	Page pages[0x1000000 >> WDC65C816::kPageSizeBits];
};

#endif
