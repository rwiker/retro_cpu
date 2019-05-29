#ifndef LIBRETRO_INTERFACE_H_
#define LIBRETRO_INTERFACE_H_

#include <stdint.h>
#include <memory>
#include <vector>
#include <string>

#include "host_system.h"
#include "libretro.h"

// Libretro is an interface that allows an embedder to run many embedded systems.

class LibretroInterface;
class LibRetroSystem;

struct LibretroFramebuffer
{
	uint8_t *data;
	uint32_t width, height, pitch;
};

class LibRetroSystem
{
public:
	virtual ~LibRetroSystem() {}
	virtual bool Initialize() { return true; }
	virtual void SetInterface(LibretroInterface *interface) = 0;
	virtual float GetFramerate() = 0;
	virtual float GetAudioFrequency() = 0;
	virtual void GetGeometry(uint32_t *width, uint32_t *maxwidth, uint32_t *height, uint32_t *maxheight) = 0;
	virtual void Tick(LibretroFramebuffer *fb) = 0;
	virtual bool IsNtsc() = 0;
	virtual void Reset() = 0;
	virtual void UpdateController(uint32_t port, uint32_t device) = 0;

	typedef LibRetroSystem* (*load_fn)(const void *data, size_t size);
	static void Register(std::vector<std::string> extensions, load_fn fn);

	template<typename T>
	static bool RegisterType(std::vector<std::string> extensions)
	{
		Register(std::move(extensions), [](const void *data, size_t size) -> LibRetroSystem* {
			return T::Load(static_cast<const uint8_t*>(data), size);
		});
		return true;
	}
};

class LibretroInterface
{
public:
	LibretroInterface(LibRetroSystem *sys);

	void PollInput();
	static int16_t GetInput(uint32_t port, uint32_t device, uint32_t index, uint32_t id);

	enum PixelFormat
	{
		XRGB_8888,
	};
	void SetPixelFormat(PixelFormat pf);

	void ProduceAudioSample(int16_t l, int16_t r);

	retro_pixel_format pixel_format() const { return current_pf; }


	// Do not call, used by the system implementation
	void Run();
	void FillAvInfo(retro_system_av_info *av);

private:
	void AllocateFramebuffer();
	uint32_t GetBytesPerPixel();

	LibRetroSystem *sys = nullptr;
	retro_pixel_format current_pf = RETRO_PIXEL_FORMAT_XRGB8888;
	bool queried_input = false;
	LibretroFramebuffer current_framebuffer;
	std::unique_ptr<NativeMemory> fb_memory;
};

#endif
