#ifndef _SLAKE_JIT_H_
#define _SLAKE_JIT_H_

#include <cstddef>
#include <slake/object.h>

namespace slake {
	class Runtime;

	class CodePage {
	public:
		virtual ~CodePage() = default;
		virtual size_t getSize() = 0;
		virtual void *getPtr() = 0;
		virtual void lock() = 0;
		virtual void jump() = 0;
	};

	class FnObject;

	CodePage* genCodePage(size_t size);
	CodePage *compileRegularFn(RegularFnOverloadingObject* fn);
}

#endif