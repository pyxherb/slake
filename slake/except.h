#ifndef _SLAKE_EXCEPT_H_
#define _SLAKE_EXCEPT_H_

#include "except_base.h"
#include "obj/object.h"
#include <peff/containers/string.h>

namespace slake {
	class OutOfMemoryError : public InternalException {
	public:
		SLAKE_API OutOfMemoryError();
		SLAKE_API virtual ~OutOfMemoryError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static OutOfMemoryError *alloc() noexcept;
	};

	extern OutOfMemoryError g_globalOutOfMemoryError;

	class IdRefObject;

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

		/// @brief Invalid argument index.
		InvalidArgumentIndex,

		/// @brief Invalid argument number.
		InvalidArgumentNumber,

		/// @brief Invalid array index.
		InvalidArrayIndex,

		/// @brief Stack overflowed.
		StackOverflow,

		/// @breif Malformed class structure.
		MalformedClassStructure,

		/// @brief An error occurred during the generic instantiation of a member.
		GenericInstantiationError,
	};

	enum class LoaderErrorCode : uint8_t {
		BadMagicNumber = 0,
		ReadError,
		PrematuredEndOfFile,
		DuplicatedMember
	};

	class RuntimeExecError : public InternalException {
	public:
		RuntimeExecErrorCode errorCode;

		SLAKE_API RuntimeExecError(peff::Alloc *selfAllocator, RuntimeExecErrorCode errorCode);
		SLAKE_API virtual ~RuntimeExecError();
	};

	class MismatchedVarTypeError : public RuntimeExecError {
	public:
		SLAKE_API MismatchedVarTypeError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~MismatchedVarTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MismatchedVarTypeError *alloc(peff::Alloc *selfAllocator);
	};

	class FrameBoundaryExceededError : public RuntimeExecError {
	public:
		SLAKE_API FrameBoundaryExceededError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~FrameBoundaryExceededError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static FrameBoundaryExceededError *alloc(peff::Alloc *selfAllocator);
	};

	class InvalidOpcodeError : public RuntimeExecError {
	public:
		Opcode opcode;

		SLAKE_API InvalidOpcodeError(peff::Alloc *selfAllocator, Opcode opcode);
		SLAKE_API virtual ~InvalidOpcodeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidOpcodeError *alloc(peff::Alloc *selfAllocator, Opcode index);
	};

	class InvalidOperandsError : public RuntimeExecError {
	public:
		SLAKE_API InvalidOperandsError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~InvalidOperandsError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidOperandsError *alloc(peff::Alloc *selfAllocator);
	};

	class InvalidLocalVarIndexError : public RuntimeExecError {
	public:
		uint32_t index;

		SLAKE_API InvalidLocalVarIndexError(peff::Alloc *selfAllocator, uint32_t index);
		SLAKE_API virtual ~InvalidLocalVarIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidLocalVarIndexError *alloc(peff::Alloc *selfAllocator, uint32_t index);
	};

	class InvalidArrayIndexError : public RuntimeExecError {
	public:
		size_t index;

		SLAKE_API InvalidArrayIndexError(peff::Alloc *selfAllocator, size_t index);
		SLAKE_API virtual ~InvalidArrayIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArrayIndexError *alloc(peff::Alloc *selfAllocator, size_t index);
	};

	class StackOverflowError : public RuntimeExecError {
	public:
		SLAKE_API StackOverflowError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~StackOverflowError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static StackOverflowError *alloc(peff::Alloc *selfAllocator);
	};

	class InvalidArgumentNumberError : public RuntimeExecError {
	public:
		uint32_t nArgs;

		SLAKE_API InvalidArgumentNumberError(peff::Alloc *selfAllocator, uint32_t nArgs);
		SLAKE_API virtual ~InvalidArgumentNumberError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArgumentNumberError *alloc(peff::Alloc *selfAllocator, uint32_t nArgs);
	};

	class ReferencedMemberNotFoundError : public RuntimeExecError {
	public:
		HostObjectRef<IdRefObject> idRef;

		SLAKE_API ReferencedMemberNotFoundError(
			peff::Alloc *selfAllocator,
			IdRefObject *idRef);
		SLAKE_API virtual ~ReferencedMemberNotFoundError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ReferencedMemberNotFoundError *alloc(
			peff::Alloc *selfAllocator,
			IdRefObject *idRef);
	};

	class NullRefError : public RuntimeExecError {
	public:
		SLAKE_API NullRefError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~NullRefError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static NullRefError *alloc(peff::Alloc *selfAllocator);
	};

	class UncaughtExceptionError : public RuntimeExecError {
	public:
		Value exceptionValue;

		SLAKE_API UncaughtExceptionError(
			peff::Alloc *selfAllocator,
			Value exceptionValue);
		SLAKE_API virtual ~UncaughtExceptionError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static UncaughtExceptionError *alloc(
			peff::Alloc *selfAllocator,
			Value exceptionValue);
	};

	class MalformedClassStructureError : public RuntimeExecError {
	public:
		ClassObject *classObject;

		SLAKE_API MalformedClassStructureError(
			peff::Alloc *selfAllocator,
			ClassObject *classObject);
		SLAKE_API virtual ~MalformedClassStructureError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MalformedClassStructureError *alloc(
			peff::Alloc *selfAllocator,
			ClassObject *classObject);
	};

	enum class GenericInstantiationErrorCode : uint8_t {
		MismatchedGenericArgumentNumber = 0,
		GenericParameterNotFound,
		GenericArgTypeError
	};

	class GenericInstantiationError : public RuntimeExecError {
	public:
		GenericInstantiationErrorCode instantiationErrorCode;

		SLAKE_API GenericInstantiationError(
			peff::Alloc *selfAllocator,
			GenericInstantiationErrorCode instantiationErrorCode);
		SLAKE_API virtual ~GenericInstantiationError();
	};

	class MismatchedGenericArgumentNumberError : public RuntimeExecError {
	public:
		SLAKE_API MismatchedGenericArgumentNumberError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~MismatchedGenericArgumentNumberError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MismatchedGenericArgumentNumberError *alloc(peff::Alloc *selfAllocator);
	};

	class GenericParameterNotFoundError : public RuntimeExecError {
	public:
		peff::String name;

		SLAKE_API GenericParameterNotFoundError(
			peff::Alloc *selfAllocator,
			peff::String &&name);
		SLAKE_API virtual ~GenericParameterNotFoundError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericParameterNotFoundError *alloc(
			peff::Alloc *selfAllocator,
			peff::String &&name);
	};

	class GenericArgTypeError : public RuntimeExecError {
	public:
		peff::String name;

		SLAKE_API GenericArgTypeError(
			peff::Alloc *selfAllocator,
			peff::String &&name);
		SLAKE_API virtual ~GenericArgTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericArgTypeError *alloc(
			peff::Alloc *selfAllocator,
			peff::String &&name);
	};

	class GenericFieldInitError : public RuntimeExecError {
	public:
		HostObjectRef<ModuleObject> object;
		size_t idxRecord;

		SLAKE_API GenericFieldInitError(
			peff::Alloc *selfAllocator,
			ModuleObject *object,
			size_t idxRecord);
		SLAKE_API virtual ~GenericFieldInitError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericFieldInitError *alloc(
			peff::Alloc *selfAllocator,
			ModuleObject *object,
			size_t idxRecord);
	};

	enum class OptimizerErrorCode {
		MalformedProgram = 0,
		MalformedCfg,
		ErrorEvaluatingObjectType
	};

	class OptimizerError : public InternalException {
	public:
		OptimizerErrorCode optimizerErrorCode;

		SLAKE_API OptimizerError(peff::Alloc *selfAllocator, OptimizerErrorCode optimizerErrorCode);
		SLAKE_API virtual ~OptimizerError();
	};

	class RegularFnOverloadingObject;

	class MalformedProgramError : public OptimizerError {
	public:
		HostObjectRef<RegularFnOverloadingObject> fnOverloading;
		size_t offIns;

		SLAKE_API MalformedProgramError(
			peff::Alloc *selfAllocator,
			RegularFnOverloadingObject *fnOverloading,
			size_t offIns);
		SLAKE_API virtual ~MalformedProgramError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MalformedProgramError *alloc(
			peff::Alloc *selfAllocator,
			RegularFnOverloadingObject *fnOverloading,
			size_t offIns);
	};

	namespace opti {
		struct ControlFlowGraph;
	}
	class MalformedCfgError : public OptimizerError {
	public:
		const opti::ControlFlowGraph *cfg;
		size_t idxBasicBlock;
		size_t offIns;

		SLAKE_API MalformedCfgError(
			peff::Alloc *selfAllocator,
			const opti::ControlFlowGraph *cfg,
			size_t idxBasicBlock,
			size_t offIns);
		SLAKE_API virtual ~MalformedCfgError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MalformedCfgError *alloc(
			peff::Alloc *selfAllocator,
			const opti::ControlFlowGraph *cfg,
			size_t idxBasicBlock,
			size_t offIns);
	};

	class ErrorEvaluatingObjectTypeError : public OptimizerError {
	public:
		HostObjectRef<Object> object;

		SLAKE_API ErrorEvaluatingObjectTypeError(
			peff::Alloc *selfAllocator,
			Object *object);
		SLAKE_API virtual ~ErrorEvaluatingObjectTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ErrorEvaluatingObjectTypeError *alloc(
			peff::Alloc *selfAllocator,
			Object *object);
	};

	class LoaderError : public InternalException {
	public:
		LoaderErrorCode errorCode;

		SLAKE_API LoaderError(peff::Alloc *selfAllocator, LoaderErrorCode errorCode);
		SLAKE_API virtual ~LoaderError();
	};

	class BadMagicError : public LoaderError {
	public:
		SLAKE_API BadMagicError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~BadMagicError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static BadMagicError *alloc(peff::Alloc *selfAllocator);
	};

	class ReadError : public LoaderError {
	public:
		SLAKE_API ReadError(peff::Alloc *selfAllocator);
		SLAKE_API virtual ~ReadError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ReadError *alloc(peff::Alloc *selfAllocator);
	};

	SLAKE_API InternalExceptionPointer allocOutOfMemoryErrorIfAllocFailed(InternalExceptionPointer e);
}

#endif
