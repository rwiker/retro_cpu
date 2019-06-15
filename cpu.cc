#include "cpu.h"

#include <stdio.h>

bool SystemBus::QueryIo(cpuaddr_t addr)
{
	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	if((p.io_mask & addr) == p.io_eq)
		return io_devices.is_io_device_address(io_devices.context, addr);
	return false;
}

uint32_t SystemBus::ReadByte(cpuaddr_t addr, uint8_t *data)
{
	if(!open_bus_is_data)
		open_bus = addr & 0xFF;

	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	if((p.io_mask & addr) == p.io_eq) {
		io_devices.read(io_devices.context, addr, &open_bus, 1);
	} else if(p.ptr) {
		open_bus = p.ptr[addr & memory.page_mask];
	}
	*data = open_bus;
	return p.cycles_per_access;
}
uint32_t SystemBus::ReadByteNoIo(cpuaddr_t addr, uint8_t *data)
{
	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	*data = p.ptr[addr & memory.page_mask];
	return p.cycles_per_access;
}

uint32_t SystemBus::WriteByte(uint32_t addr, uint8_t v)
{
	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	if((p.io_mask & addr) == p.io_eq) {
		io_devices.write(io_devices.context, addr, &v, 1);
	} else if(!(p.flags & Page::kReadOnly) && p.ptr) {
		p.ptr[addr & memory.page_mask] = v;
	}
	open_bus = open_bus_is_data ? v : addr & 0xFF;
	return p.cycles_per_access;
}
uint32_t SystemBus::WriteByteNoIo(uint32_t addr, uint8_t v)
{
	addr &= mem_mask;
	Page& p = memory.pages[addr >> memory.page_shift];
	if(!(p.flags & Page::kReadOnly)) {
		p.ptr[addr & memory.page_mask] = v;
	}
	return p.cycles_per_access;
}
void SystemBus::Map(cpuaddr_t addr, uint8_t *ptr, uint32_t len, bool readonly)
{
	uint32_t first_page = addr / memory.page_size;
	for(uint32_t i = 0; i < len / memory.page_size; i++) {
		memory.pages[first_page + i].ptr = ptr + i * memory.page_size;
		if(readonly)
			memory.pages[first_page + i].flags |= Page::kReadOnly;
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

void EmulatedCpu::SingleStep(EventQueue *events)
{
	auto exec = GetExecInfo();
	auto state = GetCpuState();
	if(events) {
		events->Expire(state->cycle);
	}
	state->ip &= state->ip_mask;
	uint32_t pending_interrupts = state->pending_interrupts.load(std::memory_order_acquire);
	if(pending_interrupts + state->interrupts >= 3) {
		uint32_t type;
		if(pending_interrupts & 4) {
			type = 1;
		} else {
			type = 0;
		}
		exec->interrupt(exec->interrupt_context, type);
		return;
	}
	if(breakpoints.find(state->GetCanonicalAddress()) != breakpoints.end()) {
		breakpoints.find(state->GetCanonicalAddress())->second(this);
	}
	exec->emu(exec->emu_context);
}

void EmulatedCpu::Emulate(EventQueue *events)
{
	auto exec = GetExecInfo();
	auto state = GetCpuState();
	state->event_cycle = state->cycle_stop;
	if(events)
		events->Start(&state->event_cycle, state->cycle_stop);
	do {
		while(state->cycle < state->event_cycle) {
			state->ip &= state->ip_mask;
			uint32_t pending_interrupts = state->pending_interrupts.load(std::memory_order_acquire);
			if(pending_interrupts + state->interrupts >= 3) {
				uint32_t type;
				if(pending_interrupts & 4) {
					type = 1;
				} else {
					type = 0;
				}
				exec->interrupt(exec->interrupt_context, type);
				continue;
			}
			if(has_breakpoints && breakpoints.find(state->GetCanonicalAddress()) != breakpoints.end()) {
				breakpoints.find(state->GetCanonicalAddress())->second(this);
			}
			exec->emu(exec->emu_context);
		}
		events->Expire(state->cycle);
	} while(state->cycle < state->cycle_stop);
}

void EventQueue::ScheduleNoLock(uint64_t t, std::function<void()> f)
{
	entries.emplace_back(Entry{t, std::move(f)});
	std::push_heap(entries.begin(), entries.end(), std::greater<uint64_t>());

	*cycle = std::min(stop, entries.begin()->t);
}

void EventQueue::Schedule(uint64_t t, std::function<void()> f)
{
	std::unique_lock<std::mutex> l(lock);
	ScheduleNoLock(t, std::move(f));
}

void EventQueue::Expire(uint64_t t)
{
	std::unique_lock<std::mutex> l(lock);
	while(!entries.empty() && entries.front().t <= t) {
		auto f = std::move(entries.front().f);
		std::pop_heap(entries.begin(), entries.end(), std::greater<uint64_t>());
		entries.resize(entries.size() - 1);
		f();
	}
	if(entries.empty())
		*cycle = stop;
	else
		*cycle = std::min(stop, entries.begin()->t);
}

void EventQueue::Start(uint64_t *event_cycle, uint64_t stop_cycle)
{
	cycle = event_cycle;
	stop = stop_cycle;
	if(!entries.empty()) {
		*cycle = std::min(stop_cycle, entries.begin()->t);
	}
}


std::vector<CpuInstruction> Disassembler::Disassemble(const Config& config, uint32_t& canonical_address)
{
	std::vector<CpuInstruction> instructions;
	instructions.reserve(config.max_instruction_count);
	for(uint32_t i = 0; i < config.max_instruction_count; i++) {
		instructions.emplace_back();
		if(!DisassembleOneInstruction(config, canonical_address, instructions.back())) {
			instructions.pop_back();
			break;
		}
	}
	return instructions;
}

std::vector<CpuInstruction> Disassembler::Disassemble(const Config& config, const CpuState *state)
{
	uint32_t addr = state->GetCanonicalAddress();
	return Disassemble(config, addr);
}



void DoPanic(const char *file, int line, const char *function)
{
	printf("Panic hit at %s:%d in %s\n", file, line, function);
#ifdef _DEBUG
	DEBUGBREAK();
#endif
}
