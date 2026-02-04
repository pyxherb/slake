#include "../runtime.h"
#include <slake/flib/math/fmod.h>
#include <slake/flib/bitop.h>
#include <slake/flib/cmp.h>
#include <peff/base/scope_guard.h>
#include <cmath>

using namespace slake;

template <bool hasOutput, size_t nOperands>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandCount(
	Runtime *runtime,
	size_t output,
	size_t nOperandsIn) noexcept {
	if constexpr (hasOutput) {
		if ((output == UINT32_MAX) || (nOperands != nOperandsIn))
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	} else {
		if (nOperands != nOperandsIn) {
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
	}

	return {};
}

template <ValueType valueType>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandType(
	Runtime *runtime,
	const Value &operand) noexcept {
	if (operand.valueType != valueType) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

template <ReferenceKind kind>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectRefOperandType(
	Runtime *runtime,
	const Reference &operand) noexcept {
	if (operand.kind != kind) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

template <ObjectKind typeId>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectOperandType(
	Runtime *runtime,
	const Object *const object) noexcept {
	if (object && object->getObjectKind() != typeId) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

#define _isRegisterValid(curMajorFrame, index) ((index) < (curMajorFrame)->resumableContextData->nRegs)

static SLAKE_FORCEINLINE Value *_calcRegPtr(
	char *stackData,
	size_t stackSize,
	const MajorFrame *curMajorFrame,
	uint32_t index) noexcept {
	return (Value *)calcStackAddr(stackData, stackSize, curMajorFrame->offRegs + sizeof(Value) * index + sizeof(Value));
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _setRegisterValue(
	Runtime *runtime,
	char *stackData,
	const size_t stackSize,
	const MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) noexcept {
	if (!_isRegisterValid(curMajorFrame, index)) {
		// The register does not present.
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	*_calcRegPtr(stackData, stackSize, curMajorFrame, index) = value;
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _unwrapRegOperand(
	Runtime *const runtime,
	const char *stackData,
	const size_t stackSize,
	const MajorFrame *curMajorFrame,
	const Value &value,
	Value &valueOut) noexcept {
	if (value.valueType == ValueType::RegIndex) {
		if (value.data.asU32 >= curMajorFrame->resumableContextData->nRegs) {
			// The register does not present.
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
		valueOut = *(Value *)calcStackAddr(stackData, stackSize, curMajorFrame->offRegs + sizeof(Value) * (value.data.asU32 + 1));
		return {};
	}
	valueOut = value;
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _unwrapRegOperandIntoPtr(
	Runtime *const runtime,
	const char *stackData,
	const size_t stackSize,
	const MajorFrame *curMajorFrame,
	const Value &value,
	const Value *&valueOut) noexcept {
	if (value.valueType == ValueType::RegIndex) {
		if (value.data.asU32 >= curMajorFrame->resumableContextData->nRegs) {
			// The register does not present.
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
		valueOut = (Value *)calcStackAddr(stackData, stackSize, curMajorFrame->offRegs + sizeof(Value) * (((size_t)value.data.asU32) + 1));
		return {};
	}
	valueOut = &value;
	return {};
}

template <typename LT>
static void _castToLiteralValue(const Value &x, Value &valueOut) noexcept {
	switch (x.valueType) {
		case ValueType::I8:
			valueOut = ((LT)(x.getI8()));
			break;
		case ValueType::I16:
			valueOut = ((LT)(x.getI16()));
			break;
		case ValueType::I32:
			valueOut = ((LT)(x.getI32()));
			break;
		case ValueType::I64:
			valueOut = ((LT)(x.getI64()));
			break;
		case ValueType::ISize:
			valueOut = ((LT)(x.getISize()));
			break;
		case ValueType::U8:
			valueOut = ((LT)(x.getU8()));
			break;
		case ValueType::U16:
			valueOut = ((LT)(x.getU16()));
			break;
		case ValueType::U32:
			valueOut = ((LT)(x.getU32()));
			break;
		case ValueType::U64:
			valueOut = ((LT)(x.getU64()));
			break;
		case ValueType::USize:
			valueOut = ((LT)(x.getUSize()));
			break;
		case ValueType::F32:
			valueOut = ((LT)(x.getF32()));
			break;
		case ValueType::F64:
			valueOut = ((LT)(x.getF64()));
			break;
		case ValueType::Bool:
			valueOut = ((LT)(x.getBool()));
			break;
		default:
			std::terminate();
	}
}

SLAKE_API InternalExceptionPointer Runtime::_fillArgs(
	Context *context,
	MajorFrame *newMajorFrame,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	HostRefHolder &holder) {
	if (nArgs < fn->paramTypes.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidArgumentNumberError::alloc(getFixedAlloc(), nArgs));
	}

	for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
		TypeRef t = fn->paramTypes.at(i);
		if (!isCompatible(t, args[i]))
			return MismatchedVarTypeError::alloc(getFixedAlloc());
	}
	char *pArgs = context->alignedStackAlloc(sizeof(Value) * nArgs, alignof(Value));
	if (!pArgs)
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	size_t offArgs = newMajorFrame->curCoroutine ? context->stackTop - newMajorFrame->curCoroutine->offStackTop : context->stackTop;
	memcpy(pArgs, args, sizeof(Value) * nArgs);
	newMajorFrame->resumableContextData->offArgs = offArgs;
	newMajorFrame->resumableContextData->nArgs = nArgs;

	return {};
}

SLAKE_API AllocaRecord* Runtime::_allocAllocaRecord(Context *context, const MajorFrame *frame, uint32_t outputReg) {
	MinorFrame *mf = _fetchMinorFrame(context, frame, frame->resumableContextData->offCurMinorFrame);
	char *pRecord;
	if (!(pRecord = context->alignedStackAlloc(sizeof(AllocaRecord), alignof(AllocaRecord))))
		return nullptr;
	AllocaRecord *record = (AllocaRecord *)pRecord;
	record->defReg = outputReg;
	record->offNext = mf->offAllocaRecords;
	mf->offAllocaRecords = frame->curCoroutine
							   ? context->stackTop - frame->curCoroutine->offStackTop
							   : context->stackTop;
	return record;
}

SLAKE_API MinorFrame *Runtime::_fetchMinorFrame(
	Context *context,
	const MajorFrame *majorFrame,
	size_t stackOffset) {
	size_t offset;
	if (majorFrame->curCoroutine) {
		offset = stackOffset + majorFrame->curCoroutine->offStackTop;
	} else {
		offset = stackOffset;
	}

	return (MinorFrame *)calcStackAddr(context->dataStack,
		context->stackSize,
		offset);
}

SLAKE_API Value *Runtime::_fetchArgStack(
	char *dataStack,
	size_t stackSize,
	const MajorFrame *majorFrame,
	size_t stackOffset,
	size_t nArgs) const {
	if (!nArgs)
		return nullptr;
	size_t offset;
	if (majorFrame && majorFrame->curCoroutine) {
		offset = stackOffset + majorFrame->curCoroutine->offStackTop;
	} else {
		offset = stackOffset;
	}

	return (Value *)calcStackAddr(dataStack,
		stackSize,
		offset);
}

SLAKE_API AllocaRecord *Runtime::_fetchAllocaRecord(
	Context *context,
	const MajorFrame *majorFrame,
	size_t stackOffset) {
	size_t offset;
	if (majorFrame->curCoroutine) {
		offset = stackOffset + majorFrame->curCoroutine->offStackTop;
	} else {
		offset = stackOffset;
	}

	return (AllocaRecord *)calcStackAddr(context->dataStack,
		context->stackSize,
		offset);
}

SLAKE_API MajorFrame *Runtime::_fetchMajorFrame(
	Context *context,
	size_t stackOffset) const {
	return (MajorFrame *)calcStackAddr(context->dataStack,
		context->stackSize,
		stackOffset);
}

SLAKE_API ExceptHandler *Runtime::_fetchExceptHandler(
	Context *context,
	MajorFrame *majorFrame,
	size_t stackOffset) {
	size_t offset;
	if (majorFrame->curCoroutine) {
		offset = stackOffset + majorFrame->curCoroutine->offStackTop;
	} else {
		offset = stackOffset;
	}

	return (ExceptHandler *)calcStackAddr(context->dataStack,
		context->stackSize,
		offset);
}

SLAKE_API InternalExceptionPointer Runtime::_createNewCoroutineMajorFrame(
	Context *context,
	CoroutineObject *coroutine,
	uint32_t returnValueOut,
	const Reference *returnStructRef) noexcept {
	HostRefHolder holder(context->runtime->getFixedAlloc());

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop, coroutine]() noexcept {
		context->stackTop = prevStackTop;
		coroutine->offStackTop = 0;
	});

	// TODO: Restore resumable context data.

	char *pMajorFrame;
	if (!(pMajorFrame = context->alignedStackAlloc(sizeof(MajorFrame), alignof(MajorFrame))))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	peff::constructAt<MajorFrame>((MajorFrame *)pMajorFrame, this);
	MajorFrame &newMajorFrame = *(MajorFrame *)pMajorFrame;
	newMajorFrame.offPrevFrame = context->offCurMajorFrame;

	if (coroutine->resumable.hasValue()) {
		newMajorFrame.resumableContextData = std::move(coroutine->resumable);
	} else {
		newMajorFrame.resumableContextData = ResumableContextData(context->selfAllocator.get());
	}

	newMajorFrame.curFn = coroutine->overloading;
	newMajorFrame.curCoroutine = coroutine;

	newMajorFrame.offRegs = context->stackTop + coroutine->offRegs;

	size_t offMajorFrame = context->stackTop;

	if (coroutine->stackData) {
		if (!context->alignStack(alignof(std::max_align_t)))
			return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		coroutine->offStackTop = context->stackTop;
		char *initialData = context->alignedStackAlloc(coroutine->lenStackData, alignof(std::max_align_t));
		if (!initialData) {
			return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
		memcpy(initialData, coroutine->stackData, coroutine->lenStackData);
		coroutine->releaseStackData();
	} else {
		// Create minor frame.
		if (!context->alignedStackAlloc(sizeof(MinorFrame), alignof(MinorFrame)))
			return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

		size_t mfStackOff = context->stackTop;

		MinorFrame *mf = _fetchMinorFrame(context, &newMajorFrame, context->stackTop);

		peff::constructAt<MinorFrame>(mf);

		if (newMajorFrame.curCoroutine) {
			mfStackOff -= newMajorFrame.curCoroutine->offStackTop;
		}

		mf->offLastMinorFrame = newMajorFrame.resumableContextData->offCurMinorFrame;
		mf->stackBase = newMajorFrame.curCoroutine ? prevStackTop - newMajorFrame.curCoroutine->offStackTop : prevStackTop;
		newMajorFrame.resumableContextData->offCurMinorFrame = mfStackOff;

		switch (coroutine->overloading->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)coroutine->overloading;
				newMajorFrame.resumableContextData->nRegs = ol->nRegisters;
				newMajorFrame.offRegs = context->stackTop;
				Value *regs = (Value *)context->stackAlloc(sizeof(Value) * ol->nRegisters);
				if (!regs)
					return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
				for (size_t i = 0; i < ol->nRegisters; ++i)
					regs[i] = Value(ValueType::Undefined);
				break;
			}
			default:;
		}
	}

	newMajorFrame.returnValueOutReg = returnValueOut;
	if (returnStructRef)
		newMajorFrame.returnStructRef = *returnStructRef;

	newMajorFrame.stackBase = prevStackTop;

	coroutine->bindToContext(context, &newMajorFrame);

	restoreStackTopGuard.release();

	if (context->offCurMajorFrame != SIZE_MAX)
		_fetchMajorFrame(context, context->offCurMajorFrame)->offNextFrame = offMajorFrame;
	context->offCurMajorFrame = offMajorFrame;
	++context->nMajorFrames;
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_createNewMajorFrame(
	Context *context,
	Object *thisObject,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	uint32_t returnValueOut,
	const Reference *returnStructRef) noexcept {
	HostRefHolder holder(getFixedAlloc());

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop]() noexcept {
		context->stackTop = prevStackTop;
	});

	// TODO: Restore resumable context data.

	char *pMajorFrame;
	if (!(pMajorFrame = context->stackAlloc(sizeof(MajorFrame))))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	peff::constructAt<MajorFrame>((MajorFrame *)pMajorFrame, this);
	MajorFrame &newMajorFrame = *(MajorFrame *)pMajorFrame;
	newMajorFrame.offPrevFrame = context->offCurMajorFrame;

	newMajorFrame.resumableContextData = ResumableContextData(context->selfAllocator.get());

	size_t offMajorFrame = context->stackTop;

	// Create minor frame.
	if (!context->alignedStackAlloc(sizeof(MinorFrame), alignof(MinorFrame)))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

	size_t mfStackOff = context->stackTop;

	MinorFrame *mf = _fetchMinorFrame(context, &newMajorFrame, context->stackTop);

	peff::constructAt<MinorFrame>(mf);

	if (newMajorFrame.curCoroutine) {
		mfStackOff -= newMajorFrame.curCoroutine->offStackTop;
	}

	mf->offLastMinorFrame = newMajorFrame.resumableContextData->offCurMinorFrame;
	mf->stackBase = newMajorFrame.curCoroutine ? prevStackTop - newMajorFrame.curCoroutine->offStackTop : prevStackTop;
	newMajorFrame.resumableContextData->offCurMinorFrame = mfStackOff;

	if (!fn) {
		// Used in the creation of top major frame.
		newMajorFrame.curFn = nullptr;
		newMajorFrame.resumableContextData->nRegs = 1;
		newMajorFrame.offRegs = context->stackTop;
		Value *regs = (Value *)context->alignedStackAlloc(sizeof(Value) * 1, alignof(Value));
		*regs = Value(ValueType::Undefined);
	} else {
		newMajorFrame.curFn = fn;
		newMajorFrame.resumableContextData->thisObject = thisObject;

		SLAKE_RETURN_IF_EXCEPT(_fillArgs(context, &newMajorFrame, fn, args, nArgs, holder));

		switch (fn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn;
				newMajorFrame.resumableContextData->nRegs = ol->nRegisters;
				newMajorFrame.offRegs = context->stackTop;
				Value *regs = (Value *)context->alignedStackAlloc(sizeof(Value) * ol->nRegisters, alignof(Value));
				memset(regs, 0, sizeof(Value) * ol->nRegisters);
				break;
			}
			default:
				break;
		}
	}

	newMajorFrame.returnValueOutReg = returnValueOut;
	if (returnStructRef)
		newMajorFrame.returnStructRef = *returnStructRef;
	newMajorFrame.stackBase = prevStackTop;

	restoreStackTopGuard.release();

	if (context->offCurMajorFrame != SIZE_MAX)
		_fetchMajorFrame(context, context->offCurMajorFrame)->offNextFrame = offMajorFrame;
	context->offCurMajorFrame = offMajorFrame;
	++context->nMajorFrames;

	return {};
}

SLAKE_API void Runtime::_leaveMajorFrame(Context *context) noexcept {
	MajorFrame *mf = _fetchMajorFrame(context, context->offCurMajorFrame), *pmf;

	assert(mf->offNextFrame == SIZE_MAX);

	if (mf->offPrevFrame != SIZE_MAX) {
		pmf = _fetchMajorFrame(context, mf->offPrevFrame);

		pmf->offNextFrame = SIZE_MAX;
	}

	context->offCurMajorFrame = mf->offPrevFrame;
	context->stackTop = mf->stackBase;
	--context->nMajorFrames;

	std::destroy_at<MajorFrame>(mf);
}

SLAKE_FORCEINLINE InternalExceptionPointer slake::Runtime::_addLocalVar(Context *context, const MajorFrame *frame, TypeRef type, uint32_t outputReg, Reference &objectRefOut) noexcept {
	size_t originalStackTop = context->stackTop;

	peff::ScopeGuard restoreStackTopGuard([originalStackTop, context]() noexcept {
		context->stackTop = originalStackTop;
	});

	switch (type.typeId) {
		case TypeId::StructInstance: {
			assert(type.getCustomTypeDef()->typeObject->getObjectKind() == ObjectKind::Struct);
			SLAKE_RETURN_IF_EXCEPT(prepareStructForInstantiation((StructObject *)type.getCustomTypeDef()->typeObject));
			break;
		}
		default:
			break;
	}

	size_t size = sizeofType(type), align = alignofType(type);

	if (!context->alignedStackAlloc(size, align))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

	switch (type.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
		case TypeId::String:
		case TypeId::Any:
			break;
		case TypeId::Instance:
		case TypeId::GenericArg:
		case TypeId::Array:
		case TypeId::Ref: {
			// The data is already aligned, just directly assign to them.
			Object **typeInfo = (Object **)context->stackAlloc(sizeof(void *));
			if (!typeInfo)
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
#ifndef _NDEBUG
			const size_t diff = alignof(void *) - ((uintptr_t)(calcStackAddr(context->dataStack, context->stackSize, context->stackTop)) % alignof(void *));
			assert((diff == alignof(void *) || (!diff)));
#endif
			*typeInfo = type.typeDef;
			break;
		}
		case TypeId::StructInstance: {
			TypeDefObject **typeInfo = (TypeDefObject **)context->stackAlloc(sizeof(void *));
			if (!typeInfo)
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
			memcpy(typeInfo, &type.typeDef, sizeof(void *));
			break;
		}
		default:
			std::terminate();
	}

	TypeModifier *typeModifier = (TypeModifier *)context->stackAlloc(sizeof(TypeModifier));
	if (!typeModifier)
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	*typeModifier = type.typeModifier;

	TypeId *typeId = (TypeId *)context->stackAlloc(sizeof(TypeId));
	if (!typeId)
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	*typeId = type.typeId;

	size_t offOut = context->stackTop;

	{
		MinorFrame *mf = _fetchMinorFrame(context, frame, frame->resumableContextData->offCurMinorFrame);
		char *pRecord;
		if (!(pRecord = context->alignedStackAlloc(sizeof(AllocaRecord), alignof(AllocaRecord))))
			return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		AllocaRecord *record = (AllocaRecord *)pRecord;
		record->defReg = outputReg;
		record->offNext = mf->offAllocaRecords;
		mf->offAllocaRecords = frame->curCoroutine
								   ? context->stackTop - frame->curCoroutine->offStackTop
								   : context->stackTop;
	}
	if(!_allocAllocaRecord(context, frame, outputReg))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

	restoreStackTopGuard.release();

	if (frame->curCoroutine)
		objectRefOut = CoroutineLocalVarRef(frame->curCoroutine, offOut - frame->curCoroutine->offStackTop);
	else
		objectRefOut = LocalVarRef(context, offOut);
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(Context *context, MajorFrame *majorFrame, Runtime *rt, uint32_t off, Reference &objectRefOut) {
	if (off >= majorFrame->resumableContextData->nArgs) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(rt->getFixedAlloc()));
	}

	if (majorFrame->curCoroutine) {
		objectRefOut = CoroutineArgRef(majorFrame->curCoroutine, off);
	} else {
		objectRefOut = ArgRef(majorFrame, context->dataStack, context->stackSize, off);
	}
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer Runtime::_execIns(ContextObject *const context, MajorFrame *const curMajorFrame, const Opcode opcode, const size_t output, const size_t nOperands, const Value *const operands, bool &isContextChangedOut) noexcept {
	InternalExceptionPointer exceptPtr;
	char *const dataStack = context->_context.dataStack;
	const size_t stackSize = context->_context.stackSize;

	switch (opcode) {
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			const Value *dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, *dest));

			if (!_isRegisterValid(curMajorFrame, output)) {
				// The register does not present.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			readVar(dest->getReference(), *_calcRegPtr(dataStack, stackSize, curMajorFrame, output));

			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

			const Value *destValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], destValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, *destValue));

			const Value *data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], data));

			if (!isCompatible(typeofVar(destValue->getReference()), *data))
				return MismatchedVarTypeError::alloc(getFixedAlloc());
			writeVar(destValue->getReference(), *data);
			break;
		}
		case Opcode::JMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[0]));

			curMajorFrame->resumableContextData->lastJumpSrc = curMajorFrame->resumableContextData->curIns;
			curMajorFrame->resumableContextData->curIns = operands[0].getU32();
			return {};
		}
		case Opcode::BR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 3>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[1]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[2]));
			const Value *condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Bool>(this, *condition));

			curMajorFrame->resumableContextData->lastJumpSrc = curMajorFrame->resumableContextData->curIns;
			curMajorFrame->resumableContextData->curIns = operands[((uint8_t)!condition->getBool()) + 1].getU32();
			return {};
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() + y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() + y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() + y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() + y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() + y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() + y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() + y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() + y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() + y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() + y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() - y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() - y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() - y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() - y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() - y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() - y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() - y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() - y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() - y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() - y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() * y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() * y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() * y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() * y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() * y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() * y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() * y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() * y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() * y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() * y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() / y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() / y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() / y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() / y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() / y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() / y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() / y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() / y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() / y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() / y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() % y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() % y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() % y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() % y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() % y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() % y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() % y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() % y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)flib::fmodf(x->getF32(), y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)flib::fmod(x->getF64(), y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::AND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() & y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() & y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() & y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() & y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() & y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() & y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() & y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() & y->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::OR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() | y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() | y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() | y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() | y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() | y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() | y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() | y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() | y->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::XOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() ^ y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() ^ y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() ^ y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() ^ y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() ^ y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() ^ y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() ^ y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() ^ y->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LAND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x->getBool() && y->getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x->getBool() || y->getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::EQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() == y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() == y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() == y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() == y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() == y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() == y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() == y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() == y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() == y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() == y->getF64());
					break;
				case ValueType::Bool:
					valueOut = (bool)(x->getBool() == y->getBool());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::NEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() != y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() != y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() != y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() != y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() != y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() != y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() != y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() != y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() != y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() != y->getF64());
					break;
				case ValueType::Bool:
					valueOut = (bool)(x->getBool() != y->getBool());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() < y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() < y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() < y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() < y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() < y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() < y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() < y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() < y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() < y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() < y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::GT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() > y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() > y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() > y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() > y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() > y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() > y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() > y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() > y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() > y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() > y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() <= y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() <= y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() <= y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() <= y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() <= y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() <= y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() <= y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() <= y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() <= y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() <= y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::GTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() >= y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() >= y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() >= y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() >= y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() >= y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() >= y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() >= y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() >= y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() >= y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() >= y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			uint64_t lhs = x->getU64(), rhs = y->getU64();
			if (lhs > rhs) {
				valueOut = Value((int32_t)1);
			} else if (lhs < rhs) {
				valueOut = Value((int32_t)-1);
			} else
				valueOut = Value((int32_t)0);
			break;
		}
		case Opcode::CMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int32_t)flib::compareI8(x->getI8(), y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int32_t)flib::compareI16(x->getI16(), y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)flib::compareI32(x->getI32(), y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int32_t)flib::compareI64(x->getI64(), y->getI64());
					break;
				case ValueType::U8:
					valueOut = (int32_t)flib::compareU8(x->getU8(), y->getU8());
					break;
				case ValueType::U16:
					valueOut = (int32_t)flib::compareU16(x->getU16(), y->getU16());
					break;
				case ValueType::U32:
					valueOut = (int32_t)flib::compareU32(x->getU32(), y->getU32());
					break;
				case ValueType::U64:
					valueOut = (int32_t)flib::compareU64(x->getU64(), y->getU64());
					break;
				case ValueType::F32:
					valueOut = (int32_t)flib::compareF32(x->getF32(), y->getF32());
					break;
				case ValueType::F64:
					valueOut = (int32_t)flib::compareF64(x->getF64(), y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			break;
		}
		case Opcode::LSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, *y));

			uint32_t rhs = y->getU32();

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = flib::shlSigned8(x->getI8(), rhs);
					break;
				case ValueType::I16:
					valueOut = flib::shlSigned16(x->getI16(), rhs);
					break;
				case ValueType::I32:
					valueOut = flib::shlSigned32(x->getI32(), rhs);
					break;
				case ValueType::I64:
					valueOut = flib::shlSigned64(x->getI64(), rhs);
					break;
				case ValueType::U8:
					valueOut = flib::shlUnsigned8(x->getU8(), rhs);
					break;
				case ValueType::U16:
					valueOut = flib::shlUnsigned16(x->getU16(), rhs);
					break;
				case ValueType::U32:
					valueOut = flib::shlUnsigned32(x->getU32(), rhs);
					break;
				case ValueType::U64:
					valueOut = flib::shlUnsigned64(x->getU64(), rhs);
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, y));

			uint32_t rhs = y->getU32();

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = flib::shrSigned8(x->getI8(), rhs);
					break;
				case ValueType::I16:
					valueOut = flib::shrSigned16(x->getI16(), rhs);
					break;
				case ValueType::I32:
					valueOut = flib::shrSigned32(x->getI32(), rhs);
					break;
				case ValueType::I64:
					valueOut = flib::shrSigned64(x->getI64(), rhs);
					break;
				case ValueType::U8:
					valueOut = flib::shrUnsigned8(x->getU8(), rhs);
					break;
				case ValueType::U16:
					valueOut = flib::shrUnsigned16(x->getU16(), rhs);
					break;
				case ValueType::U32:
					valueOut = flib::shrUnsigned32(x->getU32(), rhs);
					break;
				case ValueType::U64:
					valueOut = flib::shrUnsigned64(x->getU64(), rhs);
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::NOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], x));
			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(~x->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(~x->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(~x->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(~x->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(~x->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(~x->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(~x->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(~x->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LNOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], x));
			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(!x->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(!x->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(!x->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(!x->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(!x->getI8());
					break;
				case ValueType::U16:
					valueOut = (bool)(!x->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(!x->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(!x->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(!x->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(!x->getF64());
					break;
				case ValueType::Bool:
					valueOut = (bool)(!x->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			if (!_isRegisterValid(curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], x));
			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(-x->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(-x->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(-x->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(-x->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(-x->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(-x->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::AT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			Value arrayValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], arrayValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, arrayValue));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, index));

			auto arrayIn = arrayValue.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, arrayIn));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::Array>(this, arrayIn.asObject));
			ArrayObject *arrayObject = (ArrayObject *)arrayIn.asObject;

			uint32_t indexIn = index.getU32();

			if (indexIn > arrayObject->length) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidArrayIndexError::alloc(getFixedAlloc(), indexIn));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(Reference(ArrayElementRef(arrayObject, indexIn)))));

			break;
		}
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, operands[0]));
			auto refPtr = operands[0].getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, refPtr));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::IdRef>(this, refPtr.asObject));

			Reference entityRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asObject, entityRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(entityRef)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::RegIndex>(this, operands[0]));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, lhs));

			Value rhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], rhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, rhs));

			auto &lhsEntityRef = lhs.getReference();

			auto &idRefEntityRef = rhs.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, idRefEntityRef));

			if (!lhsEntityRef) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (!idRefEntityRef) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (idRefEntityRef.asObject->getObjectKind() != ObjectKind::IdRef) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			IdRefObject *idRef = (IdRefObject *)idRefEntityRef.asObject;

			Reference entityRef;

			switch (lhsEntityRef.kind) {
				case ReferenceKind::StructRef: {
					StructObject *structObject = (StructObject *)(((CustomTypeDefObject *)typeofVar(lhsEntityRef).typeDef)->typeObject);
					IdRefEntry &curName = idRef->entries.at(0);

					if (auto it = structObject->cachedObjectLayout->fieldNameMap.find(curName.name); it != structObject->cachedObjectLayout->fieldNameMap.end()) {
						entityRef = Reference::makeStructFieldRef(lhsEntityRef.asStruct.structRef, lhsEntityRef.asStruct.innerReferenceKind, it.value());
					} else {
						entityRef = structObject->getMember(curName.name);

						if (curName.genericArgs.size()) {
							peff::NullAlloc nullAlloc;
							GenericInstantiationContext genericInstantiationContext(&nullAlloc, getFixedAlloc());

							genericInstantiationContext.genericArgs = &curName.genericArgs;
							MemberObject *m;
							SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject((MemberObject *)entityRef.asObject, m, &genericInstantiationContext));
							entityRef = Reference(m);
						}
					}

					break;
				}
				case ReferenceKind::ObjectRef:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef(idRef, entityRef, lhsEntityRef.asObject));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (!entityRef) {
				std::terminate();
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(entityRef)));
			break;
		}
		case Opcode::LCURFN: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 0>(this, output, nOperands));

			if (_isRegisterValid(curMajorFrame, output)) {
				Value &outputReg = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);
				outputReg.valueType = ValueType::Reference;
				outputReg.getReference().kind = ReferenceKind::ObjectRef;
				outputReg.getReference().asObject = const_cast<FnOverloadingObject *>(curMajorFrame->curFn);
			}
			break;
		}
		case Opcode::COPY: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			const Value *value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], value));

			if (output != UINT32_MAX) {
				if (!_isRegisterValid(curMajorFrame, output)) {
					// The register does not present.
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
				}
				Value *outputReg = _calcRegPtr(dataStack, stackSize, curMajorFrame, output);
				switch (value->valueType) {
					case ValueType::I8:
						outputReg->data.asI8 = value->getI8();
						outputReg->valueType = ValueType::I8;
						break;
					case ValueType::I16:
						outputReg->data.asI16 = value->getI16();
						outputReg->valueType = ValueType::I16;
						break;
					case ValueType::I32:
						outputReg->data.asI32 = value->getI32();
						outputReg->valueType = ValueType::I32;
						break;
					case ValueType::I64:
						outputReg->data.asI64 = value->getI64();
						outputReg->valueType = ValueType::I64;
						break;
					case ValueType::ISize:
						outputReg->data.asISize = value->getISize();
						outputReg->valueType = ValueType::ISize;
						break;
					case ValueType::U8:
						outputReg->data.asU8 = value->getU8();
						outputReg->valueType = ValueType::U8;
						break;
					case ValueType::U16:
						outputReg->data.asU16 = value->getU16();
						outputReg->valueType = ValueType::U16;
						break;
					case ValueType::U32:
						outputReg->data.asU32 = value->getU32();
						outputReg->valueType = ValueType::U32;
						break;
					case ValueType::U64:
						outputReg->data.asU64 = value->getU64();
						outputReg->valueType = ValueType::U64;
						break;
					case ValueType::USize:
						outputReg->data.asUSize = value->getUSize();
						outputReg->valueType = ValueType::USize;
						break;
					case ValueType::F32:
						outputReg->data.asF32 = value->getF32();
						outputReg->valueType = ValueType::F32;
						break;
					case ValueType::F64:
						outputReg->data.asF64 = value->getF64();
						outputReg->valueType = ValueType::F64;
						break;
					case ValueType::Bool:
						outputReg->data.asBool = value->getBool();
						outputReg->valueType = ValueType::Bool;
						break;
					case ValueType::Reference: {
						const Reference &ref = value->getReference();
						switch (ref.kind) {
							case ReferenceKind::Invalid:
								return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
							case ReferenceKind::LocalVarRef:
								if (!_allocAllocaRecord(&context->getContext(), curMajorFrame, output))
									return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
								outputReg->data.asReference.asLocalVar = value->data.asReference.asLocalVar;
								outputReg->data.asReference.kind = ReferenceKind::LocalVarRef;
								break;
							case ReferenceKind::CoroutineLocalVarRef:
								if (!_allocAllocaRecord(&context->getContext(), curMajorFrame, output))
									return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
								outputReg->data.asReference.asCoroutineLocalVar = value->data.asReference.asCoroutineLocalVar;
								outputReg->data.asReference.kind = ReferenceKind::CoroutineLocalVarRef;
								break;
							default:
								outputReg->data.asReference = ref;
								break;
						}
						outputReg->valueType = ValueType::Reference;
						break;
					}
					case ValueType::TypelessScopedEnum:
						outputReg->data.asTypelessScopedEnum = value->getTypelessScopedEnum();
						outputReg->valueType = ValueType::Bool;
						break;
					case ValueType::RegIndex:
						outputReg->data.asU32 = value->getRegIndex();
						outputReg->valueType = ValueType::RegIndex;
						break;
					case ValueType::TypeName:
						outputReg->data.asU32 = value->getRegIndex();
						outputReg->valueType = ValueType::TypeName;
						break;
					case ValueType::Label:
						outputReg->data.asU32 = value->getLabel();
						outputReg->valueType = ValueType::Label;
						break;
					default:
						std::terminate();
				}
			}
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[0]));

			Reference entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, larg(&context->getContext(), curMajorFrame, this, operands[0].getU32(), entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(entityRef)));
			break;
		}
		case Opcode::LAPARG:
			return InvalidOpcodeError::alloc(getFixedAlloc(), opcode);
		case Opcode::LVAR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));

			TypeRef type = operands[0].getTypeName();

			Reference entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(&context->_context, curMajorFrame, type, output, entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, entityRef));
			break;
		}
		case Opcode::ALLOCA:
			return InvalidOpcodeError::alloc(getFixedAlloc(), opcode);
		case Opcode::ENTER: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 0>(this, output, nOperands));
			size_t prevStackTop = context->getContext().stackTop;

			if (!context->_context.alignedStackAlloc(sizeof(MinorFrame), alignof(MinorFrame)))
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

			size_t mfStackOff = context->_context.stackTop;

			if (curMajorFrame->curCoroutine) {
				mfStackOff -= curMajorFrame->curCoroutine->offStackTop;
			}

			MinorFrame *mf = _fetchMinorFrame(&context->getContext(), curMajorFrame, context->getContext().stackTop);

			peff::constructAt<MinorFrame>(mf);

			mf->offLastMinorFrame = curMajorFrame->resumableContextData->offCurMinorFrame;
			mf->stackBase = curMajorFrame->curCoroutine ? prevStackTop - curMajorFrame->curCoroutine->offStackTop : prevStackTop;
			curMajorFrame->resumableContextData->offCurMinorFrame = mfStackOff;
			break;
		}
		case Opcode::LEAVE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[0]));
			uint32_t level = operands[0].getU32();
			for (uint32_t i = 0; i < level; ++i) {
				MinorFrame *mf = _fetchMinorFrame(&context->_context, curMajorFrame, curMajorFrame->resumableContextData->offCurMinorFrame);

				if (mf->offLastMinorFrame == SIZE_MAX)
					return allocOutOfMemoryErrorIfAllocFailed(FrameBoundaryExceededError::alloc(getFixedAlloc()));

				// Invalidate alloca records to prevent the runtime from dangling references.
				size_t offAllocaRecord = mf->offAllocaRecords;
				while (offAllocaRecord != SIZE_MAX) {
					AllocaRecord *ar = _fetchAllocaRecord(&context->getContext(), curMajorFrame, offAllocaRecord);

					_calcRegPtr(dataStack, stackSize, curMajorFrame, ar->defReg)->valueType = ValueType::Invalid;

					offAllocaRecord = ar->offNext;
				}

				size_t stackTop = mf->offLastMinorFrame;
				context->_context.stackTop = mf->stackBase;
				curMajorFrame->resumableContextData->offCurMinorFrame = stackTop;
			}
			break;
		}
		case Opcode::PUSHARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], value));
			/* if (curMajorFrame->resumableContextData->nNextArgs) {
				if (curMajorFrame->resumableContextData->offNextArgs + sizeof(Value) * curMajorFrame->resumableContextData->nNextArgs != context->getContext().stackTop)
					std::terminate();
			}*/
			if (char *p = context->getContext().stackAlloc(sizeof(Value)); p) {
				memcpy(p, &value, sizeof(Value));
			} else
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
			curMajorFrame->resumableContextData->offNextArgs = curMajorFrame->curCoroutine ? context->getContext().stackTop - curMajorFrame->curCoroutine->offStackTop : context->getContext().stackTop;
			++curMajorFrame->resumableContextData->nNextArgs;
			break;
		}
		case Opcode::PUSHAP:
			std::terminate();
		case Opcode::CTORCALL:
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;

			switch (opcode) {
				case Opcode::CTORCALL:
				case Opcode::MCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], thisObjectValue));
					const Reference &thisObjectRef = thisObjectValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, thisObjectValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, thisObjectRef));
					thisObject = thisObjectRef.asObject;
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (fn->returnType.typeId == TypeId::StructInstance) {
				if (output != UINT32_MAX) {
					// TODO: Untested!!!
					Reference ref;

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(&context->getContext(), curMajorFrame, fn->returnType, output, ref));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, ref));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _createNewMajorFrame(
																	&context->_context,
																	thisObject,
																	fn,
																	_fetchArgStack(context->getContext().dataStack, context->getContext().stackSize, curMajorFrame, curMajorFrame->resumableContextData->offNextArgs, curMajorFrame->resumableContextData->nNextArgs),
																	curMajorFrame->resumableContextData->nNextArgs,
																	UINT32_MAX,
																	&ref));
				} else
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _createNewMajorFrame(
																	&context->_context,
																	thisObject,
																	fn,
																	_fetchArgStack(context->getContext().dataStack, context->getContext().stackSize, curMajorFrame, curMajorFrame->resumableContextData->offNextArgs, curMajorFrame->resumableContextData->nNextArgs),
																	curMajorFrame->resumableContextData->nNextArgs,
																	output,
																	nullptr));
			} else {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _createNewMajorFrame(
																&context->_context,
																thisObject,
																fn,
																_fetchArgStack(context->getContext().dataStack, context->getContext().stackSize, curMajorFrame, curMajorFrame->resumableContextData->offNextArgs, curMajorFrame->resumableContextData->nNextArgs),
																curMajorFrame->resumableContextData->nNextArgs,
																output,
																nullptr));
			}

			curMajorFrame->resumableContextData->offNextArgs = SIZE_MAX;
			curMajorFrame->resumableContextData->nNextArgs = 0;

			isContextChangedOut = true;
			break;
		}
		case Opcode::RET: {
			const uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;

			Value returnValue = Value(ValueType::Invalid);

			switch (nOperands) {
				case 0:
					break;
				case 1:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], returnValue));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (CoroutineObject *co = curMajorFrame->curCoroutine; co) {
				co->resumable = std::move(curMajorFrame->resumableContextData);
				// TODO: Implement returning structure.
				if (returnValue != ValueType::Invalid)
					co->finalResult = returnValue;
				co->setDone();
			}

			TypeRef returnType = curMajorFrame->curFn->returnType;

			if (returnValue.valueType == ValueType::Invalid) {
				if (returnType.typeId != TypeId::Void)
					// TODO: Handle this.
					std::terminate();
				if (returnValueOutReg != UINT32_MAX)
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
				_leaveMajorFrame(&context->getContext());
			} else {
				if (!isCompatible(returnType, returnValue))
					// TODO: Handle this.
					std::terminate();
				_leaveMajorFrame(&context->getContext());
				if (returnType.typeId == TypeId::StructInstance) {
					writeVar(curMajorFrame->returnStructRef, returnValue);
				} else if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, _fetchMajorFrame(&context->getContext(), curMajorFrame->offPrevFrame), returnValueOutReg, returnValue));
				}
			}

			isContextChangedOut = true;
			return {};
		}
		case Opcode::COCALL:
		case Opcode::COMCALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;
			uint32_t returnValueOutputReg = UINT32_MAX;

			if (output != UINT32_MAX) {
				returnValueOutputReg = output;
			}

			switch (opcode) {
				case Opcode::COMCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], thisObjectValue));
					const Reference &thisObjectRef = thisObjectValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, thisObjectValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, thisObjectRef));
					thisObject = thisObjectRef.asObject;
					break;
				}
				case Opcode::COCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			HostObjectRef<CoroutineObject> co;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, createCoroutineInstance(
															fn,
															thisObject,
															_fetchArgStack(context->getContext().dataStack, context->getContext().stackSize, curMajorFrame, curMajorFrame->resumableContextData->offNextArgs, curMajorFrame->resumableContextData->nNextArgs),
															curMajorFrame->resumableContextData->nNextArgs,
															co));
			curMajorFrame->resumableContextData->offNextArgs = SIZE_MAX;
			curMajorFrame->resumableContextData->nNextArgs = 0;

			if (returnValueOutputReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, returnValueOutputReg, Reference(co.get())));
			}
			break;
		}
		case Opcode::YIELD: {
			if (!curMajorFrame->curCoroutine) {
				// TODO: Return an exception,
				std::terminate();
			}

			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
			Value returnValue = Value(ValueType::Invalid);

			switch (nOperands) {
				case 0:
					break;
				case 1:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], returnValue));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (size_t szFrame = context->_context.stackTop - curMajorFrame->curCoroutine->offStackTop; szFrame) {
				char *p = curMajorFrame->curCoroutine->allocStackData(szFrame);
				if (!p) {
					return OutOfMemoryError::alloc();
				}
				memcpy(p, calcStackAddr(context->_context.dataStack, context->_context.stackSize, context->_context.stackTop), szFrame);
			}

			curMajorFrame->curCoroutine->unbindContext();
			++curMajorFrame->resumableContextData->curIns;

			curMajorFrame->curCoroutine->offRegs = curMajorFrame->offRegs - curMajorFrame->curCoroutine->offStackTop;
			curMajorFrame->curCoroutine->resumable = std::move(curMajorFrame->resumableContextData);

			if (returnValue == ValueType::Invalid) {
				if (returnValueOutReg != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
				}
				_leaveMajorFrame(&context->getContext());
			} else {
				_leaveMajorFrame(&context->getContext());
				MajorFrame *prevFrame = _fetchMajorFrame(&context->getContext(), curMajorFrame->offPrevFrame);
				if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, prevFrame, returnValueOutReg, returnValue));
				}
			}

			isContextChangedOut = true;
			return {};
		}
		case Opcode::RESUME: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			HostObjectRef<CoroutineObject> co;

			Value fnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
			const Reference &fnObjectRef = fnValue.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
			co = (CoroutineObject *)fnObjectRef.asObject;

			if (co->isDone()) {
				if (output != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, co->finalResult));
				}
			} else {
				if (co->overloading->returnType.typeId == TypeId::StructInstance) {
					Reference ref;

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(&context->getContext(), curMajorFrame, co->overloading->returnType, output, ref));

					SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, co.get(), output, &ref));
				} else
					SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, co.get(), output, nullptr));
			}

			isContextChangedOut = true;
			break;
		}
		case Opcode::CODONE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			HostObjectRef<CoroutineObject> co;

			Value fnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
			const Reference &fnObjectRef = fnValue.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
			co = (CoroutineObject *)fnObjectRef.asObject;

			SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, co->isDone()));
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 0>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Reference(curMajorFrame->resumableContextData->thisObject)));
			break;
		}
		case Opcode::NEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));

			TypeRef type = operands[0].getTypeName();

			switch (type.typeId) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)(type.getCustomTypeDef())->typeObject;
					HostObjectRef<InstanceObject> instance = newClassInstance(cls, 0);
					if (!instance)
						// TODO: Return more detail exceptions.
						return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Reference(instance.get())));
					break;
				}
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::ARRNEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[1]));

			TypeRef type = operands[0].getTypeName();
			uint32_t size = operands[1].getU32();

			auto instance = newArrayInstance(this, type, size);
			if (!instance)
				// TODO: Return more detailed exceptions.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Reference(instance.get())));

			break;
		}
		// case Opcode::THROW: {
		//  TODO: Implement it.
		//  return allocOutOfMemoryErrorIfAllocFailed(UncaughtExceptionError::alloc(getFixedAlloc(), x));
		//}
		case Opcode::PUSHEH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[1]));

			MinorFrame *mf = _fetchMinorFrame(&context->getContext(), curMajorFrame, curMajorFrame->resumableContextData->offCurMinorFrame);

			if (!context->_context.alignedStackAlloc(sizeof(ExceptHandler), alignof(ExceptHandler)))
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

			size_t ehStackOff = context->_context.stackTop;

			if (curMajorFrame->curCoroutine) {
				ehStackOff -= curMajorFrame->curCoroutine->offStackTop;
			}

			ExceptHandler *eh = _fetchExceptHandler(&context->getContext(), curMajorFrame, context->getContext().stackTop);

			eh->type = operands[0].getTypeName();
			eh->offNext = mf->offExceptHandler;

			mf->offExceptHandler = ehStackOff;
			break;
		}
		case Opcode::CAST: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));
			if (!_isRegisterValid(curMajorFrame, output)) {
				// The register does not present.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			Value v;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], v));

			auto t = operands[0].getTypeName();
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			switch (t.typeId) {
				case TypeId::I8:
					_castToLiteralValue<int8_t>(v, valueOut);
					break;
				case TypeId::I16:
					_castToLiteralValue<int16_t>(v, valueOut);
					break;
				case TypeId::I32:
					_castToLiteralValue<int32_t>(v, valueOut);
					break;
				case TypeId::I64:
					_castToLiteralValue<int64_t>(v, valueOut);
					break;
				case TypeId::U8:
					_castToLiteralValue<uint8_t>(v, valueOut);
					break;
				case TypeId::U16:
					_castToLiteralValue<uint16_t>(v, valueOut);
					break;
				case TypeId::U32:
					_castToLiteralValue<uint32_t>(v, valueOut);
					break;
				case TypeId::U64:
					_castToLiteralValue<uint64_t>(v, valueOut);
					break;
				case TypeId::Bool:
					_castToLiteralValue<bool>(v, valueOut);
					break;
				case TypeId::F32:
					_castToLiteralValue<float>(v, valueOut);
					break;
				case TypeId::F64:
					_castToLiteralValue<double>(v, valueOut);
					break;
				case TypeId::Instance:
					valueOut = v;
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			break;
		}
		case Opcode::PHI: {
			if (output == UINT32_MAX) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if ((nOperands < 2) || ((nOperands & 1))) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			Value v;

			for (size_t i = 0; i < nOperands; i += 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[i]));

				uint32_t off = operands[i].getU32();

				if (off == curMajorFrame->resumableContextData->lastJumpSrc) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[i + 1], v));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, v));

					goto succeeded;
				}
			}

			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));

		succeeded:
			break;
		}
		default:
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOpcodeError::alloc(getFixedAlloc(), opcode));
	}
	++curMajorFrame->resumableContextData->curIns;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::execContext(ContextObject *context) noexcept {
	size_t initialMajorFrameDepth = context->_context.nMajorFrames;
	InternalExceptionPointer exceptPtr;
	ExecutionRunnable *const managedThread = managedThreadRunnables.at(currentThreadHandle());

	while (context->getContext().nMajorFrames >= initialMajorFrameDepth) {
		MajorFrame *const curMajorFrame = _fetchMajorFrame(&context->getContext(), context->getContext().offCurMajorFrame);
		const FnOverloadingObject *const curFn = curMajorFrame->curFn;

		if (!curFn) {
			break;
		}

		// Pause if the runtime is in GC
		/*while (_flags & _RT_INGC)
			yieldCurrentThread();*/

		switch (curFn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				const RegularFnOverloadingObject *const ol = (RegularFnOverloadingObject *)curFn;

				const size_t nIns = ol->instructions.size();

				bool isContextChanged = false;
				while (!isContextChanged) {
					const size_t idxCurIns = curMajorFrame->resumableContextData->curIns;
					if (idxCurIns >=
						nIns) {
						// Raise out of fn body error.
						std::terminate();
					}

					// Interrupt execution if the thread is explicitly specified to be killed.
					if (managedThread->status == ThreadStatus::Dead) {
						return {};
					}

					if ((fixedAlloc.szAllocated > _szComputedGcLimit)) {
						gc();
					}

					const Instruction *const instruction = ol->instructions.data() + idxCurIns;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, instruction->opcode, instruction->output, instruction->nOperands, instruction->operands, isContextChanged));
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)curFn;

				Value returnValue = ol->callback(
					&context->getContext(),
					curMajorFrame);
				uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
				_leaveMajorFrame(&context->getContext());
				if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.dataStack, context->_context.stackSize, curMajorFrame, returnValueOutReg, returnValue));
				}

				break;
			}
			default:
				std::terminate();
		}
	}

	context->_context.flags |= CTX_DONE;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::execFn(
	const FnOverloadingObject *overloading,
	ContextObject *prevContext,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	Value &valueOut) {
	HostObjectRef<ContextObject> context(prevContext);

	Context &ctxt = context->getContext();

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, nullptr, nullptr, nullptr, 0, UINT32_MAX, nullptr));
	MajorFrame &bottomFrame = *_fetchMajorFrame(&ctxt, ctxt.offCurMajorFrame);
	if (overloading->returnType.typeId == TypeId::StructInstance) {
		Reference structRef;
		SLAKE_RETURN_IF_EXCEPT(_addLocalVar(&ctxt, &bottomFrame, overloading->returnType, 0, structRef));
		SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, ctxt.dataStack, ctxt.stackSize, &bottomFrame, 0, Value(structRef)));
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, thisObject, overloading, args, nArgs, 0, &structRef));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, thisObject, overloading, args, nArgs, 0, nullptr));
	}

	ExecutionRunnable runnable;

	runnable.context = context;

	if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
		return OutOfMemoryError::alloc();
	}

	NativeThreadHandle threadHandle = currentThreadHandle();

	peff::ScopeGuard removeManagedThreadRunnablesGuard([this, threadHandle]() noexcept {
		managedThreadRunnables.remove(threadHandle);
	});

	runnable.run();

	InternalExceptionPointer exceptPtr = std::move(runnable.exceptPtr);

	return exceptPtr;
}

