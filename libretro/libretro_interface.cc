#include "libretro.h"
#include "libretro_interface.h"

#include "host_system.h"

#include <malloc.h>

// Implemented by the embedder. retro_set_* will be called
namespace {
retro_environment_t g_env;
retro_video_refresh_t g_refresh;
retro_audio_sample_t g_sample;
retro_audio_sample_batch_t g_sample_batch;
retro_input_poll_t g_input_poll;
retro_input_state_t g_input_state;

LibretroInterface *g_itf;
LibRetroSystem *g_sys;

struct CoreInfo {
	std::string ext_list;
	std::vector<LibRetroSystem::load_fn> load_fns;
} *reg_info;
}

void retro_set_environment(retro_environment_t e)
{
	bool v = true;
	g_env = e;
	g_env(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &v);
}
void retro_set_video_refresh(retro_video_refresh_t e) { g_refresh = e; }
void retro_set_audio_sample(retro_audio_sample_t e) { g_sample = e; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t e) { g_sample_batch = e; }
void retro_set_input_poll(retro_input_poll_t e) { g_input_poll = e; }
void retro_set_input_state(retro_input_state_t e) { g_input_state = e; }

void retro_init()
{
}

void retro_deinit()
{
}

unsigned retro_api_version()
{
	return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name = "MultiCoreEmulator";
	info->library_version = "0.1";
	info->valid_extensions = reg_info ? reg_info->ext_list.c_str() : "";
	info->need_fullpath = false;
	info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	if(!g_itf)
		return;
	retro_pixel_format format = g_itf->pixel_format();
	g_env(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format);

	g_itf->FillAvInfo(info);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	g_sys->UpdateController(port, device);
}

void retro_reset()
{
	g_sys->Reset();
}

void retro_run()
{
	g_itf->Run();
}

size_t retro_serialize_size()
{
	return 0;
}

bool retro_serialize(void *data, size_t size) { return false; }

bool retro_unserialize(const void *data, size_t size) { return false; }

void retro_cheat_reset() {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}


bool retro_load_game(const struct retro_game_info *game)
{
	if(!reg_info)
		return false;

	for(auto& fn : reg_info->load_fns) {
		auto sys = fn(game->data, game->size);
		if(sys) {
			g_sys = sys;
			g_itf = new LibretroInterface(sys);
			g_sys->SetInterface(g_itf);
			if(!g_sys->Initialize()) {
				delete g_sys;
				delete g_itf;
				g_sys = nullptr;
				g_itf = nullptr;
				continue;
			}
			return true;
		}
	}
	return false;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	return false;
}

void retro_unload_game()
{
	delete g_sys;
	delete g_itf;
	g_sys = nullptr;
	g_itf = nullptr;
}

unsigned retro_get_region()
{
	return (!g_sys || g_sys->IsNtsc()) ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

void *retro_get_memory_data(unsigned id)
{
	return nullptr;
}

size_t retro_get_memory_size(unsigned id)
{
	return 0;
}

void LibRetroSystem::Register(std::vector<std::string> extensions, load_fn fn)
{
	if(!reg_info)
		reg_info = new CoreInfo();
	for(auto& e : extensions) {
		if(!reg_info->ext_list.empty())
			reg_info->ext_list.append("|");
		reg_info->ext_list.append(e);
	}
	reg_info->load_fns.push_back(fn);
}

LibretroInterface::LibretroInterface(LibRetroSystem *sys) : sys(sys)
{
	current_framebuffer.data = nullptr;
}

void LibretroInterface::SetPixelFormat(PixelFormat pf)
{
	switch(pf) {
	case XRGB_8888:
		current_pf = RETRO_PIXEL_FORMAT_XRGB8888;
		break;
	default:
		break;
	}
}

void LibretroInterface::AllocateFramebuffer()
{
	fb_memory = NativeMemory::Create(240 * current_framebuffer.pitch);
	current_framebuffer.data = fb_memory->Pointer();
}

uint32_t LibretroInterface::GetBytesPerPixel()
{
	return 4;
}

void LibretroInterface::FillAvInfo(retro_system_av_info *av)
{
	av->timing.fps = sys->GetFramerate();
	av->timing.sample_rate = sys->GetAudioFrequency();
	uint32_t w, mw, h, mh;
	sys->GetGeometry(&w, &mw, &h, &mh);
	av->geometry.base_height = h;
	av->geometry.base_width = w;
	av->geometry.max_height = mh;
	av->geometry.max_width = mw;
	av->geometry.aspect_ratio = 0.0f;

	current_framebuffer.width = w;
	current_framebuffer.height = h;
	current_framebuffer.pitch = w * GetBytesPerPixel();
	AllocateFramebuffer();
}

int16_t LibretroInterface::GetInput(uint32_t port, uint32_t device, uint32_t index, uint32_t id)
{
	return g_input_state(port, device, index, id);
}

void LibretroInterface::PollInput()
{
	queried_input = true;
	g_input_poll();
}

void LibretroInterface::Run()
{
	queried_input = false;
	sys->Tick(&current_framebuffer);
	if(!queried_input)
		g_input_poll();
	g_refresh(current_framebuffer.data + current_framebuffer.pitch * 8,
		current_framebuffer.width, current_framebuffer.height, current_framebuffer.pitch);
}
