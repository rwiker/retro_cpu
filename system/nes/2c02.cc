#include "2c02.h"

#include <memory.h>
#include <assert.h>

namespace nes {

#define RGB(r, g, b) \
	(r * 65536) | (g * 256) | b

const uint32_t global_rgb_palette[64] = {
	RGB(102, 102, 102),
RGB(0, 42, 136),
RGB(20, 18, 168),
RGB(59, 0, 164),
RGB(92, 0, 126),
RGB(110, 0, 64),
RGB(108, 7, 0),
RGB(87, 29, 0),
RGB(52, 53, 0),
RGB(12, 73, 0),
RGB(0, 82, 0),
RGB(0, 79, 8),
RGB(0, 64, 78),
RGB(0, 0, 0),
RGB(0, 0, 0),
RGB(0, 0, 0),
RGB(174, 174, 174),
RGB(21, 95, 218),
RGB(66, 64, 254),
RGB(118, 39, 255),
RGB(161, 27, 205),
RGB(184, 30, 124),
RGB(181, 50, 32),
RGB(153, 79, 0),
RGB(108, 110, 0),
RGB(56, 135, 0),
RGB(13, 148, 0),
RGB(0, 144, 50),
RGB(0, 124, 142),
RGB(0, 0, 0),
RGB(0, 0, 0),
RGB(0, 0, 0),
RGB(254, 254, 254),
RGB(100, 176, 254),
RGB(147, 144, 254),
RGB(199, 119, 254),
RGB(243, 106, 254),
RGB(254, 110, 205),
RGB(254, 130, 112),
RGB(235, 159, 35),
RGB(189, 191, 0),
RGB(137, 217, 0),
RGB(93, 229, 48),
RGB(69, 225, 130),
RGB(72, 206, 223),
RGB(79, 79, 79),
RGB(0, 0, 0),
RGB(0, 0, 0),
RGB(254, 254, 254),
RGB(193, 224, 254),
RGB(212, 211, 254),
RGB(233, 200, 254),
RGB(251, 195, 254),
RGB(254, 197, 235),
RGB(254, 205, 198),
RGB(247, 217, 166),
RGB(229, 230, 149),
RGB(208, 240, 151),
RGB(190, 245, 171),
RGB(180, 243, 205),
RGB(181, 236, 243),
RGB(184, 184, 184),
RGB(0, 0, 0),
RGB(0, 0, 0),
};

void PPU_2C02::PowerOn()
{
	Reset();
}

void PPU_2C02::Reset()
{
	ppu_frame_start_time = 0;
}

void PPU_2C02::Initialize(bool is_ntsc)
{
	this->is_ntsc = is_ntsc;

	if(is_ntsc) {
		num_render_scanlines = 240;
		num_postrender_scanlines = 1;
		num_vblank_scanlines = 20;
		total_scanlines = 262;
	}
	current_scanline = 0;
	current_pixel_clock = 0;

	ppu_cycle_of_next_nmi = 0;

	fb.width = 256;
	fb.height = 224;
	fb.stride = fb.width * 4;
}

uint8_t PPU_2C02::ReadNameTableByte()
{
	//	future.nametable = ((current_scanline -1) / 8) * 16 + (current_pixel_clock - 1) /  8;
	return Read(0x2000 | (vram_addr & 0xFFF));
}
uint8_t PPU_2C02::ReadAttribTableByte()
{
	uint8_t index = ((vram_addr >> 4) & 0x38) | ((vram_addr >> 2) & 7);
	uint8_t b = Read(0x23C0 | (vram_addr & 0xC00) | index);
	if(vram_addr & 0x40)
		b >>= 4;
	if(vram_addr & 0x2)
		b >>= 2;
	return b & 0x3;
}
uint8_t PPU_2C02::ReadBackgroundBit0(uint16_t nt)
{
	return Read(bg_pattern_addr + nt * 16 + ((vram_addr >> 12) & 7));
}
uint8_t PPU_2C02::ReadBackgroundBit1(uint16_t nt)
{
	return Read(bg_pattern_addr + nt * 16 + ((vram_addr >> 12) & 7) + 8);
}

uint8_t PPU_2C02::ReadSpriteBit0(uint16_t idx, uint8_t y)
{
	uint16_t addr;
	if(tall_sprites) {
		addr = (idx & 1) << 12;
		idx &= 0xFE;
		addr += 16 * idx;
		if(y >= 8) {
			return Read(addr + y + 8);
		}
		return Read(addr + y);
	} else {
		return Read(sprite_pattern_addr + 16 * idx + y);
	}
}

uint8_t PPU_2C02::ReadSpriteBit1(uint16_t idx, uint8_t y)
{
	uint16_t addr;
	if(tall_sprites) {
		addr = (idx & 1) << 12;
		idx &= 0xFE;
		addr += 16 * idx;
		if(y >= 8) {
			return Read(addr + y + 16);
		}
		return Read(addr + y + 8);
	} else {
		return Read(sprite_pattern_addr + 16 * idx + y + 8);
	}
}

void PPU_2C02::PpuStep()
{
	if(rendering_enabled && current_scanline == 0 && current_pixel_clock >= 280 && current_pixel_clock <= 304) {
		vram_addr = (vram_addr & 0xC1F) | (reg_t & 0xF3E0);
	}
	switch(state) {
	case FETCH_SCANLINE_START:
		current.high <<= scroll_fine_x;
		current.low <<= scroll_fine_x;
		state = rendering_enabled ? FETCH_NT : RENDER_DISABLED_SCANLINE;
		if(current_scanline == 0) {
			status = 0;
		}
		if(current_scanline == 1 && rendering_enabled && (frame_id & 1)) {
			current_pixel_clock++;
			return PpuStep();
		}
		break;
	case FETCH_NT:
		if(!(current_pixel_clock & 1)) {
			future.nametable = ReadNameTableByte();
			state = FETCH_AT;
		}
		break;
	case FETCH_AT:
		if(!(current_pixel_clock & 1)) {
			future.attribute = ReadAttribTableByte();
			state = FETCH_BG;
		}
		break;
	case FETCH_BG:
		if(!(current_pixel_clock & 1)) {
			future.low = ReadBackgroundBit0(future.nametable);
			state = FETCH_BG2;
		}
		break;
	case FETCH_BG2:
		if(!(current_pixel_clock & 1)) {
			future.high = ReadBackgroundBit1(future.nametable);
			if(current_scanline != 0) {
				DrawPixels();
			}
			if(current_pixel_clock == 256) {
				state = FETCH_BAD_NT;
				if(current_scanline > 0) {
					IncrVert();
					EvaluateSprites();
				}
			} else {
				IncrHoriz();
				state = FETCH_NT;
				next = future;
			}
		}
		break;
	case FETCH_BAD_NT:
		if(!(current_pixel_clock & 1)) {
			ReadNameTableByte();
			state = FETCH_BAD_NT2;
		}
		break;
	case FETCH_BAD_NT2:
		if(!(current_pixel_clock & 1)) {
			ReadNameTableByte();
			state = FETCH_SPRITE_LOW;
		}
		break;
	case FETCH_SPRITE_LOW:
		if(!(current_pixel_clock & 1)) {
			state = FETCH_SPRITE_HIGH;
		}
		break;
	case FETCH_SPRITE_HIGH:
		if(!(current_pixel_clock & 1)) {
			state = (current_pixel_clock == 320) ? FETCH_TAIL_NT : FETCH_BAD_NT;
		}
		break;
	case FETCH_TAIL_NT:
		if(!(current_pixel_clock & 1)) {
			next.nametable = ReadNameTableByte();
			state = FETCH_TAIL_AT;
		}
		break;
	case FETCH_TAIL_AT:
		if(!(current_pixel_clock & 1)) {
			next.attribute = ReadAttribTableByte();
			state = FETCH_TAIL_BG;
		}
		break;
	case FETCH_TAIL_BG:
		if(!(current_pixel_clock & 1)) {
			next.low = ReadBackgroundBit0(next.nametable);
			state = FETCH_TAIL_BG2;
		}
		break;
	case FETCH_TAIL_BG2:
		if(!(current_pixel_clock & 1)) {
			next.high = ReadBackgroundBit1(next.nametable);
			IncrHoriz();
			if(current_pixel_clock == 336) {
				state = END_NT1;
			} else {
				state = FETCH_TAIL_NT;
				current = next;
			}
		}
		break;
	case END_NT1:
		if(!(current_pixel_clock & 1)) {
			state = END_NT2;
		}
		break;
	case RENDER_DISABLED_SCANLINE:
		if(current_pixel_clock != 340)
			break;
	case END_NT2:
		if(!(current_pixel_clock & 1)) {
			state = FETCH_SCANLINE_START;
			current_pixel_clock = 0;
			current_scanline++;
			if(current_scanline > num_render_scanlines) {
				frame_produced = true;
				state = POSTRENDER_SCANLINE;
			}
			return;
		}
		break;
	case POSTRENDER_SCANLINE:
		if(current_scanline == num_render_scanlines + num_postrender_scanlines) {
			if(current_pixel_clock == 1) {
				status |= 0x80;
				frame_id++;
				ppu_cycle_of_next_nmi = last_ppu_cycle + nominal_ppu_frame_time + 2 * ppu_clock_size;
				if(!(frame_id & 1))
					ppu_cycle_of_next_nmi += ppu_clock_size;
			} else if((status & control & 0x80) && current_pixel_clock == 4) {
				// Trigger NMI
				assert_nmi(assert_nmi_context);
			}
		}

		if(++current_pixel_clock == 341) {
			current_pixel_clock = 0;
			if(++current_scanline == total_scanlines) {
				current_scanline = 0;
				state = FETCH_SCANLINE_START;
			}
		}
		return;
	}
	current_pixel_clock++;
}

void PPU_2C02::EvaluateSprites()
{
	uint32_t y = current_scanline - 1;

	num_sprites = 0;

	unsigned index = 0;
	while(index < 253) {
		uint8_t s_y = oam[index];
		uint8_t e_y = s_y + 8;
		if(tall_sprites)
			e_y += 8;

		if(s_y <= y && y < e_y) {
			// This sprite will be drawn
			if(num_sprites < 8) {
				uint8_t idx = oam[index + 1];
				if(oam[index + 2] & 0x80) {
					// Vertically flipped
					sprites[num_sprites].low = ReadSpriteBit0(idx, e_y - y - 1);
					sprites[num_sprites].high = ReadSpriteBit1(idx, e_y - y - 1);
				} else {
					sprites[num_sprites].low = ReadSpriteBit0(idx, y - s_y);
					sprites[num_sprites].high = ReadSpriteBit1(idx, y - s_y);
				}
				sprites[num_sprites].attrib = oam[index + 2] & 0xE3;
				sprites[num_sprites].x = oam[index + 3];
				if(index == 0)
					sprites[num_sprites].attrib |= 0x10;

				num_sprites++;
			}
		} else {
			if(num_sprites == 8) {
				index += 4;
				index = (index & 0xFFC) + ((index + 1) & 3);
				continue;
			}
		}
		index += 4;
	}
}

void PPU_2C02::IncrHoriz()
{
	if((vram_addr & 0x1F) == 0x1F) {
		vram_addr ^= 0x41F;
	} else {
		vram_addr++;
	}
}

void PPU_2C02::IncrVert()
{
	if((vram_addr & 0x7000) != 0x7000) {
		vram_addr += 0x1000;
		vram_addr = (vram_addr & 0xF3E0) | (reg_t & 0xC1F);
	} else {
		vram_addr &= 0xFFF;
		uint32_t y = (vram_addr >> 5) & 0x1F;
		if(y == 29) {
			y = 0;
			vram_addr ^= 0x800;
		} else if(y == 31) {
			y = 0;
		} else {
			y++;
		}
		vram_addr = (vram_addr & 0xF000) | (y << 5) | (reg_t & 0xC1F);
	}
}

void PPU_2C02::DrawPixels()
{
	if(current_pixel_clock > 256)
		return;
	uint32_t x = current_pixel_clock - 8;
	uint32_t y = current_scanline - 1;
	uint8_t *p = fb.video_frame + fb.stride*y + x * 4;

	auto get_color = [&]() -> uint8_t {
		unsigned color = 0;
		if(bg_enabled && (x >= 8 || (mask & 2)))
			color = 4 * current.attribute + (current.low >> 7) | ((current.high >> 6) & 2);
		if(!(color & 3))
			color = 0;
		current.low <<= 1;
		current.high <<= 1;
		if(sprite_enabled && (x >= 8 || (mask & 4))) {
			for(unsigned j = 0; j < num_sprites; j++) {
				unsigned px = x - sprites[j].x;
				if(px < 8) {
					if(sprites[j].attrib & 0x40)
						px = 7 - px;

					unsigned idx = (sprites[j].low >> (7 - px)) & 1;
					if(px == 7)
						idx |= (sprites[j].high << 1) & 2;
					else
						idx |= (sprites[j].high >> (6 - px)) & 2;
					if(idx != 0) {
						// This sprite has a visible pixel here
						if(color && (sprites[j].attrib & 0x10)) {
							// Sprite 0 hit
							status |= 0x40;
						}
						if(!(sprites[j].attrib & 0x20) || !color) {
							return 0x10 + 4 * (sprites[j].attrib & 3) + idx;
						}
						break;
					}
				}
			}
		}
		return color;
	};

	unsigned n = 8 - scroll_fine_x;

	for(unsigned i = 0; i < n; i++, x++, p+=4) {
		uint8_t color = get_color();
		memcpy(p, &rgb_palette[color], 4);
	}

	current = next;
	for(unsigned i = n; i < 8; i++, x++, p+=4) {
		uint8_t color = get_color();
		memcpy(p, &rgb_palette[color], 4);
	}
}

void PPU_2C02::CatchUpToCpu()
{
	uint64_t current_cycle = *cpu_cycle;
	while(last_ppu_cycle < current_cycle) {
		last_ppu_cycle += ppu_clock_size;
		PpuStep();
	}
}

void PPU_2C02::OamDma(SystemBus *bus, uint16_t base, uint32_t rate)
{
	CatchUpToCpu();
	uint64_t cycle = *cpu_cycle + rate / 2;
	for(uint32_t i = 0; i < 256; i++) {
		cycle += bus->ReadByte(base + i, &oam[i]);
	}
}

uint8_t PPU_2C02::ReadPPU(void *self, uint32_t offset, bool peek)
{
	PPU_2C02 *ppu = (PPU_2C02*)self;
	ppu->CatchUpToCpu();
	switch(offset & 7) {
	case 2: // STATUS
		// Clear the address latch
		ppu->latch = (ppu->latch & 0x1F) | ppu->status;
		ppu->status &= 0x7F;
		ppu->addr_write = 0;
		break;
	case 4: // OAMDATA
		ppu->latch = ppu->oam[ppu->oamaddr];
		break;
	case 7: // PPUDATA
		ppu->latch = ppu->ppudata_buffer;
		ppu->ReadByte(ppu->vram_addr, &ppu->ppudata_buffer);
		ppu->vram_addr += ppu->increment;
		if(ppu->vram_addr >= 0x3F00)
			ppu->latch = ppu->Read(ppu->vram_addr);
		break;
	}
	return ppu->latch;
}

void PPU_2C02::WritePPU(void *self, uint32_t addr, uint8_t value)
{
	PPU_2C02 *ppu = (PPU_2C02*)self;
	ppu->CatchUpToCpu();
	ppu->latch = value;

	switch(addr & 7) {
	case 0:
		ppu->control = value;
		ppu->increment = (value & 4) ? 32 : 1;
		ppu->reg_t = (ppu->reg_t & 0xF3FF) | ((uint16_t)(value & 3) << 10);
		ppu->tall_sprites = !!(value & 0x20);
		ppu->sprite_pattern_addr = (!ppu->tall_sprites && (value & 8)) ? 0x1000 : 0;
		ppu->bg_pattern_addr = (value & 0x10) ? 0x1000 : 0;
		break;
	case 1: // PPUMASK
		ppu->mask = value;
		if(ppu->rendering_enabled) {
			if(!(value & 0x18))
				ppu->rendering_enabled = false;
		} else if(value & 0x18) {
			ppu->rendering_enabled = true;
		}
		ppu->bg_enabled = !!(value & 8);
		ppu->sprite_enabled = !!(value & 0x10);
		break;
	case 3:
		if(ppu->rendering_enabled) {
			panic();
		} else {
			ppu->oamaddr = value;
		}
		break;
	case 4:
		if(ppu->rendering_enabled) {
			panic();
		} else {
			ppu->oam[ppu->oamaddr++] = value;
		}
		break;
	case 5:
		if(ppu->addr_write == 0) {
			ppu->reg_t = (ppu->reg_t & 0xFFE0) | ((value >> 3) & 0x1F);
			ppu->scroll_fine_x = value & 7;
		} else {
			ppu->reg_t &= 0x0C1F;
			ppu->reg_t |= (uint16_t)(value & 0xF8) << 2;
			ppu->reg_t |= (uint16_t)(value & 0x7) << 12;
		}
		ppu->addr_write = !ppu->addr_write;
		break;
	case 6:
		if(ppu->addr_write == 0) {
			ppu->reg_t = (ppu->reg_t & 0xFF) | (uint16_t)(value & 0x3F) << 8;
		} else {
			ppu->reg_t = (ppu->reg_t & 0xFF00) | value;
			ppu->vram_addr = ppu->reg_t;
		}
		ppu->addr_write = !ppu->addr_write;
		break;
	case 7:
		ppu->Write(ppu->vram_addr, value);
		ppu->vram_addr += ppu->increment;
		break;
	}
}

uint8_t PPU_2C02::Read(uint32_t addr)
{
	addr &= 0x3FFF;

	if(addr >= 0x3F00) {
		uint16_t b = addr & 0x1F;
		if((b & 0x13) == 0x10)
			b ^= 0x10;
		return palette_indices[b];
	}
	uint8_t v;
	ReadByte(addr, &v);
	return v;
}

void PPU_2C02::Write(uint32_t addr, uint8_t value)
{
	addr &= 0x3FFF;

	if(addr >= 0x3F00) {
		uint16_t b = addr & 0x1F;
		if((b & 0x13) == 0x10)
			b ^= 0x10;
		value &= 0x3F;
		palette_indices[b] = value;
		rgb_palette[b] = global_rgb_palette[value];
	} else {
		WriteByte(addr, value);
	}
}

}
