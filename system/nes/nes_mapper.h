#ifndef NES_MAPPER_H_
#define NES_MAPPER_H_

#include <memory>

#include "cpu.h"
#include "rom.h"

namespace nes {

class Mapper
{
public:
	static std::unique_ptr<Mapper> CreateMapper(uint32_t mapper, uint32_t submapper);

	virtual ~Mapper() {}

	virtual void PowerOn() {}
	virtual void Reset() {}
	virtual bool Setup(SystemBus *mainbus, SystemBus *ppubus, std::shared_ptr<Rom> rom) = 0;

	virtual bool IoRead(uint32_t addr, uint8_t *data) { return false; }
	virtual bool IoWrite(uint32_t addr, uint8_t data) { return false; }
};

}

#endif
