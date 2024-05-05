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
	auto value = (LiteralValue<TS, getValueType<TS>()> *)v.get();
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

		if (type == TypeId::Any)
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
		case TypeId::Bool:
			return new T(rt, (LT)(((BoolValue *)x)->getData()));
		default:
			throw IncompatibleTypeError("Invalid type conversion");
	}
}

template <typename T>
static Value *_execBinaryOp(ValueRef<> x, ValueRef<> y, Opcode opcode) {
	// Corresponding value type
	using V = LiteralValue<T, getValueType<T>()>;

	auto _x = (V *)x.get();
	auto rt = _x->getRuntime();

	if constexpr (std::is_arithmetic<T>::value) {
		if constexpr (std::is_same<T, bool>::value) {
			// Boolean
			switch (opcode) {
				case Opcode::LAND: {
					return new V(
						rt,
						_x->getData() && ((V *)y.get())->getData());
				}
				case Opcode::LOR: {
					return new V(rt, _x->getData() || ((V *)y.get())->getData());
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
					return new V(rt, _x->getData() + ((V *)y.get())->getData());
				case Opcode::SUB:
					return new V(rt, _x->getData() - ((V *)y.get())->getData());
				case Opcode::MUL:
					return new V(rt, _x->getData() * ((V *)y.get())->getData());
				case Opcode::DIV:
					return new V(rt, _x->getData() / ((V *)y.get())->getData());
				case Opcode::MOD: {
					T result;
					if constexpr (std::is_same<T, float>::value)
						result = fmodf(_x->getData(), ((V *)y.get())->getData());
					else if constexpr (std::is_same<T, double>::value)
						result = fmod(_x->getData(), ((V *)y.get())->getData());
					else
						result = _x->getData() % ((V *)y.get())->getData();
					return new V(rt, result);
				}
				case Opcode::AND:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() & ((V *)y.get())->getData());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::OR:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() | ((V *)y.get())->getData());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::XOR:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() ^ ((V *)y.get())->getData());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::LAND:
					return new BoolValue(rt, _x->getData() && ((V *)y.get())->getData());
				case Opcode::LOR:
					return new BoolValue(rt, _x->getData() || ((V *)y.get())->getData());
				case Opcode::EQ:
					return new BoolValue(rt, _x->getData() == ((V *)y.get())->getData());
				case Opcode::NEQ:
					return new BoolValue(rt, _x->getData() != ((V *)y.get())->getData());
				case Opcode::LT:
					return new BoolValue(rt, _x->getData() < ((V *)y.get())->getData());
				case Opcode::GT:
					return new BoolValue(rt, _x->getData() > ((V *)y.get())->getData());
				case Opcode::LTEQ:
					return new BoolValue(rt, _x->getData() <= ((V *)y.get())->getData());
				case Opcode::GTEQ:
					return new BoolValue(rt, _x->getData() >= ((V *)y.get())->getData());
				case Opcode::LSH:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() << ((U32Value *)y.get())->getData());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(uint32_t *)(&_x->getData())) << ((U32Value *)y.get())->getData();
						return new V(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(uint64_t *)(&_x->getData())) << ((U32Value *)y.get())->getData();
						return new V(rt, *(double *)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::RSH:
					if constexpr (std::is_integral<T>::value)
						return new V(rt, _x->getData() >> ((U32Value *)y.get())->getData());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(uint32_t *)(&_x->getData())) >> ((U32Value *)y.get())->getData();
						return new V(rt, *(float *)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(uint64_t *)(&_x->getData())) >> ((U32Value *)y.get())->getData();
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
				return new LiteralValue<T, TypeId::String>(rt, _x->getData() + ((V *)y.get())->getData());
			case Opcode::EQ:
				return new BoolValue(rt, _x->getData() == ((V *)y.get())->getData());
			case Opcode::NEQ:
				return new BoolValue(rt, _x->getData() != ((V *)y.get())->getData());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	} else {
		switch (opcode) {
			case Opcode::EQ:
				return new BoolValue(rt, _x->getData() == ((V *)y.get())->getData());
			case Opcode::NEQ:
				return new BoolValue(rt, _x->getData() != ((V *)y.get())->getData());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	}
}

