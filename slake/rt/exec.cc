#include "../runtime.h"
#include <cmath>

using namespace slake;

static void _checkOperandCount(
	const Instruction &ins,
	bool hasOutput,
	int_fast8_t nOperands) {
	if (hasOutput) {
		if (ins.output.valueType == ValueType::Undefined)
			throw InvalidOperandsError("Invalid operand combination");
	}
	if (ins.operands.size() != nOperands)
		throw InvalidOperandsError("Invalid operand combination");
}

static void _checkOperandType(
	const Value &operand,
	ValueType valueType) {
	if (operand.valueType != valueType)
		throw InvalidOperandsError("Invalid operand combination");
}

static void _checkObjectOperandType(
	Object *object,
	ObjectKind typeId) {
	if (object->getKind() != typeId)
		throw InvalidOperandsError("Invalid operand combination");
}

static void _setRegisterValue(
	MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) {
	if (index >= curMajorFrame->regs.size())
		throw InvalidRegisterIndexError("Invalid register index", index);
	Value &reg = curMajorFrame->regs[index];
	if (reg.valueType != ValueType::Undefined)
		throw AccessViolationError("The register is already assigned");
	curMajorFrame->regs[index] = value;
}

static Value &_fetchRegValue(
	MajorFrame *curMajorFrame,
	uint32_t index) {
	if (index >= curMajorFrame->regs.size())
		throw InvalidRegisterIndexError("Invalid register index", index);
	return curMajorFrame->regs[index];
}

static Value &_unwrapRegOperand(
	MajorFrame *curMajorFrame,
	Value &value) {
	if (value.valueType == ValueType::RegRef)
		return _fetchRegValue(curMajorFrame, value.getRegIndex());
	return value;
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
			throw IncompatibleTypeError("Invalid type conversion");
	}
}

