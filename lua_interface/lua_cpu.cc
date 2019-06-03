#include "lua_cpu.h"

#include "lua.hpp"

LuaCpu::LuaCpu(EmulatedCpu *cpu, EventQueue *events, SystemBus *bus, bool cross_thread)
	: bus(bus), events(events), cpu(cpu)
{
	if(!cross_thread)
		pause = 1;
	breakpoints_pause = cross_thread;
}

LuaCpu::~LuaCpu()
{
}

void LuaCpu::Push(lua_State *L)
{
	*(LuaCpu**)lua_newuserdata(L, sizeof(LuaCpu*)) = this;

	lua_newtable(L);
	lua_newtable(L);
	lua_pushcfunction(L, SetBreakpoint);
	lua_setfield(L, -2, "set_breakpoint");
	lua_pushcfunction(L, ClearBreakpoint);
	lua_setfield(L, -2, "clear_breakpoint");
	lua_pushcfunction(L, Peek);
	lua_setfield(L, -2, "peek");
	lua_pushcfunction(L, Poke);
	lua_setfield(L, -2, "poke");
	lua_pushcfunction(L, Pause);
	lua_setfield(L, -2, "pause");
	lua_pushcfunction(L, Resume);
	lua_setfield(L, -2, "resume");
	lua_pushcfunction(L, GetState);
	lua_setfield(L, -2, "get_state");
	lua_pushcfunction(L, SingleStep);
	lua_setfield(L, -2, "step");
	lua_pushcfunction(L, Disassemble);
	lua_setfield(L, -2, "disas");

	lua_setfield(L, -2, "__index");
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, "__metatable");
	lua_setmetatable(L, -2);
}

void LuaCpu::SetBreakpointImpl(ptrdiff_t addr, std::function<void(EmulatedCpu*)> fn)
{
	events->Schedule(0, std::bind(&EmulatedCpu::AddBreakpoint, cpu, (cpuaddr_t)addr, std::move(fn)));
}
void LuaCpu::ClearBreakpointImpl(ptrdiff_t addr)
{
	events->Schedule(0, std::bind(&EmulatedCpu::RemoveBreakpoint, cpu, (cpuaddr_t)addr));
}

uint8_t LuaCpu::PeekImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io)
{
	uint8_t ret;
	PauseImpl();
	if(access_io)
		bus->ReadByte(addr, &ret);
	else
		bus->ReadByteNoIo(addr, &ret);
	ResumeImpl();
	return ret;
}

void LuaCpu::PokeImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io, ptrdiff_t value)
{
	PauseImpl();
	if(access_io)
		bus->WriteByte(addr, value);
	else
		bus->WriteByteNoIo(addr, value);
	ResumeImpl();
}

void LuaCpu::PausedFunc()
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

void LuaCpu::PauseImpl(bool schedule_event)
{
	if(pause++)
		return;
	std::unique_lock<std::mutex> l(pause_lock);
	if(schedule_event)
		events->Schedule(0, std::bind(&LuaCpu::PausedFunc, this));
	while(!pause_response) {
		pause_wait_var.wait(l);
	}
}

void LuaCpu::ResumeImpl()
{
	if(--pause)
		return;
	std::unique_lock<std::mutex> l(pause_lock);
	pause_var.notify_all();
	while(pause_response) {
		pause_wait_var.wait(l);
	}
}

void LuaCpu::OnBreakpoint(LuaState *L, int refid)
{
	++pause;
	if(refid > 0)
		L->CallRefFunction(refid);
	else
		printf("Breakpoint hit at %X\n", cpu->GetCpuState()->GetCanonicalAddress());
	if(breakpoints_pause)
		PausedFunc();
}

