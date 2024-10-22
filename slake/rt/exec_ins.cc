#include "../runtime.h"
#include <cmath>

using namespace slake;

static [[nodiscard]] bool _checkOperandCount(
	Runtime *rt,
	const Instruction &ins,
	bool hasOutput,
	int_fast8_t nOperands) {
	if (hasOutput) {
		if (ins.output.valueType == ValueType::Undefined) {
			rt->setThreadLocalInternalException(
				std::this_thread::get_id(),
				InvalidOperandsError::alloc(rt));
			return false;
		}
	}
	if (ins.operands.size() != nOperands) {
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidOperandsError::alloc(rt));
		return false;
	}

	return true;
}

static [[nodiscard]] bool _checkOperandType(
	Runtime *rt,
	const Value &operand,
	ValueType valueType) {
	if (operand.valueType != valueType) {
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidOperandsError::alloc(rt));
		return false;
	}
	return true;
}

static [[nodiscard]] bool _checkObjectOperandType(
	Runtime *rt,
	Object *object,
	ObjectKind typeId) {
	if (object->getKind() != typeId) {
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidOperandsError::alloc(rt));
		return false;
	}
	return true;
}

static [[nodiscard]] bool _setRegisterValue(
	Runtime *rt,
	MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) {
	if (index >= curMajorFrame->regs.size()) {
		// The register does not present.
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidOperandsError::alloc(rt));
		return false;
	}
	Value &reg = curMajorFrame->regs[index];
	if (reg.valueType != ValueType::Undefined) {
		// The register is already assigned.
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidOperandsError::alloc(rt));
		return false;
	}
	curMajorFrame->regs[index] = value;
	return true;
}

static [[nodiscard]] bool _fetchRegValue(
	Runtime *rt,
	MajorFrame *curMajorFrame,
	uint32_t index,
	Value &valueOut) {
	if (index >= curMajorFrame->regs.size()) {
		// The register does not present.
		rt->setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidOperandsError::alloc(rt));
		return false;
	}
	valueOut = curMajorFrame->regs[index];
	return true;
}

static bool _unwrapRegOperand(
	Runtime *rt,
	MajorFrame *curMajorFrame,
	const Value &value,
	Value &valueOut) {
	if (value.valueType == ValueType::RegRef)
		return _fetchRegValue(rt, curMajorFrame, value.getRegIndex(), valueOut);
	valueOut = value;
	return true;
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

SLAKE_API bool slake::Runtime::_createNewMajorFrame(
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
		setThreadLocalInternalException(
			std::this_thread::get_id(),
			InvalidArgumentNumberError::alloc(this, nArgs));
		return false;
	}

	newMajorFrame->argStack.resize(fn->paramTypes.size());
	for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
		auto varObject = RegularVarObject::alloc(this, ACCESS_PUB, fn->paramTypes[i]);
		holder.addObject(varObject.get());
		newMajorFrame->argStack[i] = varObject.get();
		bool result = varObject->setData({}, args[i]);
		assert(result);
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

		bool result = varArgEntryVarObject->setData({}, Value(varArgArrayObject.get()));
		assert(result);
	}

	context->majorFrames.push_back(std::move(newMajorFrame));
	return true;
}