VarRef slake::Runtime::_addLocalVar(MajorFrame *frame, Type type) {
	LocalVarRecord localVarRecord;

	switch (type.typeId) {
		case TypeId::Value:
			switch (type.getValueTypeExData()) {
				case ValueType::I8:
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(int8_t));
					break;
				case ValueType::I16:
					frame->context->stackAlloc((2 - (frame->context->stackTop & 1)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(int16_t));
					break;
				case ValueType::I32:
					frame->context->stackAlloc((4 - (frame->context->stackTop & 3)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(int32_t));
					break;
				case ValueType::I64:
					frame->context->stackAlloc((8 - (frame->context->stackTop & 7)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(int64_t));
					break;
				case ValueType::U8:
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(uint8_t));
					break;
				case ValueType::U16:
					frame->context->stackAlloc((2 - (frame->context->stackTop & 1)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(uint16_t));
					break;
				case ValueType::U32:
					frame->context->stackAlloc((4 - (frame->context->stackTop & 3)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(uint32_t));
					break;
				case ValueType::U64:
					frame->context->stackAlloc((8 - (frame->context->stackTop & 7)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(uint64_t));
					break;
				case ValueType::F32:
					frame->context->stackAlloc((4 - (frame->context->stackTop & 3)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(float));
					break;
				case ValueType::F64:
					frame->context->stackAlloc((8 - (frame->context->stackTop & 7)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(double));
					break;
				case ValueType::Bool:
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(bool));
					break;
				case ValueType::ObjectRef:
					frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1)));
					localVarRecord.stackOffset = frame->context->stackTop;
					frame->context->stackAlloc(sizeof(Object *));
					break;
			}
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			frame->context->stackAlloc(sizeof(Object *) - (frame->context->stackTop & (sizeof(Object *) - 1)));
			localVarRecord.stackOffset = frame->context->stackTop;
			frame->context->stackAlloc(sizeof(Object *));
			break;
		default:
			throw std::runtime_error("The variable has an inconstructible type");
	}

	localVarRecord.type = type;

	size_t index = frame->localVarRecords.size();
	frame->localVarRecords.push_back(std::move(localVarRecord));
	return VarRef(frame->localVarAccessor, VarRefContext::makeLocalVarContext(index));
}

void slake::Runtime::_addLocalReg(MajorFrame *frame) {
	frame->regs.push_back({});
}

void slake::Runtime::_execIns(Context *context, Instruction ins) {
	if (((globalHeapPoolResource.szAllocated >> 1) > _szMemUsedAfterLastGc)) {
		gc();
	}

	auto curMajorFrame = context->majorFrames.back().get();
	auto &curMinorFrame = curMajorFrame->minorFrames.back();

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			_checkOperandCount(ins, false, 1);
			_checkOperandType(ins.operands[0], ValueType::TypeName);

			auto &type = ins.operands[0].getTypeName();
			type.loadDeferredType(this);

			_addLocalVar(curMajorFrame, type);
			break;
		}
		case Opcode::REG: {
			_checkOperandCount(ins, false, 1);
			_checkOperandType(ins.operands[0], ValueType::U32);

			uint32_t index = ins.operands[0].getU32();
			if (index > curMajorFrame->regs.size()) {
				throw InvalidOperandsError("");
			} else if (index < curMajorFrame->regs.size()) {
				curMajorFrame->regs[index] = Value();
			} else {
				_addLocalReg(curMajorFrame);
			}
			break;
		}
		case Opcode::LOAD: {
			_checkOperandCount(ins, true, 1);

			_checkOperandType(ins.output, ValueType::RegRef);

			_checkOperandType(ins.operands[0], ValueType::ObjectRef);
			auto refPtr = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(refPtr, ObjectKind::IdRef);

			VarRefContext varRefContext;

			auto v = resolveIdRef((IdRefObject *)refPtr, &varRefContext, curMajorFrame->thisObject);
			if (!v) {
				if (!(v = resolveIdRef((IdRefObject *)refPtr, &varRefContext, curMajorFrame->scopeObject)))
					v = resolveIdRef((IdRefObject *)refPtr, &varRefContext);
			}

			if (!v)
				throw NotFoundError("Member not found", (IdRefObject *)refPtr);

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), _wrapObjectIntoValue(v, varRefContext));
			break;
		}
		case Opcode::RLOAD: {
			_checkOperandCount(ins, true, 2);

			_checkOperandType(ins.output, ValueType::RegRef);

			_checkOperandType(ins.operands[0], ValueType::RegRef);
			auto &lhs = _fetchRegValue(curMajorFrame, ins.operands[0].getRegIndex());
			_checkOperandType(lhs, ValueType::ObjectRef);
			auto lhsPtr = lhs.getObjectRef().objectPtr;

			_checkOperandType(ins.operands[1], ValueType::ObjectRef);
			auto refPtr = ins.operands[1].getObjectRef().objectPtr;

			if (!lhsPtr)
				throw NullRefError();

			VarRefContext varRefContext;

			if (!(lhsPtr = resolveIdRef((IdRefObject *)refPtr, &varRefContext, lhsPtr))) {
				throw NotFoundError("Member not found", (IdRefObject *)refPtr);
			}

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), _wrapObjectIntoValue(lhsPtr, varRefContext));
			break;
		}
		case Opcode::STORE: {
			_checkOperandCount(ins, false, 2);

			Value destValue = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
			_checkOperandType(destValue, ValueType::VarRef);

			const VarRef varRef = destValue.getVarRef();

			if (!varRef.varPtr)
				throw NullRefError();

			varRef.varPtr->setData(
				varRef.context,
				_unwrapRegOperand(curMajorFrame, ins.operands[1]));
			break;
		}
		case Opcode::MOV: {
			_checkOperandCount(ins, true, 1);

			_setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				_unwrapRegOperand(curMajorFrame, ins.operands[0]));
			break;
		}
		case Opcode::LLOAD: {
			_checkOperandCount(ins, true, 1);

			_checkOperandType(ins.output, ValueType::RegRef);
			_checkOperandType(ins.operands[0], ValueType::U32);

			_setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				Value(curMajorFrame->lload(ins.operands[0].getU32())));
			break;
		}
		case Opcode::LARG: {
			_checkOperandCount(ins, true, 1);

			_checkOperandType(ins.output, ValueType::RegRef);
			_checkOperandType(ins.operands[0], ValueType::U32);

			_setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				Value(curMajorFrame->larg(ins.operands[0].getU32())));
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, true, 1);

			_checkOperandType(ins.output, ValueType::RegRef);

			Value dest = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
			_checkOperandType(dest, ValueType::VarRef);

			const VarRef varRef = dest.getVarRef();

			if (!varRef.varPtr)
				throw NullRefError();

			if (((VarObject *)varRef.varPtr)->getVarKind() == VarKind::ArrayElementAccessor)
				puts("");

			_setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				((VarObject *)varRef.varPtr)->getData(varRef.context));
			break;
		}
		case Opcode::ENTER: {
			_checkOperandCount(ins, false, 0);
			MinorFrame frame(this, (uint32_t)curMajorFrame->localVarRecords.size(), (uint32_t)curMajorFrame->regs.size(), context->stackTop);

			curMajorFrame->minorFrames.push_back(frame);
			break;
		}
		case Opcode::LEAVE: {
			_checkOperandCount(ins, false, 0);
			if (curMajorFrame->minorFrames.size() < 2)
				throw FrameError("Leaving the only frame");
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
			_checkOperandCount(ins, true, 2);

			_checkOperandType(ins.output, ValueType::RegRef);

			Value x(_unwrapRegOperand(curMajorFrame, ins.operands[0])),
				y(_unwrapRegOperand(curMajorFrame, ins.operands[1])),
				valueOut;
			if (x.valueType != y.valueType)
				throw InvalidOperandsError("LHS and RHS must have the same type");

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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							valueOut = Value((double)fmodf(x.getF64(), y.getF64()));
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
					}
					break;
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), valueOut);
			break;
		}
		case Opcode::LSH:
		case Opcode::RSH: {
			_checkOperandCount(ins, true, 2);

			_checkOperandType(ins.output, ValueType::RegRef);

			Value x(_unwrapRegOperand(curMajorFrame, ins.operands[0])),
				y(_unwrapRegOperand(curMajorFrame, ins.operands[1])),
				valueOut;

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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
					}
					break;
				case ValueType::F32:
				case ValueType::F64:
				case ValueType::Bool:
					throw InvalidOperandsError("Unsupported operation");
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), valueOut);
			break;
		}
		case Opcode::NOT:
		case Opcode::LNOT:
		case Opcode::NEG: {
			_checkOperandCount(ins, true, 1);

			_checkOperandType(ins.output, ValueType::RegRef);

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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
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
							throw InvalidOperandsError("Unsupported operation");
					}
					break;
				case ValueType::Bool:
					switch (ins.opcode) {
						case Opcode::LNOT:
							valueOut = Value((bool)(!x.getU64()));
							break;
						default:
							throw InvalidOperandsError("Unsupported operation");
					}
					break;
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), valueOut);
			break;
		}
		case Opcode::AT: {
			_checkOperandCount(ins, true, 2);

			Value &arrayValue = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
			_checkOperandType(arrayValue, ValueType::ObjectRef);
			Value &index = _unwrapRegOperand(curMajorFrame, ins.operands[1]);
			_checkOperandType(index, ValueType::U32);

			auto arrayIn = arrayValue.getObjectRef().objectPtr;
			_checkObjectOperandType(arrayIn, ObjectKind::Array);

			uint32_t indexIn = index.getU32();

			if (indexIn > ((ArrayObject *)arrayIn)->length)
				throw OutOfRangeError();

			_setRegisterValue(
				curMajorFrame,
				ins.output.getRegIndex(),
				Value(VarRef(
					((ArrayObject *)arrayIn)->accessor,
					VarRefContext::makeArrayContext(indexIn))));

			break;
		}
		case Opcode::JMP: {
			_checkOperandCount(ins, false, 1);

			_checkOperandType(ins.operands[0], ValueType::U32);

			curMajorFrame->curIns = ins.operands[0].getU32();
			return;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, false, 2);

			_checkOperandType(ins.operands[0], ValueType::U32);
			Value &condition = _unwrapRegOperand(curMajorFrame, ins.operands[1]);
			_checkOperandType(condition, ValueType::Bool);

			if (condition.getBool()) {
				if (ins.opcode == Opcode::JT) {
					curMajorFrame->curIns = ins.operands[0].getU32();
					return;
				}
			} else if (ins.opcode == Opcode::JF) {
				curMajorFrame->curIns = ins.operands[0].getU32();
				return;
			}

			break;
		}
		case Opcode::PUSHARG: {
			_checkOperandCount(ins, false, 2);

			curMajorFrame->nextArgStack.push_back(_unwrapRegOperand(curMajorFrame, ins.operands[0]));
			switch (ins.operands[1].valueType) {
				case ValueType::TypeName:
					curMajorFrame->nextArgTypes.push_back(ins.operands[1].getTypeName());
					break;
				case ValueType::ObjectRef:
					if (!ins.operands[1].getObjectRef().objectPtr)
						break;
					[[fallthrough]];
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}
			break;
		}
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnObject *fn;
			Object *thisObject = nullptr;

			if (ins.opcode == Opcode::MCALL) {
				_checkOperandCount(ins, false, 2);

				Value fnValue = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
				_checkOperandType(fnValue, ValueType::ObjectRef);
				_checkObjectOperandType(fnValue.getObjectRef().objectPtr, ObjectKind::Fn);
				fn = (FnObject *)fnValue.getObjectRef().objectPtr;

				Value thisObjectValue = _unwrapRegOperand(curMajorFrame, ins.operands[1]);
				_checkOperandType(thisObjectValue, ValueType::ObjectRef);
				thisObject = thisObjectValue.getObjectRef().objectPtr;
			} else {
				_checkOperandCount(ins, false, 1);

				Value fnValue = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
				_checkOperandType(fnValue, ValueType::ObjectRef);
				_checkObjectOperandType(fnValue.getObjectRef().objectPtr, ObjectKind::Fn);
				fn = (FnObject *)fnValue.getObjectRef().objectPtr;
			}

			if (!fn)
				throw NullRefError();

			HostRefHolder holder;

			for(auto &i : curMajorFrame->nextArgTypes)
				i.loadDeferredType(this);

			curMajorFrame->returnValue = fn->call(
				thisObject,
				curMajorFrame->nextArgStack,
				curMajorFrame->nextArgTypes,
				&holder);

			curMajorFrame->nextArgStack.clear();
			curMajorFrame->nextArgTypes.clear();

			break;
		}
		case Opcode::RET: {
			_checkOperandCount(ins, false, 1);

			Value returnValue = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
			context->majorFrames.pop_back();
			context->majorFrames.back()->returnValue = returnValue;
			break;
		}
		case Opcode::LRET: {
			_checkOperandCount(ins, false, 0);
			_checkOperandType(ins.output, ValueType::RegRef);

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), curMajorFrame->returnValue);
			break;
		}
		case Opcode::ACALL:
		case Opcode::AMCALL: {
			break;
		}
		case Opcode::YIELD: {
			_checkOperandCount(ins, false, 1);

			context->flags |= CTX_YIELDED;
			curMajorFrame->returnValue = _unwrapRegOperand(curMajorFrame, ins.operands[0]);
			break;
		}
		case Opcode::AWAIT: {
			break;
		}
		case Opcode::LTHIS: {
			_checkOperandCount(ins, false, 0);
			_checkOperandType(ins.output, ValueType::RegRef);

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), curMajorFrame->thisObject);
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, true, 2);

			_checkOperandType(ins.operands[0], ValueType::TypeName);
			_checkOperandType(ins.operands[1], ValueType::ObjectRef);

			auto constructorRef = ins.operands[1].getObjectRef().objectPtr;

			Type &type = ins.operands[0].getTypeName();
			type.loadDeferredType(this);

			switch (type.typeId) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)type.getCustomTypeExData();
					HostObjectRef<InstanceObject> instance = newClassInstance(cls, 0);
					_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), instance.get());

					if (constructorRef) {
						_checkObjectOperandType(constructorRef, ObjectKind::IdRef);

						if (auto v = resolveIdRef((IdRefObject *)constructorRef, nullptr); v) {
							if ((v->getKind() != ObjectKind::Fn))
								throw InvalidOperandsError("Specified constructor is not a function");

							FnObject *constructor = (FnObject *)v;

							HostRefHolder holder;

							constructor->call(instance.get(), curMajorFrame->nextArgStack, curMajorFrame->nextArgTypes, &holder);
							curMajorFrame->nextArgStack.clear();
							curMajorFrame->nextArgTypes.clear();
						} else
							throw InvalidOperandsError("Specified constructor is not found");
					}
					break;
				}
				default:
					throw InvalidOperandsError("Specified type cannot be instantiated");
			}
			break;
		}
		case Opcode::ARRNEW: {
			_checkOperandCount(ins, true, 2);

			_checkOperandType(ins.operands[0], ValueType::TypeName);
			_checkOperandType(ins.operands[1], ValueType::U32);

			Type &type = ins.operands[1].getTypeName();
			uint32_t size = ins.operands[2].getU32();
			type.loadDeferredType(this);

			auto instance = newArrayInstance(this, type, size);

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), instance.get());

			break;
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, false, 1);

			Value x = ins.operands[0];

			for (size_t i = context->majorFrames.size(); i; --i) {
				auto &majorFrame = context->majorFrames[i - 1];

				for (size_t j = majorFrame->minorFrames.size(); j; --j) {
					auto &minorFrame = majorFrame->minorFrames[j - 1];

					if (uint32_t off = _findAndDispatchExceptHandler(majorFrame->curExcept, minorFrame);
						off != UINT32_MAX) {
						context->majorFrames.resize(i);
						majorFrame->minorFrames.resize(j);
						// Do not increase the current instruction offset,
						// the offset has been set to offset to first instruction
						// of the exception handler.
						majorFrame->curIns = off;
						return;
					}
				}
			}

			curMajorFrame->curExcept = x;
			throw UncaughtExceptionError("Uncaught exception", x);
		}
		case Opcode::PUSHXH: {
			_checkOperandCount(ins, false, 2);

			_checkOperandType(ins.operands[0], ValueType::TypeName);
			_checkOperandType(ins.operands[1], ValueType::U32);

			ExceptionHandler xh;

			ins.operands[0].getTypeName().loadDeferredType(this);

			xh.type = ins.operands[0].getTypeName();
			xh.off = ins.operands[1].getU32();

			curMajorFrame->minorFrames.back().exceptHandlers.push_back(xh);
			break;
		}
		case Opcode::CAST: {
			_checkOperandCount(ins, true, 2);

			_checkOperandType(ins.output, ValueType::RegRef);
			_checkOperandType(ins.operands[0], ValueType::TypeName);

			Value v = _unwrapRegOperand(curMajorFrame, ins.operands[1]);

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
							throw InvalidOperandsError("Invalid cast target type");
					}
				}
				case TypeId::Instance:
					break;
				default:
					throw InvalidOperandsError("Invalid cast target type");
			}

			_setRegisterValue(curMajorFrame, ins.output.getRegIndex(), v);
			break;
		}
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((uint8_t)ins.opcode));
	}
	++curMajorFrame->curIns;
}
