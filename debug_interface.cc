#include "debug_interface.h"

DebugInterface::DebugInterface(EmulatedCpu *cpu, EventQueue *events, SystemBus *bus, bool cross_thread)
	: bus(bus), events(events), cpu(cpu)
{
	if(!cross_thread)
		pause = 1;
	breakpoints_pause = cross_thread;
}

DebugInterface::~DebugInterface() {}

void DebugInterface::PausedFunc()
{
	std::unique_lock<std::mutex> l(pause_lock);
	pause_response = true;
	pause_wait_var.notify_all();
	while(pause) {
		pause_var.wait(l);
	}
	pause_response = false;
	pause_wait_var.notify_all();
}

void DebugInterface::SingleStepFunc()
{
	{
		std::unique_lock<std::mutex> l(pause_lock);
		step_pending = false;
		pause++;
	}
	PausedFunc();
}

void DebugInterface::Pause(bool schedule_event)
{
	std::unique_lock<std::mutex> l(pause_lock);
	if(pause++)
		return;
	if(schedule_event)
		events->Schedule(0, std::bind(&DebugInterface::PausedFunc, this));
	while(!pause_response) {
		pause_wait_var.wait(l);
	}
}

void DebugInterface::Resume()
{
	std::unique_lock<std::mutex> l(pause_lock);
	if(--pause)
		return;
	pause_var.notify_all();
	while(pause_response && !pause) {
		pause_wait_var.wait(l);
	}
}

void DebugInterface::SingleStep(uint32_t n_clocks)
{
	// The CPU is paused here
	if(!step_pending) {
		step_pending = true;
		events->ScheduleNoLock(cpu->GetCpuState()->cycle + n_clocks,
			std::bind(&DebugInterface::SingleStepFunc, this));
	}
	Resume();

	// Wait for the run thread to pause
	std::unique_lock<std::mutex> l(pause_lock);
	while(!pause_response) {
		pause_wait_var.wait(l);
	}
}

void DebugInterface::SetBreakpoint(cpuaddr_t addr, std::function<void(EmulatedCpu*)> fn, bool pause_on_hit)
{
	std::function<void(EmulatedCpu*)> bp_fn = 
		[this, pause_on_hit, moved_fn{std::move(fn)}](EmulatedCpu *cpu) {
			bool was_stepping;
			{
				std::unique_lock<std::mutex> l(pause_lock);
				was_stepping = step_pending;
				if(breakpoints_pause && !was_stepping)
					++pause;
			}
			if(moved_fn)
				moved_fn(cpu);
			// Note: if the client called Resume() prematurely, this will result in a no-op because
			// Resume will set pause to 0, see pause_response == false and return, and PausedFunc
			// will see pause == 0 and return
			if(pause_on_hit && breakpoints_pause && !was_stepping) {
				PausedFunc();
			} else {
				std::unique_lock<std::mutex> l(pause_lock);
				--pause;
			}
		};
	if(pause)
		cpu->AddBreakpoint(addr, std::move(bp_fn));
	else
		events->Schedule(0, std::bind(&EmulatedCpu::AddBreakpoint, cpu, addr, std::move(bp_fn)));
}

void DebugInterface::ClearBreakpoint(cpuaddr_t addr)
{
	if(pause)
		cpu->RemoveBreakpoint(addr);
	else
		events->Schedule(0, std::bind(&EmulatedCpu::RemoveBreakpoint, cpu, (cpuaddr_t)addr));
}
