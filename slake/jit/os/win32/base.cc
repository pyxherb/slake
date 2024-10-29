#include "../../base.hh"
#include <cstdint>
#include <cstring>
#include <cassert>

#include <Windows.h>

class Win32CodePage : public slake::CodePage {
public:
	char *ptr;
	size_t size;
	DWORD oldProtect;
	bool locked = false;

	inline Win32CodePage(size_t size) : ptr(ptr), size(size) {
		ptr = new char[size];
		VirtualProtect(ptr, size, PAGE_READWRITE, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), ptr, size);
	}
	virtual inline ~Win32CodePage() {
		VirtualProtect(ptr, size, oldProtect, &oldProtect);
		delete[] ptr;
	}
	virtual inline size_t getSize() override { return size; }
	virtual inline void *getPtr() override { return ptr; }

	virtual void lock() override {
		DWORD tmp;
		VirtualProtect(ptr, size, PAGE_EXECUTE_READ, &tmp);
		locked = true;
	}
	virtual inline void jump() override {
		assert(locked);
		((void (*)())ptr)();
	}
};

slake::CodePage *slake::genCodePage(size_t size) {
	return new Win32CodePage(size);
}
