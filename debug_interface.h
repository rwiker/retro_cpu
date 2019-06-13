#pragma once

#include <condition_variable>
#include <mutex>

#include "cpu.h"

class DebugInterface
{
public:
	DebugInterface(EmulatedCpu *cpu, EventQueue *events, SystemBus *bus, bool cross_thread);
	~DebugInterface();

	void SetBreakpoint(cpuaddr_t addr, std::function<void(EmulatedCpu*)> fn, bool pause_on_hit = true);
	void ClearBreakpoint(cpuaddr_t addr);

	void SingleStep(uint32_t n_clocks = 1);

	void Pause(bool schedule_event = true);
	void Resume();

	EmulatedCpu* GetCpu() { return cpu; }
	SystemBus* GetBus() { return bus; }
	EventQueue* GetEventQueue() { return events; }

	bool paused() const { return pause != 0; }
	bool recursively_paused() const { return pause > 1; }

protected:
	void PausedFunc();
	void SingleStepFunc();

	SystemBus *bus;
	EventQueue *events;
	EmulatedCpu *cpu;

	int pause = 0;
	bool pause_response = false;
	bool step_pending = false;
	bool breakpoints_pause;
	std::condition_variable pause_var;
	std::condition_variable pause_wait_var;
	std::mutex pause_lock;
};
