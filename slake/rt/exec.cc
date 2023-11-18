#include "../runtime.h"
#include <cmath>

using namespace slake;

/// @brief Check if value of a value object is in range of specified type.
/// @tparam TD Destination type.
/// @tparam TS Source type.
/// @param v Input value object.
/// @return Converted value.
template <typename TD, typename TS>
static TD _checkOperandRange(ValueRef<> v) {
	auto value = (LiteralValue<TS, getValueType<TS>()> *)*v;
	if ((TD)value->getData() > std::numeric_limits<TD>::max() ||
		(TD)value->getData() < std::numeric_limits<TD>::min())
		throw InvalidOperandsError("Invalid operand value");
	return (TD)value->getData();
}

static void _checkOperandType(
	Instruction &ins,
	std::initializer_list<TypeId> types) {
	if (ins.operands.size() < types.size())
		throw InvalidOperandsError("Invalid operand combination");

	auto it = types.begin();
	for (size_t i = 0; i < ins.operands.size(); ++i) {
		auto type = *(it++);

		if (!ins.operands[i])
			continue;

		if (type == TypeId::ANY)
			continue;

		if (ins.operands[i]->getType() != type)
			throw InvalidOperandsError("Invalid operand combination");
	}
}

static void _checkOperandCount(Instruction &ins, uint8_t n) {
	if (ins.operands.size() != n)
		throw InvalidOperandsError("Invalid operand count");
}

template <typename LT>
static Value *_castToLiteralValue(Runtime *rt, Value *x) {
	using T = LiteralValue<LT, getValueType<LT>()>;
	if (getValueType<LT>() == x->getType().typeId)
		return x;
	switch (x->getType().typeId) {
		case TypeId::I8:
			return new T(rt, (LT)(((I8Value *)x)->getData()));
		case TypeId::I16:
			return new T(rt, (LT)(((I16Value *)x)->getData()));
		case TypeId::I32:
			return new T(rt, (LT)(((I32Value *)x)->getData()));
		case TypeId::I64:
			return new T(rt, (LT)(((I64Value *)x)->getData()));
		case TypeId::U8:
			return new T(rt, (LT)(((U8Value *)x)->getData()));
		case TypeId::U16:
			return new T(rt, (LT)(((U16Value *)x)->getData()));
		case TypeId::U32:
			return new T(rt, (LT)(((U32Value *)x)->getData()));
		case TypeId::U64:
			return new T(rt, (LT)(((U64Value *)x)->getData()));
		case TypeId::F32:
			return new T(rt, (LT)(((F32Value *)x)->getData()));
		case TypeId::F64:
			return new T(rt, (LT)(((F64Value *)x)->getData()));
		case TypeId::BOOL:
			return new T(rt, (LT)(((BoolValue *)x)->getData()));
		default:
			throw IncompatibleTypeError("Invalid type conversion");
	}
}

