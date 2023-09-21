#include "../../base.hh"
#include <cstdint>
#include <cstring>
#include <cassert>
#include <memory_resource>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#include <unistd.h>

static size_t _PAGESIZE = 0;

class LinuxCodePage : public slake::ICodePage {
public:
	char *ptr;
	size_t size;
	bool firmed = false;

	inline LinuxCodePage(size_t size) : ptr(ptr), size(size) {
		struct sysinfo info;
		sysinfo(&info);
		if(!_PAGESIZE)
			_PAGESIZE = info.mem_unit;
		ptr = (char*)aligned_alloc(_PAGESIZE, size);
	}
	virtual inline ~LinuxCodePage() {
		mprotect(ptr, size, PROT_READ);
		free(ptr);
	}
	virtual inline size_t getSize() override { return size; }
	virtual inline void *getPtr() override { return ptr; }

	virtual void firm() override {
		mprotect(ptr, size, PROT_EXEC | PROT_READ);
		firmed = true;
	}
	virtual inline void jump() override {
		assert(firmed);
		((void (*)())ptr)();
	}
};

slake::ICodePage *slake::genCodePage(size_t size) {
	return new LinuxCodePage(size);
}
