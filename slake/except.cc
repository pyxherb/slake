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
	Runtime *associatedRuntime,
	RuntimeExecErrorCode errorCode)
	: InternalException(associatedRuntime, ErrorKind::RuntimeExecError),
	  errorCode(errorCode) {}
SLAKE_API RuntimeExecError::~RuntimeExecError() {}

SLAKE_API MismatchedVarTypeError::MismatchedVarTypeError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::MismatchedVarType) {}
SLAKE_API MismatchedVarTypeError::~MismatchedVarTypeError() {}

SLAKE_API const char *MismatchedVarTypeError::what() const {
	return "Mismatched variable type";
}

SLAKE_API void MismatchedVarTypeError::dealloc() {
	peff::destroyAndRelease<MismatchedVarTypeError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API MismatchedVarTypeError *MismatchedVarTypeError::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<MismatchedVarTypeError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API FrameBoundaryExceededError::FrameBoundaryExceededError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::FrameBoundaryExceeded) {}
SLAKE_API FrameBoundaryExceededError::~FrameBoundaryExceededError() {}

SLAKE_API void FrameBoundaryExceededError::dealloc() {
	peff::destroyAndRelease<FrameBoundaryExceededError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API const char *FrameBoundaryExceededError::what() const {
	return "Frame boundary exceeded";
}

SLAKE_API FrameBoundaryExceededError *FrameBoundaryExceededError::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<FrameBoundaryExceededError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API InvalidOpcodeError::InvalidOpcodeError(
	Runtime *associatedRuntime,
	Opcode opcode) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidOpcode), opcode(opcode) {}
SLAKE_API InvalidOpcodeError::~InvalidOpcodeError() {}

SLAKE_API const char *InvalidOpcodeError::what() const {
	return "Invalid opcode";
}

SLAKE_API void InvalidOpcodeError::dealloc() {
	peff::destroyAndRelease<InvalidOpcodeError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InvalidOpcodeError *InvalidOpcodeError::alloc(Runtime *associatedRuntime, Opcode opcode) {
	return peff::allocAndConstruct<InvalidOpcodeError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, opcode);
}

SLAKE_API InvalidOperandsError::InvalidOperandsError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidOperands) {}
SLAKE_API InvalidOperandsError::~InvalidOperandsError() {}

SLAKE_API const char *InvalidOperandsError::what() const {
	return "Invalid operands";
}

SLAKE_API void InvalidOperandsError::dealloc() {
	peff::destroyAndRelease<InvalidOperandsError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InvalidOperandsError *InvalidOperandsError::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<InvalidOperandsError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API InvalidLocalVarIndexError::InvalidLocalVarIndexError(
	Runtime *associatedRuntime,
	uint32_t index) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidLocalVarIndex), index(index) {}
SLAKE_API InvalidLocalVarIndexError::~InvalidLocalVarIndexError() {}

SLAKE_API const char *InvalidLocalVarIndexError::what() const {
	return "Invalid local variable index";
}

SLAKE_API void InvalidLocalVarIndexError::dealloc() {
	peff::destroyAndRelease<InvalidLocalVarIndexError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InvalidLocalVarIndexError *InvalidLocalVarIndexError::alloc(Runtime *associatedRuntime, uint32_t index) {
	return peff::allocAndConstruct<InvalidLocalVarIndexError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, index);
}

SLAKE_API InvalidArrayIndexError::InvalidArrayIndexError(
	Runtime *associatedRuntime,
	size_t index) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArrayIndex), index(index) {}
SLAKE_API InvalidArrayIndexError::~InvalidArrayIndexError() {}

SLAKE_API const char *InvalidArrayIndexError::what() const {
	return "Invalid array index";
}

SLAKE_API void InvalidArrayIndexError::dealloc() {
	peff::destroyAndRelease<InvalidArrayIndexError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InvalidArrayIndexError *InvalidArrayIndexError::alloc(Runtime *associatedRuntime, size_t index) {
	return peff::allocAndConstruct<InvalidArrayIndexError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, index);
}

SLAKE_API StackOverflowError::StackOverflowError(Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::StackOverflow) {}
SLAKE_API StackOverflowError::~StackOverflowError() {}

