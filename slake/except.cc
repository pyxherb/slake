#include <slake/runtime.h>

using namespace slake;

SLAKE_API OutOfMemoryError::OutOfMemoryError() : InternalException(nullptr, ErrorKind::OutOfMemoryError) {
}

SLAKE_API OutOfMemoryError::~OutOfMemoryError() {
}

SLAKE_API const char *OutOfMemoryError::what() const {
	return "Out of memory";
}

SLAKE_API void OutOfMemoryError::dealloc() {
	// DO NOT free the object since it is not on the heap.
}

SLAKE_API OutOfMemoryError *OutOfMemoryError::alloc() noexcept {
	return &g_global_out_of_memory_error;
}

OutOfMemoryError slake::g_global_out_of_memory_error = OutOfMemoryError();

SLAKE_API RuntimeExecError::RuntimeExecError(
	peff::Alloc *self_allocator,
	RuntimeExecErrorCode error_code)
	: InternalException(self_allocator, ErrorKind::RuntimeExecError),
	  error_code(error_code) {}
SLAKE_API RuntimeExecError::~RuntimeExecError() {}

SLAKE_API MismatchedVarTypeError::MismatchedVarTypeError(
	peff::Alloc *self_allocator) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::MismatchedVarType) {}
SLAKE_API MismatchedVarTypeError::~MismatchedVarTypeError() {}

SLAKE_API const char *MismatchedVarTypeError::what() const {
	return "Mismatched variable type";
}

SLAKE_API void MismatchedVarTypeError::dealloc() {
	peff::destroy_and_release<MismatchedVarTypeError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MismatchedVarTypeError *MismatchedVarTypeError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<MismatchedVarTypeError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API FrameBoundaryExceededError::FrameBoundaryExceededError(
	peff::Alloc *self_allocator) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::FrameBoundaryExceeded) {}
SLAKE_API FrameBoundaryExceededError::~FrameBoundaryExceededError() {}

SLAKE_API void FrameBoundaryExceededError::dealloc() {
	peff::destroy_and_release<FrameBoundaryExceededError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API const char *FrameBoundaryExceededError::what() const {
	return "Frame boundary exceeded";
}

SLAKE_API FrameBoundaryExceededError *FrameBoundaryExceededError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<FrameBoundaryExceededError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API InvalidOpcodeError::InvalidOpcodeError(
	peff::Alloc *self_allocator,
	Opcode opcode) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidOpcode), opcode(opcode) {}
SLAKE_API InvalidOpcodeError::~InvalidOpcodeError() {}

SLAKE_API const char *InvalidOpcodeError::what() const {
	return "Invalid opcode";
}

SLAKE_API void InvalidOpcodeError::dealloc() {
	peff::destroy_and_release<InvalidOpcodeError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidOpcodeError *InvalidOpcodeError::alloc(peff::Alloc *self_allocator, Opcode opcode) {
	return peff::alloc_and_construct<InvalidOpcodeError>(self_allocator, sizeof(std::max_align_t), self_allocator, opcode);
}

SLAKE_API InvalidOperandsError::InvalidOperandsError(
	peff::Alloc *self_allocator) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidOperands) {}
SLAKE_API InvalidOperandsError::~InvalidOperandsError() {}

SLAKE_API const char *InvalidOperandsError::what() const {
	return "Invalid operands";
}

SLAKE_API void InvalidOperandsError::dealloc() {
	peff::destroy_and_release<InvalidOperandsError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidOperandsError *InvalidOperandsError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<InvalidOperandsError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API InvalidArrayIndexError::InvalidArrayIndexError(
	peff::Alloc *self_allocator,
	size_t index) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArrayIndex), index(index) {}
SLAKE_API InvalidArrayIndexError::~InvalidArrayIndexError() {}

SLAKE_API const char *InvalidArrayIndexError::what() const {
	return "Invalid array index";
}

SLAKE_API void InvalidArrayIndexError::dealloc() {
	peff::destroy_and_release<InvalidArrayIndexError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidArrayIndexError *InvalidArrayIndexError::alloc(peff::Alloc *self_allocator, size_t index) {
	return peff::alloc_and_construct<InvalidArrayIndexError>(self_allocator, sizeof(std::max_align_t), self_allocator, index);
}

SLAKE_API StackOverflowError::StackOverflowError(peff::Alloc *self_allocator) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::StackOverflow) {}
SLAKE_API StackOverflowError::~StackOverflowError() {}

