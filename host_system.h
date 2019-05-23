#ifndef HOST_SYSTEM_H_
#define HOST_SYSTEM_H_

#include <memory>
#include <string>
#include <vector>
#include <stddef.h>
#include <stdint.h>

class NativeFile
{
public:
	virtual ~NativeFile() {}

	virtual std::vector<uint8_t> ReadToVector() = 0;

	static std::shared_ptr<NativeFile> Open(const std::string& string);
};

// mmap() on linux, VirtualAlloc() on Windows, etc
// This is a block of contiguous virtual memory that can potentially be remapped.
class NativeMemory
{
public:
	virtual ~NativeMemory() {}
	virtual uint8_t* Pointer() = 0;
	virtual size_t GetSize() = 0;
	virtual void MakeReadonly() = 0;
	virtual void MapForWrite() = 0;
	virtual void MapForExecute() = 0;

	static std::unique_ptr<NativeMemory> Create(size_t size);
	static std::unique_ptr<NativeMemory> Create(NativeFile *file, size_t offset, size_t size);

	static size_t GetNativeSize();
};


#endif
