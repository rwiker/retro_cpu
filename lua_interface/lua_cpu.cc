#include "lua_cpu.h"

#include "lua.hpp"

LuaCpu::LuaCpu(DebugInterface *debug) : debug(debug)
{
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
	lua_setfield(L, -2, "disassemble");
	lua_pushcfunction(L, Assemble);
	lua_setfield(L, -2, "assemble");
	lua_pushcfunction(L, SetRegister);
	lua_setfield(L, -2, "set_reg");

	lua_setfield(L, -2, "__index");
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, "__metatable");
	lua_setmetatable(L, -2);
}

void LuaCpu::SetBreakpointImpl(ptrdiff_t addr, std::function<void(EmulatedCpu*)> fn)
{
	debug->SetBreakpoint((cpuaddr_t)addr, std::move(fn));
}
void LuaCpu::ClearBreakpointImpl(ptrdiff_t addr)
{
	debug->ClearBreakpoint((cpuaddr_t)addr);
}

bool LuaCpu::PeekImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io, uint8_t& v)
{
	bool ok = false;
	PauseImpl();
	if(access_io || !debug->GetBus()->QueryIo((cpuaddr_t)addr)) {
		debug->GetBus()->ReadByte((cpuaddr_t)addr, &v);
		ok = true;
	}
	ResumeImpl();
	return ok;
}

bool LuaCpu::PokeImpl(ptrdiff_t addr, uint32_t nbytes, bool access_io, uint8_t value)
{
	bool ok = false;
	PauseImpl();
	if(access_io || !debug->GetBus()->QueryIo((cpuaddr_t)addr)) {
		debug->GetBus()->WriteByte((cpuaddr_t)addr, value);
		ok = true;
	}
	ResumeImpl();
	return ok;
}

void LuaCpu::PauseImpl(bool schedule_event)
{
	debug->Pause(schedule_event);
}

void LuaCpu::ResumeImpl()
{
	debug->Resume();
}

void LuaCpu::OnBreakpoint(LuaState *L, int refid)
{
	if(refid > 0)
		L->CallRefFunction(refid);
	else
		printf("Breakpoint hit at %X\n", debug->GetCpu()->GetCpuState()->GetCanonicalAddress());
}

void LuaCpu::PushState(lua_State *L)
{
	auto state = debug->GetCpu()->GetCpuState();
	uint32_t addr = state->GetCanonicalAddress();
	lua_newtable(L);
	lua_pushinteger(L, addr);
	lua_setfield(L, -2, "ip");
	lua_pushinteger(L, state->cycle);
	lua_setfield(L, -2, "cycle");
	lua_pushinteger(L, state->mode);
	lua_setfield(L, -2, "mode");

	auto disas = debug->GetCpu()->GetDisassembler();
	if(disas) {
		Disassembler::Config config;
		config.max_instruction_count = 1;
		auto insn = disas->Disassemble(config, addr);
		lua_pushstring(L, insn[0].asm_string.c_str());
		lua_setfield(L, -2, "asm");
	}

	std::vector<EmulatedCpu::DebugReg> regs;
	if(debug->GetCpu()->GetDebugRegState(regs)) {
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
	uint8_t result;
	if(!self->PeekImpl(addr, 1, access_io, result)) {
		lua_pushnil(L);
		lua_pushliteral(L, "LuaCpu: Would access I/O");
		return 2;
	}
	lua_pushinteger(L, result);
	return 1;
}

int LuaCpu::Poke(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	auto addr = lua_tointeger(L, 2);
	const uint8_t *data;
	size_t n;
	uint8_t v;
	if(lua_isnumber(L, 3)) {
		v = (uint8_t)lua_tonumber(L, 3);
		n = 1;
		data = &v;
	} else if(lua_isstring(L, 3)) {
		data = (const uint8_t*)lua_tolstring(L, 3, &n);
	}
	bool access_io = lua_toboolean(L, 4);
	for(size_t i = 0; i < n; i++) {
		if(!self->PokeImpl(addr + i, 1, access_io, data[i])) {
			lua_pushnil(L);
			lua_pushliteral(L, "LuaCpu: Would access I/O");
			return 2;
		}
	}
	lua_pushboolean(L, 1);
	return 1;
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
	if(!self->debug->paused())
		return luaL_error(L, "LuaCpu: cannot get state unless paused");
	self->PushState(L);
	return 1;
}

int LuaCpu::SingleStep(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	if(!self->debug->paused())
		return luaL_error(L, "LuaCpu: cannot singlestep unless paused");
	if(self->debug->recursively_paused())
		return luaL_error(L, "LuaCpu: recursively paused");
	self->debug->SingleStep();
	self->PushState(L);
	return 1;
}

int LuaCpu::Disassemble(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	auto disas = self->debug->GetCpu()->GetDisassembler();
	if(!disas)
		return luaL_error(L, "LuaCpu: cpu doesn't implement disassembler");

	cpuaddr_t addr;
	if(lua_isnumber(L, 2)) {
		addr = (cpuaddr_t)lua_tointeger(L, 2);
	} else if(!self->debug->paused()) {
		return luaL_error(L, "LuaCpu: cannot disassembe from current while paused");
	} else {
		addr = self->debug->GetCpu()->GetCpuState()->GetCanonicalAddress();
	}
	Disassembler::Config config;
	if(lua_isnumber(L, 3))
		config.max_instruction_count = (uint32_t)lua_tointeger(L, 3);

	auto list = disas->Disassemble(config, addr);
	lua_newtable(L);
	for(uint32_t i = 0; i < list.size(); i++) {
		lua_pushlstring(L, list[i].asm_string.c_str(), list[i].asm_string.size());
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

int LuaCpu::Assemble(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	auto as = self->debug->GetCpu()->GetAssembler();
	if(!as)
		return luaL_error(L, "LuaCpu: cpu doesn't implement assembler");

	std::string error;
	std::vector<uint8_t> bytes;
	const char *str = lua_tostring(L, 2);
	if(!str)
		return luaL_error(L, "LuaCpu: need string to assemble");
	if(bytes.empty()) {
		lua_pushnil(L);
		lua_pushliteral(L, "LuaCpu: didn't assemble to any bytes");
		return 2;
	}
	if(!as->Assemble(str, error, bytes)) {
		lua_pushnil(L);
		lua_pushlstring(L, error.c_str(), error.size());
		return 2;
	}
	lua_pushlstring(L, (const char*)bytes.data(), bytes.size());
	return 1;
}

int LuaCpu::SetRegister(lua_State *L)
{
	if(!lua_isuserdata(L, 1))
		return luaL_error(L, "LuaCpu: must pass userdata pointer as arg #1");
	auto self = *(LuaCpu**)lua_touserdata(L, 1);
	if(!self->debug->paused())
		return luaL_error(L, "LuaCpu: cannot modify registers unless paused");
	const char *reg = luaL_checkstring(L, 2);
	auto v = luaL_checkinteger(L, 3);

	auto state = self->debug->GetCpu()->GetCpuState();
	lua_pushboolean(L, self->debug->GetCpu()->SetRegister(reg, v));

	return 1;
}
