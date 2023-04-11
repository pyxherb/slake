#ifndef _SLAKE_JIT_H_
#define _SLAKE_JIT_H_

#include <cstddef>

namespace Slake {
	class ICodePage {
	public:
		virtual inline ~ICodePage() {}
		virtual std::size_t getSize() = 0;
		virtual void *getPtr() = 0;
		virtual void firm() = 0;
		virtual void jump() = 0;
	};

	class FnValue;

	ICodePage* genCodePage(std::size_t size);
	ICodePage *compileFn(FnValue* fn);
}

#endif
