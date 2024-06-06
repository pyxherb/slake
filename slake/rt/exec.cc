#include "../runtime.h"
#include <cmath>

using namespace slake;

static void _checkOperandType(
	Instruction &ins,
	std::initializer_list<ValueType> types) {
	if (ins.operands.size() < types.size())
		throw InvalidOperandsError("Invalid operand combination");

	auto it = types.begin();
	for (size_t i = 0; i < ins.operands.size(); ++i) {
		auto type = *(it++);

		if (type == ValueType::Undefined)
			continue;

		if (ins.operands[i].valueType != type)
			throw InvalidOperandsError("Invalid operand combination");
	}
}

static void _checkObjectOperandType(
	Object *object,
	TypeId typeId) {
	if (object->getType().typeId != typeId)
		throw InvalidOperandsError("Invalid operand combination");
}

static void _checkOperandCount(Instruction &ins, uint8_t n) {
	if (ins.operands.size() != n)
		throw InvalidOperandsError("Invalid operand count");
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

VarObject *slake::Runtime::_addLocalVar(MajorFrame &frame, Type type) {
	auto v = new VarObject(this, ACCESS_PUB, type);
	frame.localVars.push_back(v);
	return v;
}

VarObject *slake::Runtime::_addLocalReg(MajorFrame &frame) {
	auto v = new VarObject(this, ACCESS_PUB, TypeId::Any);
	frame.regs.push_back(v);
	return v;
}

void slake::Runtime::_execIns(Context *context, Instruction ins) {
	auto &curMajorFrame = context->majorFrames.back();
	auto &curMinorFrame = curMajorFrame.minorFrames.back();

	for (auto &i : ins.operands) {
		bool unwrapObject = false;
		switch (i.valueType) {
			case ValueType::LocalVarRef: {
				unwrapObject = i.getIndexedRef().unwrap;

				auto index = i.getIndexedRef().index;

				if (index >= curMajorFrame.localVars.size())
					throw InvalidLocalVarIndexError("Invalid local variable index", index);

				i = curMajorFrame.localVars.at(index);
				break;
			}
			case ValueType::RegRef: {
				unwrapObject = i.getIndexedRef().unwrap;

				auto index = i.getIndexedRef().index;

				if (index >= curMajorFrame.regs.size())
					throw InvalidRegisterIndexError("Invalid register index", index);

				i = curMajorFrame.regs.at(index);
				break;
			}
			case ValueType::ArgRef:
				unwrapObject = i.getIndexedRef().unwrap;

				auto index = i.getIndexedRef().index;

				if (index >= curMajorFrame.argStack.size())
					throw InvalidRegisterIndexError("Invalid argument index", index);

				i = curMajorFrame.argStack[index];
				break;
		}

		if (unwrapObject)
			i = ((VarObject *)i.getObjectRef().objectPtr)->getData();
	}

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { ValueType::TypeName });

			auto &type = ins.operands[0].getTypeName();
			type.loadDeferredType(this);

			_addLocalVar(curMajorFrame, type);
			break;
		}
		case Opcode::REG: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { ValueType::U32 });

			uint32_t times = ins.operands[0].getU32();
			while (times--)
				_addLocalReg(curMajorFrame);
			break;
		}
		case Opcode::PUSH:
			_checkOperandCount(ins, 1);
			curMinorFrame.push(ins.operands[0]);
			break;
		case Opcode::POP: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { ValueType::ObjectRef });

			auto varOutPtr = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOutPtr, TypeId::Var);

			((VarObject *)varOutPtr)->setData(curMinorFrame.pop());
			break;
		}
		case Opcode::LOAD: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::ObjectRef });

			auto varOutPtr = ins.operands[0].getObjectRef().objectPtr;
			auto refPtr = ins.operands[1].getObjectRef().objectPtr;
			_checkObjectOperandType(varOutPtr, TypeId::Var);
			_checkObjectOperandType(refPtr, TypeId::IdRef);

			auto v = resolveIdRef((IdRefObject *)refPtr, curMajorFrame.thisObject);
			if (!v) {
				if (!(v = resolveIdRef((IdRefObject *)refPtr, curMajorFrame.scopeObject)))
					v = resolveIdRef((IdRefObject *)refPtr);
			}

			if (!v)
				throw NotFoundError("Member not found", (IdRefObject *)refPtr);
			((VarObject *)varOutPtr)->setData(v);
			break;
		}
		case Opcode::RLOAD: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::ObjectRef, ValueType::ObjectRef });

			auto varOutPtr = ins.operands[0].getObjectRef().objectPtr;
			auto lhsPtr = ins.operands[1].getObjectRef().objectPtr;
			auto refPtr = ins.operands[2].getObjectRef().objectPtr;

			if (!lhsPtr)
				throw NullRefError();

			if (!(lhsPtr = resolveIdRef((IdRefObject *)refPtr, lhsPtr))) {
				throw NotFoundError("Member not found", (IdRefObject *)refPtr);
			}
			((VarObject *)varOutPtr)->setData(lhsPtr);
			break;
		}
		case Opcode::STORE: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::Undefined });

			auto varOutPtr = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOutPtr, TypeId::Var);

			if (!varOutPtr)
				throw NullRefError();

			((VarObject *)varOutPtr)->setData(ins.operands[1]);
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::ObjectRef });

			auto varOutPtr = ins.operands[0].getObjectRef().objectPtr,
				 varInPtr = ins.operands[1].getObjectRef().objectPtr;
			_checkObjectOperandType(varOutPtr, TypeId::Var);
			_checkObjectOperandType(varInPtr, TypeId::Var);

			if (!varOutPtr)
				throw NullRefError();
			if (!varInPtr)
				throw NullRefError();

			((VarObject *)varOutPtr)->setData(((VarObject *)varInPtr)->getData());
			break;
		}
		case Opcode::ENTER: {
			_checkOperandCount(ins, 0);
			MinorFrame frame((uint32_t)curMajorFrame.localVars.size(), (uint32_t)curMajorFrame.regs.size());

			curMajorFrame.minorFrames.push_back(frame);
			break;
		}
		case Opcode::LEAVE: {
			_checkOperandCount(ins, 0);
			if (curMajorFrame.minorFrames.size() < 2)
				throw FrameError("Leaving the only frame");
			curMajorFrame.leave();
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
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::Undefined, ValueType::Undefined });

			Value x(ins.operands[1]), y(ins.operands[2]), valueOut;
			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			if (!varOut)
				throw NullRefError();
			_checkObjectOperandType(varOut, TypeId::Var);

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

			((VarObject *)varOut)->setData(valueOut);
			break;
		}
		case Opcode::NOT:
		case Opcode::LNOT:
		case Opcode::NEG: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::Undefined });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

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
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}

			((VarObject *)varOut)->setData(valueOut);
			break;
		}
		case Opcode::AT: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::ObjectRef, ValueType::U32 });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

			auto arrayIn = ins.operands[1].getObjectRef().objectPtr;
			_checkObjectOperandType(arrayIn, TypeId::Array);

			uint32_t indexIn = ins.operands[2].getU32();

			if (indexIn > ((ArrayObject *)arrayIn)->values.size())
				throw OutOfRangeError();

			((VarObject *)varOut)->setData(((ArrayObject *)arrayIn)->values[indexIn]);

			break;
		}
		case Opcode::JMP: {
			_checkOperandCount(ins, 1);

			_checkOperandType(ins, { ValueType::U32 });

			curMajorFrame.curIns = ins.operands[0].getU32();
			return;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { ValueType::U32, ValueType::Bool });

			if (ins.operands[1].getBool()) {
				if (ins.opcode == Opcode::JT) {
					curMajorFrame.curIns = ins.operands[0].getU32();
					return;
				}
			} else if (ins.opcode == Opcode::JF) {
				curMajorFrame.curIns = ins.operands[0].getU32();
				return;
			}

			break;
		}
		case Opcode::PUSHARG: {
			_checkOperandCount(ins, 2);
			_checkOperandType(ins, { ValueType::Undefined, ValueType::Undefined });

			curMajorFrame.nextArgStack.push_back(ins.operands[0]);
			switch (ins.operands[1].valueType) {
				case ValueType::TypeName:
					curMajorFrame.nextArgTypes.push_back(ins.operands[1].getTypeName());
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
			if (ins.opcode == Opcode::MCALL) {
				_checkOperandCount(ins, 2);

				_checkOperandType(ins, { ValueType::ObjectRef, ValueType::ObjectRef });
			} else {
				_checkOperandCount(ins, 1);

				_checkOperandType(ins, { ValueType::ObjectRef });
			}
			_checkObjectOperandType(ins.operands[0].getObjectRef().objectPtr, TypeId::Fn);

			FnObject *fn = (FnObject *)ins.operands[0].getObjectRef().objectPtr;

			if (!fn)
				throw NullRefError();

			fn->call(
				ins.opcode == Opcode::MCALL
					? ins.operands[1].getObjectRef().objectPtr
					: nullptr,
				curMajorFrame.nextArgStack,
				curMajorFrame.nextArgTypes);

			curMajorFrame.nextArgStack.clear();
			curMajorFrame.nextArgTypes.clear();

			break;
		}
		case Opcode::RET: {
			_checkOperandCount(ins, 1);

			context->majorFrames.pop_back();
			context->majorFrames.back().returnValue = ins.operands[0];
			break;
		}
		case Opcode::LRET: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { ValueType::ObjectRef });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

			((VarObject *)varOut)->setData(curMajorFrame.returnValue);
			break;
		}
		case Opcode::ACALL:
		case Opcode::AMCALL: {
			break;
		}
		case Opcode::YIELD: {
			_checkOperandCount(ins, 1);

			context->flags |= CTX_YIELDED;
			curMajorFrame.returnValue = ins.operands[0];
			break;
		}
		case Opcode::AWAIT: {
			break;
		}
		case Opcode::LTHIS: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { ValueType::ObjectRef });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

			((VarObject *)varOut)->setData(curMajorFrame.thisObject);
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::TypeName, ValueType::ObjectRef });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

			auto constructorRef = ins.operands[2].getObjectRef().objectPtr;

			Type &type = ins.operands[1].getTypeName();
			type.loadDeferredType(this);

			switch (type.typeId) {
				case TypeId::Instance:
				case TypeId::Class: {
					ClassObject *cls = (ClassObject *)type.getCustomTypeExData();
					InstanceObject *instance = newClassInstance(cls);
					((VarObject *)varOut)->setData(instance);

					if (constructorRef) {
						_checkObjectOperandType(constructorRef, TypeId::IdRef);

						if (auto v = resolveIdRef((IdRefObject *)constructorRef); v) {
							if ((v->getType() != TypeId::Fn))
								throw InvalidOperandsError("Specified constructor is not a function");

							FnObject *constructor = (FnObject *)v;

							constructor->call(instance, curMajorFrame.nextArgStack, curMajorFrame.nextArgTypes);
							curMajorFrame.nextArgStack.clear();
							curMajorFrame.nextArgTypes.clear();
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
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::TypeName, ValueType::U32 });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

			Type &type = ins.operands[1].getTypeName();
			uint32_t size = ins.operands[2].getU32();
			type.loadDeferredType(this);

			auto instance = newArrayInstance(type, size);

			((VarObject *)varOut)->setData(instance);

			break;
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, 1);

			_checkOperandType(ins, { ValueType::ObjectRef });

			Object *x = ins.operands[0].getObjectRef().objectPtr;

			auto tmpContext = *context;

			while (tmpContext.majorFrames.size()) {
				// Pass the exception to current level of major frames.
				// Because curMajorFrame expires once we unwind a major frame,
				// we use `context->majorFrames.back()` instead.
				tmpContext.majorFrames.back().curExcept = x;

				while (tmpContext.majorFrames.back().minorFrames.size()) {
					bool found = _findAndDispatchExceptHandler(&tmpContext);

					// Leave current minor frame.
					tmpContext.majorFrames.back().leave();

					if (found) {
						*context = tmpContext;
						// Do not increase the current instruction offset,
						// the offset has been set to offset to first instruction
						// of the exception handler.
						return;
					}
				}

				// Unwind current major frame if no handler was matched.
				tmpContext.majorFrames.pop_back();
			}

			curMajorFrame.curExcept = x;
			throw UncaughtExceptionError("Uncaught exception: " + std::to_string(x->getType(), this));
		}
		case Opcode::PUSHXH: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { ValueType::TypeName, ValueType::U32 });

			ExceptionHandler xh;

			ins.operands[0].getTypeName().loadDeferredType(this);

			xh.type = ins.operands[0].getTypeName();
			xh.off = ins.operands[1].getU32();

			curMajorFrame.minorFrames.back().exceptHandlers.push_back(xh);
			break;
		}
		case Opcode::ABORT:
			_checkOperandCount(ins, 0);
			throw UncaughtExceptionError("Use chose to abort the execution");
		case Opcode::CAST: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { ValueType::ObjectRef, ValueType::TypeName, ValueType::Undefined });

			auto varOut = ins.operands[0].getObjectRef().objectPtr;
			_checkObjectOperandType(varOut, TypeId::Var);

			Value v = ins.operands[2];

			auto t = ins.operands[1].getTypeName();

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

			((VarObject *)varOut)->setData(v);
			break;
		}
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((uint8_t)ins.opcode));
	}
	++curMajorFrame.curIns;
}
