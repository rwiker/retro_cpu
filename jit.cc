#include "jit.h"

JitCoreImpl::JitCoreImpl(JittableCpu *cpu, SystemBus *system) : cpu(cpu), system(system)
{
	state = system->cpu->GetCpuState();
	page_size = system->memory.page_size;
	page_mask = page_size - 1;
	inv_page_mask = ~page_mask;
	memory_pages = system->memory.pages;
}

#if PLATFORM_UNKNOWN
JitCoreFactory* JitCoreFactory::Get()
{
	return nullptr;
}
#endif