template <typename T>
static Value *_execUnaryOp(ValueRef<> x, Opcode opcode) {
	auto _x = (LiteralValue<T, getValueType<T>()> *)x.get();
	auto rt = _x->getRuntime();

	if constexpr (std::is_arithmetic<T>::value) {
		switch (opcode) {
			case Opcode::NOT:
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
			case Opcode::LNOT:
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
		VarValue *argVar = new VarValue(this, ACCESS_PUB, i < fn->paramTypes.size() ? fn->paramTypes[i] : TypeId::Any);
		argVar->setData(curFrame.nextArgStack[i]);
		frame.argStack.push_back(argVar);
	}

	curFrame.nextArgStack.clear();
	context->majorFrames.push_back(frame);
	return;
}

VarValue *slake::Runtime::_addLocalVar(MajorFrame &frame, Type type) {
	auto v = new VarValue(this, ACCESS_PUB, type);
	frame.localVars.push_back(v);
	return v;
}

VarValue *slake::Runtime::_addLocalReg(MajorFrame &frame) {
	auto v = new VarValue(this, ACCESS_PUB, TypeId::Any);
	frame.regs.push_back(v);
	return v;
}

void slake::Runtime::_execIns(Context *context, Instruction ins) {
	auto &curMajorFrame = context->majorFrames.back();
	auto &curMinorFrame = curMajorFrame.minorFrames.back();

	for (auto &i : ins.operands) {
		if (!i)
			continue;

		bool unwrapValue = false;
		switch (i->getType().typeId) {
			case TypeId::LocalVarRef: {
				unwrapValue = ((LocalVarRefValue *)i)->unwrapValue;

				auto index = ((LocalVarRefValue *)i)->index;

				if (index >= curMajorFrame.localVars.size())
					throw InvalidLocalVarIndexError("Invalid local variable index", index);

				i = curMajorFrame.localVars.at(index);
				break;
			}
			case TypeId::RegRef: {
				unwrapValue = ((RegRefValue *)i)->unwrapValue;

				auto index = ((RegRefValue *)i)->index;

				if (index >= curMajorFrame.regs.size())
					throw InvalidRegisterIndexError("Invalid register index", index);

				i = curMajorFrame.regs.at(index);
				break;
			}
			case TypeId::ArgRef:
				unwrapValue = ((ArgRefValue *)i)->unwrapValue;

				i = curMajorFrame.argStack[((ArgRefValue *)i)->index];
				break;
		}

		if (unwrapValue)
			i = ((VarValue *)i)->getData();
	}

	switch (ins.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LVAR: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { TypeId::TypeName });

			auto &type = ((TypeNameValue *)ins.operands[0])->_data;
			type.loadDeferredType(this);

			_addLocalVar(curMajorFrame, type);
			break;
		}
		case Opcode::REG: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { TypeId::U32 });

			uint32_t times = ((U32Value *)ins.operands[0])->getData();
			while (times--)
				_addLocalReg(curMajorFrame);
			break;
		}
		case Opcode::PUSH:
			_checkOperandCount(ins, 1);
			curMinorFrame.push(ins.operands[0]);
			break;
		case Opcode::POP:
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { TypeId::Var });

			((VarValue *)ins.operands[0])->setData(curMinorFrame.pop().get());
			break;
		case Opcode::LOAD: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { TypeId::Var, TypeId::Ref });

			RefValue *ref = (RefValue *)ins.operands[1];
			auto v = resolveRef(ref, curMajorFrame.thisObject);
			if (!v) {
				if (!(v = resolveRef(ref, curMajorFrame.scopeValue)))
					v = resolveRef(ref);
			}

			if (!v)
				throw NotFoundError("No such member", ref);
			((VarValue *)ins.operands[0])->setData(v);
			break;
		}
		case Opcode::RLOAD: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { TypeId::Var, TypeId::Any, TypeId::Ref });

			auto v = ins.operands[1];
			if (!v)
				throw NullRefError();

			if (!(v = resolveRef((RefValue *)ins.operands[2], v))) {
				throw NotFoundError("Member not found", (RefValue *)ins.operands[2]);
			}
			((VarValue *)ins.operands[0])->setData(v);
			break;
		}
		case Opcode::STORE: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { TypeId::Var, TypeId::Any });

			VarValue *x = (VarValue *)ins.operands[0];

			if (!x)
				throw NullRefError();

			x->setData(ins.operands[1]);
			break;
		}
		case Opcode::LVALUE: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { TypeId::Var, TypeId::Var });

			((VarValue *)ins.operands[0])->setData(((VarValue *)ins.operands[1])->getData());
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
		case Opcode::GTEQ:
		case Opcode::LSH: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { TypeId::Var, TypeId::Any, TypeId::Any });

			ValueRef<> x(ins.operands[1]), y(ins.operands[2]);
			ValueRef<VarValue> out((VarValue *)ins.operands[0]);

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
				case TypeId::String:
					out->setData(_execBinaryOp<std::string>(x, y, ins.opcode));
					break;
				case TypeId::Bool:
					out->setData(_execBinaryOp<bool>(x, y, ins.opcode));
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
		case Opcode::NOT:
		case Opcode::LNOT:
		case Opcode::NEG: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { TypeId::Var, TypeId::Any });

			Value *x = ins.operands[1];	 // Value of the operand.
			VarValue *varIn, *varOut = (VarValue *)ins.operands[0];

			if (!(x && varOut))
				throw NullRefError();

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
				case Opcode::INCF:
				case Opcode::DECF:
					if (x->getType() != TypeId::Var)
						throw InvalidOperandsError("Invalid operand combination");

					switch (ins.opcode) {
						case Opcode::INCB:
						case Opcode::DECB:
							((VarValue *)varOut)->setData(((VarValue *)x)->getData());
							break;
					}

					varIn = (VarValue *)x;
					x = varIn->getData();
					break;
			}

			ValueRef<> value;
			if (!x)
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
				case TypeId::String:
					value = _execUnaryOp<std::string>(x, ins.opcode);
					break;
				default:
					throw InvalidOperandsError("Invalid operand combination");
			}

			switch (ins.opcode) {
				case Opcode::INCB:
				case Opcode::DECB:
					varIn->setData(value.get());
					break;
				case Opcode::INCF:
				case Opcode::DECF:
					varIn->setData(value.get());
					break;
				default:
					((VarValue *)varOut)->setData(value.get());
			}
			break;
		}
		case Opcode::AT: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { TypeId::Var, TypeId::Any, TypeId::Any });

			Value *x = ins.operands[1], *i = ins.operands[2];

			switch (x->getType().typeId) {
				case TypeId::Array: {
					ArrayValue *array = (ArrayValue *)x;

					if (i->getType() != TypeId::U32)
						throw InvalidOperandsError("Invalid argument for subscription");

					auto index = ((I32Value *)i)->getData();
					if (array->values.size() <= index)
						throw InvalidSubscriptionError("Out of array range");

					((VarValue *)ins.operands[0])->setData(array->values[index]);
					break;
				}
				case TypeId::Object: {
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

			curMajorFrame.curIns = ((U32Value *)ins.operands[0])->getData();
			return;
		}
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 2);

			_checkOperandType(ins, { TypeId::U32, TypeId::Bool });

			if (((BoolValue *)ins.operands[1])->getData()) {
				if (ins.opcode == Opcode::JT) {
					curMajorFrame.curIns = ((U32Value *)ins.operands[0])->getData();
					return;
				}
			} else if (ins.opcode == Opcode::JF) {
				curMajorFrame.curIns = ((U32Value *)ins.operands[0])->getData();
				return;
			}

			break;
		}
		case Opcode::PUSHARG: {
			_checkOperandCount(ins, 1);

			curMajorFrame.nextArgStack.push_back(ins.operands[0]);
			break;
		}
		case Opcode::MCALL:
		case Opcode::CALL: {
			if (ins.opcode == Opcode::MCALL) {
				_checkOperandCount(ins, 2);

				_checkOperandType(ins, { TypeId::Fn, TypeId::Any });
			} else {
				_checkOperandCount(ins, 1);

				_checkOperandType(ins, { TypeId::Fn });
			}

			FnValue *fn = (FnValue *)ins.operands[0];

			if (!fn)
				throw NullRefError();

			if (fn->isNative()) {
				curMajorFrame.returnValue = ((NativeFnValue *)fn)->call(ins.opcode == Opcode::MCALL ? (curMajorFrame.scopeValue = ins.operands[1]) : nullptr, curMajorFrame.nextArgStack).get();
				curMajorFrame.nextArgStack.clear();
			} else {
				_callFn(context, fn);

				auto &newCurFrame = context->getCurFrame();

				if (ins.opcode == Opcode::MCALL)
					newCurFrame.thisObject = (newCurFrame.scopeValue = ins.operands[1]);
				return;
			}

			break;
		}
		case Opcode::RET: {
			_checkOperandCount(ins, 1);

			context->majorFrames.pop_back();
			context->majorFrames.back().returnValue = ins.operands[0];
			++context->majorFrames.back().curIns;
			break;
		}
		case Opcode::LRET: {
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, { TypeId::Var });

			((VarValue *)ins.operands[0])->setData(curMajorFrame.returnValue);
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
			_checkOperandType(ins, { TypeId::Var });

			((VarValue *)ins.operands[0])->setData(curMajorFrame.thisObject);
			break;
		}
		case Opcode::NEW: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { TypeId::Var, TypeId::TypeName, TypeId::Ref });

			Type &type = ((TypeNameValue *)ins.operands[1])->_data;
			RefValue *constructorRef = (RefValue *)ins.operands[2];
			type.loadDeferredType(this);

			switch (type.typeId) {
				case TypeId::Object:
				case TypeId::Class: {
					ClassValue *cls = (ClassValue *)type.getCustomTypeExData();
					ObjectValue *instance = _newClassInstance(cls);
					((VarValue *)ins.operands[0])->setData(instance);

					if (constructorRef) {
						if (auto v = resolveRef(constructorRef); v) {
							if ((v->getType() != TypeId::Fn))
								throw InvalidOperandsError("Specified constructor is not a function");

							BasicFnValue *constructor = (BasicFnValue *)v;

							if (constructor->isNative()) {
								constructor->call(instance, curMajorFrame.nextArgStack);
								curMajorFrame.nextArgStack.clear();
							} else {
								_callFn(context, (FnValue *)constructor);
								context->majorFrames.back().thisObject = instance;
								return;
							}
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

			_checkOperandType(ins, { TypeId::Var, TypeId::TypeName, TypeId::U32 });

			VarValue *varOut = (VarValue *)ins.operands[0];
			Type &type = ((TypeNameValue *)ins.operands[1])->_data;
			uint32_t size = ((U32Value*)ins.operands[2])->_data;
			type.loadDeferredType(this);

			auto instance = _newArrayInstance(type, size);

			varOut->setData(instance);

			break;
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, 1);

			Value *x = ins.operands[0];

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

			_checkOperandType(ins, { TypeId::TypeName, TypeId::U32 });

			ExceptionHandler xh;

			((TypeNameValue *)ins.operands[0])->_data.loadDeferredType(this);

			xh.type = ((TypeNameValue *)ins.operands[0])->getData();
			xh.off = ((U32Value *)ins.operands[1])->getData();

			curMajorFrame.minorFrames.back().exceptHandlers.push_back(xh);
			break;
		}
		case Opcode::ABORT:
			_checkOperandCount(ins, 0);
			throw UncaughtExceptionError("Use chose to abort the execution");
		case Opcode::CAST: {
			_checkOperandCount(ins, 3);

			_checkOperandType(ins, { TypeId::Var, TypeId::TypeName, TypeId::Any });

			auto t = ((TypeNameValue *)ins.operands[1])->getData();

			ValueRef<> v = ins.operands[2];

			switch (t.typeId) {
				case TypeId::I8:
					v = _castToLiteralValue<int8_t>(this, v.get());
					break;
				case TypeId::I16:
					v = _castToLiteralValue<int16_t>(this, v.get());
					break;
				case TypeId::I32:
					v = _castToLiteralValue<int32_t>(this, v.get());
					break;
				case TypeId::I64:
					v = _castToLiteralValue<int64_t>(this, v.get());
					break;
				case TypeId::U8:
					v = _castToLiteralValue<uint8_t>(this, v.get());
					break;
				case TypeId::U16:
					v = _castToLiteralValue<uint16_t>(this, v.get());
					break;
				case TypeId::U32:
					v = _castToLiteralValue<uint32_t>(this, v.get());
					break;
				case TypeId::U64:
					v = _castToLiteralValue<uint64_t>(this, v.get());
					break;
				case TypeId::Bool:
					v = _castToLiteralValue<bool>(this, v.get());
					break;
				case TypeId::F32:
					v = _castToLiteralValue<float>(this, v.get());
					break;
				case TypeId::F64:
					v = _castToLiteralValue<double>(this, v.get());
					break;
				case TypeId::Object:
					/* stub */
					break;
				default:
					throw InvalidOperandsError("Invalid cast target type");
			}

			((VarValue *)ins.operands[0])->setData(v.get());
			break;
		}
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((uint8_t)ins.opcode));
	}
	++curMajorFrame.curIns;
}