//
// TODO: Check if the stackAlloc() was successful.
//
SLAKE_API bool slake::Runtime::_addLocalVar(MajorFrame *frame, Type type, VarRef &varRefOut) {
	LocalVarRecord localVarRecord;

	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int8_t)))
						return false;
					break;
				case ValueType::I16:
					if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int16_t)))
						return false;
					break;
				case ValueType::I32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int32_t)))
						return false;
					break;
				case ValueType::I64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(int64_t)))
						return false;
					break;
				case ValueType::U8:
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint8_t)))
						return false;
					break;
				case ValueType::U16:
					if (!frame->context->stackAlloc((2 - (frame->context->stackTop & 1))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint16_t)))
						return false;
					break;
				case ValueType::U32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint32_t)))
						return false;
					break;
				case ValueType::U64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(uint64_t)))
						return false;
					break;
				case ValueType::F32:
					if (!frame->context->stackAlloc((4 - (frame->context->stackTop & 3))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(float)))
						return false;
					break;
				case ValueType::F64:
					if (!frame->context->stackAlloc((8 - (frame->context->stackTop & 7))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(double)))
						return false;
					break;
				case ValueType::Bool:
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(bool)))
						return false;
					break;
				case ValueType::ObjectRef:
					if (!frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1))))
						return false;
					localVarRecord.stackOffset = frame->context->stackTop;
					if (!frame->context->stackAlloc(sizeof(Object *)))
						return false;
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			if (!frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1))))
				return false;
			localVarRecord.stackOffset = frame->context->stackTop;
			if (!frame->context->stackAlloc(sizeof(Object *)))
				return false;
			break;
		default:
			throw std::runtime_error("The variable has an inconstructible type");
	}

	localVarRecord.type = type;

	size_t index = frame->localVarRecords.size();
	frame->localVarRecords.push_back(std::move(localVarRecord));
	varRefOut = VarRef(frame->localVarAccessor, VarRefContext::makeLocalVarContext(index));
	return true;
}

SLAKE_API void slake::Runtime::_addLocalReg(MajorFrame *frame) {
	frame->regs.push_back({});
}