SLAKE_API InternalExceptionPointer Runtime::execFnWithSeparatedExecutionThread(
	const FnOverloadingObject *overloading,
	ContextObject *prevContext,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	HostObjectRef<ContextObject> &contextOut) {
	HostObjectRef<ContextObject> context(prevContext);

	Context &ctxt = context->getContext();

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, nullptr, nullptr, nullptr, 0, UINT32_MAX, nullptr));
	MajorFrame &bottomFrame = *_fetchMajorFrame(&ctxt, ctxt.offCurMajorFrame);
	if (overloading->returnType.typeId == TypeId::StructInstance) {
		Reference structRef;
		SLAKE_RETURN_IF_EXCEPT(_addLocalVar(&ctxt, &bottomFrame, overloading->returnType, 0, structRef));
		SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, ctxt.dataStack, ctxt.stackSize, &bottomFrame, 0, Value(structRef)));
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, thisObject, overloading, args, nArgs, 0, &structRef));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, thisObject, overloading, args, nArgs, 0, nullptr));
	}

	ExecutionRunnable runnable;

	runnable.context = context;

	if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
		return OutOfMemoryError::alloc();
	}

	NativeThreadHandle threadHandle = currentThreadHandle();

	peff::ScopeGuard removeManagedThreadRunnablesGuard([this, threadHandle]() noexcept {
		managedThreadRunnables.remove(threadHandle);
	});

	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>> t(Thread::alloc(getFixedAlloc(), &runnable, SLAKE_NATIVE_STACK_SIZE_MAX));

	t->join();

	InternalExceptionPointer exceptPtr = std::move(runnable.exceptPtr);

	return exceptPtr;
}

