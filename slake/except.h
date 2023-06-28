#ifndef _SLAKE_EXCEPT_H_
#define _SLAKE_EXCEPT_H_

#include <stdexcept>

namespace Slake {
	class RuntimeExecError : public std::runtime_error {
	public:
		inline RuntimeExecError(std::string msg) : runtime_error(msg){};
		virtual inline ~RuntimeExecError() {}
	};

	class OutOfFnBodyError : public RuntimeExecError {
	public:
		inline OutOfFnBodyError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~OutOfFnBodyError() {}
	};

	/// @brief Raises when mismatched types were detected.
	class MismatchedTypeError : public RuntimeExecError {
	public:
		inline MismatchedTypeError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~MismatchedTypeError() {}
	};

	/// @brief Raises when incompatible types were detected.
	class IncompatibleTypeError : public RuntimeExecError {
	public:
		inline IncompatibleTypeError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~IncompatibleTypeError() {}
	};

	/// @brief Raises when executing instructions with invalid opcode.
	class InvalidOpcodeError : public RuntimeExecError {
	public:
		inline InvalidOpcodeError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~InvalidOpcodeError() {}
	};

	/// @brief Raises when executing instructions with invalid operand combination.
	class InvalidOperandsError : public RuntimeExecError {
	public:
		inline InvalidOperandsError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~InvalidOperandsError() {}
	};

	class InvalidArgumentsError : public RuntimeExecError {
	public:
		inline InvalidArgumentsError(std::string msg = "Invalid arguments") : RuntimeExecError(msg){};
		virtual inline ~InvalidArgumentsError() {}
	};

	class ResourceNotFoundError : public RuntimeExecError {
	public:
		inline ResourceNotFoundError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~ResourceNotFoundError() {}
	};

	class AccessViolationError : public RuntimeExecError {
	public:
		inline AccessViolationError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~AccessViolationError() {}
	};

	class UncaughtExceptionError : public RuntimeExecError {
	public:
		inline UncaughtExceptionError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~UncaughtExceptionError() {}
	};

	class AbortedError : public RuntimeExecError {
	public:
		inline AbortedError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~AbortedError() {}
	};

	class FrameBoundaryExceededError : public RuntimeExecError {
	public:
		inline FrameBoundaryExceededError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~FrameBoundaryExceededError() {}
	};

	class InvalidSubscriptionError : public RuntimeExecError {
	public:
		inline InvalidSubscriptionError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~InvalidSubscriptionError() {}
	};

	class FrameError : public RuntimeExecError {
	public:
		inline FrameError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~FrameError() {}
	};

	class StackOverflowError : public RuntimeExecError {
	public:
		inline StackOverflowError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~StackOverflowError() {}
	};

	class LoaderError : public RuntimeExecError {
	public:
		inline LoaderError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~LoaderError() {}
	};

	class NullRefError : public RuntimeExecError {
	public:
		inline NullRefError(std::string msg = "Null reference detected") : RuntimeExecError(msg){};
		virtual inline ~NullRefError() {}
	};

	class RefParseError : public RuntimeExecError {
	public:
		inline RefParseError(std::string msg) : RuntimeExecError(msg){};
		virtual inline ~RefParseError() {}
	};
}

#endif
