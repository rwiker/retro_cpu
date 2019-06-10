#pragma once

#include "cpu.h"
#include "lua_state.h"
#include "debug_interface.h"

#include <mutex>

class LuaCpu
{
public:
	LuaCpu(DebugInterface *debug);
	~LuaCpu();

	void Push(lua_State *L);


private:
	void SetBreakpointImpl(ptrdiff_t addr, std::function<void(EmulatedCpu*)> fn);
	void ClearBreakpointImpl(ptrdiff_t addr);
	bool PeekImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io, uint8_t& v);
	bool PokeImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io, uint8_t value);

	void PauseImpl(bool schedule_event = true);
	void ResumeImpl();

	void PushState(lua_State *L);

	void OnBreakpoint(LuaState *L, int refid);

	static int GetState(lua_State *L);
	static int SetBreakpoint(lua_State *L);
	static int ClearBreakpoint(lua_State *L);
	static int Peek(lua_State *L);
	static int Poke(lua_State *L);
	static int Pause(lua_State *L);
	static int Resume(lua_State *L);
	static int SingleStep(lua_State *L);
	static int Disassemble(lua_State *L);
	static int Assemble(lua_State *L);
	static int SetRegister(lua_State *L);

	DebugInterface *debug;

};
