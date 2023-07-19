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

static uint32_t _getOperandAsAddress(ValueRef<> v) {
	switch (v->getType().typeId) {
		case TypeId::I8:
			return _checkOperandRange<uint32_t, std::int8_t>(v);
		case TypeId::I16:
			return _checkOperandRange<uint32_t, std::int16_t>(v);
		case TypeId::I32:
			return _checkOperandRange<uint32_t, std::int32_t>(v);
		case TypeId::I64:
			return _checkOperandRange<uint32_t, std::int64_t>(v);
		case TypeId::U8:
			return _checkOperandRange<uint32_t, uint8_t>(v);
		case TypeId::U16:
			return _checkOperandRange<uint32_t, uint16_t>(v);
		case TypeId::U32:
			return ((U32Value *)*v)->getData();
		case TypeId::U64:
			return _checkOperandRange<uint32_t, uint64_t>(v);
		default:
			throw InvalidOperandsError("Invalid operand combination");
	}
}

static void _checkOperandType(
	Instruction &ins,
	TypeId type0,
	TypeId type1 = TypeId::ANY,
	TypeId type2 = TypeId::ANY,
	TypeId type3 = TypeId::ANY) {
	if (ins.operands[0]->getType() != type0 ||
		(type1 != TypeId::ANY ? ins.operands[1]->getType() != type1 : false) ||
		(type2 != TypeId::ANY ? ins.operands[2]->getType() != type2 : false) ||
		(type3 != TypeId::ANY ? ins.operands[3]->getType() != type3 : false))
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
					return new V(rt, _x->getData() < ((V *)*y)->getData());
				case Opcode::GT:
					return new V(rt, _x->getData() > ((V *)*y)->getData());
				case Opcode::LTEQ:
					return new V(rt, _x->getData() <= ((V *)*y)->getData());
				case Opcode::GTEQ:
					return new V(rt, _x->getData() >= ((V *)*y)->getData());
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
	MajorFrame frame;
	frame.curIns = 0;
	frame.curFn = fn;
	frame.argStack.swap(curFrame.nextArgStack);

	context->majorFrames.push_back(frame);
	return;
}

VarValue *slake::Runtime::_addLocalVar(MajorFrame &frame, Type type) {
	auto v = new VarValue(this, ACCESS_PUB, type);
	frame.localVars.push_back(v);
	return v;
}

