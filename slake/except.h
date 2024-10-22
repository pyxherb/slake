#ifndef _SLAKE_EXCEPT_H_
#define _SLAKE_EXCEPT_H_

#include <stdexcept>
#include "opcode.h"
#include "value.h"
#include <slake/obj/idref.h>

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

		/// @brief Invalid argument number.
		InvalidArgumentNumber,

		/// @brief Invalid array index.
		InvalidArrayIndex,

		/// @brief Stack overflowed.
		StackOverflow,

		/// @brief An error occurred during the generic instantiation of a member.
		GenericInstantiationError,
	};

	enum class SLXLoaderErrorCode : uint8_t {
		BadMagicNumber = 0,
		IOError,
		PrematuredEndOfStream,
		DuplicatedMember
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

	class RuntimeExecError : public InternalException {
	public:
		RuntimeExecErrorCode errorCode;

		SLAKE_API RuntimeExecError(Runtime *associatedRuntime, RuntimeExecErrorCode errorCode);
		SLAKE_API virtual ~RuntimeExecError();
	};

	class MismatchedVarTypeError : public RuntimeExecError {
	public:
		SLAKE_API MismatchedVarTypeError(Runtime *associatedRuntime);
		SLAKE_API virtual ~MismatchedVarTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MismatchedVarTypeError *alloc(Runtime *associatedRuntime);
	};

	class FrameBoundaryExceededError : public RuntimeExecError {
	public:
		SLAKE_API FrameBoundaryExceededError(Runtime *associatedRuntime);
		SLAKE_API virtual ~FrameBoundaryExceededError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static FrameBoundaryExceededError *alloc(Runtime *associatedRuntime);
	};

	class InvalidOpcodeError : public RuntimeExecError {
	public:
		Opcode opcode;

		SLAKE_API InvalidOpcodeError(Runtime *associatedRuntime, Opcode opcode);
		SLAKE_API virtual ~InvalidOpcodeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidOpcodeError *alloc(Runtime *associatedRuntime, Opcode index);
	};

	class InvalidOperandsError : public RuntimeExecError {
	public:
		SLAKE_API InvalidOperandsError(Runtime *associatedRuntime);
		SLAKE_API virtual ~InvalidOperandsError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidOperandsError *alloc(Runtime *associatedRuntime);
	};

	class InvalidRegisterIndexError : public RuntimeExecError {
	public:
		uint32_t index;

		SLAKE_API InvalidRegisterIndexError(Runtime *associatedRuntime, uint32_t index);
		SLAKE_API virtual ~InvalidRegisterIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidRegisterIndexError *alloc(Runtime *associatedRuntime, uint32_t index);
	};

	class InvalidLocalVarIndexError : public RuntimeExecError {
	public:
		uint32_t index;

		SLAKE_API InvalidLocalVarIndexError(Runtime *associatedRuntime, uint32_t index);
		SLAKE_API virtual ~InvalidLocalVarIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidLocalVarIndexError *alloc(Runtime *associatedRuntime, uint32_t index);
	};

	class InvalidArgumentIndexError : public RuntimeExecError {
	public:
		uint32_t index;

		SLAKE_API InvalidArgumentIndexError(Runtime *associatedRuntime, uint32_t index);
		SLAKE_API virtual ~InvalidArgumentIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArgumentIndexError *alloc(Runtime *associatedRuntime, uint32_t index);
	};

	class InvalidArrayIndexError : public RuntimeExecError {
	public:
		size_t index;

		SLAKE_API InvalidArrayIndexError(Runtime *associatedRuntime, size_t index);
		SLAKE_API virtual ~InvalidArrayIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArrayIndexError *alloc(Runtime *associatedRuntime, size_t index);
	};

	class InvalidArgumentNumberError : public RuntimeExecError {
	public:
		uint32_t nArgs;

		SLAKE_API InvalidArgumentNumberError(Runtime *associatedRuntime, uint32_t nArgs);
		SLAKE_API virtual ~InvalidArgumentNumberError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArgumentNumberError *alloc(Runtime *associatedRuntime, uint32_t nArgs);
	};

	class ReferencedMemberNotFoundError : public RuntimeExecError {
	public:
		HostObjectRef<IdRefObject> idRef;

		SLAKE_API ReferencedMemberNotFoundError(
			Runtime *associatedRuntime,
			IdRefObject *idRef);
		SLAKE_API virtual ~ReferencedMemberNotFoundError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ReferencedMemberNotFoundError *alloc(
			Runtime *associatedRuntime,
			IdRefObject *idRef);
	};

	class NullRefError : public RuntimeExecError {
	public:
		SLAKE_API NullRefError(Runtime *associatedRuntime);
		SLAKE_API virtual ~NullRefError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static NullRefError *alloc(Runtime *associatedRuntime);
	};

	class UncaughtExceptionError : public RuntimeExecError {
	public:
		Value exceptionValue;

		SLAKE_API UncaughtExceptionError(
			Runtime *associatedRuntime,
			Value exceptionValue);
		SLAKE_API virtual ~UncaughtExceptionError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static UncaughtExceptionError *alloc(
			Runtime *associatedRuntime,
			Value exceptionValue);
	};

	enum class GenericInstantiationErrorCode : uint8_t {
		MismatchedGenericArgumentNumber = 0,
		GenericParameterNotFound,
	};

	class GenericInstantiationError : public RuntimeExecError {
	public:
		GenericInstantiationErrorCode instantiationErrorCode;

		SLAKE_API GenericInstantiationError(
			Runtime *associatedRuntime,
			GenericInstantiationErrorCode instantiationErrorCode);
		SLAKE_API virtual ~GenericInstantiationError();
	};

	class MismatchedGenericArgumentNumberError : public RuntimeExecError {
	public:
		SLAKE_API MismatchedGenericArgumentNumberError(Runtime *associatedRuntime);
		SLAKE_API virtual ~MismatchedGenericArgumentNumberError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MismatchedGenericArgumentNumberError *alloc(Runtime *associatedRuntime);
	};

	class GenericParameterNotFoundError : public RuntimeExecError {
	public:
		std::pmr::string name;

		SLAKE_API GenericParameterNotFoundError(
			Runtime *associatedRuntime,
			std::pmr::string &&name);
		SLAKE_API virtual ~GenericParameterNotFoundError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericParameterNotFoundError *alloc(
			Runtime *associatedRuntime,
			std::pmr::string &&name);
	};

	// stub, remove it after work around SLXLoaderError is finished.
	class LoaderError : public std::runtime_error {
	public:
		inline LoaderError(std::string msg) : runtime_error(msg){};
		virtual ~LoaderError() = default;
	};
}

#endif
