#ifndef _SLAKE_EXCEPT_H_
#define _SLAKE_EXCEPT_H_

#include <stdexcept>

namespace Slake {
	/// @brief Raises when mismatched types were detected.
	class MismatchedTypeError : public std::runtime_error {
	public:
		inline MismatchedTypeError(std::string msg) : runtime_error(msg){};
		virtual inline ~MismatchedTypeError() {}
	};

	/// @brief Raises when incompatible types were detected.
	class IncompatibleTypeError : public std::runtime_error {
	public:
		inline IncompatibleTypeError(std::string msg) : runtime_error(msg){};
		virtual inline ~IncompatibleTypeError() {}
	};


	/// @brief Raises when executing instructions with invalid opcode.
	class InvalidOpcodeError : public std::runtime_error {
	public:
		inline InvalidOpcodeError(std::string msg) : runtime_error(msg){};
		virtual inline ~InvalidOpcodeError() {}
	};

	/// @brief Raises when executing instructions with invalid operand combination.
	class InvalidOperandsError : public std::runtime_error {
	public:
		inline InvalidOperandsError(std::string msg) : runtime_error(msg){};
		virtual inline ~InvalidOperandsError() {}
	};

	class InvalidArgumentsError : public std::runtime_error {
	public:
		inline InvalidArgumentsError(std::string msg) : runtime_error(msg){};
		virtual inline ~InvalidArgumentsError() {}
	};

	class ResourceNotFoundError : public std::runtime_error {
	public:
		inline ResourceNotFoundError(std::string msg) : runtime_error(msg){};
		virtual inline ~ResourceNotFoundError() {}
	};

	class AccessViolationError : public std::runtime_error {
	public:
		inline AccessViolationError(std::string msg) : runtime_error(msg){};
		virtual inline ~AccessViolationError() {}
	};

	class UncaughtExceptionError : public std::runtime_error {
	public:
		inline UncaughtExceptionError(std::string msg) : runtime_error(msg){};
		virtual inline ~UncaughtExceptionError() {}
	};

	class AbortedError : public std::runtime_error {
	public:
		inline AbortedError(std::string msg) : runtime_error(msg){};
		virtual inline ~AbortedError() {}
	};

	class FrameBoundaryExceededError : public std::runtime_error {
	public:
		inline FrameBoundaryExceededError(std::string msg) : runtime_error(msg){};
		virtual inline ~FrameBoundaryExceededError() {}
	};

	class InvalidSubscriptionError : public std::runtime_error {
	public:
		inline InvalidSubscriptionError(std::string msg) : runtime_error(msg){};
		virtual inline ~InvalidSubscriptionError() {}
	};

	class FrameError : public std::runtime_error {
	public:
		inline FrameError(std::string msg) : runtime_error(msg){};
		virtual inline ~FrameError() {}
	};

	class StackOverflowError : public std::runtime_error {
	public:
		inline StackOverflowError(std::string msg) : runtime_error(msg){};
		virtual inline ~StackOverflowError() {}
	};

	class LoaderError : public std::runtime_error {
	public:
		inline LoaderError(std::string msg) : runtime_error(msg){};
		virtual inline ~LoaderError() {}
	};

	class NullRefError : public std::runtime_error {
	public:
		inline NullRefError(std::string msg) : runtime_error(msg){};
		virtual inline ~NullRefError() {}
	};

	class RefParseError : public std::runtime_error {
	public:
		inline RefParseError(std::string msg) : runtime_error(msg){};
		virtual inline ~RefParseError() {}
	};
}

#endif
