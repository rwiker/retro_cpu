#pragma once

#include "cpu.h"
#include "lua_state.h"

#include <mutex>

class LuaCpu
{
public:
	LuaCpu(EmulatedCpu *cpu, EventQueue *events, SystemBus *bus, bool cross_thread);
	~LuaCpu();

	void Push(lua_State *L);

private:
	void SetBreakpointImpl(ptrdiff_t addr, std::function<void(EmulatedCpu*)> fn);
	void ClearBreakpointImpl(ptrdiff_t addr);
	uint8_t PeekImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io);
	void PokeImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io, ptrdiff_t value);

	void PauseImpl(bool schedule_event = true);
	void ResumeImpl();

	void PushState(lua_State *L);

	void OnBreakpoint(LuaState *L, int refid);
	void PausedFunc();

	static int GetState(lua_State *L);
	static int SetBreakpoint(lua_State *L);
	static int ClearBreakpoint(lua_State *L);
	static int Peek(lua_State *L);
	static int Poke(lua_State *L);
	static int Pause(lua_State *L);
	static int Resume(lua_State *L);
	static int SingleStep(lua_State *L);
	static int Disassemble(lua_State *L);

	SystemBus *bus;
	EventQueue *events;
	EmulatedCpu *cpu;

	int pause = 0;
	bool pause_response = false;
	bool breakpoints_pause = true;
	std::condition_variable pause_var;
	std::condition_variable pause_wait_var;
	std::mutex pause_lock;
};
