#include <slake/jit/base.h>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <Windows.h>

class Win32CodePage : public slake::CodePage {
public:
	char *ptr;
	size_t size;
	DWORD old_protect;
	bool locked = false;

	inline Win32CodePage(size_t size) : ptr(ptr), size(size) {
		ptr = new char[size];
		VirtualProtect(ptr, size, PAGE_READWRITE, &old_protect);
		FlushInstructionCache(GetCurrentProcess(), ptr, size);
	}
	virtual inline ~Win32CodePage() {
		VirtualProtect(ptr, size, old_protect, &old_protect);
		delete[] ptr;
	}
	virtual inline size_t get_size() override { return size; }
	virtual inline void *get_ptr() override { return ptr; }

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

slake::CodePage *slake::gen_code_page(size_t size) {
	return new Win32CodePage(size);
}
