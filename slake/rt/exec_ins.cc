#include "../runtime.h"
#include <cmath>

using namespace slake;

InternalExceptionPointer Runtime::_checkOperandCount(
	const Instruction &ins,
	bool hasOutput,
	int_fast8_t nOperands) {
	if (hasOutput) {
		if (ins.output.valueType == ValueType::Undefined) {
			return InvalidOperandsError::alloc(this);
		}
	}
	if (ins.operands.size() != nOperands) {
		return InvalidOperandsError::alloc(this);
	}

	return {};
}

InternalExceptionPointer Runtime::_checkOperandType(
	const Value &operand,
	ValueType valueType) {
	if (operand.valueType != valueType) {
		return InvalidOperandsError::alloc(this);
	}
	return {};
}

InternalExceptionPointer Runtime::_checkObjectOperandType(
	Object *object,
	ObjectKind typeId) {
	if (object->getKind() != typeId) {
		return InvalidOperandsError::alloc(this);
	}
	return {};
}

InternalExceptionPointer Runtime::_setRegisterValue(
	MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) {
	if (index >= curMajorFrame->regs.size()) {
		// The register does not present.
		return InvalidOperandsError::alloc(this);
	}
	Value &reg = curMajorFrame->regs[index];
	if (reg.valueType != ValueType::Undefined) {
		// The register is already assigned.
		return InvalidOperandsError::alloc(this);
	}
	curMajorFrame->regs[index] = value;
	return {};
}

InternalExceptionPointer Runtime::_fetchRegValue(
	MajorFrame *curMajorFrame,
	uint32_t index,
	Value &valueOut) {
	if (index >= curMajorFrame->regs.size()) {
		// The register does not present.
		return InvalidOperandsError::alloc(this);
	}
	valueOut = curMajorFrame->regs[index];
	return {};
}

InternalExceptionPointer Runtime::_unwrapRegOperand(
	MajorFrame *curMajorFrame,
	const Value &value,
	Value &valueOut) {
	if (value.valueType == ValueType::RegRef)
		return _fetchRegValue(curMajorFrame, value.getRegIndex(), valueOut);
	valueOut = value;
	return {};
}

static Value _wrapObjectIntoValue(Object *object, VarRefContext &varRefContext) {
	switch (object->getKind()) {
		case ObjectKind::Var:
			return Value(VarRef((VarObject *)object, varRefContext));
		default:
			return Value(object);
	}
}

template <typename LT>
static Value _castToLiteralValue(Value x) {
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
			throw std::logic_error("Invalid type conversion");
	}
}

SLAKE_API InternalExceptionPointer slake::Runtime::_createNewMajorFrame(
	Context *context,
	Object *thisObject,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs) {
	HostRefHolder holder;

	std::unique_ptr<MajorFrame> newMajorFrame = std::make_unique<MajorFrame>(this, context);

	newMajorFrame->curFn = fn;
	newMajorFrame->thisObject = thisObject;

	if (nArgs < fn->paramTypes.size()) {
		return InvalidArgumentNumberError::alloc(this, nArgs);
	}

	newMajorFrame->argStack.resize(fn->paramTypes.size());
	for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
		auto varObject = RegularVarObject::alloc(this, ACCESS_PUB, fn->paramTypes[i]);
		holder.addObject(varObject.get());
		newMajorFrame->argStack[i] = varObject.get();
		InternalExceptionPointer result = varObject->setData({}, args[i]);
		assert(!result);
	}

	if (fn->overloadingFlags & OL_VARG) {
		auto varArgTypeDefObject = TypeDefObject::alloc(this, Type(TypeId::Any));
		holder.addObject(varArgTypeDefObject.get());

		auto varArgEntryVarObject = RegularVarObject::alloc(this, ACCESS_PUB, Type(TypeId::Array, varArgTypeDefObject.release()));
		holder.addObject(varArgEntryVarObject.get());
		newMajorFrame->argStack.push_back(varArgEntryVarObject.get());

		size_t szVarArgArray = nArgs - fn->paramTypes.size();
		auto varArgArrayObject = AnyArrayObject::alloc(this, szVarArgArray);
		holder.addObject(varArgArrayObject.get());

		for (size_t i = 0; i < szVarArgArray; ++i) {
			varArgArrayObject->data[i] = args[fn->paramTypes.size() + i];
		}

		InternalExceptionPointer result = varArgEntryVarObject->setData({}, Value(varArgArrayObject.get()));
		assert(!result);
	}

	context->majorFrames.push_back(std::move(newMajorFrame));
	return {};
}

