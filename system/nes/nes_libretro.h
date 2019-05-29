#ifndef SYSTEM_NES_NES_LIBRETRO_H_
#define SYSTEM_NES_NES_LIBRETRO_H_

#include "libretro/libretro_interface.h"

#include "nes.h"

namespace nes {

class NesLibretro : public LibRetroSystem
{
public:
	static LibRetroSystem* Load(const uint8_t *data, size_t size);

	NesLibretro(std::shared_ptr<Rom> rom);

	bool Initialize() override;
	void SetInterface(LibretroInterface *interface) override;
	float GetFramerate() override;
	float GetAudioFrequency() override;
	void GetGeometry(uint32_t *width, uint32_t *maxwidth, uint32_t *height, uint32_t *maxheight) override;
	void Tick(LibretroFramebuffer *fb) override;
	bool IsNtsc() override;
	void Reset() override;
	void UpdateController(uint32_t port, uint32_t device) override;

private:
	void UpdateControllers(NesInputData *input);
	LibretroInterface *itf;
	std::shared_ptr<Rom> rom;
	Nes nes;
};

}

#endif
