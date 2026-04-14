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

	extern OutOfMemoryError g_global_out_of_memory_error;

	class IdRefObject;

	enum class RuntimeExecErrorCode : uint8_t {
		/// @brief A null reference was detected while the runtime requires a non-null one.
		NullRef = 0,

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

		MismatchedGenericArgumentNumber,

		GenericParameterNotFound,

		GenericArgTypeError,

		DuplicatedOverloadingError,
	};

	enum class LoaderErrorCode : uint8_t {
		BadMagicNumber = 0,
		ReadError,
		PrematuredEndOfFile,
		DuplicatedMember
	};

	class RuntimeExecError : public InternalException {
	public:
		RuntimeExecErrorCode error_code;

		SLAKE_API RuntimeExecError(peff::Alloc *self_allocator, RuntimeExecErrorCode error_code);
		SLAKE_API virtual ~RuntimeExecError();
	};

	class MismatchedVarTypeError : public RuntimeExecError {
	public:
		TypeRef expected_type;
		SLAKE_API MismatchedVarTypeError(peff::Alloc *self_allocator, const TypeRef &expected_type);
		SLAKE_API virtual ~MismatchedVarTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MismatchedVarTypeError *alloc(peff::Alloc *self_allocator, const TypeRef &expected_type);
	};

	class FrameBoundaryExceededError : public RuntimeExecError {
	public:
		SLAKE_API FrameBoundaryExceededError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~FrameBoundaryExceededError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static FrameBoundaryExceededError *alloc(peff::Alloc *self_allocator);
	};

	class InvalidOpcodeError : public RuntimeExecError {
	public:
		Opcode opcode;

		SLAKE_API InvalidOpcodeError(peff::Alloc *self_allocator, Opcode opcode);
		SLAKE_API virtual ~InvalidOpcodeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidOpcodeError *alloc(peff::Alloc *self_allocator, Opcode index);
	};

	class InvalidOperandsError : public RuntimeExecError {
	public:
		SLAKE_API InvalidOperandsError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~InvalidOperandsError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidOperandsError *alloc(peff::Alloc *self_allocator);
	};

	class InvalidLocalVarIndexError : public RuntimeExecError {
	public:
		uint32_t index;

		SLAKE_API InvalidLocalVarIndexError(peff::Alloc *self_allocator, uint32_t index);
		SLAKE_API virtual ~InvalidLocalVarIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidLocalVarIndexError *alloc(peff::Alloc *self_allocator, uint32_t index);
	};

	class InvalidArrayIndexError : public RuntimeExecError {
	public:
		size_t index;

		SLAKE_API InvalidArrayIndexError(peff::Alloc *self_allocator, size_t index);
		SLAKE_API virtual ~InvalidArrayIndexError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArrayIndexError *alloc(peff::Alloc *self_allocator, size_t index);
	};

	class StackOverflowError : public RuntimeExecError {
	public:
		SLAKE_API StackOverflowError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~StackOverflowError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static StackOverflowError *alloc(peff::Alloc *self_allocator);
	};

	class InvalidArgumentNumberError : public RuntimeExecError {
	public:
		uint32_t num_args;

		SLAKE_API InvalidArgumentNumberError(peff::Alloc *self_allocator, uint32_t num_args);
		SLAKE_API virtual ~InvalidArgumentNumberError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static InvalidArgumentNumberError *alloc(peff::Alloc *self_allocator, uint32_t num_args);
	};

	class ReferencedMemberNotFoundError : public RuntimeExecError {
	public:
		HostObjectRef<IdRefObject> id_ref;

		SLAKE_API ReferencedMemberNotFoundError(
			peff::Alloc *self_allocator,
			IdRefObject *id_ref);
		SLAKE_API virtual ~ReferencedMemberNotFoundError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ReferencedMemberNotFoundError *alloc(
			peff::Alloc *self_allocator,
			IdRefObject *id_ref);
	};

	class NullRefError : public RuntimeExecError {
	public:
		SLAKE_API NullRefError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~NullRefError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static NullRefError *alloc(peff::Alloc *self_allocator);
	};

	class UncaughtExceptionError : public RuntimeExecError {
	public:
		Value exception_value;

		SLAKE_API UncaughtExceptionError(
			peff::Alloc *self_allocator,
			Value exception_value);
		SLAKE_API virtual ~UncaughtExceptionError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static UncaughtExceptionError *alloc(
			peff::Alloc *self_allocator,
			Value exception_value);
	};

	class MalformedClassStructureError : public RuntimeExecError {
	public:
		ClassObject *class_object;

		SLAKE_API MalformedClassStructureError(
			peff::Alloc *self_allocator,
			ClassObject *class_object);
		SLAKE_API virtual ~MalformedClassStructureError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MalformedClassStructureError *alloc(
			peff::Alloc *self_allocator,
			ClassObject *class_object);
	};

	class MismatchedGenericArgumentNumberError : public RuntimeExecError {
	public:
		SLAKE_API MismatchedGenericArgumentNumberError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~MismatchedGenericArgumentNumberError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MismatchedGenericArgumentNumberError *alloc(peff::Alloc *self_allocator);
	};

	class GenericParameterNotFoundError : public RuntimeExecError {
	public:
		peff::String name;

		SLAKE_API GenericParameterNotFoundError(
			peff::Alloc *self_allocator,
			peff::String &&name);
		SLAKE_API virtual ~GenericParameterNotFoundError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericParameterNotFoundError *alloc(
			peff::Alloc *self_allocator,
			peff::String &&name);
	};

	class GenericArgTypeError : public RuntimeExecError {
	public:
		peff::String name;

		SLAKE_API GenericArgTypeError(
			peff::Alloc *self_allocator,
			peff::String &&name);
		SLAKE_API virtual ~GenericArgTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericArgTypeError *alloc(
			peff::Alloc *self_allocator,
			peff::String &&name);
	};

	class FnObject;

	class GenericDuplicatedFnOverloadingError : public RuntimeExecError {
	public:
		FnObject *original_fn;

		SLAKE_API GenericDuplicatedFnOverloadingError(
			peff::Alloc *self_allocator,
			FnObject *original_fn);
		SLAKE_API virtual ~GenericDuplicatedFnOverloadingError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericDuplicatedFnOverloadingError *alloc(
			peff::Alloc *self_allocator,
			FnObject *original_fn);
	};

	class GenericFieldInitError : public RuntimeExecError {
	public:
		HostObjectRef<BasicModuleObject> object;
		size_t idx_record;

		SLAKE_API GenericFieldInitError(
			peff::Alloc *self_allocator,
			BasicModuleObject *object,
			size_t idx_record);
		SLAKE_API virtual ~GenericFieldInitError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static GenericFieldInitError *alloc(
			peff::Alloc *self_allocator,
			BasicModuleObject *object,
			size_t idx_record);
	};

	enum class OptimizerErrorCode {
		MalformedProgram = 0,
		MalformedCfg,
		ErrorEvaluatingObjectType
	};

	class OptimizerError : public InternalException {
	public:
		OptimizerErrorCode optimizer_error_code;

		SLAKE_API OptimizerError(peff::Alloc *self_allocator, OptimizerErrorCode optimizer_error_code);
		SLAKE_API virtual ~OptimizerError();
	};

	class RegularFnOverloadingObject;

	class MalformedProgramError : public OptimizerError {
	public:
		HostObjectRef<RegularFnOverloadingObject> fn_overloading;
		size_t off_ins;

		SLAKE_API MalformedProgramError(
			peff::Alloc *self_allocator,
			RegularFnOverloadingObject *fn_overloading,
			size_t off_ins);
		SLAKE_API virtual ~MalformedProgramError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MalformedProgramError *alloc(
			peff::Alloc *self_allocator,
			RegularFnOverloadingObject *fn_overloading,
			size_t off_ins);
	};

	namespace opti {
		struct ControlFlowGraph;
	}
	class MalformedCfgError : public OptimizerError {
	public:
		const opti::ControlFlowGraph *cfg;
		size_t idx_basic_block;
		size_t off_ins;

		SLAKE_API MalformedCfgError(
			peff::Alloc *self_allocator,
			const opti::ControlFlowGraph *cfg,
			size_t idx_basic_block,
			size_t off_ins);
		SLAKE_API virtual ~MalformedCfgError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static MalformedCfgError *alloc(
			peff::Alloc *self_allocator,
			const opti::ControlFlowGraph *cfg,
			size_t idx_basic_block,
			size_t off_ins);
	};

	class ErrorEvaluatingObjectTypeError : public OptimizerError {
	public:
		HostObjectRef<Object> object;

		SLAKE_API ErrorEvaluatingObjectTypeError(
			peff::Alloc *self_allocator,
			Object *object);
		SLAKE_API virtual ~ErrorEvaluatingObjectTypeError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ErrorEvaluatingObjectTypeError *alloc(
			peff::Alloc *self_allocator,
			Object *object);
	};

	class LoaderError : public InternalException {
	public:
		LoaderErrorCode error_code;

		SLAKE_API LoaderError(peff::Alloc *self_allocator, LoaderErrorCode error_code);
		SLAKE_API virtual ~LoaderError();
	};

	class BadMagicError : public LoaderError {
	public:
		SLAKE_API BadMagicError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~BadMagicError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static BadMagicError *alloc(peff::Alloc *self_allocator);
	};

	class ReadError : public LoaderError {
	public:
		SLAKE_API ReadError(peff::Alloc *self_allocator);
		SLAKE_API virtual ~ReadError();

		SLAKE_API virtual const char *what() const override;

		SLAKE_API virtual void dealloc() override;

		SLAKE_API static ReadError *alloc(peff::Alloc *self_allocator);
	};

	SLAKE_API InternalExceptionPointer alloc_out_of_memory_error_if_alloc_failed(InternalExceptionPointer e);
}

#endif
