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
	return &g_globalOutOfMemoryError;
}

OutOfMemoryError slake::g_globalOutOfMemoryError = OutOfMemoryError();

SLAKE_API RuntimeExecError::RuntimeExecError(
	peff::Alloc *selfAllocator,
	RuntimeExecErrorCode errorCode)
	: InternalException(selfAllocator, ErrorKind::RuntimeExecError),
	  errorCode(errorCode) {}
SLAKE_API RuntimeExecError::~RuntimeExecError() {}

SLAKE_API MismatchedVarTypeError::MismatchedVarTypeError(
	peff::Alloc *selfAllocator) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::MismatchedVarType) {}
SLAKE_API MismatchedVarTypeError::~MismatchedVarTypeError() {}

SLAKE_API const char *MismatchedVarTypeError::what() const {
	return "Mismatched variable type";
}

SLAKE_API void MismatchedVarTypeError::dealloc() {
	peff::destroyAndRelease<MismatchedVarTypeError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MismatchedVarTypeError *MismatchedVarTypeError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<MismatchedVarTypeError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API FrameBoundaryExceededError::FrameBoundaryExceededError(
	peff::Alloc *selfAllocator) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::FrameBoundaryExceeded) {}
SLAKE_API FrameBoundaryExceededError::~FrameBoundaryExceededError() {}

SLAKE_API void FrameBoundaryExceededError::dealloc() {
	peff::destroyAndRelease<FrameBoundaryExceededError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API const char *FrameBoundaryExceededError::what() const {
	return "Frame boundary exceeded";
}

SLAKE_API FrameBoundaryExceededError *FrameBoundaryExceededError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<FrameBoundaryExceededError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API InvalidOpcodeError::InvalidOpcodeError(
	peff::Alloc *selfAllocator,
	Opcode opcode) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidOpcode), opcode(opcode) {}
SLAKE_API InvalidOpcodeError::~InvalidOpcodeError() {}

SLAKE_API const char *InvalidOpcodeError::what() const {
	return "Invalid opcode";
}

SLAKE_API void InvalidOpcodeError::dealloc() {
	peff::destroyAndRelease<InvalidOpcodeError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidOpcodeError *InvalidOpcodeError::alloc(peff::Alloc *selfAllocator, Opcode opcode) {
	return peff::allocAndConstruct<InvalidOpcodeError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, opcode);
}

SLAKE_API InvalidOperandsError::InvalidOperandsError(
	peff::Alloc *selfAllocator) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidOperands) {}
SLAKE_API InvalidOperandsError::~InvalidOperandsError() {}

SLAKE_API const char *InvalidOperandsError::what() const {
	return "Invalid operands";
}

SLAKE_API void InvalidOperandsError::dealloc() {
	peff::destroyAndRelease<InvalidOperandsError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidOperandsError *InvalidOperandsError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<InvalidOperandsError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API InvalidLocalVarIndexError::InvalidLocalVarIndexError(
	peff::Alloc *selfAllocator,
	uint32_t index) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidLocalVarIndex), index(index) {}
SLAKE_API InvalidLocalVarIndexError::~InvalidLocalVarIndexError() {}

SLAKE_API const char *InvalidLocalVarIndexError::what() const {
	return "Invalid local variable index";
}

SLAKE_API void InvalidLocalVarIndexError::dealloc() {
	peff::destroyAndRelease<InvalidLocalVarIndexError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidLocalVarIndexError *InvalidLocalVarIndexError::alloc(peff::Alloc *selfAllocator, uint32_t index) {
	return peff::allocAndConstruct<InvalidLocalVarIndexError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, index);
}

SLAKE_API InvalidArrayIndexError::InvalidArrayIndexError(
	peff::Alloc *selfAllocator,
	size_t index) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidArrayIndex), index(index) {}
SLAKE_API InvalidArrayIndexError::~InvalidArrayIndexError() {}

SLAKE_API const char *InvalidArrayIndexError::what() const {
	return "Invalid array index";
}

SLAKE_API void InvalidArrayIndexError::dealloc() {
	peff::destroyAndRelease<InvalidArrayIndexError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidArrayIndexError *InvalidArrayIndexError::alloc(peff::Alloc *selfAllocator, size_t index) {
	return peff::allocAndConstruct<InvalidArrayIndexError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, index);
}