void LuaCpu::PushState(lua_State *L)
{
	auto state = cpu->GetCpuState();
	uint32_t addr = state->GetCanonicalAddress();
	lua_newtable(L);
	lua_pushinteger(L, addr);
	lua_setfield(L, -2, "ip");
	lua_pushinteger(L, state->cycle);
	lua_setfield(L, -2, "cycle");
	lua_pushinteger(L, state->mode);
	lua_setfield(L, -2, "mode");

	auto disas = cpu->GetDisassembler();
	if(disas) {
		Disassembler::Config config;
		config.max_instruction_count = 1;
		auto insn = disas->Disassemble(config, addr);
		lua_pushstring(L, insn[0].asm_string.c_str());
		lua_setfield(L, -2, "asm");
	}

	std::vector<EmulatedCpu::DebugReg> regs;
	if(cpu->GetDebugRegState(regs)) {
		lua_newtable(L);
		for(auto& r : regs) {
			uint64_t mask = (1U << (r.size * 8)) - 1;
			lua_pushinteger(L, r.value & mask);
			lua_setfield(L, -2, r.name);
		}
		lua_setfield(L, -2, "regs");
	}
}

int LuaCpu::ClearBreakpoint(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	luaL_checkint(L, 2);
	auto addr = lua_tointeger(L, 2);
	self->ClearBreakpointImpl(addr);
	return 0;
}
int LuaCpu::SetBreakpoint(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	luaL_checkint(L, 2);
	auto addr = lua_tointeger(L, 2);
	if(lua_isfunction(L, 3)) {
		int ref = luaL_ref(L, 3);
		self->SetBreakpointImpl(addr, std::bind(&LuaCpu::OnBreakpoint, self, LuaState::current(), ref));
		return 0;
	} else {
		self->SetBreakpointImpl(addr, std::bind(&LuaCpu::OnBreakpoint, self, nullptr, 0));
		return 0;
	}
	return luaL_error(L, "SetBreakpoint bad args");
}

int LuaCpu::Peek(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	auto addr = lua_tointeger(L, 2);
	bool access_io = lua_toboolean(L, 3);
	uint8_t result = self->PeekImpl(addr, 1, access_io);
	lua_pushinteger(L, result);
	return 1;
}

int LuaCpu::Poke(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	auto addr = lua_tointeger(L, 2);
	auto v = lua_tointeger(L, 3);
	bool access_io = lua_toboolean(L, 4);
	self->PokeImpl(addr, 1, access_io, v);
	return 0;
}

int LuaCpu::Pause(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	self->PauseImpl();
	self->PushState(L);
	return 1;
}
int LuaCpu::Resume(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	self->ResumeImpl();
	return 0;
}

int LuaCpu::GetState(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	if(!self->pause)
		return luaL_error(L, "LuaCpu: cannot get state unless paused");
	self->PushState(L);
	return 1;
}

int LuaCpu::SingleStep(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	if(!self->pause)
		return luaL_error(L, "LuaCpu: cannot singlestep unless paused");
	if(self->pause != 1)
		return luaL_error(L, "LuaCpu: recursively paused");
	self->events->ScheduleNoLock(self->cpu->GetCpuState()->cycle + 1, std::bind(&LuaCpu::PausedFunc, self));
	self->ResumeImpl();
	self->PauseImpl(false);
	self->PushState(L);
	return 1;
}

int LuaCpu::Disassemble(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	auto disas = self->cpu->GetDisassembler();
	if(!disas)
		return luaL_error(L, "LuaCpu: cpu doesn't implement disassembler");

	cpuaddr_t addr;
	if(lua_isnumber(L, 2)) {
		addr = lua_tointeger(L, 2);
	} else if(!self->pause) {
		return luaL_error(L, "LuaCpu: cannot disassembe from current while paused");
	} else {
		addr = self->cpu->GetCpuState()->GetCanonicalAddress();
	}
	Disassembler::Config config;
	if(lua_isnumber(L, 3))
		config.max_instruction_count = lua_tointeger(L, 3);

	auto list = disas->Disassemble(config, addr);
	lua_newtable(L);
	for(uint32_t i = 0; i < list.size(); i++) {
		lua_pushlstring(L, list[i].asm_string.c_str(), list[i].asm_string.size());
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}
