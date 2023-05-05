#include "../runtime.h"

#pragma clang diagnostic ignored "-Wc++17-extensions"

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

static std::uint32_t _getOperandAsAddress(ValueRef<> v) {
	switch (v->getType().valueType) {
		case ValueType::I8:
			return _checkOperandRange<std::uint32_t, std::int8_t>(v);
		case ValueType::I16:
			return _checkOperandRange<std::uint32_t, std::int16_t>(v);
		case ValueType::I32:
			return _checkOperandRange<std::uint32_t, std::int32_t>(v);
		case ValueType::I64:
			return _checkOperandRange<std::uint32_t, std::int64_t>(v);
		case ValueType::U8:
			return _checkOperandRange<std::uint32_t, std::uint8_t>(v);
		case ValueType::U16:
			return _checkOperandRange<std::uint32_t, std::uint16_t>(v);
		case ValueType::U32:
			return ((U32Value *)*v)->getValue();
		case ValueType::U64:
			return _checkOperandRange<std::uint32_t, std::uint64_t>(v);
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

static void _checkOperandCount(Instruction &ins, std::uint8_t n) {
	if (ins.getOperandCount() > n)
		throw InvalidOperandsError("Invalid operand count");
}

static void _checkOperandCount(Instruction &ins, std::uint8_t min, std::uint8_t max) {
	auto n = ins.getOperandCount();
	if (n < min || n > max)
		throw InvalidOperandsError("Invalid operand count");
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
						auto result = (*(std::uint32_t *)(&_x->getValue())) << ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(std::uint64_t *)(&_x->getValue())) << ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(double *)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::RSH:
					if constexpr (std::is_integral<T>::value)
						return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() >> ((U32Value *)*y)->getValue());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(std::uint32_t *)(&_x->getValue())) >> ((U32Value *)*y)->getValue();
						return new LiteralValue<T, getValueType<T>()>(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(std::uint64_t *)(&_x->getValue())) >> ((U32Value *)*y)->getValue();
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
					auto result = ~(*(std::uint32_t *)(rt, &_x->getValue()));
					return new LiteralValue<T, getValueType<T>()>(rt, *((float *)&result));
				} else if constexpr (std::is_same<T, double>::value) {
					auto result = ~(*(std::uint64_t *)(rt, &_x->getValue()));
					return new LiteralValue<T, getValueType<T>()>(rt, *((double *)&result));
				} else
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
				else
					throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::DECF:
			case Opcode::DECB:
				if constexpr (std::is_integral<T>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - 1);
				else if constexpr (std::is_same<T, float>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - 1.0f);
				else if constexpr (std::is_same<T, double>::value)
					return new LiteralValue<T, getValueType<T>()>(rt, _x->getValue() - 1.0);
				else
					throw InvalidOperandsError("Binary operation with incompatible types");
			case Opcode::NEG:
				return new LiteralValue<T, getValueType<T>()>(rt, -_x->getValue());
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
				auto newVar = new VarValue(
					this,
					((VarValue *)*(i.second))->getAccess(),
					((VarValue *)*(i.second))->getVarType(), instance);
				instance->addMember(i.first, newVar);
				break;
			}
			case ValueType::FN: {
				if (!((FnValue *)*(i.second))->isStatic())
					instance->addMember(i.first, (MemberValue *)*(i.second));
				break;
			}
		}
	}
	return instance;
}

void Slake::Runtime::_callFn(Context *context, FnValue *fn) {
	if (fn->isNative()) {
		context->retValue = fn->call((uint8_t)context->execContext.args.size(), &context->execContext.args.front());
	} else {
		context->callingStack.push_back(context->execContext);

		context->execContext.curIns = 0;
		context->execContext.fn = fn;
		context->frames.push_back(Frame(context->stackBase, context->frames.back().exitOff));
		return;
	}
}

