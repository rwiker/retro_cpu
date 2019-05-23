#include "cpu.h"

uint32_t SystemBus::ReadByte(cpuaddr_t addr, uint8_t *data)
{
	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	if((p.io_mask & addr) == p.io_eq) {
		io_devices.read(io_devices.context, addr, data, 1);
	} else {
		*data = p.ptr[addr & memory.page_mask];
	}
	return p.cycles_per_access;
}
uint32_t SystemBus::WriteByte(uint32_t addr, uint8_t v)
{
	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	if((p.io_mask & addr) == p.io_eq) {
		io_devices.write(io_devices.context, addr, &v, 1);
	} else if(!(p.flags & Page::kReadOnly)) {
		p.ptr[addr & memory.page_mask] = v;
	}
	return p.cycles_per_access;
}
void SystemBus::Map(cpuaddr_t addr, uint8_t *ptr, uint32_t len)
{
	uint32_t first_page = addr / memory.page_size;
	for(uint32_t i = 0; i < len / memory.page_size; i++) {
		memory.pages[first_page + i].ptr = ptr + i * memory.page_size;
	}
}

void SystemBus::Init(uint32_t size_shift, uint32_t addr_bus_bits, Page *pages)
{
	memory.page_shift = size_shift;
	memory.page_size = 1U << size_shift;
	memory.page_mask = memory.page_size - 1;
	memory.pages = pages;
	mem_mask = (1UL << addr_bus_bits) - 1;
}

void EmulatedCpu::Emulate()
{
	auto exec = GetExecInfo();
	auto state = GetCpuState();
	while(state->cycle < state->cycle_stop) {
		state->ip &= state->ip_mask;
		if(state->interrupts >= 3) {
			uint32_t type;
			if(state->interrupts & 4) {
				type = 1;
				state->interrupts -= 4;
			} else {
				type = 0;
				state->interrupts -= 2;
			}
			exec->interrupt(exec->interrupt_context, type);
			continue;
		}
		exec->emu(exec->emu_context);
	}
}