SLAKE_API StackOverflowError::StackOverflowError(peff::Alloc *selfAllocator) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::StackOverflow) {}
SLAKE_API StackOverflowError::~StackOverflowError() {}

SLAKE_API const char *StackOverflowError::what() const {
	return "Stack overflow";
}

SLAKE_API void StackOverflowError::dealloc() {
	peff::destroyAndRelease<StackOverflowError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API StackOverflowError *StackOverflowError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<StackOverflowError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API InvalidArgumentNumberError::InvalidArgumentNumberError(
	peff::Alloc *selfAllocator,
	uint32_t nArgs) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidArgumentNumber), nArgs(nArgs) {}
SLAKE_API InvalidArgumentNumberError::~InvalidArgumentNumberError() {}

SLAKE_API const char *InvalidArgumentNumberError::what() const {
	return "Invalid argument number";
}

SLAKE_API void InvalidArgumentNumberError::dealloc() {
	peff::destroyAndRelease<InvalidArgumentNumberError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API InvalidArgumentNumberError *InvalidArgumentNumberError::alloc(peff::Alloc *selfAllocator, uint32_t nArgs) {
	return peff::allocAndConstruct<InvalidArgumentNumberError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, nArgs);
}

SLAKE_API ReferencedMemberNotFoundError::ReferencedMemberNotFoundError(
	peff::Alloc *selfAllocator,
	IdRefObject *idRef) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::ReferencedMemberNotFound), idRef(idRef) {}
SLAKE_API ReferencedMemberNotFoundError::~ReferencedMemberNotFoundError() {}

SLAKE_API const char *ReferencedMemberNotFoundError::what() const {
	return "Referenced member not found";
}

SLAKE_API void ReferencedMemberNotFoundError::dealloc() {
	peff::destroyAndRelease<ReferencedMemberNotFoundError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ReferencedMemberNotFoundError *ReferencedMemberNotFoundError::alloc(
	peff::Alloc *selfAllocator,
	IdRefObject *idRef) {
	return peff::allocAndConstruct<ReferencedMemberNotFoundError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, idRef);
}

SLAKE_API NullRefError::NullRefError(
	peff::Alloc *selfAllocator) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::NullRef) {}
SLAKE_API NullRefError::~NullRefError() {}

SLAKE_API const char *NullRefError::what() const {
	return "Detected null reference";
}

SLAKE_API void NullRefError::dealloc() {
	peff::destroyAndRelease<NullRefError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API NullRefError *NullRefError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<NullRefError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API UncaughtExceptionError::UncaughtExceptionError(
	peff::Alloc *selfAllocator,
	Value exceptionValue) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::UncaughtException), exceptionValue(exceptionValue) {}
SLAKE_API UncaughtExceptionError::~UncaughtExceptionError() {}

SLAKE_API const char *UncaughtExceptionError::what() const {
	return "Uncaught Slake exception";
}

SLAKE_API void UncaughtExceptionError::dealloc() {
	peff::destroyAndRelease<UncaughtExceptionError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API UncaughtExceptionError *UncaughtExceptionError::alloc(
	peff::Alloc *selfAllocator,
	Value exceptionValue) {
	return peff::allocAndConstruct<UncaughtExceptionError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, exceptionValue);
}

SLAKE_API MalformedClassStructureError::MalformedClassStructureError(
	peff::Alloc *selfAllocator,
	ClassObject *classObject) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::MalformedClassStructure), classObject(classObject) {}
SLAKE_API MalformedClassStructureError::~MalformedClassStructureError() {}

SLAKE_API const char *MalformedClassStructureError::what() const {
	return "Malformed class structure";
}

SLAKE_API void MalformedClassStructureError::dealloc() {
	peff::destroyAndRelease<MalformedClassStructureError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MalformedClassStructureError *MalformedClassStructureError::alloc(
	peff::Alloc *selfAllocator,
	ClassObject *classObject) {
	return peff::allocAndConstruct<MalformedClassStructureError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, classObject);
}

