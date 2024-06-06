#ifndef _SLAKE_JIT_H_
#define _SLAKE_JIT_H_

#include <cstddef>

namespace slake {
	class ICodePage {
	public:
		virtual ~ICodePage() = default;
		virtual size_t getSize() = 0;
		virtual void *getPtr() = 0;
		virtual void firm() = 0;
		virtual void jump() = 0;
	};

	class FnObject;

	ICodePage* genCodePage(size_t size);
	ICodePage *compileFn(FnObject* fn);
}

#endif