//
// TODO: Check if the stackAlloc() was successful.
//
SLAKE_API InternalExceptionPointer slake::Runtime::_addLocalVar(MajorFrame *frame, Type type, VarRef &varRefOut) {
	LocalVarRecord localVarRecord;

	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int8_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::I16:
					if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int16_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::I32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int32_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::I64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int64_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::U8:
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint8_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::U16:
					if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint16_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::U32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint32_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::U64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint64_t)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::F32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(float)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::F64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(double)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::Bool:
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(bool)))
						return StackOverflowError::alloc(this);
					break;
				case ValueType::ObjectRef:
					if (!frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1))))
						return StackOverflowError::alloc(this);
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(Object *)))
						return StackOverflowError::alloc(this);
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			if (!frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1))))
				return StackOverflowError::alloc(this);
			localVarRecord.stackOffset = frame->context->stackTop;
			if (!frame->context->stackAlloc(sizeof(Object *)))
				return StackOverflowError::alloc(this);
			break;
		default:
			throw std::runtime_error("The variable has an inconstructible type");
	}

	localVarRecord.type = type;

	size_t index = frame->localVarRecords.size();
	frame->localVarRecords.push_back(std::move(localVarRecord));
	varRefOut = VarRef(frame->localVarAccessor, VarRefContext::makeLocalVarContext(index));
	return {};
}

SLAKE_API void slake::Runtime::_addLocalReg(MajorFrame *frame) {
	frame->regs.push_back({});
}

