#include "jit_x64.h"
#include "x64_emitter.h"

extern "C" void x64EnterJitCode(JitX64*, CpuState*, uintptr_t);
extern "C" void x64Unjitted();

extern "C" void x64UnjittedShim(JitX64 *self)
{
	self->JitUnjitted();
}

enum {
	REG_CPUSTATE = RBX,
	REG_CYCLE = RAX,
	REG_R0 = RCX,
	REG_R1 = RDX,
	REG_R2 = RDI,
	REG_R3 = RSI,
	REG_R4 = R8,
	REG_T0 = R9,
	REG_T1 = R10,
	REG_T2 = R11,
	REG_T3 = R12,
};


class X64Factory : public JitCoreFactory
{
public:
	std::unique_ptr<JitCore> CreateJit(JittableCpu *cpu, SystemBus *system) override
	{
		return std::make_unique<JitX64>(cpu, system);
	}
};
JitCoreFactory* JitCoreFactory::Get()
{
	static X64Factory f;
	return &f;
}

JitX64::JitX64(JittableCpu *cpu, SystemBus *system) : JitCoreImpl(cpu, system) {}

JitX64::JitPage* JitX64::FindPage(uint32_t mode, uint32_t ip, bool create)
{
	cpuaddr_t key = mode | (ip & inv_page_mask);
	auto iter = jit_pages.find(key);
	if(iter == jit_pages.end()) {
		if(!create)
			return nullptr;
		JitPage *p = new JitPage();
		jit_pages.emplace(key, p);
		return p;
	}
	return iter->second;
}

void JitX64::Execute()
{
	cpuaddr_t ip = (state->ip & state->ip_mask) + state->code_segment_base;

	auto page = FindPage(state->mode, ip, false);
	if(page) {
		EnterJit(page->entrypoints[ip & page_mask]);
	} else {
		JitNewPageAt(ip);
		Execute();
	}
}

void JitX64::JitNewPageAt(uint32_t ip)
{
	auto page = FindPage(state->mode, ip, true);
	// Default to having all bytes vector to the unjitted entrypoint
	page->entrypoints.resize(page_size, reinterpret_cast<uintptr_t>(x64Unjitted));
	JitDoJitAt(page, ip);
}

void JitX64::JitUnjitted()
{
	cpuaddr_t ip = (state->ip & state->ip_mask) + state->code_segment_base;
	auto page = FindPage(state->mode, ip, true);
	JitDoJitAt(page, ip);
}

void JitX64::JitDoJitAt(JitPage *page, uint32_t ip)
{
	if(page->memory_list.empty() || page->memory_list.back()->write_bytes_left() < 256) {
		page->memory_list.emplace_back(std::make_unique<JitPage::Memory>(
			NativeMemory::Create(NativeMemory::GetNativeSize())));
#ifdef _DEBUG
		// Fill with int 3 instructions
		memset(page->memory_list.back()->bytes, 0xCC, page->memory_list.back()->size);
#endif
	}
	auto write = page->memory_list.back().get();
	write->buffer->MapForWrite();
	auto ptr = write->bytes + write->write_ptr;
	auto size = write->write_bytes_left();
	for(uint32_t i = 0; i < maximum_trace_length && size > 32; i++) {
		auto ops = cpu->GetJit(this, ip);

		int ret = JitFor(page, ip, ops, ptr, size);
		if(ret < 0) {
			// Failure!
		}
		page->entrypoints[ip & page_mask] = reinterpret_cast<uintptr_t>(ptr);
		ptr += ret;
		size -= ret;
	}
	write->buffer->MapForExecute();
}

void JitX64::BuildOpsList(std::vector<JitOperation>& resolved_ops, const JitOperation *ops)
{
	std::vector<const JitOperation*> lists;
	for(; ops->op != JitOperation::kEnd; ++ops) {
		if(ops->op == JitOperation::kExecuteSublist) {
			if((ops->source_or_imm & kNoDuplicateList) &&
				std::find(lists.begin(), lists.end(), ops) != lists.end()) {
				continue;
			}
			lists.push_back(ops->sublist);
			BuildOpsList(resolved_ops, ops->sublist);
		} else {
			resolved_ops.push_back(*ops);
		}
	}
}

void JitX64::EmitInstructionPrologue(X64Emitter& e)
{
	e.cmp_m_8(REG_CPUSTATE, offsetof(CpuState, interrupts), 3);
	// JA skip
	// CALL do_isr
	// skip:
	// CMP cpustate.cycle, cpustate.cycle_stop
	// JB skip2
	// CALL cycle_interrupt
	// skip2:
	// <opcode bytes>
}

int JitX64::JitFor(JitPage *page, uint32_t ip, const JitOperation *ops, uint8_t *into, uint32_t remaining)
{
	std::vector<JitOperation> resolved_ops;
	BuildOpsList(resolved_ops, ops);
	X64Emitter e(into, remaining);
	EmitInstructionPrologue(e);
	uint32_t instruction_bytes = 0;
	for(auto& op : resolved_ops) {
		switch(op.op) {
		case JitOperation::kClearFlag:
			if(op.destination == kFlagCarry) {
				e.mov_m_imm_8(REG_CPUSTATE, offsetof(CpuState, carry), 0);
			} else {
				__debugbreak();
			}
			break;
		case JitOperation::kIncrementIP:
			__debugbreak();
			break;
		default:
			__debugbreak();
		}
	}

	return 0;
}

void JitX64::EnterJit(uintptr_t entry)
{
	x64EnterJitCode(this, state, entry);
}

void JitX64::InvalidateJit(uint32_t page)
{

}
void JitX64::JitInvalidateForWrite(uint32_t addr)
{

}
