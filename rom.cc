#include "rom.h"

#include "rom_ines.h"

std::shared_ptr<Rom> Rom::ReadRom(std::shared_ptr<NativeFile> file)
{
	auto mmap = NativeMemory::Create(file.get(), 0, 0);
	mmap->MakeReadonly();
	return LoadRom(std::move(mmap));
}

std::shared_ptr<Rom> Rom::LoadRom(const uint8_t *data, size_t size)
{
	auto mem = NativeMemory::Create(size);
	memcpy(mem->Pointer(), data, size);
	return LoadRom(std::move(mem));
}

std::shared_ptr<Rom> Rom::LoadRom(std::unique_ptr<NativeMemory> memory)
{
	uint8_t *data = memory->Pointer();
	size_t size = memory->GetSize();

	if(data[0] == 0x4E && data[1] == 0x45 && data[2] == 0x53 && data[3] == 0x1A) {
		auto ines = std::make_shared<RomInes>(std::move(memory));
		if(!ines->Parse())
			return nullptr;
		return ines;
	}
	return nullptr;
}
