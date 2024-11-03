#include <slake/runtime.h>

using namespace slake;

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
	std::pmr::polymorphic_allocator<MismatchedVarTypeError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API MismatchedVarTypeError *MismatchedVarTypeError::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<MismatchedVarTypeError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<MismatchedVarTypeError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

SLAKE_API FrameBoundaryExceededError::FrameBoundaryExceededError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::FrameBoundaryExceeded) {}
SLAKE_API FrameBoundaryExceededError::~FrameBoundaryExceededError() {}

SLAKE_API void FrameBoundaryExceededError::dealloc() {
	std::pmr::polymorphic_allocator<FrameBoundaryExceededError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API const char *FrameBoundaryExceededError::what() const {
	return "Frame boundary exceeded";
}

SLAKE_API FrameBoundaryExceededError *FrameBoundaryExceededError::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<FrameBoundaryExceededError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<FrameBoundaryExceededError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

SLAKE_API InvalidOpcodeError::InvalidOpcodeError(
	Runtime *associatedRuntime,
	Opcode opcode) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidOpcode), opcode(opcode) {}
SLAKE_API InvalidOpcodeError::~InvalidOpcodeError() {}

SLAKE_API const char *InvalidOpcodeError::what() const {
	return "Invalid opcode";
}

SLAKE_API void InvalidOpcodeError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidOpcodeError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidOpcodeError *InvalidOpcodeError::alloc(Runtime *associatedRuntime, Opcode opcode) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidOpcodeError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidOpcodeError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, opcode);

	return ptr.release();
}

SLAKE_API InvalidOperandsError::InvalidOperandsError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidOperands) {}
SLAKE_API InvalidOperandsError::~InvalidOperandsError() {}

SLAKE_API const char *InvalidOperandsError::what() const {
	return "Invalid operands";
}

SLAKE_API void InvalidOperandsError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidOperandsError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidOperandsError *InvalidOperandsError::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidOperandsError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidOperandsError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

SLAKE_API InvalidRegisterIndexError::InvalidRegisterIndexError(
	Runtime *associatedRuntime,
	uint32_t index) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidRegisterIndex), index(index) {}
SLAKE_API InvalidRegisterIndexError::~InvalidRegisterIndexError() {}

SLAKE_API const char *InvalidRegisterIndexError::what() const {
	return "Invalid register index";
}

SLAKE_API void InvalidRegisterIndexError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidRegisterIndexError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidRegisterIndexError *InvalidRegisterIndexError::alloc(Runtime *associatedRuntime, uint32_t index) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidRegisterIndexError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidRegisterIndexError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, index);

	return ptr.release();
}

SLAKE_API InvalidLocalVarIndexError::InvalidLocalVarIndexError(
	Runtime *associatedRuntime,
	uint32_t index) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidLocalVarIndex), index(index) {}
SLAKE_API InvalidLocalVarIndexError::~InvalidLocalVarIndexError() {}

SLAKE_API const char *InvalidLocalVarIndexError::what() const {
	return "Invalid local variable index";
}

SLAKE_API void InvalidLocalVarIndexError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidLocalVarIndexError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidLocalVarIndexError *InvalidLocalVarIndexError::alloc(Runtime *associatedRuntime, uint32_t index) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidLocalVarIndexError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidLocalVarIndexError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, index);

	return ptr.release();
}

SLAKE_API InvalidArgumentIndexError::InvalidArgumentIndexError(
	Runtime *associatedRuntime,
	uint32_t index) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArgumentIndex), index(index) {}
SLAKE_API InvalidArgumentIndexError::~InvalidArgumentIndexError() {}

SLAKE_API const char *InvalidArgumentIndexError::what() const {
	return "Invalid argument index";
}

SLAKE_API void InvalidArgumentIndexError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidArgumentIndexError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidArgumentIndexError *InvalidArgumentIndexError::alloc(Runtime *associatedRuntime, uint32_t index) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidArgumentIndexError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidArgumentIndexError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, index);

	return ptr.release();
}

SLAKE_API InvalidArrayIndexError::InvalidArrayIndexError(
	Runtime *associatedRuntime,
	size_t index) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArrayIndex), index(index) {}
SLAKE_API InvalidArrayIndexError::~InvalidArrayIndexError() {}

SLAKE_API const char *InvalidArrayIndexError::what() const {
	return "Invalid array index";
}

SLAKE_API void InvalidArrayIndexError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidArrayIndexError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidArrayIndexError *InvalidArrayIndexError::alloc(Runtime *associatedRuntime, size_t index) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidArrayIndexError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidArrayIndexError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, index);

	return ptr.release();
}

SLAKE_API StackOverflowError::StackOverflowError(Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::StackOverflow) {}
SLAKE_API StackOverflowError::~StackOverflowError() {}

SLAKE_API const char *StackOverflowError::what() const {
	return "Stack overflow";
}

SLAKE_API void StackOverflowError::dealloc() {
	std::pmr::polymorphic_allocator<StackOverflowError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API StackOverflowError *StackOverflowError::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<StackOverflowError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<StackOverflowError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

SLAKE_API InvalidArgumentNumberError::InvalidArgumentNumberError(
	Runtime *associatedRuntime,
	uint32_t nArgs) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArgumentNumber), nArgs(nArgs) {}
