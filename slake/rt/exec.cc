#include "../runtime.h"
#include <slake/flib/math/fmod.h>
#include <slake/flib/bitop.h>
#include <slake/util/scope_guard.h>
#include <cmath>

#undef new

using namespace slake;

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandCount(
	Runtime *runtime,
	const Instruction &ins,
	bool hasOutput,
	int_fast8_t nOperands) noexcept {
	if (hasOutput) {
		if (ins.output.valueType == ValueType::Undefined) {
			return InvalidOperandsError::alloc(runtime);
		}
	}
	if (ins.nOperands != nOperands) {
		return InvalidOperandsError::alloc(runtime);
	}

	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandType(
	Runtime *runtime,
	const Value &operand,
	ValueType valueType) noexcept {
	if (operand.valueType != valueType) {
		return InvalidOperandsError::alloc(runtime);
	}
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectRefOperandType(
	Runtime *runtime,
	const EntityRef &operand,
	ObjectRefKind kind) noexcept {
	if (operand.kind != kind) {
		return InvalidOperandsError::alloc(runtime);
	}
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectOperandType(
	Runtime *runtime,
	Object *object,
	ObjectKind typeId) noexcept {
	if (object->getKind() != typeId) {
		return InvalidOperandsError::alloc(runtime);
	}
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _setRegisterValue(
	Runtime *runtime,
	MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) noexcept {
	if (index >= curMajorFrame->nRegs) {
		// The register does not present.
		return InvalidOperandsError::alloc(runtime);
	}
	curMajorFrame->regs[index] = value;
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _fetchRegValue(
	Runtime *runtime,
	MajorFrame *curMajorFrame,
	uint32_t index,
	Value &valueOut) noexcept {
	if (index >= curMajorFrame->nRegs) {
		// The register does not present.
		return InvalidOperandsError::alloc(runtime);
	}
	valueOut = curMajorFrame->regs[index];
	return {};
}

[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer _unwrapRegOperand(
	Runtime *runtime,
	MajorFrame *curMajorFrame,
	const Value &value,
	Value &valueOut) noexcept {
	if (value.valueType == ValueType::RegRef)
		return _fetchRegValue(runtime, curMajorFrame, value.getRegIndex(), valueOut);
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
		case ValueType::U8:
			return Value((LT)(x.getU8()));
		case ValueType::U16:
			return Value((LT)(x.getU16()));
		case ValueType::U32:
			return Value((LT)(x.getU32()));
		case ValueType::U64:
			return Value((LT)(x.getU64()));
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

SLAKE_API InternalExceptionPointer slake::Runtime::_createNewMajorFrame(
	Context *context,
	Object *thisObject,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	uint32_t returnValueOut) noexcept {
	HostRefHolder holder(&globalHeapPoolAlloc);

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop]() noexcept {
		context->stackTop = prevStackTop;
	});
	MajorFrame *newMajorFrame = (MajorFrame *)context->stackAlloc(sizeof(MajorFrame));
	if (!newMajorFrame)
		return OutOfMemoryError::alloc();
	peff::constructAt<MajorFrame>(newMajorFrame, this, context);
	peff::ScopeGuard releaseNewMajorFrameGuard([newMajorFrame]() noexcept {
		std::destroy_at<MajorFrame>(newMajorFrame);
	});

	if (!newMajorFrame->minorFrames.pushBack(MinorFrame(this, context->stackTop)))
		return OutOfMemoryError::alloc();

	if (!fn) {
		// Used in the creation of top major frame.
		newMajorFrame->curFn = nullptr;
		newMajorFrame->nRegs = 1;
		newMajorFrame->regs = (Value *)context->stackAlloc(sizeof(Value) * 1);
		*newMajorFrame->regs = Value(ValueType::Undefined);
	} else {
		newMajorFrame->curFn = fn;
		newMajorFrame->thisObject = thisObject;

		if (nArgs < fn->paramTypes.size()) {
			return InvalidArgumentNumberError::alloc(this, nArgs);
		}

		if (!newMajorFrame->argStack.resize(fn->paramTypes.size()))
			return OutOfMemoryError::alloc();
		for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
			newMajorFrame->argStack.at(i) = { Value(), fn->paramTypes.at(i) };
			SLAKE_RETURN_IF_EXCEPT(writeVar(EntityRef::makeArgRef(newMajorFrame, i), args[i]));
		}

		if (fn->overloadingFlags & OL_VARG) {
			auto varArgTypeDefObject = TypeDefObject::alloc(this, Type(TypeId::Any));
			if (!holder.addObject(varArgTypeDefObject.get()))
				return OutOfMemoryError::alloc();

			size_t szVarArgArray = nArgs - fn->paramTypes.size();
			auto varArgArrayObject = newArrayInstance(this, Type(TypeId::Any), szVarArgArray);
			if (!holder.addObject(varArgArrayObject.get()))
				return OutOfMemoryError::alloc();

			for (size_t i = 0; i < szVarArgArray; ++i) {
				((Value *)varArgArrayObject->data)[i] = args[fn->paramTypes.size() + i];
			}

			if (!newMajorFrame->argStack.pushBack({ EntityRef::makeObjectRef(varArgArrayObject.get()), Type(TypeId::Array, varArgTypeDefObject.get()) }))
				return OutOfMemoryError::alloc();
		}

		switch (fn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn;
				newMajorFrame->nRegs = ol->nRegisters;
				newMajorFrame->regs = (Value *)context->stackAlloc(sizeof(Value) * ol->nRegisters);
				for (size_t i = 0; i < ol->nRegisters; ++i)
					newMajorFrame->regs[i] = Value(ValueType::Undefined);
				break;
			}
			default:;
		}
	}

	newMajorFrame->returnValueOutReg = returnValueOut;

	releaseNewMajorFrameGuard.release();
	restoreStackTopGuard.release();

	if (!context->majorFrameList)
		context->stackTopMajorFrame = newMajorFrame;
	newMajorFrame->stackBase = context->stackTop;
	newMajorFrame->next = context->majorFrameList;
	context->majorFrameList = newMajorFrame;
	return {};
}

//
// TODO: Check if the stackAlloc() was successful.
//
SLAKE_API InternalExceptionPointer slake::Runtime::_addLocalVar(MajorFrame *frame, Type type, EntityRef &objectRefOut) noexcept {
	size_t stackOffset;

	switch (type.typeId) {
		case TypeId::I8:
			if (!frame->context->stackAlloc(sizeof(int8_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::I16:
			if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(int16_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::I32:
			if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(int32_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::I64:
			if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(int64_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::U8:
			if (!frame->context->stackAlloc(sizeof(uint8_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::U16:
			if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(uint16_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::U32:
			if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(uint32_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::U64:
			if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(uint64_t)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::F32:
			if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(float)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::F64:
			if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
				return StackOverflowError::alloc(this);
			if (!frame->context->stackAlloc(sizeof(double)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
		case TypeId::Bool:
			if (!frame->context->stackAlloc(sizeof(bool)))
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			break;
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::Ref: {
			if (!frame->context->stackAlloc(sizeof(void *) - (frame->context->stackTop & (sizeof(void *) - 1))))
				return StackOverflowError::alloc(this);
			Object **ptr = (Object **)frame->context->stackAlloc(sizeof(void *));
			if (!ptr)
				return StackOverflowError::alloc(this);
			stackOffset = frame->context->stackTop;
			*ptr = nullptr;
			break;
		}
		default:
			std::terminate();
	}

	objectRefOut = EntityRef::makeLocalVarRef(frame->context, stackOffset, type);
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(MajorFrame *majorFrame, Runtime *rt, uint32_t off, EntityRef &objectRefOut) {
	if (off >= majorFrame->argStack.size()) {
		return InvalidOperandsError::alloc(rt);
	}

	objectRefOut = EntityRef::makeArgRef(majorFrame, off);
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer Runtime::_execIns(ContextObject *context, MajorFrame *curMajorFrame, const Instruction &ins) noexcept {
	InternalExceptionPointer exceptPtr;

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			EntityRef entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(curMajorFrame, type, entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), entityRef));
			break;
		}
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::EntityRef));
			auto refPtr = ins.operands[0].getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, refPtr, ObjectRefKind::ObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, refPtr.asObject.instanceObject, ObjectKind::IdRef));

			EntityRef entityRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asObject.instanceObject, entityRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), Value(entityRef)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::RegRef));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _fetchRegValue(this, curMajorFrame, ins.operands[0].getRegIndex(), lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, lhs, ValueType::EntityRef));

			auto lhsPtr = lhs.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, lhsPtr, ObjectRefKind::ObjectRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[1], ValueType::EntityRef));
			auto refPtr = ins.operands[1].getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, refPtr, ObjectRefKind::ObjectRef));

			if (!lhsPtr) {
				return NullRefError::alloc(this);
			}

			EntityRef entityRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asObject.instanceObject, entityRef, lhsPtr.asObject.instanceObject));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															Value(entityRef)));
			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

			Value destValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], destValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, destValue, ValueType::EntityRef));

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, writeVar(destValue.getEntityRef(), data));
			break;
		}
		case Opcode::MOV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this,
															curMajorFrame,
															ins.operands[0],
															value));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															value));
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));

			EntityRef entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, larg(curMajorFrame, this, ins.operands[0].getU32(), entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															Value(entityRef)));
			break;
		}
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, dest, ValueType::EntityRef));

			const EntityRef &entityRef = dest.getEntityRef();

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, readVar(entityRef, data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															data));
			break;
		}
		case Opcode::ENTER: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 0));
			MinorFrame frame(
				this,
				context->_context.stackTop);

			if (!curMajorFrame->minorFrames.pushBack(std::move(frame)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::LEAVE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 0));
			if (curMajorFrame->minorFrames.size() < 2) {
				return FrameBoundaryExceededError::alloc(this);
			}
			if (!curMajorFrame->leave())
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::AND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::OR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::XOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LAND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
			}

			switch (x.valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x.getBool() && y.getBool()));
					break;
				default:
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
			}

			switch (x.valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x.getBool() || y.getBool()));
					break;
				default:
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::EQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::NEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::GT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::GTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
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
					return InvalidOperandsError::alloc(this);
			}
			uint64_t lhs = x.getU64(), rhs = y.getU64();
			if (lhs > rhs) {
				valueOut = Value((int32_t)1);
			} else if (lhs < rhs) {
				valueOut = Value((int32_t)-1);
			} else
				valueOut = Value((int32_t)0);

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::CMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));
			if (x.valueType != y.valueType) {
				return InvalidOperandsError::alloc(this);
			}

			switch (x.valueType) {
				case ValueType::I8: {
					int8_t lhs = x.getI8(), rhs = y.getI8();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::I16: {
					int16_t lhs = x.getI16(), rhs = y.getI16();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::I32: {
					int32_t lhs = x.getI32(), rhs = y.getI32();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::I64: {
					int64_t lhs = x.getI64(), rhs = y.getI64();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::U8: {
					uint8_t lhs = x.getU8(), rhs = y.getU8();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::U16: {
					uint16_t lhs = x.getU16(), rhs = y.getU16();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::U32: {
					uint32_t lhs = x.getU32(), rhs = y.getU32();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::U64: {
					uint64_t lhs = x.getU64(), rhs = y.getU64();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::F32: {
					float lhs = x.getF32(), rhs = y.getF32();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				case ValueType::F64: {
					double lhs = x.getF64(), rhs = y.getF64();
					if (lhs > rhs) {
						valueOut = Value((int32_t)1);
					} else if (lhs < rhs) {
						valueOut = Value((int32_t)-1);
					} else
						valueOut = Value((int32_t)0);
					break;
				}
				default:
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x,
				y,
				valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));

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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x,
				y,
				valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));

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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::NOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LNOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::AT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			Value arrayValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], arrayValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, arrayValue, ValueType::EntityRef));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, index, ValueType::U32));

			auto arrayIn = arrayValue.getEntityRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, arrayIn, ObjectRefKind::ObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, arrayIn.asObject.instanceObject, ObjectKind::Array));
			ArrayObject *arrayObject = (ArrayObject *)arrayIn.asObject.instanceObject;

			uint32_t indexIn = index.getU32();

			if (indexIn > arrayObject->length) {
				return InvalidArrayIndexError::alloc(this, indexIn);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															Value(EntityRef::makeArrayElementRef(arrayObject, indexIn))));

			break;
		}
		case Opcode::JMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));

			curMajorFrame->curIns = ins.operands[0].getU32();
			return {};
		}
		case Opcode::JT:
		case Opcode::JF: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));
			Value condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, condition, ValueType::Bool));

			if (condition.getBool()) {
				if (ins.opcode == Opcode::JT) {
					curMajorFrame->curIns = ins.operands[0].getU32();
					return {};
				}
			} else if (ins.opcode == Opcode::JF) {
				curMajorFrame->curIns = ins.operands[0].getU32();
				return {};
			}

			break;
		}
		case Opcode::PUSHARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], value));
			if (!curMajorFrame->nextArgStack.pushBack(std::move(value)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::CTORCALL:
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;
			uint32_t returnValueOutputReg = UINT32_MAX;

			if (ins.output.valueType != ValueType::Undefined) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

				returnValueOutputReg = ins.output.getRegIndex();
			}

			switch (ins.opcode) {
				case Opcode::CTORCALL:
				case Opcode::MCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
					const EntityRef &fnObjectRef = fnValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asObject.instanceObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], thisObjectValue));
					const EntityRef &thisObjectRef = thisObjectValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, thisObjectValue, ValueType::EntityRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, thisObjectRef, ObjectRefKind::ObjectRef));
					thisObject = thisObjectRef.asObject.instanceObject;
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::EntityRef));
					const EntityRef &fnObjectRef = fnValue.getEntityRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asObject.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asObject.instanceObject;
					break;
				}
				default:
					assert(false);
			}

			if (!fn) {
				return NullRefError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _createNewMajorFrame(
															&context->_context,
															thisObject,
															fn,
															curMajorFrame->nextArgStack.data(),
															curMajorFrame->nextArgStack.size(),
															returnValueOutputReg));
			curMajorFrame->nextArgStack.clear();

			break;
		}
		case Opcode::RET: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
			Value returnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], returnValue));
			context->_context.leaveMajor();

			if (returnValueOutReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrameList, returnValueOutReg, returnValue));
			}
			return {};
		}
		case Opcode::YIELD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
			Value returnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], returnValue));

			if (returnValueOutReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrameList, returnValueOutReg, returnValue));
			}
			context->_context.flags |= CTX_YIELDED;
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 0));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), EntityRef::makeObjectRef(curMajorFrame->thisObject)));
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
						return InvalidOperandsError::alloc(this);
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), EntityRef::makeObjectRef(instance.get())));
					break;
				}
				default:
					return InvalidOperandsError::alloc(this);
			}
			break;
		}
		case Opcode::ARRNEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[1], ValueType::U32));

			Type type = ins.operands[1].getTypeName();
			uint32_t size = ins.operands[2].getU32();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			auto instance = newArrayInstance(this, type, size);
			if (!instance)
				// TODO: Return more detailed exceptions.
				return InvalidOperandsError::alloc(this);

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), instance.get()));

			break;
		}
		case Opcode::THROW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			Value x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));

			for (MajorFrame *i = context->_context.majorFrameList; i; i = i->next) {
				for (size_t j = i->minorFrames.size(); j; --j) {
					auto &minorFrame = i->minorFrames.at(j - 1);

					if (uint32_t off = _findAndDispatchExceptHandler(i->curExcept, minorFrame);
						off != UINT32_MAX) {
						for (MajorFrame *k = context->_context.majorFrameList, *next; k != i; k = next) {
							next = k->next;
							context->_context.leaveMajor();
						}
						context->_context.majorFrameList = i;
						i->minorFrames.resize(j);
						// Do not increase the current instruction offset,
						// the offset has been set to offset to first instruction
						// of the exception handler.
						i->curIns = off;
						return {};
					}
				}
			}

			curMajorFrame->curExcept = x;
			return UncaughtExceptionError::alloc(this, x);
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

			if (!curMajorFrame->minorFrames.back().exceptHandlers.pushBack(std::move(xh)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::CAST: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::TypeName));

			Value v;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], v));

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
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), v));
			break;
		}
		default:
			return InvalidOpcodeError::alloc(this, ins.opcode);
	}
	++curMajorFrame->curIns;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::execContext(ContextObject *context) noexcept {
	const FnOverloadingObject *curFn;
	MajorFrame *curMajorFrame;
	InternalExceptionPointer exceptPtr;
	ManagedThread *managedThread = managedThreads.at(currentThreadHandle()).get();

	switch (managedThread->threadKind) {
		case ThreadKind::AttachedExecutionThread: {
			bool interruptExecution = false;

			while (!interruptExecution) {
				curMajorFrame = context->getContext().majorFrameList;
				curFn = curMajorFrame->curFn;

				if (!curFn) {
					managedThreads.erase(currentThreadHandle());
					break;
				}

				// TODO: Check if the yield request is from the top level.
				if (context->getContext().flags & CTX_YIELDED)
					return {};

				// Pause if the runtime is in GC
				while (_flags & _RT_INGC)
					yieldCurrentThread();

				// Interrupt execution if the thread is explicitly specified to be killed.
				if (managedThread->status == ThreadStatus::Dead) {
					return {};
				}

				switch (curFn->overloadingKind) {
					case FnOverloadingKind::Regular: {
						RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

						if (curMajorFrame->curIns >=
							ol->instructions.size()) {
							// Raise out of fn body error.
						}

						if ((globalHeapPoolAlloc.szAllocated > _szComputedGcLimit)) {
							gc();
						}

						const Instruction &ins = ol->instructions.at(curMajorFrame->curIns);

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, ins));

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
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrameList, returnValueOutReg, returnValue));
						}

						break;
					}
					default:
						std::terminate();
				}
			}
			break;
		}
		case ThreadKind::ExecutionThread: {
			bool interruptExecution = false;

			while (!interruptExecution) {
				curMajorFrame = context->getContext().majorFrameList;
				curFn = curMajorFrame->curFn;

				if (!curFn) {
					break;
				}

				// TODO: Check if the yield request is from the top level.
				if (context->getContext().flags & CTX_YIELDED)
					return {};

				// Pause if the runtime is in GC
				while (_flags & _RT_INGC)
					yieldCurrentThread();

				// Interrupt execution if the thread is explicitly specified to be killed.
				if (managedThread->status == ThreadStatus::Dead) {
					return {};
				}

				switch (curFn->overloadingKind) {
					case FnOverloadingKind::Regular: {
						RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

						if (curMajorFrame->curIns >=
							ol->instructions.size()) {
							// Raise out of fn body error.
						}

						if ((globalHeapPoolAlloc.szAllocated > _szComputedGcLimit)) {
							gc();
						}

						const Instruction &ins = ol->instructions.at(curMajorFrame->curIns);

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, ins));

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
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrameList, returnValueOutReg, returnValue));
						}

						break;
					}
					default:
						std::terminate();
				}
			}
			break;
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
	HostObjectRef<ContextObject> &contextOut,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	HostObjectRef<ContextObject> context(prevContext);

	if (!context) {
		context = ContextObject::alloc(this);

		contextOut = context;

		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, 0));
		SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));
	} else {
		contextOut = context;
	}

	AttachedExecutionThread *executionThread = createAttachedExecutionThreadForCurrentThread(this, context.get(), nativeStackBaseCurrentPtr, nativeStackSize);
	if (!executionThread) {
		// Note: we use out of memory error as the placeholder, it originally should be ThreadCreationFailedError.
		return OutOfMemoryError::alloc();
	}

	managedThreads.insert({ executionThread->nativeThreadHandle, std::unique_ptr<ManagedThread, util::DeallocableDeleter<ManagedThread>>(executionThread) });

	InternalExceptionPointer exceptPtr = execContext(context.get());

	return std::move(exceptPtr);
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

	ExecutionThread *executionThread = createExecutionThread(this, context.get(), SLAKE_NATIVE_STACK_MAX);
	if (!executionThread) {
		// Note: we use out of memory error as the placeholder, it originally should be ThreadCreationFailedError.
		return OutOfMemoryError::alloc();
	}

	managedThreads.insert({ executionThread->nativeThreadHandle, std::unique_ptr<ManagedThread, util::DeallocableDeleter<ManagedThread>>(executionThread) });
	executionThread->join();

	InternalExceptionPointer exceptPtr = std::move(executionThread->exceptionPtr);

	return std::move(exceptPtr);
}
