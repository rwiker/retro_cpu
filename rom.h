#ifndef ROM_H_
#define ROM_H_

#include "host_system.h"

#include <string>
#include <memory>

class Rom
{
public:
	virtual ~Rom() {}

	virtual std::string GetType() = 0;
	virtual size_t GetSize() = 0;
	virtual const uint8_t* GetRegion(const char *type, uint32_t *size) = 0;
	virtual bool HasConfig(const char *type) = 0;
	virtual bool GetConfig(const char *type, uint32_t *value) = 0;

	static std::shared_ptr<Rom> ReadRom(std::shared_ptr<NativeFile> file);
	static std::shared_ptr<Rom> LoadRom(const uint8_t *data, size_t size);
	static std::shared_ptr<Rom> LoadRom(std::unique_ptr<NativeMemory> memory);
};

#endif
