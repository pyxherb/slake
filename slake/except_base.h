#ifndef _SLAKE_EXCEPT_BASE_H_
#define _SLAKE_EXCEPT_BASE_H_

#include "opcode.h"
#include "basedefs.h"
#include "util/memory.h"
#include <memory>

namespace slake {
	class Runtime;

	enum class ErrorKind {
		RuntimeExecError = 0,
		SLXLoaderError
	};

	class InternalException {
	public:
		Runtime *associatedRuntime;
		ErrorKind kind;

		SLAKE_API InternalException(Runtime *associatedRuntime, ErrorKind kind);
		SLAKE_API virtual ~InternalException();

		virtual const char *what() const = 0;

		virtual void dealloc() = 0;
	};

	class InternalExceptionPointer {
	private:
		std::unique_ptr<InternalException, util::DeallocableDeleter<InternalException>> _ptr;

	public:
		SLAKE_FORCEINLINE InternalExceptionPointer() = default;
		SLAKE_FORCEINLINE InternalExceptionPointer(InternalException *exception) : _ptr(exception) {
		}

		SLAKE_FORCEINLINE ~InternalExceptionPointer() {
			if (_ptr) {
				assert(("Unhandled Slake internal exception: ", false));
			}
		}

		InternalExceptionPointer(const InternalExceptionPointer &) = delete;
		InternalExceptionPointer &operator=(const InternalExceptionPointer &) = delete;
		SLAKE_FORCEINLINE InternalExceptionPointer(InternalExceptionPointer &&other) {
			_ptr = std::move(other._ptr);
		}
		SLAKE_FORCEINLINE InternalExceptionPointer &operator=(InternalExceptionPointer &&other) {
			_ptr = std::move(other._ptr);
			return *this;
		}

		SLAKE_FORCEINLINE InternalException *get() {
			return _ptr.get();
		}
		SLAKE_FORCEINLINE const InternalException *get() const {
			return _ptr.get();
		}

		SLAKE_FORCEINLINE void reset() {
			_ptr.reset();
		}

		SLAKE_FORCEINLINE explicit operator bool() {
			return (bool)_ptr;
		}

		SLAKE_FORCEINLINE InternalException *operator->() {
			return _ptr.get();
		}

		SLAKE_FORCEINLINE const InternalException *operator->() const {
			return _ptr.get();
		}
	};
}

#define SLAKE_RETURN_IF_EXCEPT(expr)                  \
	if (InternalExceptionPointer e = (expr); (bool)e) \
	return e
#define SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(name, expr)  \
	if ((bool)(name = (expr))) \
	return name;

#endif