SLAKE_API const char *StackOverflowError::what() const {
	return "Stack overflow";
}

SLAKE_API void StackOverflowError::dealloc() {
	peff::destroy_and_release<StackOverflowError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API StackOverflowError *StackOverflowError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<StackOverflowError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API InvalidArgumentNumberError::InvalidArgumentNumberError(
	peff::Alloc *self_allocator,
	uint32_t num_args) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArgumentNumber), num_args(num_args) {}
SLAKE_API InvalidArgumentNumberError::~InvalidArgumentNumberError() {}

SLAKE_API const char *InvalidArgumentNumberError::what() const {
	return "Invalid argument number";
}

SLAKE_API void InvalidArgumentNumberError::dealloc() {
	peff::destroy_and_release<InvalidArgumentNumberError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidArgumentNumberError *InvalidArgumentNumberError::alloc(peff::Alloc *self_allocator, uint32_t num_args) {
	return peff::alloc_and_construct<InvalidArgumentNumberError>(self_allocator, sizeof(std::max_align_t), self_allocator, num_args);
}

SLAKE_API ReferencedMemberNotFoundError::ReferencedMemberNotFoundError(
	peff::Alloc *self_allocator,
	IdRefObject *id_ref) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::ReferencedMemberNotFound), id_ref(id_ref) {}
SLAKE_API ReferencedMemberNotFoundError::~ReferencedMemberNotFoundError() {}

SLAKE_API const char *ReferencedMemberNotFoundError::what() const {
	return "Referenced member not found";
}

SLAKE_API void ReferencedMemberNotFoundError::dealloc() {
	peff::destroy_and_release<ReferencedMemberNotFoundError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ReferencedMemberNotFoundError *ReferencedMemberNotFoundError::alloc(
	peff::Alloc *self_allocator,
	IdRefObject *id_ref) {
	return peff::alloc_and_construct<ReferencedMemberNotFoundError>(self_allocator, sizeof(std::max_align_t), self_allocator, id_ref);
}

SLAKE_API NullRefError::NullRefError(
	peff::Alloc *self_allocator) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::NullRef) {}
SLAKE_API NullRefError::~NullRefError() {}

SLAKE_API const char *NullRefError::what() const {
	return "Detected null reference";
}

SLAKE_API void NullRefError::dealloc() {
	peff::destroy_and_release<NullRefError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API NullRefError *NullRefError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<NullRefError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API UncaughtExceptionError::UncaughtExceptionError(
	peff::Alloc *self_allocator,
	Value exception_value) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::UncaughtException), exception_value(exception_value) {}
SLAKE_API UncaughtExceptionError::~UncaughtExceptionError() {}

SLAKE_API const char *UncaughtExceptionError::what() const {
	return "Uncaught Slake exception";
}

SLAKE_API void UncaughtExceptionError::dealloc() {
	peff::destroy_and_release<UncaughtExceptionError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API UncaughtExceptionError *UncaughtExceptionError::alloc(
	peff::Alloc *self_allocator,
	Value exception_value) {
	return peff::alloc_and_construct<UncaughtExceptionError>(self_allocator, sizeof(std::max_align_t), self_allocator, exception_value);
}

SLAKE_API MalformedClassStructureError::MalformedClassStructureError(
	peff::Alloc *self_allocator,
	ClassObject *class_object) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::MalformedClassStructure), class_object(class_object) {}
SLAKE_API MalformedClassStructureError::~MalformedClassStructureError() {}

SLAKE_API const char *MalformedClassStructureError::what() const {
	return "Malformed class structure";
}

SLAKE_API void MalformedClassStructureError::dealloc() {
	peff::destroy_and_release<MalformedClassStructureError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MalformedClassStructureError *MalformedClassStructureError::alloc(
	peff::Alloc *self_allocator,
	ClassObject *class_object) {
	return peff::alloc_and_construct<MalformedClassStructureError>(self_allocator, sizeof(std::max_align_t), self_allocator, class_object);
}

SLAKE_API MismatchedGenericArgumentNumberError::MismatchedGenericArgumentNumberError(
	peff::Alloc *self_allocator) : RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArgumentIndex) {}
SLAKE_API MismatchedGenericArgumentNumberError::~MismatchedGenericArgumentNumberError() {}

