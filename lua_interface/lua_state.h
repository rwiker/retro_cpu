#pragma once

#include <mutex>
#include <thread>
#include <string>

struct lua_State;

class LuaState
{
public:
	LuaState();
	~LuaState();

	lua_State* state() { return L; }
	static LuaState* current();

	void ExecFile(const std::string& path);
	void CallRefFunction(int ref);

	void DoInteractiveConsole();

	char* fgets(char *buffer, int max);

private:
	bool GainOwnership();
	void ReleaseOwnership();

	std::thread::id owning_thread;
	std::mutex exec_lock;
	lua_State *L;
};
