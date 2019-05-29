#include <memory>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "host_system.h"

namespace {
class Mmap : public NativeMemory
{
public:
	Mmap(uint8_t *pointer, size_t size) : ptr(pointer), size(size) {}
	~Mmap()
	{
		munmap(ptr, size);
	}
	uint8_t* Pointer() override { return ptr; }
	size_t GetSize() override { return size; }
	void MakeReadonly() override
	{
		if(!writable)
			return;
		writable = false;
		executable = false;
		mprotect(ptr, size, PROT_READ);
	}
	void MapForWrite() override
	{
		if(writable)
			return;
		mprotect(ptr, size, PROT_READ|PROT_WRITE);
		writable = true;
		executable = false;
	}
	void MapForExecute() override
	{
		if(executable)
			return;
		executable = true;
		writable = false;
		mprotect(ptr, size, PROT_READ|PROT_EXEC);
	}

private:
	uint8_t *ptr;
	size_t size;
	bool writable = true;
	bool executable = false;
};

class File : public NativeFile
{
public:
	File(int fd) : fd_(fd) {}
	~File() { close(fd_); }
	int fd() const { return fd_; }

	std::vector<uint8_t> ReadToVector() override
	{
		std::vector<uint8_t> ret;
		struct stat s;
		if(fstat(fd_, &s))
			return ret;
		ret.resize(s.st_size);
		read(fd_, ret.data(), s.st_size);
		return ret;
	}

private:
	int fd_;
};
}

std::unique_ptr<NativeMemory> NativeMemory::Create(size_t size)
{
	void *mem = mmap(nullptr, size, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if(mem == (void*)-1LL)
		return nullptr;
	return std::make_unique<Mmap>((uint8_t*)mem, size);
}

std::unique_ptr<NativeMemory> NativeMemory::Create(NativeFile *file, size_t offset, size_t size)
{
	void *mem = mmap(nullptr, size, PROT_WRITE|PROT_READ, MAP_PRIVATE,
		static_cast<File*>(file)->fd(), offset);
	if(mem == (void*)-1LL)
		return nullptr;
	return std::make_unique<Mmap>((uint8_t*)mem, size);
}

size_t NativeMemory::GetNativeSize()
{
	return 0x1000;
}
