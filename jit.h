#ifndef JIT_H_
#define JIT_H_

#include <memory>

#include <stddef.h>
#include <stdint.h>

#include "cpu.h"

static constexpr uint32_t kFlagZero = 0x10;
static constexpr uint32_t kFlagNegative = 0x20;
static constexpr uint32_t kFlagCarry = 0x40;

static constexpr uint32_t kNoDuplicateList = 0x80;

struct JitOperation
{
	static constexpr uint32_t Bytes(uint32_t n)
	{
		return n << 10;
	}
	enum Oper
	{
		kEnd,
		kExecuteSublist,
		kCustom,
		kInterrupt,
		kMove,
		kAdd,
		kAnd,
		kAndImm,
		kOr,
		kXor,
		kClearFlag,
		kSetFlag,
		kShiftLeftImm,
		kIncrementIP,
		kUpdateFlags,
		kRead,
		kReadNoSegment,
		kReadImm,
		kWrite,
		kWriteNoSegment,
	} op;

	static constexpr uint32_t kMemory = 0x20000000;
	static constexpr uint32_t kTempReg = 0x10000000;
	static constexpr uint32_t kIpReg = 0x40000000;
	static constexpr uint32_t kDataBase = 0x8000000;

	uint32_t destination;
	uint32_t source_or_imm;
	const JitOperation *sublist;
};

class JittableCpu;

class JitCore
{
public:
	virtual ~JitCore() {}

	virtual void Execute() = 0;

	virtual void InvalidateJit(uint32_t page) = 0;

	virtual void JitInvalidateForWrite(uint32_t addr) = 0;
};

class JittableCpu
{
public:
	virtual ~JittableCpu() {}

	virtual const JitOperation* GetJit(JitCore *core, cpuaddr_t addr) = 0;
};

class JitCoreImpl : public JitCore
{
public:
	JitCoreImpl(JittableCpu *cpu, SystemBus *system);

protected:
	JittableCpu *cpu;
	SystemBus *system;
	CpuState *state;
	uint32_t page_size;
	uint32_t page_mask;
	uint32_t inv_page_mask;
	Page *memory_pages;
};

class JitCoreFactory
{
public:
	virtual ~JitCoreFactory() {}
	virtual std::unique_ptr<JitCore> CreateJit(JittableCpu *cpu, SystemBus *system) = 0;

	static JitCoreFactory* Get();
};

#endif
