#include "../runtime.h"
#include <slake/flib/math/fmod.h>
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
	if (ins.operands.size() != nOperands) {
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
	const ObjectRef &operand,
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
	Value &reg = curMajorFrame->regs.at(index);
	new (&reg) Value(value);
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
	valueOut = curMajorFrame->regs.at(index);
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

	std::unique_ptr<MajorFrame> newMajorFrame = std::make_unique<MajorFrame>(this, context);

	newMajorFrame->minorFrames.pushBack(MinorFrame(this, 0, context->stackTop));

	newMajorFrame->localVarAccessor = LocalVarAccessorVarObject::alloc(this, context, newMajorFrame.get()).get();

	newMajorFrame->curFn = fn;
	newMajorFrame->thisObject = thisObject;

	if (nArgs < fn->paramTypes.size()) {
		return InvalidArgumentNumberError::alloc(this, nArgs);
	}

	newMajorFrame->argStack.resize(fn->paramTypes.size());
	for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
		newMajorFrame->argStack.at(i) = { Value(), fn->paramTypes.at(i) };
		SLAKE_RETURN_IF_EXCEPT(writeVar(ObjectRef::makeArgRef(newMajorFrame.get(), i), args[i]));
	}

	if (fn->overloadingFlags & OL_VARG) {
		auto varArgTypeDefObject = TypeDefObject::alloc(this, Type(TypeId::Any));
		holder.addObject(varArgTypeDefObject.get());

		size_t szVarArgArray = nArgs - fn->paramTypes.size();
		auto varArgArrayObject = newArrayInstance(this, Type(TypeId::Any), szVarArgArray);
		holder.addObject(varArgArrayObject.get());

		for (size_t i = 0; i < szVarArgArray; ++i) {
			((Value *)varArgArrayObject->data)[i] = args[fn->paramTypes.size() + i];
		}

		newMajorFrame->argStack.pushBack({ ObjectRef::makeInstanceRef(varArgArrayObject.get()), Type(TypeId::Array, varArgTypeDefObject.get()) });
	}

	switch (fn->overloadingKind) {
		case FnOverloadingKind::Regular: {
			RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn;
			newMajorFrame->resizeRegs(ol->nRegisters);
			break;
		}
	}

	newMajorFrame->returnValueOutReg = returnValueOut;

	context->majorFrames.push_back(std::move(newMajorFrame));
	return {};
}