SLAKE_API GenericInstantiationError::GenericInstantiationError(
	peff::Alloc *selfAllocator,
	GenericInstantiationErrorCode instantiationErrorCode)
	: RuntimeExecError(selfAllocator, RuntimeExecErrorCode::GenericInstantiationError),
	  instantiationErrorCode(instantiationErrorCode) {
}

SLAKE_API GenericInstantiationError::~GenericInstantiationError() {
}

SLAKE_API MismatchedGenericArgumentNumberError::MismatchedGenericArgumentNumberError(
	peff::Alloc *selfAllocator) : RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidArgumentIndex) {}
SLAKE_API MismatchedGenericArgumentNumberError::~MismatchedGenericArgumentNumberError() {}

SLAKE_API const char *MismatchedGenericArgumentNumberError::what() const {
	return "Mismatched generic argument number";
}

SLAKE_API void MismatchedGenericArgumentNumberError::dealloc() {
	peff::destroyAndRelease<MismatchedGenericArgumentNumberError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MismatchedGenericArgumentNumberError *MismatchedGenericArgumentNumberError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<MismatchedGenericArgumentNumberError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API GenericParameterNotFoundError::GenericParameterNotFoundError(
	peff::Alloc *selfAllocator,
	peff::String &&name)
	: RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  name(std::move(name)) {}
SLAKE_API GenericParameterNotFoundError::~GenericParameterNotFoundError() {}

SLAKE_API const char *GenericParameterNotFoundError::what() const {
	return "Generic parameter not found";
}

SLAKE_API void GenericParameterNotFoundError::dealloc() {
	peff::destroyAndRelease<GenericParameterNotFoundError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericParameterNotFoundError *GenericParameterNotFoundError::alloc(
	peff::Alloc *selfAllocator,
	peff::String &&name) {
	return peff::allocAndConstruct<GenericParameterNotFoundError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, std::move(name));
}

SLAKE_API GenericArgTypeError::GenericArgTypeError(
	peff::Alloc *selfAllocator,
	peff::String &&name)
	: RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  name(std::move(name)) {}
SLAKE_API GenericArgTypeError::~GenericArgTypeError() {}

SLAKE_API const char *GenericArgTypeError::what() const {
	return "Generic argument type mismatched";
}

SLAKE_API void GenericArgTypeError::dealloc() {
	peff::destroyAndRelease<GenericArgTypeError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericArgTypeError *GenericArgTypeError::alloc(
	peff::Alloc *selfAllocator,
	peff::String &&name) {
	return peff::allocAndConstruct<GenericArgTypeError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, std::move(name));
}

SLAKE_API GenericFieldInitError::GenericFieldInitError(
	peff::Alloc *selfAllocator,
	ModuleObject *object,
	size_t idxRecord)
	: RuntimeExecError(selfAllocator, RuntimeExecErrorCode::InvalidArgumentIndex),
	  object(object),
	  idxRecord(idxRecord) {}
SLAKE_API GenericFieldInitError::~GenericFieldInitError() {}

SLAKE_API const char *GenericFieldInitError::what() const {
	return "Generic parameter not found";
}

SLAKE_API void GenericFieldInitError::dealloc() {
	peff::destroyAndRelease<GenericFieldInitError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API GenericFieldInitError *GenericFieldInitError::alloc(
	peff::Alloc *selfAllocator,
	ModuleObject *object,
	size_t idxRecord) {
	return peff::allocAndConstruct<GenericFieldInitError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, object, idxRecord);
}

SLAKE_API OptimizerError::OptimizerError(
	peff::Alloc *selfAllocator,
	OptimizerErrorCode optimizerErrorCode)
	: InternalException(selfAllocator, ErrorKind::OptimizerError),
	  optimizerErrorCode(optimizerErrorCode) {
}

SLAKE_API OptimizerError::~OptimizerError() {
}

SLAKE_API MalformedProgramError::MalformedProgramError(
	peff::Alloc *selfAllocator,
	RegularFnOverloadingObject *fnOverloading,
	size_t offIns)
	: OptimizerError(selfAllocator,
		  OptimizerErrorCode::MalformedProgram),
	  fnOverloading(fnOverloading),
	  offIns(offIns) {}
SLAKE_API MalformedProgramError::~MalformedProgramError() {}

SLAKE_API const char *MalformedProgramError::what() const {
	return "Malformed program";
}

SLAKE_API void MalformedProgramError::dealloc() {
	peff::destroyAndRelease<MalformedProgramError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MalformedProgramError *MalformedProgramError::alloc(
	peff::Alloc *selfAllocator,
	RegularFnOverloadingObject *fnOverloading,
	size_t offIns) {
	return peff::allocAndConstruct<MalformedProgramError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, fnOverloading, offIns);
}

SLAKE_API MalformedCfgError::MalformedCfgError(
	peff::Alloc *selfAllocator,
	const opti::ControlFlowGraph *cfg,
	size_t idxBasicBlock,
	size_t offIns)
	: OptimizerError(selfAllocator,
		  OptimizerErrorCode::MalformedCfg),
	  cfg(cfg),
	  idxBasicBlock(idxBasicBlock),
	  offIns(offIns) {}
SLAKE_API MalformedCfgError::~MalformedCfgError() {}

SLAKE_API const char *MalformedCfgError::what() const {
	return "Malformed CFG";
}

SLAKE_API void MalformedCfgError::dealloc() {
	peff::destroyAndRelease<MalformedCfgError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MalformedCfgError *MalformedCfgError::alloc(
	peff::Alloc *selfAllocator,
	const opti::ControlFlowGraph *cfg,
	size_t idxBasicBlock,
	size_t offIns) {
	return peff::allocAndConstruct<MalformedCfgError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, cfg, idxBasicBlock, offIns);
}

SLAKE_API ErrorEvaluatingObjectTypeError::ErrorEvaluatingObjectTypeError(
	peff::Alloc *selfAllocator,
	Object *object)
	: OptimizerError(selfAllocator,
		  OptimizerErrorCode::ErrorEvaluatingObjectType),
	  object(object) {}
SLAKE_API ErrorEvaluatingObjectTypeError::~ErrorEvaluatingObjectTypeError() {}

SLAKE_API const char *ErrorEvaluatingObjectTypeError::what() const {
	return "Error evaluating object type";
}

SLAKE_API void ErrorEvaluatingObjectTypeError::dealloc() {
	peff::destroyAndRelease<ErrorEvaluatingObjectTypeError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ErrorEvaluatingObjectTypeError *ErrorEvaluatingObjectTypeError::alloc(
	peff::Alloc *selfAllocator,
	Object *object) {
	return peff::allocAndConstruct<ErrorEvaluatingObjectTypeError>(selfAllocator, sizeof(std::max_align_t), selfAllocator, object);
}

SLAKE_API InternalExceptionPointer slake::allocOutOfMemoryErrorIfAllocFailed(InternalExceptionPointer e) {
	if (!e) {
		return OutOfMemoryError::alloc();
	}
	return e;
}

SLAKE_API LoaderError::LoaderError(
	peff::Alloc *selfAllocator,
	LoaderErrorCode errorCode)
	: InternalException(selfAllocator, ErrorKind::LoaderError),
	  errorCode(errorCode) {}
SLAKE_API LoaderError::~LoaderError() {}

SLAKE_API BadMagicError::BadMagicError(peff::Alloc *selfAllocator)
	: LoaderError(selfAllocator, LoaderErrorCode::BadMagicNumber) {}
SLAKE_API BadMagicError::~BadMagicError() {}

SLAKE_API const char *BadMagicError::what() const {
	return "Bad magic number";
}

SLAKE_API void BadMagicError::dealloc() {
	peff::destroyAndRelease<BadMagicError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API BadMagicError *BadMagicError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<BadMagicError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API ReadError::ReadError(peff::Alloc *selfAllocator)
	: LoaderError(selfAllocator, LoaderErrorCode::ReadError) {}
SLAKE_API ReadError::~ReadError() {}

SLAKE_API const char *ReadError::what() const {
	return "Bad magic number";
}

SLAKE_API void ReadError::dealloc() {
	peff::destroyAndRelease<ReadError>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API ReadError *ReadError::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<ReadError>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}