void slake::Runtime::_execIns(Context *context, Instruction &ins) {
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
		case Opcode::LVAR: {
			_checkOperandCount(ins, 0, 2);
			_checkOperandType(ins, TypeId::TYPENAME);

			auto &type = ((TypeNameValue*)*ins.operands[0])->_data;
			type.loadDeferredType(this);

			_addLocalVar(curMajorFrame, type);
			break;
		}
		case Opcode::LLOAD:
			_checkOperandCount(ins, 0, 1);
			curMajorFrame.push(curMajorFrame.lload(_getOperandAsAddress(
				ins.getOperandCount()
					? ins.operands[0]
					: curMajorFrame.pop())));
			break;
		case Opcode::LOAD: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, TypeId::REF);
			auto v = resolveRef(ins.operands[0], *curMajorFrame.thisObject);
			if (!v) {
				if (!(v = resolveRef(ins.operands[0], *curMajorFrame.scopeValue)))
					v = resolveRef(ins.operands[0]);
			}
			curMajorFrame.push(v);
			break;
		}
		case Opcode::RLOAD: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, TypeId::REF);

			ValueRef<> v = curMajorFrame.pop();
			if (!v)
				throw InvalidOperandsError("Invalid operand combination");

			if (!(v = resolveRef((RefValue *)*(ins.operands[0]), *v))) {
				throw NotFoundError("No such resource", ins.operands[0]);
			}
			curMajorFrame.push(v);
			break;
		}
		case Opcode::LSTORE: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<U32Value> x(nullptr);
			if (!ins.getOperandCount())
				x = (U32Value *)*(curMajorFrame.pop());
			else
				x = ins.operands[0];

			if (x->getType() != TypeId::U32)
				throw InvalidOperandsError("Invalid operand combination");

			if(x->getData()>=curMajorFrame.localVars.size())
				throw InvalidLocalVarIndexError("Invalid local variable index", x->getData());
			curMajorFrame.localVars.at(x->getData())->setData(*curMajorFrame.pop());
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

			if (x->getType() == TypeId::REF) {
				x = (VarValue *)resolveRef((RefValue *)*x, *(curMajorFrame.thisObject));

				if ((!x) || (x->getType() != TypeId::VAR)) {
					if ((!(x = (VarValue *)resolveRef((RefValue *)*x))) || (x->getType() != TypeId::VAR))
						throw NotFoundError("No such variable", x);
				}

				if ((((VarValue *)*x)->getAccess() & ACCESS_CONST))
					throw AccessViolationError("Cannot store to a constant");
			}

			if (x->getType() != TypeId::VAR)
				throw InvalidOperandsError("Only can store to a variable");

			// TODO: Implement the type checker.
			((VarValue *)*x)->setData(*curMajorFrame.pop());
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, 0);

			auto v = curMajorFrame.pop();
			if (v->getType() != TypeId::VAR)
				throw InvalidOperandsError("Invalid operand combination");
			curMajorFrame.push(((VarValue *)*v)->getData());
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

			switch (x->getType().typeId) {
				case TypeId::I8:
					curMajorFrame.push(_execBinaryOp<std::int8_t>(x, y, ins.opcode));
					break;
				case TypeId::I16:
					curMajorFrame.push(_execBinaryOp<std::int16_t>(x, y, ins.opcode));
					break;
				case TypeId::I32:
					curMajorFrame.push(_execBinaryOp<std::int32_t>(x, y, ins.opcode));
					break;
				case TypeId::I64:
					curMajorFrame.push(_execBinaryOp<std::int64_t>(x, y, ins.opcode));
					break;
				case TypeId::U8:
					curMajorFrame.push(_execBinaryOp<uint8_t>(x, y, ins.opcode));
					break;
				case TypeId::U16:
					curMajorFrame.push(_execBinaryOp<uint16_t>(x, y, ins.opcode));
					break;
				case TypeId::U32:
					curMajorFrame.push(_execBinaryOp<uint32_t>(x, y, ins.opcode));
					break;
				case TypeId::U64:
					curMajorFrame.push(_execBinaryOp<uint64_t>(x, y, ins.opcode));
					break;
				case TypeId::F32:
					curMajorFrame.push(_execBinaryOp<float>(x, y, ins.opcode));
					break;
				case TypeId::F64:
					curMajorFrame.push(_execBinaryOp<double>(x, y, ins.opcode));
					break;
				case TypeId::STRING:
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
					if (x->getType() != TypeId::VAR)
						throw InvalidOperandsError("Invalid binary operation for operands");

					switch (ins.opcode) {
						case Opcode::INCB:
						case Opcode::DECB:
							curMajorFrame.push(((VarValue *)*x)->getData());
					}

					v = x;
					x = v->getData();
			}

			if (!x)
				throw NullRefError("");

			ValueRef<> value;
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
					throw InvalidOperandsError("Invalid binary operation for operands");
			}

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
					v->setData(*value);
					break;
				case Opcode::INCF:
				case Opcode::DECF:
					v->setData(*value);
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

			switch (x->getType().typeId) {
				case TypeId::ARRAY: {
					ArrayValue *_x = (ArrayValue *)*x;

					if (i->getType() != TypeId::U32)
						throw InvalidOperandsError("Invalid argument for subscription");

					auto index = ((I32Value *)*i)->getData();
					if (_x->getSize() <= index)
						throw InvalidSubscriptionError("Out of array range");

					curMajorFrame.push((*_x)[index]);
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

			ValueRef<U32Value> x = ins.operands[0];
			if (x->getType() != TypeId::U32)
				throw InvalidOperandsError("Invalid operand type");

			curMajorFrame.curIns = x->getData();
			break;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 1);

			ValueRef<U32Value> x = ins.operands[0];
			if (x->getType() != TypeId::U32)
				throw InvalidOperandsError("Invalid operand type");

			curMajorFrame.curIns = x->getData();

			ValueRef<BoolValue> v = curMajorFrame.pop();
			if (v->getType() != TypeId::BOOL)
				throw InvalidOperandsError("Invalid operand type");

			if (v->getData()) {
				if (ins.opcode == Opcode::JT)
					curMajorFrame.curIns = x->getData();
			} else if (ins.opcode == Opcode::JF)
				curMajorFrame.curIns = x->getData();
			break;
		}
		case Opcode::PUSHARG: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x = ins.getOperandCount() ? ins.operands[0] : curMajorFrame.pop();
			curMajorFrame.nextArgStack.push_back(x);
			break;
		}
		case Opcode::LARG: {
			_checkOperandCount(ins, 0, 1);
			ValueRef<> x = ins.getOperandCount() ? ins.operands[0] : curMajorFrame.pop();

			if (x->getType() != TypeId::U32)
				throw InvalidOperandsError("Invalid operand type");

			curMajorFrame.push(curMajorFrame.argStack.at(((U32Value *)*x)->getData()));
			break;
		}
		case Opcode::LTHIS: {
			_checkOperandCount(ins, 0);

			curMajorFrame.push(curMajorFrame.thisObject);
			break;
		}
		case Opcode::MCALL:
		case Opcode::CALL: {
			_checkOperandCount(ins, 0, 1);

			ValueRef<> x;
			if (!ins.getOperandCount())
				x = curMajorFrame.pop();
			else
				x = ins.operands[0];

			ValueRef<FnValue> fn = (FnValue *)*x;
			if (fn->getType() != TypeId::FN) {
				fn = (FnValue *)resolveRef((RefValue *)*x, *(curMajorFrame.thisObject));
				if ((!fn) || (fn->getType() != TypeId::FN)) {
					fn = (FnValue *)resolveRef((RefValue *)*x, *(curMajorFrame.scopeValue));
					if ((!fn) || (fn->getType() != TypeId::FN)) {
						auto fn = resolveRef((RefValue *)*x);

						if ((!fn) || (fn->getType() != TypeId::FN))
							throw NotFoundError("No such function or method", x);
					}
				}
			}

			if (fn->isNative()) {
				if (ins.opcode == Opcode::MCALL)
					curMajorFrame.scopeValue = (curMajorFrame.thisObject = curMajorFrame.pop());

				curMajorFrame.returnValue = ((NativeFnValue *)*fn)->call((uint8_t)curMajorFrame.nextArgStack.size(), curMajorFrame.nextArgStack.size() ? &(curMajorFrame.nextArgStack[0]) : nullptr);
				curMajorFrame.nextArgStack.clear();
			} else {
				auto v = curMajorFrame.pop();

				_callFn(context, *fn);

				auto &newCurFrame = context->getCurFrame();

				if (ins.opcode == Opcode::MCALL)
					newCurFrame.scopeValue = (newCurFrame.thisObject = v);
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
			if ((!fn) || (fn->getType() != TypeId::FN)) {
				ValueRef<> fn = resolveRef((RefValue *)*x);

				if ((!fn) || (fn->getType() != TypeId::FN))
					throw NotFoundError("No such method", x);
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
			++context->majorFrames.back().curIns;
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, TypeId::TYPENAME);

			Type &type = ((TypeNameValue*)*ins.operands[0])->_data;
			type.loadDeferredType(this);

			switch (type.typeId) {
				case TypeId::OBJECT:
				case TypeId::CLASS: {
					ClassValue* cls = (ClassValue *)*type.getCustomTypeExData();
					ObjectValue *instance = _newClassInstance(cls);
					curMajorFrame.push(instance);

					FnValue *constructor = (FnValue *)cls->getMember("new");
					if (constructor && constructor->getType() == TypeId::FN) {
						_callFn(context, constructor);
						context->majorFrames.back().thisObject = instance;
						return;
					}
					break;
				}
				case TypeId::STRUCT: {
					// TODO:Implement it
					break;
				}
				default:
					throw InvalidOperandsError("Type cannot be instantiated");
			}
			break;
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

			curMajorFrame.minorFrames.back().exceptHandlers.push_back(((I32Value *)*x)->getData());
			break;
		}
		case Opcode::ABORT:
			throw UncaughtExceptionError("Execution aborted");
		case Opcode::CAST: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, TypeId::TYPENAME);

			auto t = ((TypeNameValue*)*ins.operands[0])->getData();

			ValueRef<> v;

			switch(t.typeId) {
				case TypeId::I8:
					v = _castToLiteralValue<int8_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::I16:
					v = _castToLiteralValue<int16_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::I32:
					v = _castToLiteralValue<int32_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::I64:
					v = _castToLiteralValue<int64_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::U8:
					v = _castToLiteralValue<uint8_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::U16:
					v = _castToLiteralValue<uint16_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::U32:
					v = _castToLiteralValue<uint32_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::U64:
					v = _castToLiteralValue<uint64_t>(this, *curMajorFrame.pop());
					break;
				case TypeId::BOOL:
					v = _castToLiteralValue<bool>(this, *curMajorFrame.pop());
					break;
				case TypeId::F32:
					v = _castToLiteralValue<float>(this, *curMajorFrame.pop());
					break;
				case TypeId::F64:
					v = _castToLiteralValue<double>(this, *curMajorFrame.pop());
					break;
				case TypeId::OBJECT:
					/* stub */
				default:
					throw InvalidOperandsError("Invalid target type");
			}

			curMajorFrame.push(v);
			break;
		}
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((uint8_t)ins.opcode));
	}
	++curMajorFrame.curIns;
}
