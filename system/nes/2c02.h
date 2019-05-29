#ifndef PPU_2C02_H_
#define PPU_2C02_H_

#include "cpu.h"

#include <stdint.h>

#include <vector>

namespace nes {

static constexpr uint32_t ntsc_clocks_per_scanline = 341;
static constexpr uint32_t ntsc_total_scanlines = 262;
static constexpr uint32_t ntsc_visible_scanlines = 240;

static constexpr uint32_t ntsc_clocks_until_vblank = ntsc_clocks_per_scanline * (ntsc_visible_scanlines + 2) + 2;
static constexpr uint32_t ntsc_clocks_per_frame = ntsc_clocks_per_scanline * ntsc_total_scanlines;

static const uint32_t ntsc_cpu_clocks_per_frame = ntsc_clocks_per_frame / 3;

enum PPUSTATE {
	FETCH_SCANLINE_START,

	FETCH_NT,
	FETCH_AT,
	FETCH_BG,
	FETCH_BG2,

	FETCH_BAD_NT,
	FETCH_BAD_NT2,
	FETCH_SPRITE_LOW,
	FETCH_SPRITE_HIGH,

	FETCH_TAIL_NT,
	FETCH_TAIL_AT,
	FETCH_TAIL_BG,
	FETCH_TAIL_BG2,

	END_NT1,
	END_NT2,

	POSTRENDER_SCANLINE,
	RENDER_DISABLED_SCANLINE,
};

struct Framebuffer
{
	uint32_t width, height, stride;
	uint8_t *video_frame;
};

struct PPU_2C02 : public SystemBus
{
	void Initialize(bool is_ntsc);

	void PowerOn();
	void Reset();

	void PpuStep();

	void EvaluateSprites();
	void DrawPixels();

	void IncrHoriz();
	void IncrVert();

	void OamDma(SystemBus *bus, uint16_t base, uint32_t rate);
	void CatchUpToCpu();

	uint8_t Read(uint32_t addr);
	void Write(uint32_t addr, uint8_t value);

	uint8_t ReadNameTableByte();
	uint8_t ReadAttribTableByte();
	uint8_t ReadBackgroundBit0(uint16_t nt);
	uint8_t ReadBackgroundBit1(uint16_t nt);
	uint8_t ReadSpriteBit0(uint16_t idx, uint8_t y);
	uint8_t ReadSpriteBit1(uint16_t idx, uint8_t y);

	void ScheduleNmiNotification();

	static uint8_t ReadPPU(void *self, uint32_t offset, bool peek);
	static void WritePPU(void *self, uint32_t offset, uint8_t value);

	static void PostCpuTick(void *context);

	void (*assert_nmi)(void*);
	void *assert_nmi_context;

	Framebuffer fb;

	EventQueue *event_queue;
	const uint64_t *cpu_cycle;
	uint64_t last_ppu_cycle = 0;
	uint32_t nominal_ppu_frame_time;
	uint64_t ppu_cycle_of_next_nmi;
	uint32_t ppu_clock_size;

	uint32_t num_render_scanlines, num_postrender_scanlines, num_vblank_scanlines, total_scanlines;
	float refresh_rate;

	uint32_t current_scanline = 0;
	uint32_t current_pixel_clock = 0;

	uint32_t tile_x;
	uint32_t tile_y;

	struct tile_data
	{
		union {
			struct {
				uint8_t nametable;
				uint8_t attribute;
				uint8_t low, high;
			};
			uint8_t bytes[4];
			uint32_t u32;
		};
	};

	// Current refers to the tile currently being rendered
	// Next gets loaded when current is exhausted
	// Future is being fetched and populates next when done
	tile_data current, next, future;

	uint8_t palette_indices[32];
	uint32_t rgb_palette[32];

	struct sprite
	{
		union {
			struct {
				uint8_t low;
				uint8_t high;
				uint8_t attrib;
				uint8_t x;
			};
			uint32_t data;
			uint8_t bytes[4];
		};
	} sprites[16];
	unsigned num_sprites = 0;

	PPUSTATE state = FETCH_SCANLINE_START;

	uint8_t latch;
	uint8_t ppudata_buffer = 0;
	uint8_t status = 0;
	uint8_t control = 0;
	uint8_t mask = 0;
	bool bg_enabled;
	bool sprite_enabled;

	uint8_t oamaddr = 0;
	uint16_t vram_addr = 0;

	uint16_t reg_t = 0;
	uint16_t scroll_fine_x = 0;

	uint32_t frame_id = 0;

	struct PendingWrite
	{
		uint64_t clock;
		uint8_t reg;
		uint8_t value;
	};

	union {
		struct {
			uint8_t y;
			uint8_t tile;
			uint8_t attr;
			uint8_t x;
		} typed_oam[64];
		uint8_t oam[256];
	};

	uint16_t sprite_pattern_addr;
	uint16_t bg_pattern_addr;

	uint8_t addr_write = 0;

	uint16_t increment;

	uint32_t x, y;
	uint8_t *pixel_current;

	bool rendering_enabled = false;
	bool tall_sprites = false;
	uint64_t ppu_frame_start_time;
	uint64_t ppu_first_pixel_time;
	uint64_t ppu_current_pixel_clock;
	
	bool is_ntsc = true;
	bool frame_produced = false;
};

}

#endif
