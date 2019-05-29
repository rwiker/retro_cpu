#ifndef SYSTEM_NES_NES_H_
#define SYSTEM_NES_NES_H_

#include "rom.h"
#include "cpu/65816/cpu_65c816.h"

#include "2c02.h"
#include "nes_mapper.h"

namespace nes {

struct NesInputData
{
	struct Device
	{
		uint32_t serial_bits[5];

		uint8_t Shift()
		{
			uint8_t ret = 0;
			for(int i = 0; i < 5; i++) {
				ret |= (serial_bits[i] & 1) << i;
				serial_bits[i] >>= 1;
			}
			return ret;
		}
	} devices[2];
};

class Nes
{
public:
	Nes();
	bool LoadRom(std::shared_ptr<Rom> rom);

	void SetUpdateControllersFunc(std::function<void(NesInputData*)> fn);

	void RunForOneFrame(Framebuffer *fb);
	void Run();
	void Step();

	void Reset();

	bool is_ntsc() const { return system == 0; }

	void PreCpuCycle() { }
	void PostCpuCycle() { ppu.CatchUpToCpu(); }

//private:
	static bool IsIoDeviceAddress(void *context, cpuaddr_t addr);
	static void IoRead(void *context, cpuaddr_t addr, uint8_t *data, uint32_t size);
	static void IoWrite(void *context, cpuaddr_t addr, const uint8_t *data, uint32_t size);

	static void AssertNMI(void *context);

	void ReadReg(uint32_t reg, uint8_t *v);
	void WriteReg(uint32_t reg, uint8_t v);

	void LatchControllerData();
	void ReadControllerData(uint32_t controller, uint8_t *v);

	uint32_t master_clocks_per_frame;
	uint32_t cpu_clock_size;
	uint32_t frames_per_extra_cycle;
	uint32_t frame_id = 1;
	uint64_t current_frame_start_cycle = 0;

	bool controller_latch = false;
	NesInputData latched_input_data = {};
	std::function<void(NesInputData*)> update_controllers;

	uint32_t system;

	WDC65C816 cpu;
	SystemBus main_bus;

	PPU_2C02 ppu;

	// 1 kB pages
	Page main_pages[64];
	Page ppu_pages[16];

	uint8_t sysram[2048];

	std::unique_ptr<Mapper> mapper;

	// Various memory blocks
	std::unique_ptr<NativeMemory> chr_ram;
	std::unique_ptr<NativeMemory> chr_nvram;
	std::unique_ptr<NativeMemory> prg_nvram;
};

}

#endif