SLAKE_API bool slake::Runtime::_execIns(ContextObject *context, const Instruction &ins) {
	if (((globalHeapPoolResource.szAllocated >> 1) > _szMemUsedAfterLastGc)) {
		gc();
	}

	MajorFrame *curMajorFrame = context->_context.majorFrames.back().get();
	auto &curMinorFrame = curMajorFrame->minorFrames.back();

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			if (!_checkOperandCount(this, ins, false, 1))
				return false;
			if (!_checkOperandType(this, ins.operands[0], ValueType::TypeName))
				return false;

			Type type = ins.operands[0].getTypeName();
			if (!type.loadDeferredType(this))
				return false;

			VarRef varRef;
			if (!_addLocalVar(curMajorFrame, type, varRef))
				return false;
			break;
		}
		case Opcode::REG: {
			if (!_checkOperandCount(this, ins, false, 1))
				return false;
			if (!_checkOperandType(this, ins.operands[0], ValueType::U32))
				return false;

			uint32_t index = ins.operands[0].getU32();
			if (index > curMajorFrame->regs.size()) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					InvalidOperandsError::alloc(this));
				return false;
			} else if (index < curMajorFrame->regs.size()) {
				curMajorFrame->regs[index] = Value();
			} else {
				_addLocalReg(curMajorFrame);
			}
			break;
		}
		case Opcode::LOAD: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::ObjectRef))
				return false;
			auto refPtr = ins.operands[0].getObjectRef();
			if (!_checkObjectOperandType(this, refPtr, ObjectKind::IdRef))
				return false;

			VarRefContext varRefContext;

			auto v = resolveIdRef((IdRefObject *)refPtr, &varRefContext, curMajorFrame->thisObject);
			if (!v) {
				if (!(v = resolveIdRef((IdRefObject *)refPtr, &varRefContext, curMajorFrame->scopeObject)))
					v = resolveIdRef((IdRefObject *)refPtr, &varRefContext);
			}

			if (!v) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					ReferencedMemberNotFoundError::alloc(this, (IdRefObject *)refPtr));
				return false;
			}

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), _wrapObjectIntoValue(v, varRefContext)))
				return false;
			break;
		}
		case Opcode::RLOAD: {
			if (!_checkOperandCount(this, ins, true, 2))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::RegRef))
				return false;

			Value lhs;
			if (!_fetchRegValue(this, curMajorFrame, ins.operands[0].getRegIndex(), lhs))
				return false;
			if (!_checkOperandType(this, lhs, ValueType::ObjectRef))
				return false;

			auto lhsPtr = lhs.getObjectRef();

			if (!_checkOperandType(this, ins.operands[1], ValueType::ObjectRef))
				return false;
			auto refPtr = ins.operands[1].getObjectRef();

			if (!lhsPtr) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					NullRefError::alloc(this));
				return false;
			}

			VarRefContext varRefContext;

			if (!(lhsPtr = resolveIdRef((IdRefObject *)refPtr, &varRefContext, lhsPtr))) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					ReferencedMemberNotFoundError::alloc(this, (IdRefObject *)refPtr));
			}

			if (!_setRegisterValue(
					this,
					curMajorFrame,
					ins.output.getRegIndex(),
					_wrapObjectIntoValue(lhsPtr, varRefContext)))
				return false;
			break;
		}
		case Opcode::STORE: {
			if (!_checkOperandCount(this, ins, false, 2))
				return false;

			Value destValue;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], destValue))
				return false;
			if (!_checkOperandType(this, destValue, ValueType::VarRef))
				return false;

			const VarRef varRef = destValue.getVarRef();

			if (!varRef.varPtr) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					NullRefError::alloc(this));
			}

			Value data;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], data))
				return false;
			if (!varRef.varPtr->setData(
					varRef.context,
					data))
				return false;
			break;
		}
		case Opcode::MOV: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			Value value;
			if (!_unwrapRegOperand(
					this,
					curMajorFrame,
					ins.operands[0],
					value))
				return false;
			if (!_setRegisterValue(
					this,
					curMajorFrame,
					ins.output.getRegIndex(),
					value))
				return false;
			break;
		}
		case Opcode::LLOAD: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;
			if (!_checkOperandType(this, ins.operands[0], ValueType::U32))
				return false;

			VarRef varRef;
			if (!curMajorFrame->lload(this, ins.operands[0].getU32(), varRef))
				return false;

			_setRegisterValue(
				this,
				curMajorFrame,
				ins.output.getRegIndex(),
				Value(varRef));
			break;
		}
		case Opcode::LARG: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;
			if (!_checkOperandType(this, ins.operands[0], ValueType::U32))
				return false;

			VarRef varRef;
			if (!curMajorFrame->larg(this, ins.operands[0].getU32(), varRef))
				return false;
			if (!_setRegisterValue(
					this,
					curMajorFrame,
					ins.output.getRegIndex(),
					Value(varRef)))
				return false;
			break;
		}
		case Opcode::LVALUE: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			Value dest;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], dest))
				return false;
			if (!_checkOperandType(this, dest, ValueType::VarRef))
				return false;

			const VarRef varRef = dest.getVarRef();

			if (!varRef.varPtr) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					NullRefError::alloc(this));
				return false;
			}

			Optional result = ((VarObject *)varRef.varPtr)->getData(varRef.context);
			if (!result)
				return false;
			if (!_setRegisterValue(
					this,
					curMajorFrame,
					ins.output.getRegIndex(),
					result.unwrap()))
				return false;
			break;
		}
		case Opcode::ENTER: {
			if (!_checkOperandCount(this, ins, false, 0))
				return false;
			MinorFrame frame(
				this,
				(uint32_t)curMajorFrame->localVarRecords.size(),
				(uint32_t)curMajorFrame->regs.size(),
				context->_context.stackTop);

			curMajorFrame->minorFrames.push_back(frame);
			break;
		}
		case Opcode::LEAVE: {
			if (!_checkOperandCount(this, ins, false, 0))
				return false;
			if (curMajorFrame->minorFrames.size() < 2) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					FrameBoundaryExceededError::alloc(this));
				return false;
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
			if (!_checkOperandCount(this, ins, true, 2))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			Value x, y, valueOut;

			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], x))
				return false;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], y))
				return false;
			if (x.valueType != y.valueType) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					InvalidOperandsError::alloc(this));
				return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
					}
					break;
				default:
					setThreadLocalInternalException(
						std::this_thread::get_id(),
						InvalidOperandsError::alloc(this));
					return false;
			}

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut))
				return false;
			break;
		}
		case Opcode::LSH:
		case Opcode::RSH: {
			if (!_checkOperandCount(this, ins, true, 2))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			Value x,
				y,
				valueOut;

			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], x))
				return false;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], y))
				return false;

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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							break;
						default:
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
					}
					break;
				case ValueType::F32:
				case ValueType::F64:
				case ValueType::Bool:
					setThreadLocalInternalException(
						std::this_thread::get_id(),
						InvalidOperandsError::alloc(this));
					return false;
				default:
					setThreadLocalInternalException(
						std::this_thread::get_id(),
						InvalidOperandsError::alloc(this));
					return false;
			}

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut))
				return false;
			break;
		}
		case Opcode::NOT:
		case Opcode::LNOT:
		case Opcode::NEG: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
					}
					break;
				case ValueType::Bool:
					switch (ins.opcode) {
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getU64()));
							break;
						default:
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
					}
					break;
				default:
					setThreadLocalInternalException(
						std::this_thread::get_id(),
						InvalidOperandsError::alloc(this));
					return false;
			}

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), valueOut))
				return false;
			break;
		}
		case Opcode::AT: {
			if (!_checkOperandCount(this, ins, true, 2))
				return false;

			Value arrayValue;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], arrayValue))
				return false;
			if (!_checkOperandType(this, arrayValue, ValueType::ObjectRef))
				return false;
			Value index;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], index))
				return false;
			if (!_checkOperandType(this, index, ValueType::U32))
				return false;

			auto arrayIn = arrayValue.getObjectRef();
			if (!_checkObjectOperandType(this, arrayIn, ObjectKind::Array))
				return false;

			uint32_t indexIn = index.getU32();

			if (indexIn > ((ArrayObject *)arrayIn)->length) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					InvalidArrayIndexError::alloc(this, indexIn));
				return false;
			}

			if (!_setRegisterValue(
					this,
					curMajorFrame,
					ins.output.getRegIndex(),
					Value(VarRef(
						((ArrayObject *)arrayIn)->accessor,
						VarRefContext::makeArrayContext(indexIn)))))
				return false;

			break;
		}
		case Opcode::JMP: {
			if (!_checkOperandCount(this, ins, false, 1))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::U32))
				return false;

			curMajorFrame->curIns = ins.operands[0].getU32();
			return true;
		}
		case Opcode::JT:
		case Opcode::JF: {
			if (!_checkOperandCount(this, ins, false, 2))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::U32))
				return false;
			Value condition;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], condition))
				return false;
			if (!_checkOperandType(this, condition, ValueType::Bool))
				return false;

			if (condition.getBool()) {
				if (ins.opcode == Opcode::JT) {
					curMajorFrame->curIns = ins.operands[0].getU32();
					return true;
				}
			} else if (ins.opcode == Opcode::JF) {
				curMajorFrame->curIns = ins.operands[0].getU32();
				return true;
			}

			break;
		}
		case Opcode::PUSHARG: {
			if (!_checkOperandCount(this, ins, false, 2))
				return false;

			Value value;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], value))
				return false;
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
					if (!_checkOperandCount(this, ins, false, 2))
						return false;

					Value fnValue;
					if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], fnValue))
						return false;
					if (!_checkOperandType(this, fnValue, ValueType::ObjectRef))
						return false;
					if (!_checkObjectOperandType(this, fnValue.getObjectRef(), ObjectKind::FnOverloading))
						return false;
					fn = (FnOverloadingObject *)fnValue.getObjectRef();

					Value thisObjectValue;
					if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], thisObjectValue))
						return false;
					if (!_checkOperandType(this, thisObjectValue, ValueType::ObjectRef))
						return false;
					thisObject = thisObjectValue.getObjectRef();
					break;
				}
				case Opcode::CALL: {
					if (!_checkOperandCount(this, ins, false, 1))
						return false;

					Value fnValue;
					if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], fnValue))
						return false;
					if (!_checkOperandType(this, fnValue, ValueType::ObjectRef))
						return false;
					if (!_checkObjectOperandType(this, fnValue.getObjectRef(), ObjectKind::FnOverloading))
						return false;
					fn = (FnOverloadingObject *)fnValue.getObjectRef();
					break;
				}
				default:
					assert(false);
			}

			if (!fn) {
				setThreadLocalInternalException(
					std::this_thread::get_id(),
					NullRefError::alloc(this));
				return false;
			}

			if (!_createNewMajorFrame(
					&context->_context,
					thisObject,
					fn,
					curMajorFrame->nextArgStack.data(),
					curMajorFrame->nextArgStack.size()))
				return false;
			curMajorFrame->nextArgStack.clear();

			break;
		}
		case Opcode::RET: {
			if (!_checkOperandCount(this, ins, false, 1))
				return false;

			Value returnValue;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], returnValue))
				return false;
			context->_context.majorFrames.pop_back();
			context->_context.majorFrames.back()->returnValue = returnValue;
			return true;
		}
		case Opcode::LRET: {
			if (!_checkOperandCount(this, ins, false, 0))
				return false;
			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), curMajorFrame->returnValue))
				return false;
			break;
		}
		case Opcode::ACALL:
		case Opcode::AMCALL: {
			break;
		}
		case Opcode::YIELD: {
			if (!_checkOperandCount(this, ins, false, 1))
				return false;

			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], curMajorFrame->returnValue))
				return false;
			context->_context.flags |= CTX_YIELDED;
			break;
		}
		case Opcode::AWAIT: {
			break;
		}
		case Opcode::LTHIS: {
			if (!_checkOperandCount(this, ins, false, 0))
				return false;
			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), curMajorFrame->thisObject))
				return false;
			break;
		}
		case Opcode::NEW: {
			if (!_checkOperandCount(this, ins, true, 1))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::TypeName))
				return false;

			Type type = ins.operands[0].getTypeName();
			if (!type.loadDeferredType(this))
				return false;

			switch (type.typeId) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)type.getCustomTypeExData();
					HostObjectRef<InstanceObject> instance = newClassInstance(cls, 0);
					if (!instance)
						return false;
					if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), instance.get()))
						return false;
					break;
				}
				default:
					setThreadLocalInternalException(
						std::this_thread::get_id(),
						InvalidOperandsError::alloc(this));
					return false;
			}
			break;
		}
		case Opcode::ARRNEW: {
			if (!_checkOperandCount(this, ins, true, 2))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::TypeName))
				return false;
			if (!_checkOperandType(this, ins.operands[1], ValueType::U32))
				return false;

			Type type = ins.operands[1].getTypeName();
			uint32_t size = ins.operands[2].getU32();
			if (!type.loadDeferredType(this))
				return false;

			auto instance = newArrayInstance(this, type, size);
			if (!instance)
				return false;

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), instance.get()))
				return false;

			break;
		}
		case Opcode::THROW: {
			if (!_checkOperandCount(this, ins, false, 1))
				return false;

			Value x;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[0], x))
				return false;

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
						return true;
					}
				}
			}

			curMajorFrame->curExcept = x;
			setThreadLocalInternalException(
				std::this_thread::get_id(),
				UncaughtExceptionError::alloc(this, x));
			return false;
		}
		case Opcode::PUSHXH: {
			if (!_checkOperandCount(this, ins, false, 2))
				return false;

			if (!_checkOperandType(this, ins.operands[0], ValueType::TypeName))
				return false;
			if (!_checkOperandType(this, ins.operands[1], ValueType::U32))
				return false;

			ExceptionHandler xh;

			Type type = ins.operands[0].getTypeName();
			if (!type.loadDeferredType(this))
				return false;

			xh.type = ins.operands[0].getTypeName();
			xh.off = ins.operands[1].getU32();

			curMajorFrame->minorFrames.back().exceptHandlers.push_back(xh);
			break;
		}
		case Opcode::CAST: {
			if (!_checkOperandCount(this, ins, true, 2))
				return false;

			if (!_checkOperandType(this, ins.output, ValueType::RegRef))
				return false;
			if (!_checkOperandType(this, ins.operands[0], ValueType::TypeName))
				return false;

			Value v;
			if (!_unwrapRegOperand(this, curMajorFrame, ins.operands[1], v))
				return false;

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
							setThreadLocalInternalException(
								std::this_thread::get_id(),
								InvalidOperandsError::alloc(this));
							return false;
					}
					break;
				}
				case TypeId::Instance:
					break;
				default:
					setThreadLocalInternalException(
						std::this_thread::get_id(),
						InvalidOperandsError::alloc(this));
					return false;
			}

			if (!_setRegisterValue(this, curMajorFrame, ins.output.getRegIndex(), v))
				return false;
			break;
		}
		default:
			setThreadLocalInternalException(
				std::this_thread::get_id(),
				InvalidOpcodeError::alloc(this, ins.opcode));
			return false;
	}
	++curMajorFrame->curIns;
	return true;
}
