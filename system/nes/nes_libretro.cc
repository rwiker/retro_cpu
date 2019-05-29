#include "nes_libretro.h"

namespace nes {
namespace {
bool reg = LibRetroSystem::RegisterType<NesLibretro>(std::vector<std::string>{"nes"});
}

LibRetroSystem* NesLibretro::Load(const uint8_t *data, size_t size)
{
	auto rom = Rom::LoadRom(data, size);
	if(rom->GetType() == "nes")
		return new NesLibretro(rom);
	return nullptr;
}

NesLibretro::NesLibretro(std::shared_ptr<Rom> rom) : rom(rom) {}

void NesLibretro::UpdateControllers(NesInputData *input)
{
	itf->PollInput();
	// Input byte LSB A B select start U D L R MSB
	static constexpr int buttons[] = {
		RETRO_DEVICE_ID_JOYPAD_A,
		RETRO_DEVICE_ID_JOYPAD_B,
		RETRO_DEVICE_ID_JOYPAD_SELECT,
		RETRO_DEVICE_ID_JOYPAD_START,
		RETRO_DEVICE_ID_JOYPAD_UP,
		RETRO_DEVICE_ID_JOYPAD_DOWN,
		RETRO_DEVICE_ID_JOYPAD_LEFT,
		RETRO_DEVICE_ID_JOYPAD_RIGHT,
	};
	for(int i = 0; i < 2; i++) {
		uint8_t byte = 0;
		for(int j = 0; j < 8; j++) {
			auto v = itf->GetInput(i, RETRO_DEVICE_JOYPAD, 0, buttons[j]);
			if(v)
				byte |= 1U << j;
		}
		input->devices[i].serial_bits[0] = byte;
		for(int j = 0; j < 4; j++)
			input->devices[i].serial_bits[j+1] = 0;
	}
}

bool NesLibretro::Initialize()
{
	if(!nes.LoadRom(rom))
		return false;
	nes.SetUpdateControllersFunc(std::bind(&NesLibretro::UpdateControllers, this, std::placeholders::_1));
	return true;
}
void NesLibretro::SetInterface(LibretroInterface *interface)
{
	itf = interface;
}
float NesLibretro::GetFramerate()
{
	return 60.0988f;
}

float NesLibretro::GetAudioFrequency()
{
	return 0;
}
void NesLibretro::GetGeometry(uint32_t *width, uint32_t *maxwidth, uint32_t *height, uint32_t *maxheight)
{
	*width = *maxwidth = 256;
	*height = *maxheight = 224;
}
void NesLibretro::Tick(LibretroFramebuffer *fb)
{
	nes::Framebuffer nes_fb;

	nes_fb.height = fb->height;
	nes_fb.width = fb->width;
	nes_fb.stride = fb->pitch;
	nes_fb.video_frame = fb->data;
//	memset(fb->data, 0, fb->pitch * fb->height);
	nes.RunForOneFrame(&nes_fb);
}
bool NesLibretro::IsNtsc()
{
	return nes.is_ntsc();
}
void NesLibretro::Reset()
{
	nes.Reset();
}
void NesLibretro::UpdateController(uint32_t port, uint32_t device)
{
}

}
