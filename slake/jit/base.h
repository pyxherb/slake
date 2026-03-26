#ifndef _SLAKE_JIT_H_
#define _SLAKE_JIT_H_

#include <cstddef>
#include <slake/object.h>

namespace slake {
	class Runtime;

	class CodePage {
	public:
		virtual ~CodePage() = default;
		virtual size_t get_size() = 0;
		virtual void *get_ptr() = 0;
		virtual void lock() = 0;
		virtual void jump() = 0;
	};

	class FnObject;

	struct JITCompilerOptions {
		SLAKE_FORCEINLINE JITCompilerOptions() {
		}
	};

	CodePage *gen_code_page(size_t size);
	[[nodiscard]] InternalExceptionPointer compile_regular_fn(RegularFnOverloadingObject *fn, peff::Alloc *resource_allocator, const JITCompilerOptions &options);
}

#endif
