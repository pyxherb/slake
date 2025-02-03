#ifndef _SLAKE_EXCEPT_BASE_H_
#define _SLAKE_EXCEPT_BASE_H_

#include "opcode.h"
#include "basedefs.h"
#include "util/memory.h"
#include <memory>

namespace slake {
	class Runtime;

	enum class ErrorKind {
		OutOfMemoryError = 0,
		RuntimeExecError,
		SLXLoaderError,
		OptimizerError
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
		InternalException *_ptr = nullptr;

	public:
		SLAKE_FORCEINLINE InternalExceptionPointer() noexcept = default;
		SLAKE_FORCEINLINE InternalExceptionPointer(InternalException *exception) noexcept : _ptr(exception) {
		}

		SLAKE_FORCEINLINE ~InternalExceptionPointer() noexcept {
			unwrap();
			reset();
		}

		InternalExceptionPointer(const InternalExceptionPointer &) = delete;
		InternalExceptionPointer &operator=(const InternalExceptionPointer &) = delete;
		SLAKE_FORCEINLINE InternalExceptionPointer(InternalExceptionPointer &&other) noexcept {
			_ptr = other._ptr;
			other._ptr = nullptr;
		}
		SLAKE_FORCEINLINE InternalExceptionPointer &operator=(InternalExceptionPointer &&other) noexcept {
			_ptr = other._ptr;
			other._ptr = nullptr;
			return *this;
		}

		SLAKE_FORCEINLINE InternalException *get() noexcept {
			return _ptr;
		}
		SLAKE_FORCEINLINE const InternalException *get() const noexcept {
			return _ptr;
		}

		SLAKE_FORCEINLINE void reset() noexcept {
			if (_ptr) {
				_ptr->dealloc();
			}
			_ptr = nullptr;
		}

		SLAKE_FORCEINLINE void unwrap() noexcept {
			if (_ptr) {
				assert(("Unhandled Slake internal exception: ", false));
			}
		}

		SLAKE_FORCEINLINE explicit operator bool() noexcept {
			return (bool)_ptr;
		}

		SLAKE_FORCEINLINE InternalException *operator->() noexcept {
			return _ptr;
		}

		SLAKE_FORCEINLINE const InternalException *operator->() const noexcept {
			return _ptr;
		}
	};
}

#define SLAKE_UNWRAP_EXCEPT(expr) (expr).unwrap()
#define SLAKE_RETURN_IF_EXCEPT(expr)                  \
	if (InternalExceptionPointer _ = (expr); (bool)_) \
	return _
#define SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(name, expr)  \
	if ((bool)(name = (expr))) \
	return name;

#endif
