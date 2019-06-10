#ifndef HOST_SYSTEM_H_
#define HOST_SYSTEM_H_

#if _WIN32
#define DEBUGBREAK() __debugbreak()
#define PRETTY_FUNCTION __FUNCSIG__

#if _M_AMD64 == 100
#define PLATFORM_X64 1
#else
#define PLATFORM_UNKNOWN 1
#endif
#else
#define DEBUGBREAK() assert(false)
#define PRETTY_FUNCTION __PRETTY_FUNCTION__
#include <strings.h>
#define _stricmp strcasecmp


#ifdef  __x86_64__ 
#define PLATFORM_X64 1
#else
#define PLATFORM_UNKNOWN 1
#endif
#endif


#include <memory>
#include <string>
#include <vector>
#include <assert.h>
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

class UnownedMemory : public NativeMemory
{
public:
	UnownedMemory(const void *data, size_t size)
		: data(const_cast<uint8_t*>(static_cast<const uint8_t*>(data))), size(size) {}
	uint8_t* Pointer() override { return data; }
	size_t GetSize() override { return size; }
	void MakeReadonly() override {}
	void MapForWrite() override {}
	void MapForExecute() override {}

private:
	uint8_t *data;
	size_t size;
};


#endif