SLAKE_API const char *MismatchedGenericArgumentNumberError::what() const {
	return "Mismatched generic argument number";
}

SLAKE_API void MismatchedGenericArgumentNumberError::dealloc() {
	peff::destroy_and_release<MismatchedGenericArgumentNumberError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MismatchedGenericArgumentNumberError *MismatchedGenericArgumentNumberError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<MismatchedGenericArgumentNumberError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API GenericParameterNotFoundError::GenericParameterNotFoundError(
	peff::Alloc *self_allocator,
	peff::String &&name)
	: RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  name(std::move(name)) {}
SLAKE_API GenericParameterNotFoundError::~GenericParameterNotFoundError() {}

SLAKE_API const char *GenericParameterNotFoundError::what() const {
	return "Generic parameter not found";
}

SLAKE_API void GenericParameterNotFoundError::dealloc() {
	peff::destroy_and_release<GenericParameterNotFoundError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericParameterNotFoundError *GenericParameterNotFoundError::alloc(
	peff::Alloc *self_allocator,
	peff::String &&name) {
	return peff::alloc_and_construct<GenericParameterNotFoundError>(self_allocator, sizeof(std::max_align_t), self_allocator, std::move(name));
}

SLAKE_API GenericArgTypeError::GenericArgTypeError(
	peff::Alloc *self_allocator,
	peff::String &&name)
	: RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  name(std::move(name)) {}
SLAKE_API GenericArgTypeError::~GenericArgTypeError() {}

SLAKE_API const char *GenericArgTypeError::what() const {
	return "Generic argument type mismatched";
}

SLAKE_API void GenericArgTypeError::dealloc() {
	peff::destroy_and_release<GenericArgTypeError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericArgTypeError *GenericArgTypeError::alloc(
	peff::Alloc *self_allocator,
	peff::String &&name) {
	return peff::alloc_and_construct<GenericArgTypeError>(self_allocator, sizeof(std::max_align_t), self_allocator, std::move(name));
}

SLAKE_API GenericDuplicatedFnOverloadingError::GenericDuplicatedFnOverloadingError(
	peff::Alloc *self_allocator,
	FnObject *original_fn)
	: RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  original_fn(original_fn) {}
SLAKE_API GenericDuplicatedFnOverloadingError::~GenericDuplicatedFnOverloadingError() {}

SLAKE_API const char *GenericDuplicatedFnOverloadingError::what() const {
	return "Duplicated function overloadings detected during generic instantiation";
}

SLAKE_API void GenericDuplicatedFnOverloadingError::dealloc() {
	peff::destroy_and_release<GenericDuplicatedFnOverloadingError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericDuplicatedFnOverloadingError *GenericDuplicatedFnOverloadingError::alloc(
	peff::Alloc *self_allocator,
	FnObject *original_fn) {
	return peff::alloc_and_construct<GenericDuplicatedFnOverloadingError>(self_allocator, sizeof(std::max_align_t), self_allocator, original_fn);
}

SLAKE_API GenericFieldInitError::GenericFieldInitError(
	peff::Alloc *self_allocator,
	BasicModuleObject *object,
	size_t idx_record)
	: RuntimeExecError(self_allocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  object(object),
	  idx_record(idx_record) {}
SLAKE_API GenericFieldInitError::~GenericFieldInitError() {}

SLAKE_API const char *GenericFieldInitError::what() const {
	return "Generic parameter not found";
}

SLAKE_API void GenericFieldInitError::dealloc() {
	peff::destroy_and_release<GenericFieldInitError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericFieldInitError *GenericFieldInitError::alloc(
	peff::Alloc *self_allocator,
	BasicModuleObject *object,
	size_t idx_record) {
	return peff::alloc_and_construct<GenericFieldInitError>(self_allocator, sizeof(std::max_align_t), self_allocator, object, idx_record);
}

SLAKE_API OptimizerError::OptimizerError(
	peff::Alloc *self_allocator,
	OptimizerErrorCode optimizer_error_code)
	: InternalException(self_allocator, ErrorKind::OptimizerError),
	  optimizer_error_code(optimizer_error_code) {
}

SLAKE_API OptimizerError::~OptimizerError() {
}

SLAKE_API MalformedProgramError::MalformedProgramError(
	peff::Alloc *self_allocator,
	RegularFnOverloadingObject *fn_overloading,
	size_t off_ins)
	: OptimizerError(self_allocator,
		  OptimizerErrorCode::MalformedProgram),
	  fn_overloading(fn_overloading),
	  off_ins(off_ins) {}
SLAKE_API MalformedProgramError::~MalformedProgramError() {}

SLAKE_API const char *MalformedProgramError::what() const {
	return "Malformed program";
}

SLAKE_API void MalformedProgramError::dealloc() {
	peff::destroy_and_release<MalformedProgramError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MalformedProgramError *MalformedProgramError::alloc(
	peff::Alloc *self_allocator,
	RegularFnOverloadingObject *fn_overloading,
	size_t off_ins) {
	return peff::alloc_and_construct<MalformedProgramError>(self_allocator, sizeof(std::max_align_t), self_allocator, fn_overloading, off_ins);
}

SLAKE_API MalformedCfgError::MalformedCfgError(
	peff::Alloc *self_allocator,
	const opti::ControlFlowGraph *cfg,
	size_t idx_basic_block,
	size_t off_ins)
	: OptimizerError(self_allocator,
		  OptimizerErrorCode::MalformedCfg),
	  cfg(cfg),
	  idx_basic_block(idx_basic_block),
	  off_ins(off_ins) {}
SLAKE_API MalformedCfgError::~MalformedCfgError() {}

SLAKE_API const char *MalformedCfgError::what() const {
	return "Malformed CFG";
}

SLAKE_API void MalformedCfgError::dealloc() {
	peff::destroy_and_release<MalformedCfgError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MalformedCfgError *MalformedCfgError::alloc(
	peff::Alloc *self_allocator,
	const opti::ControlFlowGraph *cfg,
	size_t idx_basic_block,
	size_t off_ins) {
	return peff::alloc_and_construct<MalformedCfgError>(self_allocator, sizeof(std::max_align_t), self_allocator, cfg, idx_basic_block, off_ins);
}

SLAKE_API ErrorEvaluatingObjectTypeError::ErrorEvaluatingObjectTypeError(
	peff::Alloc *self_allocator,
	Object *object)
	: OptimizerError(self_allocator,
		  OptimizerErrorCode::ErrorEvaluatingObjectType),
	  object(object) {}
SLAKE_API ErrorEvaluatingObjectTypeError::~ErrorEvaluatingObjectTypeError() {}

SLAKE_API const char *ErrorEvaluatingObjectTypeError::what() const {
	return "Error evaluating object type";
}

SLAKE_API void ErrorEvaluatingObjectTypeError::dealloc() {
	peff::destroy_and_release<ErrorEvaluatingObjectTypeError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ErrorEvaluatingObjectTypeError *ErrorEvaluatingObjectTypeError::alloc(
	peff::Alloc *self_allocator,
	Object *object) {
	return peff::alloc_and_construct<ErrorEvaluatingObjectTypeError>(self_allocator, sizeof(std::max_align_t), self_allocator, object);
}

SLAKE_API InternalExceptionPointer slake::alloc_out_of_memory_error_if_alloc_failed(InternalExceptionPointer e) {
	if (!e) {
		return OutOfMemoryError::alloc();
	}
	return e;
}

SLAKE_API LoaderError::LoaderError(
	peff::Alloc *self_allocator,
	LoaderErrorCode error_code)
	: InternalException(self_allocator, ErrorKind::LoaderError),
	  error_code(error_code) {}
SLAKE_API LoaderError::~LoaderError() {}

SLAKE_API BadMagicError::BadMagicError(peff::Alloc *self_allocator)
	: LoaderError(self_allocator, LoaderErrorCode::BadMagicNumber) {}
SLAKE_API BadMagicError::~BadMagicError() {}

SLAKE_API const char *BadMagicError::what() const {
	return "Bad magic number";
}

SLAKE_API void BadMagicError::dealloc() {
	peff::destroy_and_release<BadMagicError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API BadMagicError *BadMagicError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<BadMagicError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API ReadError::ReadError(peff::Alloc *self_allocator)
	: LoaderError(self_allocator, LoaderErrorCode::ReadError) {}
SLAKE_API ReadError::~ReadError() {}

SLAKE_API const char *ReadError::what() const {
	return "I/O reading error";
}

SLAKE_API void ReadError::dealloc() {
	peff::destroy_and_release<ReadError>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ReadError *ReadError::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<ReadError>(self_allocator, sizeof(std::max_align_t), self_allocator);
}
