#if _WIN32
#include "host_system.h"

#include <Windows.h>

#include <filesystem>

namespace {
class Mmap : public NativeMemory
{
public:
	Mmap(uint8_t *pointer, size_t size) : ptr(pointer), size(size) {}
	~Mmap()
	{
		VirtualFree(ptr, size, MEM_FREE);
	}
	uint8_t* Pointer() override { return ptr; }
	size_t GetSize() override { return size; }
	void MakeReadonly() override
	{
		if(!writable)
			return;
		writable = false;
		executable = false;
		DWORD n;
		VirtualProtect(ptr, size, PAGE_READONLY, &n);
	}
	void MapForWrite() override
	{
		if(writable)
			return;
		writable = true;
		executable = false;
		DWORD n;
		VirtualProtect(ptr, size, PAGE_READWRITE, &n);
	}
	void MapForExecute() override
	{
		if(executable)
			return;
		executable = true;
		writable = false;
		DWORD n;
		VirtualProtect(ptr, size, PAGE_EXECUTE_READ, &n);
	}

private:
	uint8_t *ptr;
	size_t size;
	bool writable = true;
	bool executable = false;
};

class FileMap : public NativeMemory
{
public:
	FileMap(HANDLE mapping, size_t offset, size_t size) : size(size), mapping(mapping)
	{
		uint32_t high = 0;
		if constexpr(sizeof(offset) > 4)
			high >>= 32;
		ptr = (uint8_t*)MapViewOfFile(mapping, FILE_MAP_READ, high, offset & 0xFFFFFFFF, size);
	}
	~FileMap()
	{
		UnmapViewOfFile(ptr);
		CloseHandle(mapping);
	}

	uint8_t* Pointer() override { return ptr; }
	size_t GetSize() override { return size; }
	void MakeReadonly() override
	{
	}
	void MapForWrite() override
	{
	}
	void MapForExecute() override
	{
	}

private:
	uint8_t *ptr;
	size_t size;
	HANDLE mapping;
};

class File : public NativeFile
{
public:
	File(HANDLE fd) : fd_(fd) {}
	~File() { CloseHandle(fd_); }
	HANDLE fd() const { return fd_; }

	std::vector<uint8_t> ReadToVector() override
	{
		LARGE_INTEGER size;
		GetFileSizeEx(fd_, &size);
		std::vector<uint8_t> data;
		data.resize((size_t)size.QuadPart);
		DWORD n;
		ReadFile(fd_, data.data(), (DWORD)data.size(), &n, nullptr);

		return data;
	}

private:
	HANDLE fd_;
};
}

std::unique_ptr<NativeMemory> NativeMemory::Create(size_t size)
{
	void *mem = VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
	if(!mem)
		return nullptr;
	return std::make_unique<Mmap>((uint8_t*)mem, size);
}

std::unique_ptr<NativeMemory> NativeMemory::Create(NativeFile *file, size_t offset, size_t size)
{
	auto f = static_cast<File*>(file)->fd();
	if(size == 0) {
		LARGE_INTEGER li;
		GetFileSizeEx(f, &li);
		size = li.QuadPart;
	}
	auto mapping = CreateFileMapping(f, NULL, PAGE_WRITECOPY, 0, 0, NULL);
	if(!mapping)
		return nullptr;
	return std::make_unique<FileMap>(mapping, offset, size);
}

size_t NativeMemory::GetNativeSize()
{
	return 0x10000;
}

std::shared_ptr<NativeFile> NativeFile::Open(const std::string& string)
{
	std::experimental::filesystem::path p(string);
	auto winstr = p.native().c_str();

	HANDLE file = CreateFileW(winstr, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if(file == INVALID_HANDLE_VALUE)
		return nullptr;
	return std::make_unique<File>(file);
}

#endif