SLAKE_API const char *StackOverflowError::what() const {
	return "Stack overflow";
}

SLAKE_API void StackOverflowError::dealloc() {
	peff::destroyAndRelease<StackOverflowError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API StackOverflowError *StackOverflowError::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<StackOverflowError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API InvalidArgumentNumberError::InvalidArgumentNumberError(
	Runtime *associatedRuntime,
	uint32_t nArgs) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArgumentNumber), nArgs(nArgs) {}
SLAKE_API InvalidArgumentNumberError::~InvalidArgumentNumberError() {}

SLAKE_API const char *InvalidArgumentNumberError::what() const {
	return "Invalid argument number";
}

SLAKE_API void InvalidArgumentNumberError::dealloc() {
	peff::destroyAndRelease<InvalidArgumentNumberError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API InvalidArgumentNumberError *InvalidArgumentNumberError::alloc(Runtime *associatedRuntime, uint32_t nArgs) {
	return peff::allocAndConstruct<InvalidArgumentNumberError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, nArgs);
}

SLAKE_API ReferencedMemberNotFoundError::ReferencedMemberNotFoundError(
	Runtime *associatedRuntime,
	IdRefObject *idRef) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::ReferencedMemberNotFound), idRef(idRef) {}
SLAKE_API ReferencedMemberNotFoundError::~ReferencedMemberNotFoundError() {}

SLAKE_API const char *ReferencedMemberNotFoundError::what() const {
	return "Referenced member not found";
}

SLAKE_API void ReferencedMemberNotFoundError::dealloc() {
	peff::destroyAndRelease<ReferencedMemberNotFoundError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API ReferencedMemberNotFoundError *ReferencedMemberNotFoundError::alloc(
	Runtime *associatedRuntime,
	IdRefObject *idRef) {
	return peff::allocAndConstruct<ReferencedMemberNotFoundError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, idRef);
}

SLAKE_API NullRefError::NullRefError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::NullRef) {}
SLAKE_API NullRefError::~NullRefError() {}

SLAKE_API const char *NullRefError::what() const {
	return "Detected null reference";
}

SLAKE_API void NullRefError::dealloc() {
	peff::destroyAndRelease<NullRefError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API NullRefError *NullRefError::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<NullRefError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API UncaughtExceptionError::UncaughtExceptionError(
	Runtime *associatedRuntime,
	Value exceptionValue) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::UncaughtException), exceptionValue(exceptionValue) {}
SLAKE_API UncaughtExceptionError::~UncaughtExceptionError() {}

SLAKE_API const char *UncaughtExceptionError::what() const {
	return "Uncaught Slake exception";
}

SLAKE_API void UncaughtExceptionError::dealloc() {
	peff::destroyAndRelease<UncaughtExceptionError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API UncaughtExceptionError *UncaughtExceptionError::alloc(
	Runtime *associatedRuntime,
	Value exceptionValue) {
	return peff::allocAndConstruct<UncaughtExceptionError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, exceptionValue);
}

SLAKE_API MalformedClassStructureError::MalformedClassStructureError(
	Runtime *associatedRuntime,
	ClassObject *classObject) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::MalformedClassStructure), classObject(classObject) {}
SLAKE_API MalformedClassStructureError::~MalformedClassStructureError() {}

SLAKE_API const char *MalformedClassStructureError::what() const {
	return "Malformed class structure";
}

SLAKE_API void MalformedClassStructureError::dealloc() {
	peff::destroyAndRelease<MalformedClassStructureError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API MalformedClassStructureError *MalformedClassStructureError::alloc(
	Runtime *associatedRuntime,
	ClassObject *classObject) {
	return peff::allocAndConstruct<MalformedClassStructureError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, classObject);
}

SLAKE_API GenericInstantiationError::GenericInstantiationError(
	Runtime *associatedRuntime,
	GenericInstantiationErrorCode instantiationErrorCode)
	: RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::GenericInstantiationError),
	  instantiationErrorCode(instantiationErrorCode) {
}

SLAKE_API GenericInstantiationError::~GenericInstantiationError() {
}

