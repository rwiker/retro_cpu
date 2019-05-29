#include "nes_mapper.h"

namespace nes {

class Mapper0 : public Mapper
{
public:
	bool Setup(SystemBus *mainbus, SystemBus *ppubus, std::shared_ptr<Rom> rom) override
	{
		uint32_t prg_rom_size;
		uint32_t chr_rom_size;

		auto prg_rom = rom->GetRegion("PRG ROM", &prg_rom_size);
		auto chr_rom = rom->GetRegion("CHR ROM", &chr_rom_size);

		if(!prg_rom || !chr_rom)
			return false;

		uint32_t vram_config = 0;
		rom->GetConfig("VRAM CONFIG", &vram_config);
		vram = NativeMemory::Create(0x1000);
		// Map VRAM in mirroring modes
		if(vram_config == 0) {
			// vertical mirror
			ppubus->Map(0x2000, vram->Pointer(), 0x400);
			ppubus->Map(0x2400, vram->Pointer() + 0x400, 0x400);
			ppubus->Map(0x2800, vram->Pointer(), 0x400);
			ppubus->Map(0x2C00, vram->Pointer() + 0x400, 0x400);
			ppubus->Map(0x3000, vram->Pointer(), 0x400);
			ppubus->Map(0x3400, vram->Pointer() + 0x400, 0x400);
			ppubus->Map(0x3800, vram->Pointer(), 0x400);
			ppubus->Map(0x3C00, vram->Pointer() + 0x400, 0x400);
		} else if(vram_config == 1) {
			// horizontal mirror
			ppubus->Map(0x2000, vram->Pointer(), 0x400);
			ppubus->Map(0x2400, vram->Pointer(), 0x400);
			ppubus->Map(0x2800, vram->Pointer() + 0x400, 0x400);
			ppubus->Map(0x2C00, vram->Pointer() + 0x400, 0x400);
			ppubus->Map(0x3000, vram->Pointer(), 0x400);
			ppubus->Map(0x3400, vram->Pointer(), 0x400);
			ppubus->Map(0x3800, vram->Pointer() + 0x400, 0x400);
			ppubus->Map(0x3C00, vram->Pointer() + 0x400, 0x400);
		} else if(vram_config == 2) {
			// quad screen
			ppubus->Map(0x2000, vram->Pointer(), 0x1000);
			ppubus->Map(0x3000, vram->Pointer(), 0x1000);
		}

		// Map CHR ROM
		for(uint32_t addr = 0; addr < 0x2000; addr += chr_rom_size) {
			ppubus->Map(addr, const_cast<uint8_t*>(chr_rom), chr_rom_size, true);
		}

		// Map PRG ROM
		for(uint32_t addr = 0x8000; addr < 0x10000; addr += prg_rom_size) {
			mainbus->Map(addr, const_cast<uint8_t*>(prg_rom), prg_rom_size, true);
		}
		return true;
	}
	std::unique_ptr<NativeMemory> vram;
};

std::unique_ptr<Mapper> Mapper::CreateMapper(uint32_t mapper, uint32_t submapper)
{
	switch(mapper) {
	case 0:
		return std::make_unique<Mapper0>();
	default:
		return nullptr;
	}
}

}
