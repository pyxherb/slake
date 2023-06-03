#include "../runtime.h"

using namespace Slake;

/// @brief Check if value of a value object is in range of specified type.
/// @tparam TD Destination type.
/// @tparam TS Source type.
/// @param v Input value object.
/// @return Converted value.
template <typename TD, typename TS>
static TD _checkOperandRange(ValueRef<> v) {
	auto value = (LiteralValue<TS, getValueType<TS>()> *)*v;
	if ((TD)value->getValue() > std::numeric_limits<TD>::max() ||
		(TD)value->getValue() < std::numeric_limits<TD>::min())
		throw InvalidOperandsError("Invalid operand value");
	return (TD)value->getValue();
}

static uint32_t _getOperandAsAddress(ValueRef<> v) {
	switch (v->getType().valueType) {
		case ValueType::I8:
			return _checkOperandRange<uint32_t, std::int8_t>(v);
		case ValueType::I16:
			return _checkOperandRange<uint32_t, std::int16_t>(v);
		case ValueType::I32:
			return _checkOperandRange<uint32_t, std::int32_t>(v);
		case ValueType::I64:
			return _checkOperandRange<uint32_t, std::int64_t>(v);
		case ValueType::U8:
			return _checkOperandRange<uint32_t, uint8_t>(v);
		case ValueType::U16:
			return _checkOperandRange<uint32_t, uint16_t>(v);
		case ValueType::U32:
			return ((U32Value *)*v)->getValue();
		case ValueType::U64:
			return _checkOperandRange<uint32_t, uint64_t>(v);
		default:
			throw InvalidOperandsError("Invalid operand combination");
	}
}

static void _checkOperandType(
	Instruction &ins,
	ValueType type0,
	ValueType type1 = ValueType::ANY,
	ValueType type2 = ValueType::ANY,
	ValueType type3 = ValueType::ANY) {
	if (ins.operands[0]->getType() != type0 ||
		(type1 != ValueType::ANY ? ins.operands[1]->getType() != type1 : false) ||
		(type2 != ValueType::ANY ? ins.operands[2]->getType() != type2 : false) ||
		(type3 != ValueType::ANY ? ins.operands[3]->getType() != type3 : false))
		throw InvalidOperandsError("Invalid operand combination");
}

static void _checkOperandCount(Instruction &ins, uint8_t n) {
	if (ins.getOperandCount() > n)
		throw InvalidOperandsError("Invalid operand count");
}

static void _checkOperandCount(Instruction &ins, uint8_t min, uint8_t max) {
	auto n = ins.getOperandCount();
	if (n < min || n > max)
		throw InvalidOperandsError("Invalid operand count");
}

template <typename LT>
static Value *_castToLiteralValue(Runtime *rt, Value *x) {
	using T = LiteralValue<LT, getValueType<LT>()>;
	if (getValueType<LT>() == x->getType().valueType)
		return x;
	switch (x->getType().valueType) {
		case ValueType::I8:
			return new T(rt, (LT)(((I8Value *)x)->getValue()));
		case ValueType::I16:
			return new T(rt, (LT)(((I16Value *)x)->getValue()));
		case ValueType::I32:
			return new T(rt, (LT)(((I32Value *)x)->getValue()));
		case ValueType::I64:
			return new T(rt, (LT)(((I64Value *)x)->getValue()));
		case ValueType::U8:
			return new T(rt, (LT)(((U8Value *)x)->getValue()));
		case ValueType::U16:
			return new T(rt, (LT)(((U16Value *)x)->getValue()));
		case ValueType::U32:
			return new T(rt, (LT)(((U32Value *)x)->getValue()));
		case ValueType::U64:
			return new T(rt, (LT)(((U64Value *)x)->getValue()));
		case ValueType::F32:
			return new T(rt, (LT)(((F32Value *)x)->getValue()));
		case ValueType::F64:
			return new T(rt, (LT)(((F64Value *)x)->getValue()));
		case ValueType::BOOL:
			return new T(rt, (LT)(((BoolValue *)x)->getValue()));
		default:
			throw IncompatibleTypeError("Invalid type conversion");
	}
}