template <typename T>
static Value *_execBinaryOp(ValueRef<> x, ValueRef<> y, Opcode opcode) {
	// Corresponding value type
	using V = LiteralValue<T, getValueType<T>()>;

	auto _x = (V *)*x;
	auto rt = _x->getRuntime();

	if constexpr (std::is_arithmetic<T>::value) {
		if constexpr (std::is_same<T, bool>::value) {
			// Boolean
			switch (opcode) {
				case Opcode::LAND: {
					return new V(
						rt,
						_x->getData() + ((V *)*y)->getData());
				}
				case Opcode::LOR: {
					return new V(rt, _x->getData() + ((V *)*y)->getData());
				}
				default:
					throw InvalidOperandsError("Binary operation with incompatible types");
			}
		} else {
			if (opcode == Opcode::LSH || opcode == Opcode::RSH) {
				if (y->getType() != TypeId::U32)
					throw InvalidOperandsError("Binary operation with incompatible types");
			} else if (_x->getType() != y->getType())
				throw InvalidOperandsError("Binary operation with incompatible types");

			switch (opcode) {
				case Opcode::ADD:
					return new V(rt, _x->getData() + ((V *)*y)->getData());
				case Opcode::SUB:
					return new V(rt, _x->getData() - ((V *)*y)->getData());
				case Opcode::MUL:
					return new V(rt, _x->getData() * ((V *)*y)->getData());
				case Opcode::DIV:
					return new V(rt, _x->getData() / ((V *)*y)->getData());
				case Opcode::MOD: {
					T result;
					if constexpr (std::is_same<T, float>::value)
						result = fmodf(_x->getData(), ((V *)*y)->getData());
					else if constexpr (std::is_same<T, double>::value)
						result = fmod(_x->getData(), ((V *)*y)->getData());
					else
						result = _x->getData() % ((V *)*y)->getData();
					return new V(rt, result);
				}
				case Opcode::AND:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() & ((V *)*y)->getData());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::OR:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() | ((V *)*y)->getData());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::XOR:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() ^ ((V *)*y)->getData());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::LAND:
					return new BoolValue(rt, _x->getData() && ((V *)*y)->getData());
				case Opcode::LOR:
					return new BoolValue(rt, _x->getData() || ((V *)*y)->getData());
				case Opcode::EQ:
					return new BoolValue(rt, _x->getData() == ((V *)*y)->getData());
				case Opcode::NEQ:
					return new BoolValue(rt, _x->getData() != ((V *)*y)->getData());
				case Opcode::LT:
					return new BoolValue(rt, _x->getData() < ((V *)*y)->getData());
				case Opcode::GT:
					return new BoolValue(rt, _x->getData() > ((V *)*y)->getData());
				case Opcode::LTEQ:
					return new BoolValue(rt, _x->getData() <= ((V *)*y)->getData());
				case Opcode::GTEQ:
					return new BoolValue(rt, _x->getData() >= ((V *)*y)->getData());
				case Opcode::LSH:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() << ((U32Value *)*y)->getData());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(uint32_t *)(&_x->getData())) << ((U32Value *)*y)->getData();
						return new V(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(uint64_t *)(&_x->getData())) << ((U32Value *)*y)->getData();
						return new V(rt, *(double *)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::RSH:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() >> ((U32Value *)*y)->getData());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(uint32_t *)(&_x->getData())) >> ((U32Value *)*y)->getData();
						return new V(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(uint64_t *)(&_x->getData())) >> ((U32Value *)*y)->getData();
						return new V(rt, *(double *)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				default:
					throw InvalidOperandsError("Binary operation with incompatible types");
			}
		}
	} else if constexpr (std::is_same<T, std::string>::value) {
		// String
		switch (opcode) {
			case Opcode::ADD:
				return new LiteralValue<T, TypeId::STRING>(rt, _x->getData() + ((V *)*y)->getData());
			case Opcode::EQ:
				return new BoolValue(rt, _x->getData() == ((V *)*y)->getData());
			case Opcode::NEQ:
				return new BoolValue(rt, _x->getData() != ((V *)*y)->getData());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	} else {
		switch (opcode) {
			case Opcode::EQ:
				return new BoolValue(rt, _x->getData() == ((V *)*y)->getData());
			case Opcode::NEQ:
				return new BoolValue(rt, _x->getData() != ((V *)*y)->getData());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	}
}

template <typename T>
static Value *_execUnaryOp(ValueRef<> x, Opcode opcode) {
	auto _x = (LiteralValue<T, getValueType<T>()> *)*x;
	auto rt = _x->getRuntime();

	if constexpr (std::is_arithmetic<T>::value) {
		switch (opcode) {
			case Opcode::REV:
				if constexpr (std::is_integral<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, ~(_x->getData()));
				else if constexpr (std::is_same<T, float>::value) {
					auto result = ~(*(uint32_t *)(rt, &_x->getData()));
					return new LiteralValue<T, getValueType<T>()>(rt, *((float *)&result));
				} else if constexpr (std::is_same<T, double>::value) {
					auto result = ~(*(uint64_t *)(rt, &_x->getData()));
					return new LiteralValue<T, getValueType<T>()>(rt, *((double *)&result));
				}
				throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::NOT:
				return new BoolValue(rt, !_x->getData());
			case Opcode::INCF:
			case Opcode::INCB:
				if constexpr (std::is_integral<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData() + 1);
				else if constexpr (std::is_same<T, float>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData() + 1.0f);
				else if constexpr (std::is_same<T, double>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData() + 1.0);
				throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::DECF:
			case Opcode::DECB:
				if constexpr (std::is_integral<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData() - 1);
				else if constexpr (std::is_same<T, float>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData() - 1.0f);
				else if constexpr (std::is_same<T, double>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData() - 1.0);
				throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::NEG:
				if constexpr (std::is_signed<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, -_x->getData());
				else
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getData());
		}
	}
	throw InvalidOperandsError("Binary operation with incompatible types");
}