/// @brief Execute a single instruction.
/// @param context Context for execution.
/// @param ins Instruction to execute.
void Slake::Runtime::_execIns(Context *context, Instruction &ins) {
	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::PUSH:
			_checkOperandCount(ins, 1);
			context->dataStack.push_back(ins.operands[0]);
			break;
		case Opcode::POP:
			_checkOperandCount(ins, 0);
			context->dataStack.pop_back();
			break;
		case Opcode::LLOAD:
		case Opcode::LOAD:
			if (ins.opcode == Opcode::LLOAD) {
				_checkOperandCount(ins, 0, 1);
				auto off = _getOperandAsAddress(ins.getOperandCount() ? ins.operands[0] : context->pop());
				context->push(context->lload(off));
			} else {
				_checkOperandCount(ins, 1);
				_checkOperandType(ins, ValueType::REF);
				context->push(resolveRef(ins.operands[0]));
			}
			break;
		case Opcode::RLOAD: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, ValueType::REF);

			ValueRef<> v = context->pop();
			if (!v)
				throw InvalidOperandsError("Invalid operand combination");

			if (!(v = resolveRef((RefValue *)*(ins.operands[0]), *v)))
				throw ResourceNotFoundError("No such resource");
			context->push(v);
			break;
		}
		case Opcode::LSTORE: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<I32Value> x(nullptr);
			if (!ins.getOperandCount())
				x = (I32Value *)*(context->pop());
			else
				x = ins.operands[0];

			if (x->getType() != ValueType::I32)
				throw InvalidOperandsError("Invalid operand combination");

			context->dataStack[context->frames.back().stackBase] = context->pop();
			break;
		}
		case Opcode::STORE: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = (VarValue *)*(context->pop());
			else
				x = ins.operands[0];

			if (!x)
				throw InvalidOperandsError("Invalid operand combination");

			if (x->getType() == ValueType::REF) {
				x = (VarValue *)resolveRef((RefValue *)*x, *(context->execContext.pThis));

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
			((VarValue *)*x)->setValue(*context->pop());
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, 0);

			auto v = context->pop();
			if (v->getType() != ValueType::VAR)
				throw InvalidOperandsError("Invalid operand combination");
			context->push(((VarValue *)*v)->getValue());
			break;
		}
		case Opcode::EXPAND: {
			_checkOperandCount(ins, 0, 1);
			std::uint32_t n = _getOperandAsAddress(ins.operands[0]);
			context->expand(n);
			break;
		}
		case Opcode::SHRINK: {
			_checkOperandCount(ins, 0, 1);
			std::uint32_t n = _getOperandAsAddress(ins.operands[0]);
			context->shrink(n);
			break;
		}
		case Opcode::ENTER: {
			_checkOperandCount(ins, 1);
			context->frames.push_back(Frame(context->dataStack.size(), _getOperandAsAddress(ins.operands[0])));
			break;
		}
		case Opcode::LEAVE: {
			_checkOperandCount(ins, 0);
			if (context->frames.size() < 2)
				throw FrameError("Trying to leave the only frame");
			context->dataStack.resize(context->frames.back().stackBase);
			context->frames.pop_back();
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
				x = context->pop(), y = context->pop();
			else
				x = ins.operands[0], y = ins.operands[1];

			if (!x)
				throw NullRefError("");
			if (!y)
				throw NullRefError("");

			switch (x->getType().valueType) {
				case ValueType::I8:
					context->push(_execBinaryOp<std::int8_t>(x, y, ins.opcode));
					break;
				case ValueType::I16:
					context->push(_execBinaryOp<std::int16_t>(x, y, ins.opcode));
					break;
				case ValueType::I32:
					context->push(_execBinaryOp<std::int32_t>(x, y, ins.opcode));
					break;
				case ValueType::I64:
					context->push(_execBinaryOp<std::int64_t>(x, y, ins.opcode));
					break;
				case ValueType::U8:
					context->push(_execBinaryOp<std::uint8_t>(x, y, ins.opcode));
					break;
				case ValueType::U16:
					context->push(_execBinaryOp<std::uint16_t>(x, y, ins.opcode));
					break;
				case ValueType::U32:
					context->push(_execBinaryOp<std::uint32_t>(x, y, ins.opcode));
					break;
				case ValueType::U64:
					context->push(_execBinaryOp<std::uint64_t>(x, y, ins.opcode));
					break;
				case ValueType::FLOAT:
					context->push(_execBinaryOp<float>(x, y, ins.opcode));
					break;
				case ValueType::DOUBLE:
					context->push(_execBinaryOp<double>(x, y, ins.opcode));
					break;
				case ValueType::STRING:
					context->push(_execBinaryOp<std::string>(x, y, ins.opcode));
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
				x = context->pop();
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
							context->push(((VarValue *)*x)->getValue());
					}

					v = x;
					x = v->getValue();
			}

			Value *value;
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
					value = _execUnaryOp<std::uint8_t>(x, ins.opcode);
					break;
				case ValueType::U16:
					value = _execUnaryOp<std::uint16_t>(x, ins.opcode);
					break;
				case ValueType::U32:
					value = _execUnaryOp<std::uint32_t>(x, ins.opcode);
					break;
				case ValueType::U64:
					value = _execUnaryOp<std::uint64_t>(x, ins.opcode);
					break;
				case ValueType::FLOAT:
					value = _execUnaryOp<float>(x, ins.opcode);
					break;
				case ValueType::DOUBLE:
					value = _execUnaryOp<double>(x, ins.opcode);
					break;
				case ValueType::STRING:
					value = _execUnaryOp<std::string>(x, ins.opcode);
					break;
				default:
					throw InvalidOperandsError("Invalid binary operation for operands");
			}
			context->push(value);

			if (v)
				v->setValue(value);
			break;
		}
		case Opcode::JMP: {
			_checkOperandCount(ins, 1);

			ValueRef<U32Value> x = ins.operands[0];
			if (x->getType() != ValueType::U32)
				throw InvalidOperandsError("Invalid operand type");

			context->execContext.curIns = x->getValue();
			break;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 1);

			ValueRef<U32Value> x = ins.operands[0];
			if (x->getType() != ValueType::U32)
				throw InvalidOperandsError("Invalid operand type");

			context->execContext.curIns = x->getValue();

			ValueRef<BoolValue> v = context->pop();
			if (v->getType() != ValueType::BOOL)
				throw InvalidOperandsError("Invalid operand type");

			if (v->getValue()) {
				if (ins.opcode == Opcode::JT)
					context->execContext.curIns = x->getValue();
			} else if (ins.opcode == Opcode::JF)
				context->execContext.curIns = x->getValue();
			break;
		} /*
		case Opcode::CAST: {
		}*/
		case Opcode::SARG: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x = ins.getOperandCount() ? ins.operands[0] : context->pop();
			context->execContext.args.push_back(x);
			break;
		}
		case Opcode::LARG: {
			_checkOperandCount(ins, 0, 1);
			ValueRef<> x = ins.getOperandCount() ? ins.operands[0] : context->pop();

			if (x->getType() != ValueType::U32)
				throw InvalidOperandsError("Invalid operand type");

			context->push(context->execContext.args.at(((U32Value *)*x)->getValue()));
			break;
		}
		case Opcode::LTHIS: {
			_checkOperandCount(ins, 0);

			context->push(context->execContext.pThis);
			break;
		}
		case Opcode::STHIS: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = context->pop();
			else
				x = ins.operands[0];

			context->execContext.pThis = x;
			break;
		}
		case Opcode::CALL: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = context->pop();
			else
				x = ins.operands[0];

			FnValue *fn = (FnValue *)*x;
			if (fn->getType() != ValueType::FN) {
				fn = (FnValue *)resolveRef((RefValue *)*x, *(context->execContext.scopeValue));
				if ((!fn) || (fn->getType() != ValueType::FN)) {
					auto fn = resolveRef((RefValue *)*x);

					if ((!fn) || (fn->getType() != ValueType::FN))
						throw ResourceNotFoundError("No such function or method");
				}
			}

			_callFn(context, fn);
			return;
		}
		case Opcode::ACALL: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = context->pop();
			else
				x = ins.operands[0];

			auto fn = resolveRef((RefValue *)*x, *(context->execContext.scopeValue));
			if ((!fn) || (fn->getType() != ValueType::FN)) {
				auto fn = resolveRef((RefValue *)*x);

				if ((!fn) || (fn->getType() != ValueType::FN))
					throw ResourceNotFoundError("No such method");
			}
			break;
		}
		case Opcode::RET: {
			_checkOperandCount(ins, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = context->pop();
			else
				x = ins.operands[0];

			context->retValue = x;

			context->execContext = context->callingStack.back();
			context->callingStack.pop_back();
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, 1);

			MemberValue *cls = (MemberValue *)resolveRef(ins.operands[0], *(context->execContext.scopeValue));
			if ((!cls) || cls->getType() != ValueType::CLASS) {
				cls = (MemberValue *)resolveRef(ins.operands[0]);
				if ((!cls) || cls->getType() != ValueType::CLASS) {
					throw ResourceNotFoundError("Class not found");
				}
			}

			auto instance = _newClassInstance((ClassValue *)cls);
			context->push(instance);

			auto savedThis = context->execContext.pThis;

			FnValue *constructor = (FnValue *)cls->getMember("new");
			if (constructor && constructor->getType() == ValueType::FN)
				_callFn(context, constructor);

			context->execContext.pThis = instance;

			return;
		}
		case Opcode::LRET: {
			_checkOperandCount(ins, 0);

			context->dataStack.push_back(context->retValue);
			break;
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, 0, 1);
			break;
		}
		case Opcode::PUSHXH: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = context->pop();
			else
				x = ins.operands[0];

			context->frames.back().exceptHandlers.push_back(((I32Value *)*x)->getValue());
			break;
		}
		case Opcode::ABORT:
			throw UncaughtExceptionError("Execution aborted");
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((std::uint8_t)ins.opcode));
	}
	context->execContext.curIns++;
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