template <typename T>
static Value *_execBinaryOp(ValueRef<> x, ValueRef<> y, Opcode opcode) {
	auto _x = (LiteralValue<T, getValueType<T>()> *)*x;
	auto rt = _x->getRuntime();

	if constexpr (std::is_arithmetic<T>::value) {
		if constexpr (std::is_same<T, bool>::value) {
			// Boolean
		} else {
			if (opcode == Opcode::LSH || opcode == Opcode::RSH) {
				if (y->getType() != ValueType::U32)
					throw InvalidOperandsError("Binary operation with incompatible types");
			} else if (_x->getType() != y->getType())
				throw InvalidOperandsError("Binary operation with incompatible types");

			switch (opcode) {
				case Opcode::ADD:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() + ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::SUB:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::MUL:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() * ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::DIV:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() / ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::MOD: {
					T result;
					if constexpr (std::is_same<T, float>::value)
						result = std::fmodf(_x->getValue(), ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
					else if constexpr (std::is_same<T, double>::value)
						result = std::fmod(_x->getValue(), ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
					else
						result = _x->getValue() % ((LiteralValue<T, getValueType<T>()> *)*y)->getValue();
					return new LiteralValue<T, getValueType<T>()>(rt, result);
				}
				case Opcode::AND:
					if constexpr (std::is_integral<T>::value)
						return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() & ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::OR:
					if constexpr (std::is_integral<T>::value)
						return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() | ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::XOR:
					if constexpr (std::is_integral<T>::value)
						return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() ^ ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::LAND:
					return new BoolValue(rt, _x->getValue() && ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::LOR:
					return new BoolValue(rt, _x->getValue() || ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::EQ:
					return new BoolValue(rt, _x->getValue() == ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::NEQ:
					return new BoolValue(rt, _x->getValue() != ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::LT:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() < ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::GT:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() > ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::LTEQ:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() <= ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::GTEQ:
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() >= ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
				case Opcode::LSH:
					if constexpr (std::is_integral<T>::value)
						return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() << ((U32Value *)*y)->getValue());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(uint32_t *)(&_x->getValue())) << ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(uint64_t *)(&_x->getValue())) << ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(double *)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::RSH:
					if constexpr (std::is_integral<T>::value)
						return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() >> ((U32Value *)*y)->getValue());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(uint32_t *)(&_x->getValue())) >> ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(uint64_t *)(&_x->getValue())) >> ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(double *)(&result));
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
				return new LiteralValue<T, ValueType::STRING>(rt, _x->getValue() + ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
			case Opcode::EQ:
				return new BoolValue(rt, _x->getValue() == ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
			case Opcode::NEQ:
				return new BoolValue(rt, _x->getValue() != ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	} else {
		switch (opcode) {
			case Opcode::EQ:
				return new BoolValue(rt, _x->getValue() == ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
			case Opcode::NEQ:
				return new BoolValue(rt, _x->getValue() != ((LiteralValue<T, getValueType<T>()> *)*y)->getValue());
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
					return new LiteralValue<T, getValueType<T>()>(rt, ~(_x->getValue()));
				else if constexpr (std::is_same<T, float>::value) {
					auto result = ~(*(uint32_t *)(rt, &_x->getValue()));
					return new LiteralValue<T, getValueType<T>()>(rt, *((float *)&result));
				} else if constexpr (std::is_same<T, double>::value) {
					auto result = ~(*(uint64_t *)(rt, &_x->getValue()));
					return new LiteralValue<T, getValueType<T>()>(rt, *((double *)&result));
				}
				throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::NOT:
				return new BoolValue(rt, !_x->getValue());
			case Opcode::INCF:
			case Opcode::INCB:
				if constexpr (std::is_integral<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() + 1);
				else if constexpr (std::is_same<T, float>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() + 1.0f);
				else if constexpr (std::is_same<T, double>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() + 1.0);
				throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::DECF:
			case Opcode::DECB:
				if constexpr (std::is_integral<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - 1);
				else if constexpr (std::is_same<T, float>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - 1.0f);
				else if constexpr (std::is_same<T, double>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - 1.0);
				throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::NEG:
				if constexpr (std::is_signed<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, -_x->getValue());
				else
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue());
		}
	}
	throw InvalidOperandsError("Binary operation with incompatible types");
}

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
ObjectValue *Slake::Runtime::_newClassInstance(ClassValue *cls) {
	ObjectValue *instance = new ObjectValue(this, (MemberValue *)cls);
	for (auto i : cls->_members) {
		switch (i.second->getType().valueType) {
			case ValueType::VAR: {
				ValueRef<VarValue> var = new VarValue(
					this,
					((VarValue *)i.second)->getAccess(),
					((VarValue *)i.second)->getVarType(),
					instance);

				// Set value of the variable with the initial value
				auto initValue = ((VarValue *)i.second)->getValue();
				if (initValue)
					var->setValue(*initValue);

				instance->addMember(
					i.first,
					*var);
				break;
			}
			case ValueType::FN: {
				if (!((FnValue *)i.second)->isStatic())
					instance->addMember(i.first, (MemberValue *)i.second);
				break;
			}
		}
	}
	return instance;
}

void Slake::Runtime::_callFn(Context *context, FnValue *fn) {
	auto frame = MajorFrame();
	frame.curIns = 0;
	frame.curFn = fn;
	frame.argStack = context->majorFrames.back().argStack;
	frame.thisObject = context->majorFrames.back().thisObject;

	context->majorFrames.back().argStack.clear();

	context->majorFrames.push_back(frame);
	return;
}

/// @brief Execute a single instruction.
/// @param context Context for execution.
/// @param ins Instruction to execute.
void Slake::Runtime::_execIns(Context *context, Instruction &ins) {
	while (_isInGc)
		std::this_thread::yield();

	auto &curMajorFrame = context->majorFrames.back();

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::PUSH:
			_checkOperandCount(ins, 1);
			curMajorFrame.push(ins.operands[0]);
			break;
		case Opcode::POP:
			_checkOperandCount(ins, 0);
			curMajorFrame.pop();
			break;
		case Opcode::LLOAD:
			_checkOperandCount(ins, 0, 1);
			curMajorFrame.push(curMajorFrame.lload(_getOperandAsAddress(
				ins.getOperandCount()
					? ins.operands[0]
					: curMajorFrame.pop())));
			break;
		case Opcode::LOAD:
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, ValueType::REF);
			curMajorFrame.push(resolveRef(ins.operands[0]));
			break;
		case Opcode::RLOAD: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, ValueType::REF);

			ValueRef<> v = curMajorFrame.pop();
			if (!v)
				throw InvalidOperandsError("Invalid operand combination");

			if (!(v = resolveRef((RefValue *)*(ins.operands[0]), *v))) {
				throw ResourceNotFoundError("No such resource");
			}
			curMajorFrame.push(v);
			break;
		}
		case Opcode::LSTORE: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<I32Value> x(nullptr);
			if (!ins.getOperandCount())
				x = (I32Value *)*(curMajorFrame.pop());
			else
				x = ins.operands[0];

			if (x->getType() != ValueType::I32)
				throw InvalidOperandsError("Invalid operand combination");

			curMajorFrame.dataStack.at(x->getValue()) = curMajorFrame.pop();
			break;
		}
		case Opcode::STORE: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = (VarValue *)*(curMajorFrame.pop());
			else
				x = ins.operands[0];

			if (!x)
				throw InvalidOperandsError("Invalid operand combination");

			if (x->getType() == ValueType::REF) {
				x = (VarValue *)resolveRef((RefValue *)*x, *(curMajorFrame.thisObject));

				if ((!x) || (x->getType() != ValueType::VAR)) {
					if ((!(x = (VarValue *)resolveRef((RefValue *)*x, nullptr))) || (x->getType() != ValueType::VAR))
						throw ResourceNotFoundError("No such variable");
				}

				if ((((VarValue *)*x)->getAccess() & ACCESS_CONST))
					throw AccessViolationError("Cannot store to a constant");
			}

			if (x->getType() != ValueType::VAR)
				throw InvalidOperandsError("Only can store to a variable");

			// TODO: Implement the type checker.
			((VarValue *)*x)->setValue(*curMajorFrame.pop());
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, 0);

			auto v = curMajorFrame.pop();
			if (v->getType() != ValueType::VAR)
				throw InvalidOperandsError("Invalid operand combination");
			curMajorFrame.push(((VarValue *)*v)->getValue());
			break;
		}
		case Opcode::EXPAND: {
			_checkOperandCount(ins, 0, 1);
			uint32_t n = _getOperandAsAddress(ins.operands[0]);
			curMajorFrame.expand(n);
			break;
		}
		case Opcode::SHRINK: {
			_checkOperandCount(ins, 0, 1);
			uint32_t n = _getOperandAsAddress(ins.operands[0]);
			curMajorFrame.shrink(n);
			break;
		}
		case Opcode::ENTER: {
			_checkOperandCount(ins, 1);
			MinorFrame frame;
			frame.exitOff = (uint32_t)_getOperandAsAddress(ins.operands[0]);
			frame.stackBase = (uint32_t)curMajorFrame.dataStack.size();
			curMajorFrame.minorFrames.push_back(frame);
			break;
		}
		case Opcode::LEAVE: {
			_checkOperandCount(ins, 0);
			if (curMajorFrame.minorFrames.size() < 2)
				throw FrameError("Leaving the only frame");
			curMajorFrame.dataStack.resize(curMajorFrame.minorFrames.back().stackBase);
			curMajorFrame.minorFrames.pop_back();
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
			_checkOperandCount(ins, 2);

			ValueRef<> x(nullptr), y(nullptr);
			if (!ins.getOperandCount())
				x = curMajorFrame.pop(), y = curMajorFrame.pop();
			else
				x = ins.operands[0], y = ins.operands[1];

			if (!x)
				throw NullRefError("");
			if (!y)
				throw NullRefError("");

			switch (x->getType().valueType) {
				case ValueType::I8:
					curMajorFrame.push(_execBinaryOp<std::int8_t>(x, y, ins.opcode));
					break;
				case ValueType::I16:
					curMajorFrame.push(_execBinaryOp<std::int16_t>(x, y, ins.opcode));
					break;
				case ValueType::I32:
					curMajorFrame.push(_execBinaryOp<std::int32_t>(x, y, ins.opcode));
					break;
				case ValueType::I64:
					curMajorFrame.push(_execBinaryOp<std::int64_t>(x, y, ins.opcode));
					break;
				case ValueType::U8:
					curMajorFrame.push(_execBinaryOp<uint8_t>(x, y, ins.opcode));
					break;
				case ValueType::U16:
					curMajorFrame.push(_execBinaryOp<uint16_t>(x, y, ins.opcode));
					break;
				case ValueType::U32:
					curMajorFrame.push(_execBinaryOp<uint32_t>(x, y, ins.opcode));
					break;
				case ValueType::U64:
					curMajorFrame.push(_execBinaryOp<uint64_t>(x, y, ins.opcode));
					break;
				case ValueType::F32:
					curMajorFrame.push(_execBinaryOp<float>(x, y, ins.opcode));
					break;
				case ValueType::F64:
					curMajorFrame.push(_execBinaryOp<double>(x, y, ins.opcode));
					break;
				case ValueType::STRING:
					curMajorFrame.push(_execBinaryOp<std::string>(x, y, ins.opcode));
					break;
				default:
					throw InvalidOperandsError("Invalid binary operation for operands");
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
			_checkOperandCount(ins, 1);

			ValueRef<> x;
			ValueRef<VarValue> v;

			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			if (!x)
				throw NullRefError("");

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
				case Opcode::INCF:
				case Opcode::DECF:
					if (x->getType() != ValueType::VAR)
						throw InvalidOperandsError("Invalid binary operation for operands");

					switch (ins.opcode) {
						case Opcode::INCB:
						case Opcode::DECB:
							curMajorFrame.push(((VarValue *)*x)->getValue());
					}

					v = x;
					x = v->getValue();
			}


			if (!x)
				throw NullRefError("");

			ValueRef<> value;
			switch (x->getType().valueType) {
				case ValueType::I8:
					value = _execUnaryOp<std::int8_t>(x, ins.opcode);
					break;
				case ValueType::I16:
					value = _execUnaryOp<std::int16_t>(x, ins.opcode);
					break;
				case ValueType::I32:
					value = _execUnaryOp<std::int32_t>(x, ins.opcode);
					break;
				case ValueType::I64:
					value = _execUnaryOp<std::int64_t>(x, ins.opcode);
					break;
				case ValueType::U8:
					value = _execUnaryOp<uint8_t>(x, ins.opcode);
					break;
				case ValueType::U16:
					value = _execUnaryOp<uint16_t>(x, ins.opcode);
					break;
				case ValueType::U32:
					value = _execUnaryOp<uint32_t>(x, ins.opcode);
					break;
				case ValueType::U64:
					value = _execUnaryOp<uint64_t>(x, ins.opcode);
					break;
				case ValueType::F32:
					value = _execUnaryOp<float>(x, ins.opcode);
					break;
				case ValueType::F64:
					value = _execUnaryOp<double>(x, ins.opcode);
					break;
				case ValueType::STRING:
					value = _execUnaryOp<std::string>(x, ins.opcode);
					break;
				default:
					throw InvalidOperandsError("Invalid binary operation for operands");
			}

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
					v->setValue(*value);
					break;
				case Opcode::INCF:
				case Opcode::DECF:
					v->setValue(*value);
				default:
					curMajorFrame.push(value);
			}
			break;
		}
		case Opcode::AT: {
			_checkOperandCount(ins, 1);

			ValueRef<> x, i;

			if (!ins.getOperandCount())
				i = curMajorFrame.pop();
			else
				i = ins.operands[0];

			x = curMajorFrame.pop();

			switch (x->getType().valueType) {
				case ValueType::ARRAY: {
					ArrayValue *_x = (ArrayValue *)*x;

					if (i->getType() != ValueType::U32)
						throw InvalidOperandsError("Invalid argument for subscription");

					auto index = ((I32Value *)*i)->getValue();
					if (_x->getSize() <= index)
						throw InvalidSubscriptionError("Out of array range");

					curMajorFrame.push((*_x)[index]);
					break;
				}
				case ValueType::MAP: {
					throw std::logic_error("Unimplemented yet");
					break;
				}
				case ValueType::OBJECT: {
					throw std::logic_error("Unimplemented yet");
					break;
				}
				default:
					throw InvalidOperandsError("Subscription was not supported by the operand");
			}
			break;
		}
		case Opcode::JMP: {
			_checkOperandCount(ins, 1);

			ValueRef<U32Value> x = ins.operands[0];
			if (x->getType() != ValueType::U32)
				throw InvalidOperandsError("Invalid operand type");

			curMajorFrame.curIns = x->getValue();
			break;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 1);

			ValueRef<U32Value> x = ins.operands[0];
			if (x->getType() != ValueType::U32)
				throw InvalidOperandsError("Invalid operand type");

			curMajorFrame.curIns = x->getValue();

			ValueRef<BoolValue> v = curMajorFrame.pop();
			if (v->getType() != ValueType::BOOL)
				throw InvalidOperandsError("Invalid operand type");

			if (v->getValue()) {
				if (ins.opcode == Opcode::JT)
					curMajorFrame.curIns = x->getValue();
			} else if (ins.opcode == Opcode::JF)
				curMajorFrame.curIns = x->getValue();
			break;
		} /*
		case Opcode::CAST: {
		}*/
		case Opcode::SARG: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x = ins.getOperandCount() ? ins.operands[0] : curMajorFrame.pop();
			curMajorFrame.argStack.push_back(x);
			break;
		}
		case Opcode::LARG: {
			_checkOperandCount(ins, 0, 1);
			ValueRef<> x = ins.getOperandCount() ? ins.operands[0] : curMajorFrame.pop();

			if (x->getType() != ValueType::U32)
				throw InvalidOperandsError("Invalid operand type");

			curMajorFrame.push(curMajorFrame.argStack.at(((U32Value *)*x)->getValue()));
			break;
		}
		case Opcode::LTHIS: {
			_checkOperandCount(ins, 0);

			curMajorFrame.push(curMajorFrame.thisObject);
			break;
		}
		case Opcode::STHIS: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			curMajorFrame.thisObject = x;
			break;
		}
		case Opcode::CALL: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			ValueRef<FnValue> fn = (FnValue *)*x;
			if (fn->getType() != ValueType::FN) {
				fn = (FnValue *)resolveRef((RefValue *)*x, *(curMajorFrame.scopeValue));
				if ((!fn) || (fn->getType() != ValueType::FN)) {
					auto fn = resolveRef((RefValue *)*x);

					if ((!fn) || (fn->getType() != ValueType::FN))
						throw ResourceNotFoundError("No such function or method");
				}
			}

			if (fn->isNative()) {
				((NativeFnValue *)*fn)->call((uint8_t)curMajorFrame.argStack.size(), &(curMajorFrame.argStack[0]));
				curMajorFrame.argStack.clear();
			} else {
				_callFn(context, *fn);
				return;
			}
			break;
		}
		case Opcode::ACALL: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			ValueRef<> fn = resolveRef((RefValue *)*x, *(curMajorFrame.scopeValue));
			if ((!fn) || (fn->getType() != ValueType::FN)) {
				ValueRef<> fn = resolveRef((RefValue *)*x);

				if ((!fn) || (fn->getType() != ValueType::FN))
					throw ResourceNotFoundError("No such method");
			}
			break;
		}
		case Opcode::RET: {
			_checkOperandCount(ins, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			context->majorFrames.pop_back();
			context->majorFrames.back().returnValue = x;
			context->majorFrames.back().curIns++;
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, 1);

			ValueRef<MemberValue> cls = (MemberValue *)resolveRef(ins.operands[0], *(curMajorFrame.scopeValue));
			if ((!cls) || cls->getType() != ValueType::CLASS) {
				cls = (MemberValue *)resolveRef(ins.operands[0]);
				if ((!cls) || cls->getType() != ValueType::CLASS) {
					throw ResourceNotFoundError("Class not found");
				}
			}

			auto instance = _newClassInstance((ClassValue *)*cls);
			curMajorFrame.push(instance);

			auto savedThis = curMajorFrame.thisObject;

			FnValue *constructor = (FnValue *)cls->getMember("new");
			if (constructor && constructor->getType() == ValueType::FN)
				_callFn(context, constructor);

			context->majorFrames.back().thisObject = instance;

			return;
		}
		case Opcode::LRET: {
			_checkOperandCount(ins, 0);

			curMajorFrame.push(curMajorFrame.returnValue);
			break;
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];


			break;
		}
		case Opcode::PUSHXH: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			curMajorFrame.minorFrames.back().exceptHandlers.push_back(((I32Value *)*x)->getValue());
			break;
		}
		case Opcode::ABORT:
			throw UncaughtExceptionError("Execution aborted");
		case Opcode::CASTI8: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<int8_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTI16: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<int16_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTI32: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<int32_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTI64: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<int64_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTU8: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<uint8_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTU16: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<uint16_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTU32: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<uint32_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTU64: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<uint64_t>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTF32: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<float>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTF64: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<float>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTBOOL: {
			_checkOperandCount(ins, 0);
			curMajorFrame.push(_castToLiteralValue<bool>(this, *curMajorFrame.pop()));
			break;
		}
		case Opcode::CASTOBJ: {
			ValueRef<> x, type;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[1];

			break;
		}
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((uint8_t)ins.opcode));
	}
	if (_szMemInUse > (_szMemUsedAfterLastGc << 1))
		gc();
	curMajorFrame.curIns++;
}

Value *Slake::Runtime::resolveRef(ValueRef<RefValue> ref, Value *v) {
	if (!ref)
		return nullptr;

	if (!v)
		v = _rootValue;
	for (auto i : ref->scopes) {
		if (!(v = v->getMember(i)))
			return nullptr;
	}
	return v;
}
