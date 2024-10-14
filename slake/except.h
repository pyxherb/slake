#ifndef _SLAKE_EXCEPT_H_
#define _SLAKE_EXCEPT_H_

#include <stdexcept>
#include <slake/valdef/value.h>
#include <slake/valdef/idref.h>

namespace slake {
	enum class ErrorKind {
		RuntimeExecError = 0,
		SLXLoaderError
	};

	enum class RuntimeExecErrorCode : uint8_t {
		/// @brief A null reference was detected while the runtime requires a non-null one.
		NullRef = 0,

		NoMatchingOverloading,

		/// @brief The execution ran out from the function body.
		OutOfFnBody,

		/// @brief The type of the value does not match the variable's type.
		MismatchedVarType,

		/// @brief Invalid opcode.
		InvalidOpcode,

		/// @brief Invalid operand combination.
		InvalidOperands,

		/// @brief Member referenced was not found.
		ReferencedMemberNotFound,

		/// @brief An error occurred during the generic instantiation of a member.
		GenericInstantiationError,

		/// @brief There's an uncaught Slake exception.
		UncaughtException,

		/// @brief Stack frame pointer exceeds the frame boundary (usually an underflow).
		FrameBoundaryExceeded,

		/// @brief Invalid local variable index.
		InvalidLocalVarIndex,

		/// @brief Invalid register index.
		InvalidRegisterIndex,

		/// @brief Invalid argument index.
		InvalidArgumentIndex,

		/// @brief Invalid array index.
		InvalidArrayIndex,

		/// @brief Stack overflowed.
		StackOverflow
	};

	enum class SLXLoaderErrorCode : uint8_t {
		BadMagicNumber = 0,
		IOError,
		PrematuredEndOfStream,
		DuplicatedMember
	};

	class InternalExcpetion {
	public:
		ErrorKind kind;


	};

	class RuntimeExecError : public std::runtime_error {
	public:
		inline RuntimeExecError(std::string msg) : runtime_error(msg){};
		virtual ~RuntimeExecError() = default;
	};

	class NoOverloadingError : public RuntimeExecError {
	public:
		inline NoOverloadingError(std::string msg) : RuntimeExecError(msg){};
		virtual ~NoOverloadingError() = default;
	};

	class OutOfFnBodyError : public RuntimeExecError {
	public:
		inline OutOfFnBodyError(std::string msg) : RuntimeExecError(msg){};
		virtual ~OutOfFnBodyError() = default;
	};

	/// @brief Raises when mismatched types were detected.
	class MismatchedTypeError : public RuntimeExecError {
	public:
		inline MismatchedTypeError(std::string msg) : RuntimeExecError(msg){};
		virtual ~MismatchedTypeError() = default;
	};

	/// @brief Raises when incompatible types were detected.
	class IncompatibleTypeError : public RuntimeExecError {
	public:
		inline IncompatibleTypeError(std::string msg) : RuntimeExecError(msg){};
		virtual ~IncompatibleTypeError() = default;
	};

	/// @brief Raises when executing instructions with invalid opcode.
	class InvalidOpcodeError : public RuntimeExecError {
	public:
		inline InvalidOpcodeError(std::string msg) : RuntimeExecError(msg){};
		virtual ~InvalidOpcodeError() = default;
	};

	/// @brief Raises when executing instructions with invalid operand combination.
	class InvalidOperandsError : public RuntimeExecError {
	public:
		inline InvalidOperandsError(std::string msg) : RuntimeExecError(msg){};
		virtual ~InvalidOperandsError() = default;
	};

	class InvalidArgumentsError : public RuntimeExecError {
	public:
		inline InvalidArgumentsError(std::string msg = "Invalid arguments") : RuntimeExecError(msg){};
		virtual ~InvalidArgumentsError() = default;
	};

	class InvalidRegisterError : public RuntimeExecError {
	public:
		inline InvalidRegisterError(std::string msg = "Invalid register") : RuntimeExecError(msg){};
		virtual ~InvalidRegisterError() = default;
	};

	class NotFoundError : public RuntimeExecError {
	public:
		IdRefObject *ref;

		NotFoundError(std::string msg, IdRefObject *ref);
		virtual ~NotFoundError() = default;
	};

	class GenericInstantiationError : public RuntimeExecError {
	public:
		inline GenericInstantiationError(std::string msg)
			: RuntimeExecError(msg) {}
		virtual ~GenericInstantiationError() = default;
	};

	class AccessViolationError : public RuntimeExecError {
	public:
		inline AccessViolationError(std::string msg) : RuntimeExecError(msg){};
		virtual ~AccessViolationError() = default;
	};

	class UncaughtExceptionError : public RuntimeExecError {
	public:
		Value source;

		inline UncaughtExceptionError(std::string msg, const Value &source) : RuntimeExecError(msg), source(source){};
		virtual ~UncaughtExceptionError() = default;
	};

	class AbortedError : public RuntimeExecError {
	public:
		inline AbortedError(std::string msg) : RuntimeExecError(msg){};
		virtual ~AbortedError() = default;
	};

	class FrameBoundaryExceededError : public RuntimeExecError {
	public:
		inline FrameBoundaryExceededError(std::string msg) : RuntimeExecError(msg){};
		virtual ~FrameBoundaryExceededError() = default;
	};

	class InvalidLocalVarIndexError : public RuntimeExecError {
	public:
		const uint32_t index;

		inline InvalidLocalVarIndexError(std::string msg, uint32_t index) : RuntimeExecError(msg), index(index){};
		virtual ~InvalidLocalVarIndexError() = default;
	};

	class InvalidRegisterIndexError : public RuntimeExecError {
	public:
		const uint32_t index;

		inline InvalidRegisterIndexError(std::string msg, uint32_t index) : RuntimeExecError(msg), index(index){};
		virtual ~InvalidRegisterIndexError() = default;
	};

	class InvalidArgumentIndexError : public RuntimeExecError {
	public:
		const uint32_t index;

		inline InvalidArgumentIndexError(std::string msg, uint32_t index) : RuntimeExecError(msg), index(index){};
		virtual ~InvalidArgumentIndexError() = default;
	};

	class InvalidSubscriptionError : public RuntimeExecError {
	public:
		inline InvalidSubscriptionError(std::string msg) : RuntimeExecError(msg){};
		virtual ~InvalidSubscriptionError() = default;
	};

	class FrameError : public RuntimeExecError {
	public:
		inline FrameError(std::string msg) : RuntimeExecError(msg){};
		virtual ~FrameError() = default;
	};

	class StackOverflowError : public RuntimeExecError {
	public:
		inline StackOverflowError(std::string msg) : RuntimeExecError(msg){};
		virtual ~StackOverflowError() = default;
	};

	class LoaderError : public RuntimeExecError {
	public:
		inline LoaderError(std::string msg) : RuntimeExecError(msg){};
		virtual ~LoaderError() = default;
	};

	class NullRefError : public RuntimeExecError {
	public:
		inline NullRefError() : RuntimeExecError("Null reference detected"){};
		virtual ~NullRefError() = default;
	};

	class OutOfRangeError : public RuntimeExecError {
	public:
		inline OutOfRangeError() : RuntimeExecError("Out of range"){};
		virtual ~OutOfRangeError() = default;
	};
}

#endif