SLAKE_API InternalExceptionPointer Runtime::createCoroutineInstance(
	const FnOverloadingObject *fn,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	HostObjectRef<CoroutineObject> &coroutineOut) {
	HostRefHolder holder(getFixedAlloc());
	HostObjectRef<CoroutineObject> co = CoroutineObject::alloc(this);

	if (!co)
		return OutOfMemoryError::alloc();

	co->overloading = fn;

	MajorFrame newMajorFrame(this);

	newMajorFrame.resumableContextData = ResumableContextData(co->selfAllocator.get());

	if (nArgs) {
		char *p = co->allocStackData(sizeof(Value) * nArgs);
		if (!p)
			return OutOfMemoryError::alloc();
		// TODO: Check if the arguments and the parameters are matched.
		memcpy(p, args, sizeof(Value) * nArgs);
	}
	newMajorFrame.resumableContextData->thisObject = thisObject;

	coroutineOut = co;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::resumeCoroutine(
	ContextObject *context,
	CoroutineObject *coroutine,
	Value &resultOut,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	if (coroutine->isDone()) {
		resultOut = coroutine->finalResult;
		return {};
	}

	HostObjectRef<ContextObject> contextRef(context);
	Context &ctxt = context->getContext();

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&ctxt, nullptr, nullptr, nullptr, 0, UINT32_MAX, nullptr));
	MajorFrame &bottomFrame = *_fetchMajorFrame(&ctxt, ctxt.offCurMajorFrame);
	if (coroutine->overloading->returnType.typeId == TypeId::StructInstance) {
		Reference structRef;
		SLAKE_RETURN_IF_EXCEPT(_addLocalVar(&ctxt, &bottomFrame, coroutine->overloading->returnType, 0, structRef));
		SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, ctxt.dataStack, ctxt.stackSize, &bottomFrame, 0, Value(structRef)));
		SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, coroutine, 0, &structRef));
	} else
		SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, coroutine, 0, nullptr));

	{
		ExecutionRunnable runnable;

		runnable.context = context;

		{
			if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
				_leaveMajorFrame(&context->getContext());

				return OutOfMemoryError::alloc();
			}

			NativeThreadHandle threadHandle = currentThreadHandle();

			peff::ScopeGuard removeManagedThreadRunnablesGuard([this, threadHandle]() noexcept {
				managedThreadRunnables.remove(threadHandle);
			});

			runnable.run();
		}

		InternalExceptionPointer exceptPtr = std::move(runnable.exceptPtr);

		if (exceptPtr) {
			return exceptPtr;
		}

		resultOut = ((const Value *)calcStackAddr(context->_context.dataStack, context->_context.stackSize, bottomFrame.offRegs + sizeof(Value)))[0];

		_leaveMajorFrame(&context->getContext());
	}

	return {};
}
