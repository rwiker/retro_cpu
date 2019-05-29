#ifndef ROM_INES_H_
#define ROM_INES_H_

#include "rom.h"

struct INES
{
	char hdr[4];
	uint8_t n_prg_rom;
	uint8_t n_chr_rom;
	uint8_t flags6;
	uint8_t flags7;
	uint8_t flags8;
	uint8_t flags9;
	uint8_t flags10;
	uint8_t flags11;
	uint8_t flags12;
};

class RomInes : public Rom
{
public:
	enum VramConfig : uint32_t {
		VERTICAL_MIRROR, HORIZONTAL_MIRROR, QUAD_SCREEN
	};
	RomInes(std::unique_ptr<NativeMemory> memory) : memory(std::move(memory)) {}

	bool Parse()
	{
		uint8_t *data = memory->Pointer();
		INES *hdr = (INES*)data;
		data += 16;
		has_trainer = !!(hdr->flags6 & 4);
		if(has_trainer) {
			memcpy(trainer, data, 512);
			data += 512;
		}
		bool ines2 = (hdr->flags7 & 0xC) == 8;
		mapper_num = (hdr->flags7 & 0xF0) | (hdr->flags6 >> 4);
		submapper = 0;

		chr_rom_size = hdr->n_chr_rom;
		prg_rom_size = hdr->n_prg_rom;
		if(ines2) {
			mapper_num = ((uint16_t)(hdr->flags8 & 0xF) << 8);
			submapper = hdr->flags8 >> 4;

			prg_rom_size |= (uint16_t)(hdr->flags9 & 0xF) << 8;
			chr_rom_size |= (uint16_t)(hdr->flags9 & 0xF0) << 4;
			auto get_size = [](uint8_t shift) {
				if(!shift)return 0;
				return 1 << (shift + 6);
			};
			ram_size = get_size(hdr->flags10 & 0xF);
			ram_size_backed = get_size((hdr->flags10 >> 4) & 0xF);
			chr_ram_size = get_size(hdr->flags11 & 0xF);
			chr_ram_size_backed = get_size((hdr->flags11 >> 4) & 0xF);

			system = hdr->flags12 & 3;
		} else {
			ram_size = 8192 * hdr->flags8;
			if(ram_size == 0)
				ram_size = 8192;
			ram_size_backed = 0;

			// We assume that everything is NTSC here
			system = 0;
		}
		chr_rom_size <<= 13;
		prg_rom_size <<= 14;

		if(hdr->flags6 & 0x8) {
			vram_config = QUAD_SCREEN;
		} else if(hdr->flags6 & 1) {
			vram_config = VERTICAL_MIRROR;
		} else {
			vram_config = HORIZONTAL_MIRROR;
		}

		prg_rom = data;
		data += prg_rom_size;
		chr_rom = data;
		data += chr_rom_size;

		return true;
	}

	std::string GetType() { return "nes"; }
	size_t GetSize() { return memory->GetSize(); }
	const uint8_t* GetRegion(const char *type, uint32_t *size)
	{
		if(!strcmp(type, "CHR ROM")) {
			*size = chr_rom_size;
			return chr_rom;
		}
		if(!strcmp(type, "PRG ROM")) {
			*size = prg_rom_size;
			return prg_rom;
		}
		if(has_trainer && !strcmp(type, "TRAINER")) {
			*size = 512;
			return trainer;
		}
		*size = 0;
		return nullptr;
	}
	bool HasConfig(const char *type) override
	{
		return true;
	}
	bool GetConfig(const char *type, uint32_t *value) override
	{
		if(!strcmp(type, "PRG RAM SIZE")) {
			*value = ram_size;
			return true;
		}
		if(!strcmp(type, "PRG NVRAM SIZE")) {
			*value = ram_size_backed;
			return true;
		}
		if(!strcmp(type, "CHR RAM SIZE")) {
			*value = chr_ram_size;
			return true;
		}
		if(!strcmp(type, "CHR NVRAM SIZE")) {
			*value = chr_ram_size_backed;
			return true;
		}
		if(!strcmp(type, "PRG NVRAM SIZE")) {
			*value = ram_size_backed;
			return true;
		}
		if(!strcmp(type, "PRG NVRAM SIZE")) {
			*value = ram_size_backed;
			return true;
		}
		if(!strcmp(type, "PRG NVRAM SIZE")) {
			*value = ram_size_backed;
			return true;
		}
		if(!strcmp(type, "DISPLAY")) {
			*value = system;
			return true;
		}
		if(!strcmp(type, "VRAM CONFIG")) {
			*value = vram_config;
			return true;
		}
		if(!strcmp(type, "MAPPER")) {
			*value = (mapper_num << 8) | submapper;
			return true;
		}
		return false;
	}

private:
	std::unique_ptr<NativeMemory> memory;

	uint8_t *chr_rom, *prg_rom;
	uint32_t chr_rom_size, prg_rom_size;

	uint32_t ram_size = 0, ram_size_backed = 0;
	uint32_t chr_ram_size = 0, chr_ram_size_backed = 0;

	uint32_t mapper_num, submapper;

	VramConfig vram_config;
	int system;

	bool has_trainer;
	uint8_t trainer[512];
};

#endif