SLAKE_API InternalExceptionPointer slake::Runtime::_execIns(ContextObject *context, const Instruction &ins) {
	InternalExceptionPointer exceptPtr;

	if (((globalHeapPoolResource.szAllocated >> 1) > _szMemUsedAfterLastGc)) {
		gc();
	}

	MajorFrame *curMajorFrame = context->_context.majorFrames.back().get();
	auto &curMinorFrame = curMajorFrame->minorFrames.back();

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::TypeName));

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			VarRef varRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(curMajorFrame, type, varRef));
			break;
		}
		case Opcode::REG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::U32));

			uint32_t index = ins.operands[0].getU32();
			if (index > curMajorFrame->regs.size()) {
				return InvalidOperandsError::alloc(this);
			} else if (index < curMajorFrame->regs.size()) {
				curMajorFrame->regs[index] = Value();
			} else {
				_addLocalReg(curMajorFrame);
			}
			break;
		}
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::ObjectRef));
			auto refPtr = ins.operands[0].getObjectRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(refPtr, ObjectKind::IdRef));

			VarRefContext varRefContext;

			Object *v;
			if (auto e = resolveIdRef((IdRefObject *)refPtr, &varRefContext, v, curMajorFrame->thisObject);
				e) {
				e.reset();
				if (e = resolveIdRef((IdRefObject *)refPtr, &varRefContext, v, curMajorFrame->scopeObject)) {
					e.reset();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr, &varRefContext, v));
				}
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), _wrapObjectIntoValue(v, varRefContext)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::RegRef));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _fetchRegValue(curMajorFrame, ins.operands[0].getRegIndex(), lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(lhs, ValueType::ObjectRef));

			auto lhsPtr = lhs.getObjectRef();

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[1], ValueType::ObjectRef));
			auto refPtr = ins.operands[1].getObjectRef();

			if (!lhsPtr) {
				return NullRefError::alloc(this);
			}

			VarRefContext varRefContext;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr, &varRefContext, lhsPtr, lhsPtr));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				_wrapObjectIntoValue(lhsPtr, varRefContext)));
			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 2));

			Value destValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], destValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(destValue, ValueType::VarRef));

			const VarRef varRef = destValue.getVarRef();

			if (!varRef.varPtr) {
				return NullRefError::alloc(this);
			}

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, varRef.varPtr->setData(
				varRef.context,
				data));
			break;
		}
		case Opcode::MOV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(
				curMajorFrame,
				ins.operands[0],
				value));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				value));
			break;
		}
		case Opcode::LLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::U32));

			VarRef varRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, curMajorFrame->lload(this, ins.operands[0].getU32(), varRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, 
				_setRegisterValue(
					curMajorFrame,
					ins.output.getRegIndex(),
					Value(varRef)));
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::U32));

			VarRef varRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, curMajorFrame->larg(this, ins.operands[0].getU32(), varRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				Value(varRef)));
			break;
		}
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

			Value dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(dest, ValueType::VarRef));

			const VarRef varRef = dest.getVarRef();

			if (!varRef.varPtr) {
				return NullRefError::alloc(this);
			}

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, ((VarObject *)varRef.varPtr)->getData(varRef.context, data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				data));
			break;
		}
		case Opcode::ENTER: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 0));
			MinorFrame frame(
				this,
				(uint32_t)curMajorFrame->localVarRecords.size(),
				(uint32_t)curMajorFrame->regs.size(),
				context->_context.stackTop);

			curMajorFrame->minorFrames.push_back(frame);
			break;
		}
		case Opcode::LEAVE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 0));
			if (curMajorFrame->minorFrames.size() < 2) {
				return FrameBoundaryExceededError::alloc(this);
			}
			curMajorFrame->leave();
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
		case Opcode::GTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

			Value x, y, valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], y));
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
							valueOut = Value((float)fmodf(x.getF32(), y.getF32()));
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
							valueOut = Value((double)fmod(x.getF64(), y.getF64()));
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

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::LSH:
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

			Value x,
				y,
				valueOut;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], y));

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

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::NOT:
		case Opcode::LNOT:
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

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

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), valueOut));
			break;
		}
		case Opcode::AT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 2));

			Value arrayValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], arrayValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(arrayValue, ValueType::ObjectRef));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(index, ValueType::U32));

			auto arrayIn = arrayValue.getObjectRef();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(arrayIn, ObjectKind::Array));

			uint32_t indexIn = index.getU32();

			if (indexIn > ((ArrayObject *)arrayIn)->length) {
				return InvalidArrayIndexError::alloc(this, indexIn);
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				Value(VarRef(
					((ArrayObject *)arrayIn)->accessor,
					VarRefContext::makeArrayContext(indexIn)))));

			break;
		}
		case Opcode::JMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::U32));

			curMajorFrame->curIns = ins.operands[0].getU32();
			return {};
		}
		case Opcode::JT:
		case Opcode::JF: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 2));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::U32));
			Value condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(condition, ValueType::Bool));

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
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 2));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], value));
			curMajorFrame->nextArgStack.push_back(value);
			break;
		}
		case Opcode::CTORCALL:
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;

			switch (ins.opcode) {
				case Opcode::CTORCALL:
				case Opcode::MCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 2));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(fnValue, ValueType::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(fnValue.getObjectRef(), ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnValue.getObjectRef();

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], thisObjectValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(thisObjectValue, ValueType::ObjectRef));
					thisObject = thisObjectValue.getObjectRef();
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(fnValue, ValueType::ObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType(fnValue.getObjectRef(), ObjectKind::FnOverloading));
					fn = (FnOverloadingObject *)fnValue.getObjectRef();
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
				curMajorFrame->nextArgStack.size()));
			curMajorFrame->nextArgStack.clear();

			break;
		}
		case Opcode::RET: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));

			Value returnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], returnValue));
			context->_context.majorFrames.pop_back();
			context->_context.majorFrames.back()->returnValue = returnValue;
			return {};
		}
		case Opcode::LRET: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 0));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), curMajorFrame->returnValue));
			break;
		}
		case Opcode::ACALL:
		case Opcode::AMCALL: {
			break;
		}
		case Opcode::YIELD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], curMajorFrame->returnValue));
			context->_context.flags |= CTX_YIELDED;
			break;
		}
		case Opcode::AWAIT: {
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 0));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), curMajorFrame->thisObject));
			break;
		}
		case Opcode::NEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 1));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::TypeName));

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			switch (type.typeId) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)type.getCustomTypeExData();
					HostObjectRef<InstanceObject> instance = newClassInstance(cls, 0);
					if (!instance)
						// TODO: Return more detail exceptions.
						return InvalidOperandsError::alloc(this);
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), instance.get()));
					break;
				}
				default:
					return InvalidOperandsError::alloc(this);
			}
			break;
		}
		case Opcode::ARRNEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::TypeName));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[1], ValueType::U32));

			Type type = ins.operands[1].getTypeName();
			uint32_t size = ins.operands[2].getU32();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			auto instance = newArrayInstance(this, type, size);
			if (!instance)
				// TODO: Return more detail exceptions.
				return InvalidOperandsError::alloc(this);

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), instance.get()));

			break;
		}
		case Opcode::THROW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 1));

			Value x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[0], x));

			for (size_t i = context->_context.majorFrames.size(); i; --i) {
				auto &majorFrame = context->_context.majorFrames[i - 1];

				for (size_t j = majorFrame->minorFrames.size(); j; --j) {
					auto &minorFrame = majorFrame->minorFrames[j - 1];

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
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, false, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::TypeName));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[1], ValueType::U32));

			ExceptionHandler xh;

			Type type = ins.operands[0].getTypeName();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, type.loadDeferredType(this));

			xh.type = ins.operands[0].getTypeName();
			xh.off = ins.operands[1].getU32();

			curMajorFrame->minorFrames.back().exceptHandlers.push_back(xh);
			break;
		}
		case Opcode::CAST: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount(ins, true, 2));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.output, ValueType::RegRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType(ins.operands[0], ValueType::TypeName));

			Value v;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(curMajorFrame, ins.operands[1], v));

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

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(curMajorFrame, ins.output.getRegIndex(), v));
			break;
		}
		default:
			return InvalidOpcodeError::alloc(this, ins.opcode);
	}
	++curMajorFrame->curIns;
	return {};
}