void slake::Runtime::_callFn(Context *context, FnValue *fn) {
	auto &curFrame = context->majorFrames.back();
	MajorFrame frame(this);
	frame.curIns = 0;
	frame.curFn = fn;

	for (size_t i = 0; i < curFrame.nextArgStack.size(); ++i) {
		ValueRef<VarValue> argVar = new VarValue(this, ACCESS_PUB, i < fn->paramTypes.size() ? fn->paramTypes[i] : TypeId::ANY);
		argVar->setData(*curFrame.nextArgStack[i]);
		frame.argStack.push_back(argVar);
	}

	context->majorFrames.push_back(frame);
	return;
}

VarValue *slake::Runtime::_addLocalVar(MajorFrame &frame, Type type) {
	auto v = new VarValue(this, ACCESS_PUB, type);
	frame.localVars.push_back(v);
	return v;
}

static inline void _unwrapRegOperand(ValueRef<> &v) {
	if (v) {
		if (v->getType().typeId == TypeId::VAR) {
			if (((VarValue *)*v)->flags & VAR_REG)
				v = ((VarValue *)*v)->getData();
		}
	}
}

void slake::Runtime::_execIns(Context *context, Instruction ins) {
	auto &curMajorFrame = context->majorFrames.back();
	auto &curMinorFrame = curMajorFrame.minorFrames.back();

	for (auto &i : ins.operands) {
		if(!i)
			continue;

		bool unwrapValue = false;
		switch (i->getType().typeId) {
			case TypeId::LVAR_REF:
				unwrapValue = ((LocalVarRefValue *)*i)->unwrapValue;
				i = curMajorFrame.lload(((LocalVarRefValue *)*i)->index);
				break;
			case TypeId::REG_REF: {
				switch (auto regId = ((RegRefValue *)*i)->reg; regId) {
					case RegId::TMP0:
					case RegId::TMP1: {
						auto index = (uint8_t)regId - (uint8_t)RegId::TMP0;
						if (index > std::size(curMinorFrame.tmpRegs))
							throw InvalidRegisterError();
						i = *curMinorFrame.tmpRegs[index];

						break;
					}
					case RegId::R0:
					case RegId::R1:
					case RegId::R2:
					case RegId::R3: {
						auto index = (uint8_t)regId - (uint8_t)RegId::R0;
						if (index > std::size(curMinorFrame.gpRegs))
							throw InvalidRegisterError();
						i = *curMinorFrame.gpRegs[index];

						break;
					}
					case RegId::RR:
						i = curMajorFrame.returnValue;
						break;
					case RegId::RTHIS:
						i = *curMajorFrame.thisObject;
						break;
					case RegId::RXCPT:
						i = curMajorFrame.curExcept;
						break;
					default:
						throw InvalidRegisterError();
				}

				break;
			}
			case TypeId::ARG_REF:
				unwrapValue = ((ArgRefValue *)*i)->unwrapValue;

				i = *curMajorFrame.argStack[((ArgRefValue *)*i)->index];
				break;
		}

		if (unwrapValue)
			i = *((VarValue *)(*i))->value;
	}

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { TypeId::TYPENAME });

			auto &type = ((TypeNameValue *)*ins.operands[0])->_data;
			type.loadDeferredType(this);

			_addLocalVar(curMajorFrame, type);
			break;
		}
		case Opcode::PUSH:
			_checkOperandCount(ins, 1);
			_unwrapRegOperand(ins.operands[0]);
			curMinorFrame.push(ins.operands[0]);
			break;
		case Opcode::POP:
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { TypeId::VAR });

			((VarValue *)*ins.operands[0])->setData(*curMinorFrame.pop());
			break;
		case Opcode::LOAD: {
			_checkOperandCount(ins, 2);

			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::REF });

			auto ref = *ins.operands[1];
			auto v = resolveRef((RefValue*)ref, curMajorFrame.thisObject->getData());
			if (!v) {
				if (!(v = resolveRef((RefValue *)ref, *curMajorFrame.scopeValue)))
					v = resolveRef((RefValue *)ref);
			}

			if (!v)
				throw NotFoundError("No such member", (RefValue *)ref);
			((VarValue *)*ins.operands[0])->setData(v);
			break;
		}
		case Opcode::RLOAD: {
			_checkOperandCount(ins, 3);

			_unwrapRegOperand(ins.operands[1]);
			_unwrapRegOperand(ins.operands[2]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::ANY, TypeId::REF });

			auto v = ins.operands[1];
			if (!v)
				throw InvalidOperandsError("Invalid operand combination");

			if (!(v = resolveRef((RefValue *)*(ins.operands[2]), *v))) {
				throw NotFoundError("Member not found", (RefValue *)*ins.operands[2]);
			}
			((VarValue *)*ins.operands[0])->setData(*v);
			break;
		}
		case Opcode::STORE:
		case Opcode::ISTORE: {
			_checkOperandCount(ins, 2);

			if (ins.opcode == Opcode::ISTORE)
				_unwrapRegOperand(ins.operands[0]);
			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::ANY });

			VarValue *x = (VarValue *)*ins.operands[0];

			if (!x)
				throw InvalidOperandsError("Invalid operand combination");

			x->setData(*ins.operands[1]);
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, 2);

			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::VAR });

			((VarValue *)*ins.operands[0])->setData(((VarValue *)*ins.operands[1])->getData());
			break;
		}
		case Opcode::ENTER: {
			_checkOperandCount(ins, 0);
			MinorFrame frame(this);

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
		case Opcode::GTEQ:
		case Opcode::LSH: {
			_checkOperandCount(ins, 3);

			_unwrapRegOperand(ins.operands[1]);
			_unwrapRegOperand(ins.operands[2]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::ANY, TypeId::ANY });

			ValueRef<> x(ins.operands[1]), y(ins.operands[2]);
			ValueRef<VarValue> out((VarValue*)*ins.operands[0]);

			if (!x)
				throw NullRefError();
			if (!y)
				throw NullRefError();

			switch (x->getType().typeId) {
				case TypeId::I8:
					out->setData(_execBinaryOp<std::int8_t>(x, y, ins.opcode));
					break;
				case TypeId::I16:
					out->setData(_execBinaryOp<std::int16_t>(x, y, ins.opcode));
					break;
				case TypeId::I32:
					out->setData(_execBinaryOp<std::int32_t>(x, y, ins.opcode));
					break;
				case TypeId::I64:
					out->setData(_execBinaryOp<std::int64_t>(x, y, ins.opcode));
					break;
				case TypeId::U8:
					out->setData(_execBinaryOp<uint8_t>(x, y, ins.opcode));
					break;
				case TypeId::U16:
					out->setData(_execBinaryOp<uint16_t>(x, y, ins.opcode));
					break;
				case TypeId::U32:
					out->setData(_execBinaryOp<uint32_t>(x, y, ins.opcode));
					break;
				case TypeId::U64:
					out->setData(_execBinaryOp<uint64_t>(x, y, ins.opcode));
					break;
				case TypeId::F32:
					out->setData(_execBinaryOp<float>(x, y, ins.opcode));
					break;
				case TypeId::F64:
					out->setData(_execBinaryOp<double>(x, y, ins.opcode));
					break;
				case TypeId::STRING:
					out->setData(_execBinaryOp<std::string>(x, y, ins.opcode));
					break;
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}
			break;
		}
		case Opcode::INCF:
		case Opcode::DECF:
		case Opcode::INCB:
		case Opcode::DECB:
		case Opcode::REV:
		case Opcode::NOT:
		case Opcode::NEG: {
			_checkOperandCount(ins, 2);

			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::ANY });

			ValueRef<> x = ins.operands[1];	 // Value of the operand.
			ValueRef<VarValue> varIn, varOut = (VarValue *)*ins.operands[0];

			if (!(x && varOut))
				throw NullRefError();

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
				case Opcode::INCF:
				case Opcode::DECF:
					if (x->getType() != TypeId::VAR)
						throw InvalidOperandsError("Invalid operand combination");

					switch (ins.opcode) {
						case Opcode::INCB:
						case Opcode::DECB:
							((VarValue *)*varOut)->setData(((VarValue *)*x)->getData());
							break;
					}

					varIn = (VarValue *)*x;
					x = varIn->getData();
					break;
			}

			ValueRef<> value;
			if(!x)
				throw NullRefError();

			switch (x->getType().typeId) {
				case TypeId::I8:
					value = _execUnaryOp<std::int8_t>(x, ins.opcode);
					break;
				case TypeId::I16:
					value = _execUnaryOp<std::int16_t>(x, ins.opcode);
					break;
				case TypeId::I32:
					value = _execUnaryOp<std::int32_t>(x, ins.opcode);
					break;
				case TypeId::I64:
					value = _execUnaryOp<std::int64_t>(x, ins.opcode);
					break;
				case TypeId::U8:
					value = _execUnaryOp<uint8_t>(x, ins.opcode);
					break;
				case TypeId::U16:
					value = _execUnaryOp<uint16_t>(x, ins.opcode);
					break;
				case TypeId::U32:
					value = _execUnaryOp<uint32_t>(x, ins.opcode);
					break;
				case TypeId::U64:
					value = _execUnaryOp<uint64_t>(x, ins.opcode);
					break;
				case TypeId::F32:
					value = _execUnaryOp<float>(x, ins.opcode);
					break;
				case TypeId::F64:
					value = _execUnaryOp<double>(x, ins.opcode);
					break;
				case TypeId::STRING:
					value = _execUnaryOp<std::string>(x, ins.opcode);
					break;
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
					varIn->setData(*value);
					break;
				case Opcode::INCF:
				case Opcode::DECF:
					varIn->setData(*value);
					break;
				default:
					((VarValue *)*varOut)->setData(*value);
			}
			break;
		}
		case Opcode::AT: {
			_checkOperandCount(ins, 3);

			_unwrapRegOperand(ins.operands[1]);
			_unwrapRegOperand(ins.operands[2]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::ANY, TypeId::ANY });

			ValueRef<> x = ins.operands[1], i = ins.operands[2];

			switch (x->getType().typeId) {
				case TypeId::ARRAY: {
					ArrayValue *_x = (ArrayValue *)*x;

					if (i->getType() != TypeId::U32)
						throw InvalidOperandsError("Invalid argument for subscription");

					auto index = ((I32Value *)*i)->getData();
					if (_x->getSize() <= index)
						throw InvalidSubscriptionError("Out of array range");

					((VarValue *)*ins.operands[0])->setData(_x->at(index));
					break;
				}
				case TypeId::MAP: {
					throw std::logic_error("Unimplemented yet");
					break;
				}
				case TypeId::OBJECT: {
					throw std::logic_error("Unimplemented yet");
					break;
				}
				default:
					throw InvalidOperandsError("Subscription is not supported by the operand");
			}
			break;
		}
		case Opcode::JMP: {
			_checkOperandCount(ins, 1);

			_checkOperandType(ins, { TypeId::U32 });

			curMajorFrame.curIns = ((U32Value *)*ins.operands[0])->getData();
			return;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 2);

			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::U32, TypeId::BOOL });

			if (((BoolValue *)*ins.operands[1])->getData()) {
				if (ins.opcode == Opcode::JT) {
					curMajorFrame.curIns = ((U32Value *)*ins.operands[0])->getData();
					return;
				}
			} else if (ins.opcode == Opcode::JF) {
				curMajorFrame.curIns = ((U32Value *)*ins.operands[0])->getData();
				return;
			}

			break;
		}
		case Opcode::PUSHARG: {
			_checkOperandCount(ins, 1);

			_unwrapRegOperand(ins.operands[0]);

			curMajorFrame.nextArgStack.push_back(ins.operands[0]);
			break;
		}
		case Opcode::MCALL:
		case Opcode::CALL: {
			if (ins.opcode == Opcode::MCALL) {
				_checkOperandCount(ins, 2);

				_unwrapRegOperand(ins.operands[0]);
				_unwrapRegOperand(ins.operands[1]);

				_checkOperandType(ins, { TypeId::FN, TypeId::ANY });
			} else {
				_checkOperandCount(ins, 1);

				_unwrapRegOperand(ins.operands[0]);

				_checkOperandType(ins, { TypeId::FN });
			}

			ValueRef<FnValue> fn = (FnValue *)*ins.operands[0];

			if (fn->isNative()) {
				if (ins.opcode == Opcode::MCALL)
					curMajorFrame.thisObject->setData(*(curMajorFrame.scopeValue = *ins.operands[1]));

				curMajorFrame.returnValue = ((NativeFnValue *)*fn)->call(curMajorFrame.nextArgStack);
				curMajorFrame.nextArgStack.clear();
			} else {
				_callFn(context, *fn);

				auto &newCurFrame = context->getCurFrame();

				if (ins.opcode == Opcode::MCALL)
					newCurFrame.thisObject->setData(*(newCurFrame.scopeValue = ins.operands[1]));
				return;
			}

			break;
		}
		case Opcode::RET: {
			_checkOperandCount(ins, 1);

			_unwrapRegOperand(ins.operands[0]);

			ValueRef<> x = ins.operands[0];

			context->majorFrames.pop_back();
			context->majorFrames.back().returnValue = x;
			++context->majorFrames.back().curIns;
			break;
		}
		case Opcode::ACALL:
		case Opcode::AMCALL: {
			break;
		}
		case Opcode::YIELD: {
			_checkOperandCount(ins, 1);

			_unwrapRegOperand(ins.operands[0]);

			ValueRef<> x = ins.operands[0];

			context->flags |= CTX_YIELDED;
			curMajorFrame.returnValue = x;
			break;
		}
		case Opcode::AWAIT: {
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, 2);

			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::TYPENAME });

			Type &type = ((TypeNameValue *)*ins.operands[1])->_data;
			type.loadDeferredType(this);

			switch (type.typeId) {
				case TypeId::OBJECT:
				case TypeId::CLASS: {
					ClassValue *cls = (ClassValue *)*type.getCustomTypeExData();
					ObjectValue *instance = _newClassInstance(cls);
					((VarValue *)*ins.operands[0])->setData(instance);

					FnValue *constructor = (FnValue *)cls->getMember("new");
					if (constructor && constructor->getType() == TypeId::FN) {
						_callFn(context, constructor);
						context->majorFrames.back().thisObject->setData(instance);
						return;
					}
					break;
				}
				default:
					throw InvalidOperandsError("The type cannot be instantiated");
			}
			break;
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, 1);

			_unwrapRegOperand(ins.operands[1]);

			ValueRef<> x = ins.operands[0];

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

			_unwrapRegOperand(ins.operands[1]);

			_checkOperandType(ins, { TypeId::TYPENAME, TypeId::U32 });

			ExceptionHandler xh;

			((TypeNameValue *)*ins.operands[0])->_data.loadDeferredType(this);

			xh.type = ((TypeNameValue *)*ins.operands[0])->getData();
			xh.off = ((U32Value *)*ins.operands[1])->getData();

			curMajorFrame.minorFrames.back().exceptHandlers.push_back(xh);
			break;
		}
		case Opcode::ABORT:
			_checkOperandCount(ins, 0);
			throw UncaughtExceptionError("Use chose to abort the execution");
		case Opcode::CAST: {
			_checkOperandCount(ins, 3);

			_unwrapRegOperand(ins.operands[2]);

			_checkOperandType(ins, { TypeId::VAR, TypeId::TYPENAME, TypeId::ANY });

			auto t = ((TypeNameValue *)*ins.operands[1])->getData();

			ValueRef<> v = ins.operands[2];

			switch (t.typeId) {
				case TypeId::I8:
					v = _castToLiteralValue<int8_t>(this, *v);
					break;
				case TypeId::I16:
					v = _castToLiteralValue<int16_t>(this, *v);
					break;
				case TypeId::I32:
					v = _castToLiteralValue<int32_t>(this, *v);
					break;
				case TypeId::I64:
					v = _castToLiteralValue<int64_t>(this, *v);
					break;
				case TypeId::U8:
					v = _castToLiteralValue<uint8_t>(this, *v);
					break;
				case TypeId::U16:
					v = _castToLiteralValue<uint16_t>(this, *v);
					break;
				case TypeId::U32:
					v = _castToLiteralValue<uint32_t>(this, *v);
					break;
				case TypeId::U64:
					v = _castToLiteralValue<uint64_t>(this, *v);
					break;
				case TypeId::BOOL:
					v = _castToLiteralValue<bool>(this, *v);
					break;
				case TypeId::F32:
					v = _castToLiteralValue<float>(this, *v);
					break;
				case TypeId::F64:
					v = _castToLiteralValue<double>(this, *v);
					break;
				case TypeId::OBJECT:
					/* stub */
				default:
					throw InvalidOperandsError("Invalid target type");
			}

			((VarValue *)*ins.operands[0])->setData(*v);
			break;
		}
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((uint8_t)ins.opcode));
	}
	++curMajorFrame.curIns;
}