SLAKE_API MismatchedGenericArgumentNumberError::MismatchedGenericArgumentNumberError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArgumentIndex) {}
SLAKE_API MismatchedGenericArgumentNumberError::~MismatchedGenericArgumentNumberError() {}

SLAKE_API const char *MismatchedGenericArgumentNumberError::what() const {
	return "Mismatched generic argument number";
}

SLAKE_API void MismatchedGenericArgumentNumberError::dealloc() {
	peff::destroyAndRelease<MismatchedGenericArgumentNumberError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API MismatchedGenericArgumentNumberError *MismatchedGenericArgumentNumberError::alloc(Runtime *associatedRuntime) {
	return peff::allocAndConstruct<MismatchedGenericArgumentNumberError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime);
}

SLAKE_API GenericParameterNotFoundError::GenericParameterNotFoundError(
	Runtime *associatedRuntime,
	peff::String &&name)
	: RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArgumentIndex),
	  name(std::move(name)) {}
SLAKE_API GenericParameterNotFoundError::~GenericParameterNotFoundError() {}

SLAKE_API const char *GenericParameterNotFoundError::what() const {
	return "Generic parameter not found";
}

SLAKE_API void GenericParameterNotFoundError::dealloc() {
	peff::destroyAndRelease<GenericParameterNotFoundError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API GenericParameterNotFoundError *GenericParameterNotFoundError::alloc(
	Runtime *associatedRuntime,
	peff::String &&name) {
	return peff::allocAndConstruct<GenericParameterNotFoundError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, std::move(name));
}

SLAKE_API OptimizerError::OptimizerError(
	Runtime *associatedRuntime,
	OptimizerErrorCode optimizerErrorCode)
	: InternalException(associatedRuntime, ErrorKind::OptimizerError),
	  optimizerErrorCode(optimizerErrorCode) {
}

SLAKE_API OptimizerError::~OptimizerError() {
}

SLAKE_API MalformedProgramError::MalformedProgramError(
	Runtime *associatedRuntime,
	RegularFnOverloadingObject *fnOverloading,
	size_t offIns)
	: OptimizerError(associatedRuntime,
		  OptimizerErrorCode::MalformedProgram),
	  fnOverloading(fnOverloading),
	  offIns(offIns) {}
SLAKE_API MalformedProgramError::~MalformedProgramError() {}

SLAKE_API const char *MalformedProgramError::what() const {
	return "Malformed program";
}

SLAKE_API void MalformedProgramError::dealloc() {
	peff::destroyAndRelease<MalformedProgramError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API MalformedProgramError *MalformedProgramError::alloc(
	Runtime *associatedRuntime,
	RegularFnOverloadingObject *fnOverloading,
	size_t offIns) {
	return peff::allocAndConstruct<MalformedProgramError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, fnOverloading, offIns);
}

SLAKE_API ErrorEvaluatingObjectTypeError::ErrorEvaluatingObjectTypeError(
	Runtime *associatedRuntime,
	Object *object)
	: OptimizerError(associatedRuntime,
		  OptimizerErrorCode::ErrorEvaluatingObjectType),
	  object(object) {}
SLAKE_API ErrorEvaluatingObjectTypeError::~ErrorEvaluatingObjectTypeError() {}

SLAKE_API const char *ErrorEvaluatingObjectTypeError::what() const {
	return "Error evaluating object type";
}

SLAKE_API void ErrorEvaluatingObjectTypeError::dealloc() {
	peff::destroyAndRelease<ErrorEvaluatingObjectTypeError>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API ErrorEvaluatingObjectTypeError *ErrorEvaluatingObjectTypeError::alloc(
	Runtime *associatedRuntime,
	Object *object) {
	return peff::allocAndConstruct<ErrorEvaluatingObjectTypeError>(&associatedRuntime->globalHeapPoolAlloc, sizeof(std::max_align_t), associatedRuntime, object);
}

SLAKE_API InternalExceptionPointer slake::allocOutOfMemoryErrorIfAllocFailed(InternalExceptionPointer e) {
	if (!e) {
		return OutOfMemoryError::alloc();
	}
	return e;
}
