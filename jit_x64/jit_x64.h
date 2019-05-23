#ifndef JIT_X64_H_
#define JIT_X64_H_

#include "jit.h"
#include "host_system.h"
#include "x64_emitter.h"

#include <unordered_map>

class JitX64 : public JitCoreImpl
{
public:
	struct JitPage
	{
		std::vector<uintptr_t> entrypoints;
		struct Memory
		{
			Memory(std::unique_ptr<NativeMemory> buffer) : buffer(std::move(buffer))
			{
				write_ptr = 0;
				size = this->buffer->GetSize();
				bytes = this->buffer->Pointer();
			}

			std::unique_ptr<NativeMemory> buffer;
			uint32_t size, write_ptr;
			uint8_t *bytes;

			uint32_t write_bytes_left() const
			{
				return size - write_ptr;
			}
		};
		std::vector<std::unique_ptr<Memory>> memory_list;
	};

	JitX64(JittableCpu *cpu, SystemBus *system);

	void Execute() override;
	void InvalidateJit(uint32_t page) override;
	void JitInvalidateForWrite(uint32_t addr) override;

	void EnterJit(uintptr_t entry);
	void JitNewPageAt(uint32_t ip);

	void EmitInstructionPrologue(X64Emitter& e);

	void JitUnjitted();
	void JitDoJitAt(JitPage *page, uint32_t ip);
	int JitFor(JitPage *page, uint32_t ip, const JitOperation *ops, uint8_t *into, uint32_t remaining);
	void BuildOpsList(std::vector<JitOperation>& resolved_ops, const JitOperation *ops);

	JitPage* FindPage(uint32_t mode, uint32_t ip, bool create = false);
	std::unordered_map<cpuaddr_t, JitPage*> jit_pages;

	uint32_t maximum_trace_length = 32;
};

#endif