SLAKE_API InvalidArgumentNumberError::~InvalidArgumentNumberError() {}

SLAKE_API const char *InvalidArgumentNumberError::what() const {
	return "Invalid array index";
}

SLAKE_API void InvalidArgumentNumberError::dealloc() {
	std::pmr::polymorphic_allocator<InvalidArgumentNumberError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API InvalidArgumentNumberError *InvalidArgumentNumberError::alloc(Runtime *associatedRuntime, uint32_t nArgs) {
	using Alloc = std::pmr::polymorphic_allocator<InvalidArgumentNumberError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<InvalidArgumentNumberError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, nArgs);

	return ptr.release();
}

SLAKE_API ReferencedMemberNotFoundError::ReferencedMemberNotFoundError(
	Runtime *associatedRuntime,
	IdRefObject *idRef) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::ReferencedMemberNotFound), idRef(idRef) {}
SLAKE_API ReferencedMemberNotFoundError::~ReferencedMemberNotFoundError() {}

SLAKE_API const char *ReferencedMemberNotFoundError::what() const {
	return "Referenced member not found";
}

SLAKE_API void ReferencedMemberNotFoundError::dealloc() {
	std::pmr::polymorphic_allocator<ReferencedMemberNotFoundError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API ReferencedMemberNotFoundError *ReferencedMemberNotFoundError::alloc(
	Runtime *associatedRuntime,
	IdRefObject *idRef) {
	using Alloc = std::pmr::polymorphic_allocator<ReferencedMemberNotFoundError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<ReferencedMemberNotFoundError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, idRef);

	return ptr.release();
}

SLAKE_API NullRefError::NullRefError(
	Runtime *associatedRuntime) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::NullRef) {}
SLAKE_API NullRefError::~NullRefError() {}

SLAKE_API const char *NullRefError::what() const {
	return "Detected null reference";
}

SLAKE_API void NullRefError::dealloc() {
	std::pmr::polymorphic_allocator<NullRefError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API NullRefError *NullRefError::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<NullRefError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<NullRefError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

SLAKE_API UncaughtExceptionError::UncaughtExceptionError(
	Runtime *associatedRuntime,
	Value exceptionValue) : RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::UncaughtException), exceptionValue(exceptionValue) {}
SLAKE_API UncaughtExceptionError::~UncaughtExceptionError() {}

SLAKE_API const char *UncaughtExceptionError::what() const {
	return "Uncaught Slake exception";
}

SLAKE_API void UncaughtExceptionError::dealloc() {
	std::pmr::polymorphic_allocator<UncaughtExceptionError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API UncaughtExceptionError *UncaughtExceptionError::alloc(
	Runtime *associatedRuntime,
	Value exceptionValue) {
	using Alloc = std::pmr::polymorphic_allocator<UncaughtExceptionError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<UncaughtExceptionError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, exceptionValue);

	return ptr.release();
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
	std::pmr::polymorphic_allocator<MismatchedGenericArgumentNumberError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API MismatchedGenericArgumentNumberError *MismatchedGenericArgumentNumberError::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<MismatchedGenericArgumentNumberError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<MismatchedGenericArgumentNumberError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

SLAKE_API GenericParameterNotFoundError::GenericParameterNotFoundError(
	Runtime *associatedRuntime,
	std::pmr::string &&name)
	: RuntimeExecError(associatedRuntime, RuntimeExecErrorCode::InvalidArgumentIndex),
	  name(name) {}
SLAKE_API GenericParameterNotFoundError::~GenericParameterNotFoundError() {}

SLAKE_API const char *GenericParameterNotFoundError::what() const {
	return "Mismatched generic argument number";
}

SLAKE_API void GenericParameterNotFoundError::dealloc() {
	std::pmr::polymorphic_allocator<GenericParameterNotFoundError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API GenericParameterNotFoundError *GenericParameterNotFoundError::alloc(
	Runtime *associatedRuntime,
	std::pmr::string &&name) {
	using Alloc = std::pmr::polymorphic_allocator<GenericParameterNotFoundError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<GenericParameterNotFoundError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, std::move(name));

	return ptr.release();
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
	std::pmr::polymorphic_allocator<MalformedProgramError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API MalformedProgramError *MalformedProgramError::alloc(
	Runtime *associatedRuntime,
	RegularFnOverloadingObject *fnOverloading,
	size_t offIns) {
	using Alloc = std::pmr::polymorphic_allocator<MalformedProgramError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<MalformedProgramError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, fnOverloading, offIns);

	return ptr.release();
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
	std::pmr::polymorphic_allocator<ErrorEvaluatingObjectTypeError> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API ErrorEvaluatingObjectTypeError *ErrorEvaluatingObjectTypeError::alloc(
	Runtime *associatedRuntime,
	Object *object) {
	using Alloc = std::pmr::polymorphic_allocator<ErrorEvaluatingObjectTypeError>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<ErrorEvaluatingObjectTypeError, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime, object);

	return ptr.release();
}
