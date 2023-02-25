#include "../../base.hh"

#include <Windows.h>

class Win32CodePage : public Slake::ICodePage {
public:
	void* ptr;
	std::size_t size;
	inline Win32CodePage(void* ptr, std::size_t size) : ptr(ptr), size(size) {
		FlushInstructionCache(GetCurrentProcess(), ptr, size);
	}
	virtual inline ~Win32CodePage() {
		VirtualFree(ptr, size, MEM_RELEASE);
	}
	virtual std::size_t getSize() override { return size; }
	virtual void* getPtr() override { return ptr; }
};

Slake::ICodePage* Slake::genCodePage(std::size_t size) {
	void* ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (ptr)
		return new Win32CodePage(ptr, size);
	return nullptr;
}
