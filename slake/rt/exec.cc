#include "../runtime.h"
#include <slake/flib/math/fmod.h>
#include <slake/flib/bitop.h>
#include <slake/flib/cmp.h>
#include <slake/util/scope_guard.h>
#include <cmath>

#undef new

using namespace slake;

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandCount(
	Runtime *runtime,
	const Instruction &ins,
	bool hasOutput,
	uint_fast8_t nOperands) noexcept {
	if (hasOutput) {
		if (ins.output == UINT32_MAX) {
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
	}
	if (ins.nOperands != nOperands) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}

	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandType(
	Runtime *runtime,
	const Value &operand,
	ValueType valueType) noexcept {
	if (operand.valueType != valueType) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectRefOperandType(
	Runtime *runtime,
	const EntityRef &operand,
	ObjectRefKind kind) noexcept {
	if (operand.kind != kind) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectOperandType(
	Runtime *runtime,
	Object *object,
	ObjectKind typeId) noexcept {
	if (/* object && (*/ object->getObjectKind() != typeId /*)*/) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _setRegisterValue(
	Runtime *runtime,
	char *stackData,
	MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) noexcept {
	if (index >= curMajorFrame->resumable->nRegs) {
		// The register does not present.
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	*(Value *)calcStackAddr((char *)stackData, SLAKE_STACK_MAX, curMajorFrame->offRegs + sizeof(Value) * index) = value;
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _fetchRegValue(
	Runtime *runtime,
	const char *stackData,
	MajorFrame *curMajorFrame,
	uint32_t index,
	Value &valueOut) noexcept {
	if (index >= curMajorFrame->resumable->nRegs) {
		// The register does not present.
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	valueOut = *(Value *)calcStackAddr((char *)stackData, SLAKE_STACK_MAX, curMajorFrame->offRegs + sizeof(Value) * index);
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _unwrapRegOperand(
	Runtime *runtime,
	const char *stackData,
	MajorFrame *curMajorFrame,
	const Value &value,
	Value &valueOut) noexcept {
	if (value.valueType == ValueType::RegRef)
		return _fetchRegValue(runtime, stackData, curMajorFrame, value.getRegIndex(), valueOut);
	valueOut = value;
	return {};
}

template <typename LT>
static Value _castToLiteralValue(Value x) noexcept {
	switch (x.valueType) {
		case ValueType::I8:
			return Value((LT)(x.getI8()));
		case ValueType::I16:
			return Value((LT)(x.getI16()));
		case ValueType::I32:
			return Value((LT)(x.getI32()));
		case ValueType::I64:
			return Value((LT)(x.getI64()));
		case ValueType::ISize:
			return Value((LT)(x.getISize()));
		case ValueType::U8:
			return Value((LT)(x.getU8()));
		case ValueType::U16:
			return Value((LT)(x.getU16()));
		case ValueType::U32:
			return Value((LT)(x.getU32()));
		case ValueType::U64:
			return Value((LT)(x.getU64()));
		case ValueType::USize:
			return Value((LT)(x.getUSize()));
		case ValueType::F32:
			return Value((LT)(x.getF32()));
		case ValueType::F64:
			return Value((LT)(x.getF64()));
		case ValueType::Bool:
			return Value((LT)(x.getBool()));
		default:
			std::terminate();
	}
}

SLAKE_API InternalExceptionPointer Runtime::_fillArgs(
	MajorFrame *newMajorFrame,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	HostRefHolder &holder) {
	if (nArgs < fn->paramTypes.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidArgumentNumberError::alloc(getFixedAlloc(), nArgs));
	}

	if (!newMajorFrame->resumable->argStack.resize(fn->paramTypes.size()))
		return OutOfMemoryError::alloc();
	for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
		newMajorFrame->resumable->argStack.at(i) = { Value(), fn->paramTypes.at(i) };
		SLAKE_RETURN_IF_EXCEPT(writeVar(EntityRef::makeArgRef(newMajorFrame, i), args[i]));
	}

	if (fn->overloadingFlags & OL_VARG) {
		auto varArgTypeDefObject = TypeDefObject::alloc(this);

		varArgTypeDefObject->type = Type(TypeId::Any);

		if (!holder.addObject(varArgTypeDefObject.get()))
			return OutOfMemoryError::alloc();

		size_t szVarArgArray = nArgs - fn->paramTypes.size();
		auto varArgArrayObject = newArrayInstance(this, Type(TypeId::Any), szVarArgArray);
		if (!holder.addObject(varArgArrayObject.get()))
			return OutOfMemoryError::alloc();

		for (size_t i = 0; i < szVarArgArray; ++i) {
			((Value *)varArgArrayObject->data)[i] = args[fn->paramTypes.size() + i];
		}

		if (!newMajorFrame->resumable->argStack.pushBack({ EntityRef::makeObjectRef(varArgArrayObject.get()), Type(TypeId::Array, varArgTypeDefObject.get()) }))
			return OutOfMemoryError::alloc();
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_createNewCoroutineMajorFrame(
	Context *context,
	CoroutineObject *coroutine,
	uint32_t returnValueOut) noexcept {
	HostRefHolder holder(context->runtime->getFixedAlloc());

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop]() noexcept {
		context->stackTop = prevStackTop;
	});
	MajorFramePtr newMajorFrame(MajorFrame::alloc(this, context));
	if (!newMajorFrame) {
		return OutOfMemoryError::alloc();
	}

	if (!(newMajorFrame->resumable = coroutine->resumable))
		return OutOfMemoryError::alloc();

	newMajorFrame->curFn = coroutine->overloading;
	newMajorFrame->curCoroutine = coroutine;

	coroutine->bindToContext(context, newMajorFrame.get());

	if (coroutine->stackData) {
		if (size_t diff = context->stackTop % sizeof(std::max_align_t); diff) {
			if (!context->stackAlloc(sizeof(std::max_align_t) - diff))
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
		newMajorFrame->offRegs = context->stackTop;
		size_t stackOffset = context->stackTop;
		char *initialData = context->stackAlloc(coroutine->lenStackData);
		if (!initialData) {
			return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
		memcpy(initialData, coroutine->stackData, coroutine->lenStackData);
		coroutine->releaseStackData();
	} else {
		switch (coroutine->overloading->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)coroutine->overloading;
				coroutine->resumable->nRegs = (newMajorFrame->resumable->nRegs = ol->nRegisters);
				newMajorFrame->offRegs = context->stackTop;
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

	newMajorFrame->resumable = coroutine->resumable;

	newMajorFrame->returnValueOutReg = returnValueOut;

	restoreStackTopGuard.release();

	newMajorFrame->stackBase = prevStackTop;
	if (!context->majorFrames.pushBack(std::move(newMajorFrame))) {
		return OutOfMemoryError::alloc();
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_createNewMajorFrame(
	Context *context,
	Object *thisObject,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	uint32_t returnValueOut) noexcept {
	HostRefHolder holder(getFixedAlloc());

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop]() noexcept {
		context->stackTop = prevStackTop;
	});
	MajorFramePtr newMajorFrame(MajorFrame::alloc(this, context));
	if (!newMajorFrame) {
		return OutOfMemoryError::alloc();
	}

	if (!(newMajorFrame->resumable = ResumableObject::alloc(this)))
		return OutOfMemoryError::alloc();

	if (!newMajorFrame->resumable->minorFrames.pushBack(MinorFrame(this, context->selfAllocator.get(), 0)))
		return OutOfMemoryError::alloc();

	if (!fn) {
		// Used in the creation of top major frame.
		newMajorFrame->curFn = nullptr;
		newMajorFrame->resumable->nRegs = 1;
		Value *regs = (Value *)context->stackAlloc(sizeof(Value) * 1);
		newMajorFrame->offRegs = context->stackTop;
		*regs = Value(ValueType::Undefined);
	} else {
		newMajorFrame->curFn = fn;
		newMajorFrame->resumable->thisObject = thisObject;

		SLAKE_RETURN_IF_EXCEPT(_fillArgs(newMajorFrame.get(), fn, args, nArgs, holder));

		switch (fn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn;
				newMajorFrame->resumable->nRegs = ol->nRegisters;
				Value *regs = (Value *)context->stackAlloc(sizeof(Value) * ol->nRegisters);
				newMajorFrame->offRegs = context->stackTop;
				for (size_t i = 0; i < ol->nRegisters; ++i)
					regs[i] = Value(ValueType::Undefined);
				break;
			}
			default:;
		}
	}

	newMajorFrame->returnValueOutReg = returnValueOut;
	newMajorFrame->stackBase = prevStackTop;
	if (!context->majorFrames.pushBack(std::move(newMajorFrame))) {
		return OutOfMemoryError::alloc();
	}

	restoreStackTopGuard.release();

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_addLocalVar(Context *context, MajorFrame *frame, Type type, EntityRef &objectRefOut) noexcept {
	size_t stackOffset;

	size_t size = sizeofType(type), align = alignofType(type);

	if (align > 1) {
		if (size_t diff = context->stackTop % align; diff) {
			if (!context->stackAlloc(align - diff))
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
	}

	if (!context->stackAlloc(size))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

	TypeId *typeInfo = (TypeId *)context->stackAlloc(sizeof(TypeId));
	if (!typeInfo)
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	*typeInfo = type.typeId;

	stackOffset = context->stackTop;

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
			TypeExData *typeInfo = (TypeExData *)context->stackAlloc(sizeof(TypeExData));
			if (!typeInfo)
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
			memcpy(typeInfo, &type.exData, sizeof(TypeExData));
			break;
		}
	}

	if (frame->curCoroutine) {
		objectRefOut = EntityRef::makeCoroutineLocalVarRef(frame->curCoroutine, stackOffset - frame->stackBase);
	} else {
		objectRefOut = EntityRef::makeLocalVarRef(context, stackOffset);
	}
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(MajorFrame *majorFrame, Runtime *rt, uint32_t off, EntityRef &objectRefOut) {
	if (off >= majorFrame->resumable->argStack.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(rt->getFixedAlloc()));
	}

	if (majorFrame->curCoroutine) {
		objectRefOut = EntityRef::makeCoroutineArgRef(majorFrame->curCoroutine, off);
	} else {
		objectRefOut = EntityRef::makeArgRef(majorFrame, off);
	}
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer Runtime::_execIns(ContextObject *context, MajorFrame *curMajorFrame, const Instruction &ins, bool &isContextChangedOut) noexcept {
	InternalExceptionPointer exceptPtr;
	char *dataStack = context->_context.dataStack;

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			EntityRef entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(&context->_context, curMajorFrame, type, entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, entityRef));
			break;
		}
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::EntityRef));
			auto refPtr = ins.operands[0].getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, refPtr, ObjectRefKind::ObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, refPtr.asObject.instanceObject, ObjectKind::IdRef));

			EntityRef entityRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asObject.instanceObject, entityRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, Value(entityRef)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::RegRef));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, lhs, ValueType::EntityRef));

			Value rhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], rhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, rhs, ValueType::EntityRef));

			auto lhsPtr = lhs.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, lhsPtr, ObjectRefKind::ObjectRef));

			auto refPtr = rhs.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, refPtr, ObjectRefKind::ObjectRef));

			if (!lhsPtr) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (!refPtr) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (refPtr.asObject.instanceObject->getObjectKind() != ObjectKind::IdRef) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			EntityRef entityRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asObject.instanceObject, entityRef, lhsPtr.asObject.instanceObject));

			if (!entityRef) {
				std::terminate();
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															dataStack,
															curMajorFrame,
															ins.output,
															Value(entityRef)));
			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

			Value destValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], destValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, destValue, ValueType::EntityRef));

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, writeVar(destValue.getEntityRef(), data));
			break;
		}
		case Opcode::MOV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this,
															dataStack,
															curMajorFrame,
															ins.operands[0],
															value));

			if (ins.output != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
																dataStack,
																curMajorFrame,
																ins.output,
																value));
			}
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));

			EntityRef entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, larg(curMajorFrame, this, ins.operands[0].getU32(), entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															dataStack,
															curMajorFrame,
															ins.output,
															Value(entityRef)));
			break;
		}
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			Value dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, dest, ValueType::EntityRef));

			const EntityRef &entityRef = dest.getEntityRef();

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, readVar(entityRef, data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															dataStack,
															curMajorFrame,
															ins.output,
															data));
			break;
		}
		case Opcode::ENTER: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 0));
			MinorFrame frame(
				this,
				context->selfAllocator.get(),
				context->_context.stackTop - curMajorFrame->stackBase);

			if (!curMajorFrame->resumable->minorFrames.pushBack(std::move(frame)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::LEAVE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));
			uint32_t level = ins.operands[0].getU32();
			if (curMajorFrame->resumable->minorFrames.size() < level + 1) {
				return allocOutOfMemoryErrorIfAllocFailed(FrameBoundaryExceededError::alloc(getFixedAlloc()));
			}
			for (uint32_t i = 0; i < level; ++i) {
				size_t stackTop = curMajorFrame->resumable->minorFrames.back().stackBase;
				if (!curMajorFrame->resumable->minorFrames.popBackAndResizeCapacity())
					return OutOfMemoryError::alloc();
				context->_context.stackTop = curMajorFrame->stackBase + stackTop;
			}
			break;
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() + y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() + y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() + y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() + y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() + y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() + y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() + y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() + y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((float)(x.getF32() + y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((double)(x.getF64() + y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() - y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() - y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() - y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() - y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() - y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() - y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() - y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() - y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((float)(x.getF32() - y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((double)(x.getF64() - y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() * y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() * y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() * y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() * y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() * y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() * y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() * y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() * y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((float)(x.getF32() * y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((double)(x.getF64() * y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() / y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() / y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() / y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() / y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() / y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() / y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() / y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() / y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((float)(x.getF32() / y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((double)(x.getF64() / y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() % y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() % y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() % y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() % y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() % y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() % y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() % y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() % y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((float)flib::fmodf(x.getF32(), y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((double)flib::fmod(x.getF64(), y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::AND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() & y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() & y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() & y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() & y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() & y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() & y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() & y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() & y.getU64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::OR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() | y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() | y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() | y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() | y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() | y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() | y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() | y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() | y.getU64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::XOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(x.getI8() ^ y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(x.getI16() ^ y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(x.getI32() ^ y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(x.getI64() ^ y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8() ^ y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16() ^ y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32() ^ y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64() ^ y.getU64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::LAND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x.getBool() && y.getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::LOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x.getBool() || y.getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::EQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(x.getI8() == y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(x.getI16() == y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(x.getI32() == y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(x.getI64() == y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(x.getU8() == y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(x.getU16() == y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(x.getU32() == y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(x.getU64() == y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(x.getF32() == y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(x.getF64() == y.getF64()));
					break;
				case ValueType::Bool:
					valueOut = Value((bool)(x.getBool() == y.getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::NEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(x.getI8() != y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(x.getI16() != y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(x.getI32() != y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(x.getI64() != y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(x.getU8() != y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(x.getU16() != y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(x.getU32() != y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(x.getU64() != y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(x.getF32() != y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(x.getF64() != y.getF64()));
					break;
				case ValueType::Bool:
					valueOut = Value((bool)(x.getBool() != y.getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::LT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(x.getI8() < y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(x.getI16() < y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(x.getI32() < y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(x.getI64() < y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(x.getU8() < y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(x.getU16() < y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(x.getU32() < y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(x.getU64() < y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(x.getF32() < y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(x.getF64() < y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::GT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(x.getI8() > y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(x.getI16() > y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(x.getI32() > y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(x.getI64() > y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(x.getU8() > y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(x.getU16() > y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(x.getU32() > y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(x.getU64() > y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(x.getF32() > y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(x.getF64() > y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::LTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(x.getI8() <= y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(x.getI16() <= y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(x.getI32() <= y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(x.getI64() <= y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(x.getU8() <= y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(x.getU16() <= y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(x.getU32() <= y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(x.getU64() <= y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(x.getF32() <= y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(x.getF64() <= y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::GTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(x.getI8() >= y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(x.getI16() >= y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(x.getI32() >= y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(x.getI64() >= y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(x.getU8() >= y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(x.getU16() >= y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(x.getU32() >= y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(x.getU64() >= y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(x.getF32() >= y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(x.getF64() >= y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			uint64_t lhs = x.getU64(), rhs = y.getU64();
			if (lhs > rhs) {
				valueOut = Value((int32_t)1);
			} else if (lhs < rhs) {
				valueOut = Value((int32_t)-1);
			} else
				valueOut = Value((int32_t)0);

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::CMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int32_t)flib::compareI8(x.getI8(), y.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int32_t)flib::compareI16(x.getI16(), y.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)flib::compareI32(x.getI32(), y.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int32_t)flib::compareI64(x.getI64(), y.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((int32_t)flib::compareU8(x.getU8(), y.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((int32_t)flib::compareU16(x.getU16(), y.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((int32_t)flib::compareU32(x.getU32(), y.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((int32_t)flib::compareU64(x.getU64(), y.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((int32_t)flib::compareF32(x.getF32(), y.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((int32_t)flib::compareF64(x.getF64(), y.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (ins.output != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			}
			break;
		}
		case Opcode::LSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x,
				y,
				valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, y, ValueType::U32));

			uint32_t rhs = y.getU32();

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = flib::shlSigned8(x.getI8(), rhs);
					break;
				case ValueType::I16:
					valueOut = flib::shlSigned16(x.getI16(), rhs);
					break;
				case ValueType::I32:
					valueOut = flib::shlSigned32(x.getI32(), rhs);
					break;
				case ValueType::I64:
					valueOut = flib::shlSigned64(x.getI64(), rhs);
					break;
				case ValueType::U8:
					valueOut = flib::shlUnsigned8(x.getU8(), rhs);
					break;
				case ValueType::U16:
					valueOut = flib::shlUnsigned16(x.getU16(), rhs);
					break;
				case ValueType::U32:
					valueOut = flib::shlUnsigned32(x.getU32(), rhs);
					break;
				case ValueType::U64:
					valueOut = flib::shlUnsigned64(x.getU64(), rhs);
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value x,
				y,
				valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, y, ValueType::U32));

			uint32_t rhs = y.getU32();

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = flib::shrSigned8(x.getI8(), rhs);
					break;
				case ValueType::I16:
					valueOut = flib::shrSigned16(x.getI16(), rhs);
					break;
				case ValueType::I32:
					valueOut = flib::shrSigned32(x.getI32(), rhs);
					break;
				case ValueType::I64:
					valueOut = flib::shrSigned64(x.getI64(), rhs);
					break;
				case ValueType::U8:
					valueOut = flib::shrUnsigned8(x.getU8(), rhs);
					break;
				case ValueType::U16:
					valueOut = flib::shrUnsigned16(x.getU16(), rhs);
					break;
				case ValueType::U32:
					valueOut = flib::shrUnsigned32(x.getU32(), rhs);
					break;
				case ValueType::U64:
					valueOut = flib::shrUnsigned64(x.getU64(), rhs);
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::NOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			Value x(ins.operands[1]), valueOut;

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(~x.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(~x.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(~x.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(~x.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(~x.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(~x.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(~x.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(~x.getU64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::LNOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			Value x(ins.operands[1]), valueOut;

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((bool)(!x.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((bool)(!x.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((bool)(!x.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((bool)(!x.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((bool)(!x.getI8()));
					break;
				case ValueType::U16:
					valueOut = Value((bool)(!x.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((bool)(!x.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((bool)(!x.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((bool)(!x.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((bool)(!x.getF64()));
					break;
				case ValueType::Bool:
					valueOut = Value((bool)(!x.getU64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			Value x(ins.operands[1]), valueOut;

			switch (x.valueType) {
				case ValueType::I8:
					valueOut = Value((int8_t)(-x.getI8()));
					break;
				case ValueType::I16:
					valueOut = Value((int16_t)(-x.getI16()));
					break;
				case ValueType::I32:
					valueOut = Value((int32_t)(-x.getI32()));
					break;
				case ValueType::I64:
					valueOut = Value((int64_t)(-x.getI64()));
					break;
				case ValueType::U8:
					valueOut = Value((uint8_t)(x.getU8()));
					break;
				case ValueType::U16:
					valueOut = Value((uint16_t)(x.getU16()));
					break;
				case ValueType::U32:
					valueOut = Value((uint32_t)(x.getU32()));
					break;
				case ValueType::U64:
					valueOut = Value((uint64_t)(x.getU64()));
					break;
				case ValueType::F32:
					valueOut = Value((float)(-x.getF32()));
					break;
				case ValueType::F64:
					valueOut = Value((double)(-x.getF64()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, valueOut));
			break;
		}
		case Opcode::AT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value arrayValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], arrayValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, arrayValue, ValueType::EntityRef));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, index, ValueType::U32));

			auto arrayIn = arrayValue.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, arrayIn, ObjectRefKind::ObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, arrayIn.asObject.instanceObject, ObjectKind::Array));
			ArrayObject *arrayObject = (ArrayObject *)arrayIn.asObject.instanceObject;

			uint32_t indexIn = index.getU32();

			if (indexIn > arrayObject->length) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidArrayIndexError::alloc(getFixedAlloc(), indexIn));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															dataStack,
															curMajorFrame,
															ins.output,
															Value(EntityRef::makeArrayElementRef(arrayObject, indexIn))));

			break;
		}
		case Opcode::JMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));

			curMajorFrame->resumable->lastJumpSrc = curMajorFrame->resumable->curIns;
			curMajorFrame->resumable->curIns = ins.operands[0].getU32();
			return {};
		}
		case Opcode::JT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));
			Value condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, condition, ValueType::Bool));

			if (condition.getBool()) {
				curMajorFrame->resumable->lastJumpSrc = curMajorFrame->resumable->curIns;
				curMajorFrame->resumable->curIns = ins.operands[0].getU32();
				return {};
			}
			curMajorFrame->resumable->lastJumpSrc = UINT32_MAX;

			break;
		}
		case Opcode::JF: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));
			Value condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, condition, ValueType::Bool));

			if (!condition.getBool()) {
				curMajorFrame->resumable->lastJumpSrc = curMajorFrame->resumable->curIns;
				curMajorFrame->resumable->curIns = ins.operands[0].getU32();
				return {};
			}
			curMajorFrame->resumable->lastJumpSrc = UINT32_MAX;

			break;
		}
		case Opcode::PUSHARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], value));
			if (!curMajorFrame->resumable->nextArgStack.pushBack(std::move(value)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::PUSHAP:
			std::terminate();
		case Opcode::CTORCALL:
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;
			uint32_t returnValueOutputReg = UINT32_MAX;

			if (ins.output != UINT32_MAX) {
				returnValueOutputReg = ins.output;
			}

			switch (ins.opcode) {
				case Opcode::CTORCALL:
				case Opcode::MCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
					const EntityRef &fnObjectRef = fnValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asObject.instanceObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], thisObjectValue));
					const EntityRef &thisObjectRef = thisObjectValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, thisObjectValue, ValueType::EntityRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, thisObjectRef, ObjectRefKind::ObjectRef));
					thisObject = thisObjectRef.asObject.instanceObject;
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
					const EntityRef &fnObjectRef = fnValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asObject.instanceObject;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _createNewMajorFrame(
															&context->_context,
															thisObject,
															fn,
															curMajorFrame->resumable->nextArgStack.data(),
															curMajorFrame->resumable->nextArgStack.size(),
															returnValueOutputReg));
			curMajorFrame->resumable->nextArgStack.clear();

			isContextChangedOut = true;
			break;
		}
		case Opcode::RET: {
			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			Value returnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], returnValue));

			if (CoroutineObject *co = curMajorFrame->curCoroutine; co) {
				co->finalResult = returnValue;
				co->resumable = curMajorFrame->resumable;
				co->setDone();
				context->_context.leaveMajor();
			} else {
				context->_context.leaveMajor();
			}

			if (returnValueOutReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, context->_context.majorFrames.back().get(), returnValueOutReg, returnValue));
			}

			isContextChangedOut = true;
			return {};
		}
		case Opcode::COCALL:
		case Opcode::COMCALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;
			uint32_t returnValueOutputReg = UINT32_MAX;

			if (ins.output != UINT32_MAX) {
				returnValueOutputReg = ins.output;
			}

			switch (ins.opcode) {
				case Opcode::COMCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
					const EntityRef &fnObjectRef = fnValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asObject.instanceObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], thisObjectValue));
					const EntityRef &thisObjectRef = thisObjectValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, thisObjectValue, ValueType::EntityRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, thisObjectRef, ObjectRefKind::ObjectRef));
					thisObject = thisObjectRef.asObject.instanceObject;
					break;
				}
				case Opcode::COCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
					const EntityRef &fnObjectRef = fnValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asObject.instanceObject;
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
															curMajorFrame->resumable->nextArgStack.data(),
															curMajorFrame->resumable->nextArgStack.size(),
															co));
			curMajorFrame->resumable->nextArgStack.clear();

			if (returnValueOutputReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, returnValueOutputReg, EntityRef::makeObjectRef(co.get())));
			}
			break;
		}
		case Opcode::YIELD: {
			if (!curMajorFrame->curCoroutine) {
				// TODO: Return an exception,
				std::terminate();
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
			Value returnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], returnValue));

			if (size_t szFrame = context->_context.stackTop - curMajorFrame->stackBase; szFrame) {
				char *p = curMajorFrame->curCoroutine->allocStackData(szFrame);
				if (!p) {
					return OutOfMemoryError::alloc();
				}
				memcpy(p, calcStackAddr(context->_context.dataStack, SLAKE_STACK_MAX, context->_context.stackTop), szFrame);
			}

			curMajorFrame->curCoroutine->unbindContext();
			++curMajorFrame->resumable->curIns;

			curMajorFrame->curCoroutine->resumable = curMajorFrame->resumable;

			context->_context.leaveMajor();

			MajorFrame *prevFrame = context->_context.majorFrames.back().get();
			if (returnValueOutReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, prevFrame, returnValueOutReg, returnValue));
			}

			isContextChangedOut = true;
			return {};
		}
		case Opcode::RESUME: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			HostObjectRef<CoroutineObject> co;

			Value fnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], fnValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
			const EntityRef &fnObjectRef = fnValue.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
			co = (CoroutineObject *)fnObjectRef.asObject.instanceObject;

			if (co->isDone()) {
				if (ins.output != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, dataStack, curMajorFrame, ins.output, co->finalResult));
				}
			} else {
				SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, co.get(), ins.output));
			}

			isContextChangedOut = true;
			break;
		}
		case Opcode::CODONE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			HostObjectRef<CoroutineObject> co;

			Value fnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], fnValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
			const EntityRef &fnObjectRef = fnValue.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
			co = (CoroutineObject *)fnObjectRef.asObject.instanceObject;

			SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, dataStack, curMajorFrame, ins.output, co->isDone()));
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 0));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, EntityRef::makeObjectRef(curMajorFrame->resumable->thisObject)));
			break;
		}
		case Opcode::NEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			switch (type.typeId) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)type.getCustomTypeExData();
					HostObjectRef<InstanceObject> instance = newClassInstance(cls, 0);
					if (!instance)
						// TODO: Return more detail exceptions.
						return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, EntityRef::makeObjectRef(instance.get())));
					break;
				}
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::ARRNEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[1], ValueType::U32));

			Type type = ins.operands[0].getTypeName();
			uint32_t size = ins.operands[1].getU32();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			auto instance = newArrayInstance(this, type, size);
			if (!instance)
				// TODO: Return more detailed exceptions.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, EntityRef::makeObjectRef(instance.get())));

			break;
		}
		case Opcode::THROW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			Value x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[0], x));

			for (size_t i = context->_context.majorFrames.size(); i; --i) {
				MajorFrame *majorFrame = context->_context.majorFrames.at(i - 1).get();
				for (size_t j = majorFrame->resumable->minorFrames.size(); j; --j) {
					auto &minorFrame = majorFrame->resumable->minorFrames.at(j - 1);

					uint32_t off;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _findAndDispatchExceptHandler(majorFrame->curExcept, minorFrame, off));
					if (off != UINT32_MAX) {
						for (size_t j = context->_context.majorFrames.size(); j > i; --j) {
							context->_context.leaveMajor();
						}
						majorFrame->resumable->minorFrames.resizeUninitialized(j);
						// Do not increase the current instruction offset,
						// the offset has been set to offset to first instruction
						// of the exception handler.
						majorFrame->resumable->curIns = off;
						return {};
					}
				}
			}

			curMajorFrame->curExcept = x;
			return allocOutOfMemoryErrorIfAllocFailed(UncaughtExceptionError::alloc(getFixedAlloc(), x));
		}
		case Opcode::PUSHXH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[1], ValueType::U32));

			ExceptionHandler xh;

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			xh.type = ins.operands[0].getTypeName();
			xh.off = ins.operands[1].getU32();

			if (!curMajorFrame->resumable->minorFrames.back().exceptHandlers.pushBack(std::move(xh)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::CAST: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));

			Value v;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[1], v));

			auto t = ins.operands[0].getTypeName();

			switch (t.typeId) {
				case TypeId::I8:
					v = _castToLiteralValue<int8_t>(v);
					break;
				case TypeId::I16:
					v = _castToLiteralValue<int16_t>(v);
					break;
				case TypeId::I32:
					v = _castToLiteralValue<int32_t>(v);
					break;
				case TypeId::I64:
					v = _castToLiteralValue<int64_t>(v);
					break;
				case TypeId::U8:
					v = _castToLiteralValue<uint8_t>(v);
					break;
				case TypeId::U16:
					v = _castToLiteralValue<uint16_t>(v);
					break;
				case TypeId::U32:
					v = _castToLiteralValue<uint32_t>(v);
					break;
				case TypeId::U64:
					v = _castToLiteralValue<uint64_t>(v);
					break;
				case TypeId::Bool:
					v = _castToLiteralValue<bool>(v);
					break;
				case TypeId::F32:
					v = _castToLiteralValue<float>(v);
					break;
				case TypeId::F64:
					v = _castToLiteralValue<double>(v);
					break;
				case TypeId::Instance:
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, curMajorFrame, ins.output, v));
			break;
		}
		case Opcode::PHI: {
			if (ins.output == UINT32_MAX) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if ((ins.nOperands < 2) || ((ins.nOperands & 1))) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			Value v;

			for (size_t i = 0; i < ins.nOperands; i += 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[i], ValueType::U32));

				uint32_t off = ins.operands[i].getU32();

				if (off == curMajorFrame->resumable->lastJumpSrc) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, curMajorFrame, ins.operands[i + 1], v));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.dataStack, curMajorFrame, ins.output, v));

					goto succeeded;
				}
			}

			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));

		succeeded:
			break;
		}
		default:
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOpcodeError::alloc(getFixedAlloc(), ins.opcode));
	}
	++curMajorFrame->resumable->curIns;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::execContext(ContextObject *context) noexcept {
	size_t initialMajorFrameDepth = context->_context.majorFrames.size();
	const FnOverloadingObject *curFn;
	MajorFrame *curMajorFrame;
	InternalExceptionPointer exceptPtr;
	ExecutionRunnable *managedThread = managedThreadRunnables.at(currentThreadHandle());

	while (context->_context.majorFrames.size() >= initialMajorFrameDepth) {
		curMajorFrame = context->_context.majorFrames.back().get();
		curFn = curMajorFrame->curFn;

		if (!curFn) {
			break;
		}

		// Pause if the runtime is in GC
		/*while (_flags & _RT_INGC)
			yieldCurrentThread();*/

		switch (curFn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

				bool isContextChanged = false;
				while (!isContextChanged) {
					if (curMajorFrame->resumable->curIns >=
						ol->instructions.size()) {
						// Raise out of fn body error.
					}

					// Interrupt execution if the thread is explicitly specified to be killed.
					if (managedThread->status == ThreadStatus::Dead) {
						return {};
					}

					if ((fixedAlloc.szAllocated > _szComputedGcLimit)) {
						gc();
					}

					const Instruction &ins = ol->instructions.at(curMajorFrame->resumable->curIns);

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, ins, isContextChanged));
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)curFn;

				Value returnValue = ol->callback(
					&context->getContext(),
					curMajorFrame);
				uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
				context->_context.leaveMajor();
				if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.dataStack, curMajorFrame, returnValueOutReg, returnValue));
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
	Value &valueOut,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	HostObjectRef<ContextObject> context(prevContext);

	if (!context) {
		context = ContextObject::alloc(this);

		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, UINT32_MAX));
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));
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

[[nodiscard]] SLAKE_API InternalExceptionPointer Runtime::execFnInAotFn(
	const FnOverloadingObject *overloading,
	ContextObject *context,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	HostObjectRef<ContextObject> contextPtr(context);

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));

	InternalExceptionPointer exceptPtr = execContext(context);

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

	if (!context) {
		context = ContextObject::alloc(this);

		contextOut = context;

		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, 0));
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));
	} else {
		contextOut = context;
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

	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>> t(Thread::alloc(getFixedAlloc(), &runnable, SLAKE_NATIVE_STACK_MAX));

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

	if (!co) {
		return OutOfMemoryError::alloc();
	}

	co->overloading = fn;

	MajorFrame newMajorFrame(this, co->selfAllocator.get());

	if (!(newMajorFrame.resumable = ResumableObject::alloc(this)))
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(_fillArgs(&newMajorFrame, fn, args, nArgs, holder));
	newMajorFrame.resumable->thisObject = thisObject;

	if (!newMajorFrame.resumable->minorFrames.pushBack(MinorFrame(this, co->selfAllocator.get(), 0)))
		return OutOfMemoryError::alloc();

	co->resumable = newMajorFrame.resumable;

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

	if (!contextRef) {
		contextRef = ContextObject::alloc(this);
	}

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, UINT32_MAX));
	MajorFrame *topMajorFrame = context->_context.majorFrames.back().get();
	SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, coroutine, 0));

	{
		ExecutionRunnable runnable;

		runnable.context = context;

		{
			if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
				contextRef->_context.leaveMajor();

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

		resultOut = ((const Value *)calcStackAddr(context->_context.dataStack, SLAKE_STACK_MAX, topMajorFrame->offRegs))[0];

		contextRef->_context.leaveMajor();
	}

	return {};
}
