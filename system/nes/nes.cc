#include "nes.h"

namespace nes {

Nes::Nes() : cpu(&main_bus)
{
	cpu.cpu_state.cycle = 0;
	ppu.cpu_cycle = &cpu.cpu_state.cycle;
	ppu.assert_nmi = &AssertNMI;
	ppu.assert_nmi_context = this;
}

void Nes::Step()
{
	cpu.cpu_state.cycle_stop = cpu.cpu_state.cycle + 1;
	cpu.Emulate();
	ppu.CatchUpToCpu();
}

void Nes::Run()
{
	cpu.cpu_state.cycle_stop = current_frame_start_cycle + master_clocks_per_frame;
	cpu.EmulateWithCycleProcessing(*this, &event_queue);
	current_frame_start_cycle += master_clocks_per_frame;
}
void Nes::RunForOneFrame(Framebuffer *fb)
{
	ppu.fb = *fb;
	Run();
}


bool Nes::LoadRom(std::shared_ptr<Rom> rom)
{
	if(!rom->GetConfig("DISPLAY", &system))
		return false;
	uint32_t mapper_num;
	if(!rom->GetConfig("MAPPER", &mapper_num))
		return false;

	uint32_t master_frequency;
	if(system == 0) { // NTSC
		master_frequency = 21477267;
		master_clocks_per_frame = 357366;
		cpu_clock_size = 12;
		ppu.ppu_clock_size = 4;
		ppu.nominal_ppu_frame_time = 89341 * 4;
	} else if(system == 1) { // PAL
		return false;
	}

	cpu.mode_native_6502 = true;
	cpu.supports_decimal = false;

	main_bus.Init(10, 16, main_pages);
	ppu.Init(10, 14, ppu_pages);
	ppu.Initialize(system == 0);

	memset(sysram, 0, sizeof(sysram));

	memset(main_pages, 0, sizeof(main_pages));
	for(auto& p: main_pages)
		p.cycles_per_access = cpu_clock_size;
	cpu.internal_cycle_timing = cpu_clock_size;
	// IO ranges 0x2000-0x401F
	for(uint32_t i = 0; i < 8; i++) {
		main_pages[i].io_eq = 1;
	}
	main_pages[16].io_mask = 0xFFE0;
	main_pages[16].io_mask = 0;
	for(uint32_t i = 17; i < 64; i++) {
		main_pages[i].io_eq = 1;
	}

	main_bus.io_devices.is_io_device_address = &IsIoDeviceAddress;
	main_bus.io_devices.read = &IoRead;
	main_bus.io_devices.write = &IoWrite;
	main_bus.io_devices.irq_taken = [](void *context, uint32_t type) {
		((Nes*)context)->cpu.cpu_state.ClearInterruptSource(type);
	};
	main_bus.io_devices.context = this;

	main_bus.Map(0, sysram, 0x800);
	main_bus.Map(0x800, sysram, 0x800);
	main_bus.Map(0x1000, sysram, 0x800);
	main_bus.Map(0x1800, sysram, 0x800);

	memset(ppu_pages, 0, sizeof(ppu_pages));
	for(auto& e: ppu_pages)
		e.io_eq = 1;

	mapper = Mapper::CreateMapper(mapper_num >> 8, mapper_num & 0xFF);
	if(!mapper)
		return false;

	if(!mapper->Setup(&main_bus, &ppu, rom))
		return false;

	mapper->PowerOn();
	cpu.PowerOn();
	ppu.PowerOn();

	current_rom = std::move(rom);

	return true;
}

void Nes::Reset()
{
	mapper->Reset();
	cpu.Reset();
	ppu.Reset();
}

void Nes::SetUpdateControllersFunc(std::function<void(NesInputData*)> fn)
{
	update_controllers = std::move(fn);
}

void Nes::ReadControllerData(uint32_t controller, uint8_t *v)
{
	uint8_t out = 0;

	bool dpcm_conflict = false;

	if(dpcm_conflict)
		latched_input_data.devices[controller].Shift();
	out = latched_input_data.devices[controller].Shift();

	*v = (*v & 0xE0) | out;
}

void Nes::LatchControllerData()
{
	if(update_controllers)
		update_controllers(&latched_input_data);
}

void Nes::AssertNMI(void *context)
{
	Nes *self = (Nes*)context;
	self->cpu.cpu_state.SetInterruptSource(2);
}

void Nes::ReadReg(uint32_t reg, uint8_t *v)
{
	if(reg == 0x16 || reg == 0x17) {
		ReadControllerData(reg & 1, v);
	}
}
void Nes::WriteReg(uint32_t reg, uint8_t v)
{
	if(reg == 0x14) {
		// There is an extra cycle if we are on an odd cycle, but because the
		// cycle for this write isn't counted yet check for even
		if(!((cpu.cpu_state.cycle / cpu_clock_size) & 1))
			cpu.cpu_state.cycle += cpu_clock_size;
		uint16_t addr = (uint16_t)v << 8;
		ppu.OamDma(&main_bus, addr, cpu_clock_size * 2);
		cpu.cpu_state.cycle += cpu_clock_size * 512;
	} else if(reg == 0x16) {
		// Controller
		if(v & 1) {
			controller_latch = true;
		} else if(controller_latch) {
			controller_latch = false;
			LatchControllerData();
		}
	}
}

bool Nes::IsIoDeviceAddress(void *context, cpuaddr_t addr)
{
	return addr <= 0x401F;
}
void Nes::IoRead(void *context, cpuaddr_t addr, uint8_t *data, uint32_t size)
{
	Nes *self = (Nes*)context;
	if(addr <= 0x401F) {
		// This is a device address
		if(addr < 0x4000) {
			// PPU
			*data = self->ppu.ReadPPU(&self->ppu, addr & 7, false);
		} else {
			// CPU reg
			self->ReadReg(addr & 0x1F, data);
		}
	} else {
		// This could be a mapper IO port or a RAM access
		if(!self->mapper->IoRead(addr, data))
			self->main_bus.ReadByteNoIo(addr, data);
	}
}
void Nes::IoWrite(void *context, cpuaddr_t addr, const uint8_t *data, uint32_t size)
{
	Nes *self = (Nes*)context;
	if(addr <= 0x401F) {
		// This is a device address
		if(addr < 0x4000) {
			// PPU
			self->ppu.WritePPU(&self->ppu, addr & 7, *data);
		} else {
			// CPU reg
			self->WriteReg(addr & 0x1F, *data);
		}
	} else {
		if(!self->mapper->IoWrite(addr, *data))
			self->main_bus.WriteByteNoIo(addr, *data);
	}
}

}