//
// TODO: Check if the stackAlloc() was successful.
//
SLAKE_API InternalExceptionPointer slake::Runtime::_addLocalVar(MajorFrame *frame, Type type, ObjectRef &objectRefOut) noexcept {
	LocalVarRecord localVarRecord;

	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					if (!frame->context->stackAlloc(sizeof(int8_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::I16:
					if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(int16_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::I32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(int32_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::I64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(int64_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::U8:
					if (!frame->context->stackAlloc(sizeof(uint8_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::U16:
					if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(uint16_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::U32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(uint32_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::U64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(uint64_t)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::F32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(float)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::F64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return StackOverflowError::alloc(this);
					if (!frame->context->stackAlloc(sizeof(double)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::Bool:
					if (!frame->context->stackAlloc(sizeof(bool)))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					break;
				case ValueType::ObjectRef: {
					if (!frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1))))
						return StackOverflowError::alloc(this);
					Object **ptr = (Object **)frame->context->stackAlloc(sizeof(Object *));
					if (!ptr)
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					*ptr = nullptr;
					break;
				}
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::Ref: {
			if (!frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1))))
				return StackOverflowError::alloc(this);
			Object **ptr = (Object **)frame->context->stackAlloc(sizeof(Object *));
			if (!ptr)
				return StackOverflowError::alloc(this);
			localVarRecord.stackOffset = frame->context->stackTop;
			*ptr = nullptr;
			break;
		}
		default:
			throw std::runtime_error("The variable has an inconstructible type");
	}

	localVarRecord.type = type;

	uint32_t index = (uint32_t)frame->localVarRecords.size();
	frame->localVarRecords.pushBack(std::move(localVarRecord));
	objectRefOut = ObjectRef::makeLocalVarRef(frame, index);
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer lload(MajorFrame *majorFrame, Runtime *rt, uint32_t off, ObjectRef &objectRefOut) {
	if (off >= majorFrame->localVarRecords.size()) {
		return InvalidLocalVarIndexError::alloc(rt, off);
	}

	objectRefOut = ObjectRef::makeLocalVarRef(majorFrame, off);
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(MajorFrame *majorFrame, Runtime *rt, uint32_t off, ObjectRef &objectRefOut) {
	if (off >= majorFrame->argStack.size()) {
		return InvalidOperandsError::alloc(rt);
	}

	objectRefOut = ObjectRef::makeArgRef(majorFrame, off);
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

			ObjectRef objectRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(curMajorFrame, type, objectRef));
			break;
		}
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::ObjectRef));
			auto refPtr = ins.operands[0].getObjectRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, refPtr, ObjectRefKind::InstanceRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, refPtr.asInstance.instanceObject, ObjectKind::IdRef));

			ObjectRef objectRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asInstance.instanceObject, objectRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), Value(objectRef)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::RegRef));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _fetchRegValue(this, curMajorFrame, ins.operands[0].getRegIndex(), lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, lhs, ValueType::ObjectRef));

			auto lhsPtr = lhs.getObjectRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, lhsPtr, ObjectRefKind::InstanceRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[1], ValueType::ObjectRef));
			auto refPtr = ins.operands[1].getObjectRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, refPtr, ObjectRefKind::InstanceRef));

			if (!lhsPtr) {
				return NullRefError::alloc(this);
			}

			ObjectRef objectRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asInstance.instanceObject, objectRef, lhsPtr.asInstance.instanceObject));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															Value(objectRef)));
			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 2));

			Value destValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], destValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, destValue, ValueType::ObjectRef));

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, writeVar(destValue.getObjectRef(), data));
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
		case Opcode::LLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));

			ObjectRef objectRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, lload(curMajorFrame, this, ins.operands[0].getU32(), objectRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr,
				_setRegisterValue(this,
					curMajorFrame,
					ins.output.getRegIndex(),
					Value(objectRef)));
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.operands[0], ValueType::U32));

			ObjectRef objectRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, larg(curMajorFrame, this, ins.operands[0].getU32(), objectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															Value(objectRef)));
			break;
		}
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, dest, ValueType::ObjectRef));

			const ObjectRef &objectRef = dest.getObjectRef();

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, readVar(objectRef, data));
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
				(uint32_t)curMajorFrame->localVarRecords.size(),
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
		case Opcode::ADD:
		case Opcode::SUB:
		case Opcode::MUL:
		case Opcode::DIV:
		case Opcode::MOD:
		case Opcode::AND:
		case Opcode::OR:
		case Opcode::XOR:
		case Opcode::LAND:
		case Opcode::LOR:
		case Opcode::EQ:
		case Opcode::NEQ:
		case Opcode::LT:
		case Opcode::GT:
		case Opcode::LTEQ:
		case Opcode::GTEQ:
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
				case ValueType::I8:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((int8_t)(x.getI8() + y.getI8()));
							break;
						case Opcode::SUB:
							valueOut = Value((int8_t)(x.getI8() - y.getI8()));
							break;
						case Opcode::MUL:
							valueOut = Value((int8_t)(x.getI8() * y.getI8()));
							break;
						case Opcode::DIV:
							valueOut = Value((int8_t)(x.getI8() / y.getI8()));
							break;
						case Opcode::MOD:
							valueOut = Value((int8_t)(x.getI8() % y.getI8()));
							break;
						case Opcode::AND:
							valueOut = Value((int8_t)(x.getI8() & y.getI8()));
							break;
						case Opcode::OR:
							valueOut = Value((int8_t)(x.getI8() | y.getI8()));
							break;
						case Opcode::XOR:
							valueOut = Value((int8_t)(x.getI8() ^ y.getI8()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getI8() && y.getI8()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getI8() || y.getI8()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getI8() == y.getI8()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getI8() != y.getI8()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getI8() < y.getI8()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getI8() > y.getI8()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getI8() <= y.getI8()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getI8() >= y.getI8()));
							break;
						case Opcode::CMP: {
							int8_t lhs = x.getI8(), rhs = y.getI8();
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
					break;
				case ValueType::I16:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((int16_t)(x.getI16() + y.getI16()));
							break;
						case Opcode::SUB:
							valueOut = Value((int16_t)(x.getI16() - y.getI16()));
							break;
						case Opcode::MUL:
							valueOut = Value((int16_t)(x.getI16() * y.getI16()));
							break;
						case Opcode::DIV:
							valueOut = Value((int16_t)(x.getI16() / y.getI16()));
							break;
						case Opcode::MOD:
							valueOut = Value((int16_t)(x.getI16() % y.getI16()));
							break;
						case Opcode::AND:
							valueOut = Value((int16_t)(x.getI16() & y.getI16()));
							break;
						case Opcode::OR:
							valueOut = Value((int16_t)(x.getI16() | y.getI16()));
							break;
						case Opcode::XOR:
							valueOut = Value((int16_t)(x.getI16() ^ y.getI16()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getI16() && y.getI16()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getI16() || y.getI16()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getI16() == y.getI16()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getI16() != y.getI16()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getI16() < y.getI16()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getI16() > y.getI16()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getI16() <= y.getI16()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getI16() >= y.getI16()));
							break;
						case Opcode::CMP: {
							int16_t lhs = x.getI16(), rhs = y.getI16();
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
					break;
				case ValueType::I32:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((int32_t)(x.getI32() + y.getI32()));
							break;
						case Opcode::SUB:
							valueOut = Value((int32_t)(x.getI32() - y.getI32()));
							break;
						case Opcode::MUL:
							valueOut = Value((int32_t)(x.getI32() * y.getI32()));
							break;
						case Opcode::DIV:
							valueOut = Value((int32_t)(x.getI32() / y.getI32()));
							break;
						case Opcode::MOD:
							valueOut = Value((int32_t)(x.getI32() % y.getI32()));
							break;
						case Opcode::AND:
							valueOut = Value((int32_t)(x.getI32() & y.getI32()));
							break;
						case Opcode::OR:
							valueOut = Value((int32_t)(x.getI32() | y.getI32()));
							break;
						case Opcode::XOR:
							valueOut = Value((int32_t)(x.getI32() ^ y.getI32()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getI32() && y.getI32()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getI32() || y.getI32()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getI32() == y.getI32()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getI32() != y.getI32()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getI32() < y.getI32()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getI32() > y.getI32()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getI32() <= y.getI32()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getI32() >= y.getI32()));
							break;
						case Opcode::CMP: {
							int32_t lhs = x.getI32(), rhs = y.getI32();
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
					break;
				case ValueType::I64:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((int64_t)(x.getI64() + y.getI64()));
							break;
						case Opcode::SUB:
							valueOut = Value((int64_t)(x.getI64() - y.getI64()));
							break;
						case Opcode::MUL:
							valueOut = Value((int64_t)(x.getI64() * y.getI64()));
							break;
						case Opcode::DIV:
							valueOut = Value((int64_t)(x.getI64() / y.getI64()));
							break;
						case Opcode::MOD:
							valueOut = Value((int64_t)(x.getI64() % y.getI64()));
							break;
						case Opcode::AND:
							valueOut = Value((int64_t)(x.getI64() & y.getI64()));
							break;
						case Opcode::OR:
							valueOut = Value((int64_t)(x.getI64() | y.getI64()));
							break;
						case Opcode::XOR:
							valueOut = Value((int64_t)(x.getI64() ^ y.getI64()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getI64() && y.getI64()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getI64() || y.getI64()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getI64() == y.getI64()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getI64() != y.getI64()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getI64() < y.getI64()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getI64() > y.getI64()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getI64() <= y.getI64()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getI64() >= y.getI64()));
							break;
						case Opcode::CMP: {
							int64_t lhs = x.getI64(), rhs = y.getI64();
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
					break;
				case ValueType::U8:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((uint8_t)(x.getU8() + y.getU8()));
							break;
						case Opcode::SUB:
							valueOut = Value((uint8_t)(x.getU8() - y.getU8()));
							break;
						case Opcode::MUL:
							valueOut = Value((uint8_t)(x.getU8() * y.getU8()));
							break;
						case Opcode::DIV:
							valueOut = Value((uint8_t)(x.getU8() / y.getU8()));
							break;
						case Opcode::MOD:
							valueOut = Value((uint8_t)(x.getU8() % y.getU8()));
							break;
						case Opcode::AND:
							valueOut = Value((uint8_t)(x.getU8() & y.getU8()));
							break;
						case Opcode::OR:
							valueOut = Value((uint8_t)(x.getU8() | y.getU8()));
							break;
						case Opcode::XOR:
							valueOut = Value((uint8_t)(x.getU8() ^ y.getU8()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getU8() && y.getU8()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getU8() || y.getU8()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getU8() == y.getU8()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getU8() != y.getU8()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getU8() < y.getU8()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getU8() > y.getU8()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getU8() <= y.getU8()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getU8() >= y.getU8()));
							break;
						case Opcode::CMP: {
							uint8_t lhs = x.getU8(), rhs = y.getU8();
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
					break;
				case ValueType::U16:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((uint8_t)(x.getU16() + y.getU16()));
							break;
						case Opcode::SUB:
							valueOut = Value((uint8_t)(x.getU16() - y.getU16()));
							break;
						case Opcode::MUL:
							valueOut = Value((uint8_t)(x.getU16() * y.getU16()));
							break;
						case Opcode::DIV:
							valueOut = Value((uint8_t)(x.getU16() / y.getU16()));
							break;
						case Opcode::MOD:
							valueOut = Value((uint8_t)(x.getU16() % y.getU16()));
							break;
						case Opcode::AND:
							valueOut = Value((uint8_t)(x.getU16() & y.getU16()));
							break;
						case Opcode::OR:
							valueOut = Value((uint8_t)(x.getU16() | y.getU16()));
							break;
						case Opcode::XOR:
							valueOut = Value((uint8_t)(x.getU16() ^ y.getU16()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getU16() && y.getU16()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getU16() || y.getU16()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getU16() == y.getU16()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getU16() != y.getU16()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getU16() < y.getU16()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getU16() > y.getU16()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getU16() <= y.getU16()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getU16() >= y.getU16()));
							break;
						case Opcode::CMP: {
							uint16_t lhs = x.getU16(), rhs = y.getU16();
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
					break;
				case ValueType::U32:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((uint32_t)(x.getU32() + y.getU32()));
							break;
						case Opcode::SUB:
							valueOut = Value((uint32_t)(x.getU32() - y.getU32()));
							break;
						case Opcode::MUL:
							valueOut = Value((uint32_t)(x.getU32() * y.getU32()));
							break;
						case Opcode::DIV:
							valueOut = Value((uint32_t)(x.getU32() / y.getU32()));
							break;
						case Opcode::MOD:
							valueOut = Value((uint32_t)(x.getU32() % y.getU32()));
							break;
						case Opcode::AND:
							valueOut = Value((uint32_t)(x.getU32() & y.getU32()));
							break;
						case Opcode::OR:
							valueOut = Value((uint32_t)(x.getU32() | y.getU32()));
							break;
						case Opcode::XOR:
							valueOut = Value((uint32_t)(x.getU32() ^ y.getU32()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getU32() && y.getU32()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getU32() || y.getU32()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getU32() == y.getU32()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getU32() != y.getU32()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getU32() < y.getU32()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getU32() > y.getU32()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getU32() <= y.getU32()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getU32() >= y.getU32()));
							break;
						case Opcode::CMP: {
							uint32_t lhs = x.getU32(), rhs = y.getU32();
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
					break;
				case ValueType::U64:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((uint64_t)(x.getU64() + y.getU64()));
							break;
						case Opcode::SUB:
							valueOut = Value((uint64_t)(x.getU64() - y.getU64()));
							break;
						case Opcode::MUL:
							valueOut = Value((uint64_t)(x.getU64() * y.getU64()));
							break;
						case Opcode::DIV:
							valueOut = Value((uint64_t)(x.getU64() / y.getU64()));
							break;
						case Opcode::MOD:
							valueOut = Value((uint64_t)(x.getU64() % y.getU64()));
							break;
						case Opcode::AND:
							valueOut = Value((uint64_t)(x.getU64() & y.getU64()));
							break;
						case Opcode::OR:
							valueOut = Value((uint64_t)(x.getU64() | y.getU64()));
							break;
						case Opcode::XOR:
							valueOut = Value((uint64_t)(x.getU64() ^ y.getU64()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getU64() && y.getU64()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getU64() || y.getU64()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getU64() == y.getU64()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getU64() != y.getU64()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getU64() < y.getU64()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getU64() > y.getU64()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getU64() <= y.getU64()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getU64() >= y.getU64()));
							break;
						case Opcode::CMP: {
							uint64_t lhs = x.getU64(), rhs = y.getU64();
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
					break;
				case ValueType::F32:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((float)(x.getF32() + y.getF32()));
							break;
						case Opcode::SUB:
							valueOut = Value((float)(x.getF32() - y.getF32()));
							break;
						case Opcode::MUL:
							valueOut = Value((float)(x.getF32() * y.getF32()));
							break;
						case Opcode::DIV:
							valueOut = Value((float)(x.getF32() / y.getF32()));
							break;
						case Opcode::MOD:
							valueOut = Value((float)flib::fmodf(x.getF32(), y.getF32()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getF32() && y.getF32()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getF32() || y.getF32()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getF32() == y.getF32()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getF32() != y.getF32()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getF32() < y.getF32()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getF32() > y.getF32()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getF32() <= y.getF32()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getF32() >= y.getF32()));
							break;
						case Opcode::CMP: {
							float lhs = x.getF32(), rhs = y.getF32();
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
					break;
				case ValueType::F64:
					switch (ins.opcode) {
						case Opcode::ADD:
							valueOut = Value((double)(x.getF64() + y.getF64()));
							break;
						case Opcode::SUB:
							valueOut = Value((double)(x.getF64() - y.getF64()));
							break;
						case Opcode::MUL:
							valueOut = Value((double)(x.getF64() * y.getF64()));
							break;
						case Opcode::DIV:
							valueOut = Value((double)(x.getF64() / y.getF64()));
							break;
						case Opcode::MOD:
							valueOut = Value((double)flib::fmod(x.getF64(), y.getF64()));
							break;
						case Opcode::LAND:
							valueOut = Value((bool)(x.getF64() && y.getF64()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getF64() || y.getF64()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getF64() == y.getF64()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getF64() != y.getF64()));
							break;
						case Opcode::LT:
							valueOut = Value((bool)(x.getF64() < y.getF64()));
							break;
						case Opcode::GT:
							valueOut = Value((bool)(x.getF64() > y.getF64()));
							break;
						case Opcode::LTEQ:
							valueOut = Value((bool)(x.getF64() <= y.getF64()));
							break;
						case Opcode::GTEQ:
							valueOut = Value((bool)(x.getF64() >= y.getF64()));
							break;
						case Opcode::CMP: {
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
					break;
				case ValueType::Bool:
					switch (ins.opcode) {
						case Opcode::LAND:
							valueOut = Value((bool)(x.getBool() && y.getBool()));
							break;
						case Opcode::LOR:
							valueOut = Value((bool)(x.getBool() || y.getBool()));
							break;
						case Opcode::EQ:
							valueOut = Value((bool)(x.getBool() == y.getBool()));
							break;
						case Opcode::NEQ:
							valueOut = Value((bool)(x.getBool() != y.getBool()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				default:
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LSH:
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x,
				y,
				valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], y));

			switch (x.valueType) {
				case ValueType::I8:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((int8_t)(x.getI8() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((int8_t)(x.getI8() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::I16:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((int16_t)(x.getI16() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((int16_t)(x.getI16() >> y.getU32()));
							break;
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::I32:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((int32_t)(x.getI32() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((int32_t)(x.getI32() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::I64:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((int64_t)(x.getI64() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((int64_t)(x.getI64() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U8:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((uint8_t)(x.getU8() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((uint8_t)(x.getU8() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U16:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((uint8_t)(x.getU16() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((uint8_t)(x.getU16() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U32:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((uint32_t)(x.getU32() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((uint32_t)(x.getU32() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U64:
					switch (ins.opcode) {
						case Opcode::LSH:
							valueOut = Value((uint64_t)(x.getU64() << y.getU32()));
							break;
						case Opcode::RSH:
							valueOut = Value((uint64_t)(x.getU64() >> y.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::F32:
				case ValueType::F64:
				case ValueType::Bool:
					return InvalidOperandsError::alloc(this);
				default:
					return InvalidOperandsError::alloc(this);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::NOT:
		case Opcode::LNOT:
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			Value x(ins.operands[1]), valueOut;

			switch (x.valueType) {
				case ValueType::I8:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((int8_t)(~x.getI8()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getI8()));
							break;
						case Opcode::NEG:
							valueOut = Value((int8_t)(-x.getI8()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::I16:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((int16_t)(~x.getI16()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getI16()));
							break;
						case Opcode::NEG:
							valueOut = Value((int16_t)(-x.getI16()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::I32:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((int32_t)(~x.getI32()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getI32()));
							break;
						case Opcode::NEG:
							valueOut = Value((int32_t)(-x.getI32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::I64:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((int64_t)(~x.getI64()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getI64()));
							break;
						case Opcode::NEG:
							valueOut = Value((int64_t)(-x.getI64()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U8:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((uint8_t)(~x.getU8()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getI8()));
							break;
						case Opcode::NEG:
							valueOut = Value((uint8_t)(x.getU8()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U16:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((uint16_t)(~x.getU16()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getU16()));
							break;
						case Opcode::NEG:
							valueOut = Value((uint16_t)(x.getU16()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U32:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((uint32_t)(~x.getU32()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getU32()));
							break;
						case Opcode::NEG:
							valueOut = Value((uint32_t)(x.getU32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::U64:
					switch (ins.opcode) {
						case Opcode::NOT:
							valueOut = Value((uint64_t)(~x.getU64()));
							break;
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getU64()));
							break;
						case Opcode::NEG:
							valueOut = Value((uint64_t)(x.getU64()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::F32:
					switch (ins.opcode) {
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getF32()));
							break;
						case Opcode::NEG:
							valueOut = Value((float)(-x.getF32()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::F64:
					switch (ins.opcode) {
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getF64()));
							break;
						case Opcode::NEG:
							valueOut = Value((double)(-x.getF64()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				case ValueType::Bool:
					switch (ins.opcode) {
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getU64()));
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
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
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, arrayValue, ValueType::ObjectRef));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, index, ValueType::U32));

			auto arrayIn = arrayValue.getObjectRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, arrayIn, ObjectRefKind::InstanceRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, arrayIn.asInstance.instanceObject, ObjectKind::Array));
			ArrayObject *arrayObject = (ArrayObject*)arrayIn.asInstance.instanceObject;

			uint32_t indexIn = index.getU32();

			if (indexIn > arrayObject->length) {
				return InvalidArrayIndexError::alloc(this, indexIn);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this,
															curMajorFrame,
															ins.output.getRegIndex(),
															Value(ObjectRef::makeArrayElementRef(arrayObject, indexIn))));

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
			curMajorFrame->nextArgStack.pushBack(std::move(value));
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
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::ObjectRef));
					const ObjectRef &fnObjectRef = fnValue.getObjectRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::InstanceRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asInstance.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asInstance.instanceObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[1], thisObjectValue));
					const ObjectRef &thisObjectRef = thisObjectValue.getObjectRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, thisObjectValue, ValueType::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, thisObjectRef, ObjectRefKind::InstanceRef));
					thisObject = thisObjectRef.asInstance.instanceObject;
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, fnValue, ValueType::ObjectRef));
					const ObjectRef &fnObjectRef = fnValue.getObjectRef();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType(this, fnObjectRef, ObjectRefKind::InstanceRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(this, fnObjectRef.asInstance.instanceObject, ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnObjectRef.asInstance.instanceObject;
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
			context->_context.majorFrames.pop_back();

			if (returnValueOutReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrames.back().get(), returnValueOutReg, returnValue));
			}
			return {};
		}
		case Opcode::YIELD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 1));

			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
			Value returnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, curMajorFrame, ins.operands[0], returnValue));

			if (returnValueOutReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrames.back().get(), returnValueOutReg, returnValue));
			}
			context->_context.flags |= CTX_YIELDED;
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(this, ins, false, 0));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(this, ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), ObjectRef::makeInstanceRef(curMajorFrame->thisObject)));
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
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), ObjectRef::makeInstanceRef(instance.get())));
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

			for (size_t i = context->_context.majorFrames.size(); i; --i) {
				auto &majorFrame = context->_context.majorFrames[i - 1];

				for (size_t j = majorFrame->minorFrames.size(); j; --j) {
					auto &minorFrame = majorFrame->minorFrames.at(j - 1);

					if (uint32_t off = _findAndDispatchExceptHandler(majorFrame->curExcept, minorFrame);
						off != UINT32_MAX) {
						context->_context.majorFrames.resize(i);
						majorFrame->minorFrames.resize(j);
						// Do not increase the current instruction offset,
						// the offset has been set to offset to first instruction
						// of the exception handler.
						majorFrame->curIns = off;
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

			curMajorFrame->minorFrames.back().exceptHandlers.pushBack(std::move(xh));
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
				case TypeId::Value: {
					switch (t.getValueTypeExData()) {
						case ValueType::I8:
							v = _castToLiteralValue<int8_t>(v);
							break;
						case ValueType::I16:
							v = _castToLiteralValue<int16_t>(v);
							break;
						case ValueType::I32:
							v = _castToLiteralValue<int32_t>(v);
							break;
						case ValueType::I64:
							v = _castToLiteralValue<int64_t>(v);
							break;
						case ValueType::U8:
							v = _castToLiteralValue<uint8_t>(v);
							break;
						case ValueType::U16:
							v = _castToLiteralValue<uint16_t>(v);
							break;
						case ValueType::U32:
							v = _castToLiteralValue<uint32_t>(v);
							break;
						case ValueType::U64:
							v = _castToLiteralValue<uint64_t>(v);
							break;
						case ValueType::Bool:
							v = _castToLiteralValue<bool>(v);
							break;
						case ValueType::F32:
							v = _castToLiteralValue<float>(v);
							break;
						case ValueType::F64:
							v = _castToLiteralValue<double>(v);
							break;
						default:
							return InvalidOperandsError::alloc(this);
					}
					break;
				}
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

			bool isExecutingDestructor = destructingThreads.count(currentThreadHandle());
			while (!interruptExecution) {
				curMajorFrame = context->getContext().majorFrames.back().get();
				curFn = curMajorFrame->curFn;

				// TODO: Check if the yield request is from the top level.
				if (context->getContext().flags & CTX_YIELDED)
					return {};

				// Pause if the runtime is in GC
				while ((_flags & _RT_INGC) && !isExecutingDestructor)
					yieldCurrentThread();

				// Interrupt execution if the thread is explicitly specified to be killed.
				if (managedThread->status == ThreadStatus::Dead) {
					return {};
				}

				switch (curFn->overloadingKind) {
					case FnOverloadingKind::Regular: {
						RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

						if (curMajorFrame->curIns == UINT32_MAX) {
							managedThreads.erase(currentThreadHandle());
							interruptExecution = true;
						} else {
							if (curMajorFrame->curIns >=
								ol->instructions.size()) {
								// Raise out of fn body error.
							}

							if ((globalHeapPoolAlloc.szAllocated > _szComputedGcLimit)) {
								gc();
							}

							const Instruction &ins = ol->instructions.at(curMajorFrame->curIns);

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, ins));
						}

						break;
					}
					case FnOverloadingKind::Native: {
						NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)curFn;

						Value returnValue = ol->callback(
							&context->getContext(),
							curMajorFrame);
						uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
						context->_context.majorFrames.pop_back();
						if (returnValueOutReg != UINT32_MAX) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrames.back().get(), returnValueOutReg, returnValue));
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

			bool isExecutingDestructor = destructingThreads.count(currentThreadHandle());
			while (!interruptExecution) {
				curMajorFrame = context->getContext().majorFrames.back().get();
				curFn = curMajorFrame->curFn;

				// TODO: Check if the yield request is from the top level.
				if (context->getContext().flags & CTX_YIELDED)
					return {};

				// Pause if the runtime is in GC
				while ((_flags & _RT_INGC) && !isExecutingDestructor)
					yieldCurrentThread();

				// Interrupt execution if the thread is explicitly specified to be killed.
				if (managedThread->status == ThreadStatus::Dead) {
					return {};
				}

				switch (curFn->overloadingKind) {
					case FnOverloadingKind::Regular: {
						RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

						if (curMajorFrame->curIns == UINT32_MAX)
							interruptExecution = true;
						else {
							if (curMajorFrame->curIns >=
								ol->instructions.size()) {
								// Raise out of fn body error.
							}

							if ((globalHeapPoolAlloc.szAllocated > _szComputedGcLimit)) {
								gc();
							}

							const Instruction &ins = ol->instructions.at(curMajorFrame->curIns);

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, ins));
						}

						break;
					}
					case FnOverloadingKind::Native: {
						NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)curFn;

						Value returnValue = ol->callback(
							&context->getContext(),
							curMajorFrame);
						uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
						context->_context.majorFrames.pop_back();
						if (returnValueOutReg != UINT32_MAX) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.majorFrames.back().get(), returnValueOutReg, returnValue));
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

		{
			auto frame = std::make_unique<MajorFrame>(this, &context->getContext());
			frame->curFn = overloading;
			frame->curIns = UINT32_MAX;
			frame->resizeRegs(1);
			context->getContext().majorFrames.push_back(std::move(frame));
		}

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

		{
			auto frame = std::make_unique<MajorFrame>(this, &context->getContext());
			frame->curFn = overloading;
			frame->curIns = UINT32_MAX;
			frame->resizeRegs(1);
			context->getContext().majorFrames.push_back(std::move(frame));
		}

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
