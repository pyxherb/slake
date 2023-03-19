#include "../rt.h"

using namespace Slake;

template <typename TD, typename TS>
static TD _checkOperandRange(void* v) {
	auto value = (LiteralObject<TS, ValueType::OBJECT>*)v;
	if (value->getValue() > std::numeric_limits<TD>::max() ||
		value->getValue() < std::numeric_limits<TD>::min())
		throw InvalidOperandsError("Invalid operand value");
	return (TD)value->getValue();
}

static std::uint32_t _getOperandAsAddress(Object* v) {
	switch (v->getType()) {
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
			return ((U32Object*)v)->getValue();
		case ValueType::U64:
			return _checkOperandRange<std::uint32_t, std::uint64_t>(v);
		default:
			throw InvalidOperandsError("Invalid operand combination");
	}
}

static void _checkOperandType(
	Instruction* ins,
	ValueType type0,
	ValueType type1 = ValueType::OBJECT,
	ValueType type2 = ValueType::OBJECT,
	ValueType type3 = ValueType::OBJECT) {
	if (ins->operands[0]->getType() != type0 ||
		(type1 != ValueType::OBJECT ? ins->operands[1]->getType() != type1 : true) ||
		(type2 != ValueType::OBJECT ? ins->operands[2]->getType() != type2 : true) ||
		(type3 != ValueType::OBJECT ? ins->operands[3]->getType() != type3 : true))
		throw InvalidOperandsError("Invalid operand combination");
}

static void _checkOperandCount(Instruction* ins, std::uint8_t n) {
	if (ins->getOperandCount() > n)
		throw InvalidOperandsError("Invalid operand count");
}

static void _checkOperandCount(Instruction* ins, std::uint8_t min, std::uint8_t max) {
	auto n = ins->getOperandCount();
	if (n < min || n > max)
		throw InvalidOperandsError("Invalid operand count");
}

template <typename T>
static Object* _execBinaryOp(Object* x, Object* y, Opcode opcode) {
	LiteralObject<T, ValueType::OBJECT>* _x = (LiteralObject<T, ValueType::OBJECT>*)x;
	if constexpr (std::is_arithmetic<T>::value) {
		if constexpr (std::is_same<T, bool>::value) {
		} else {
			if (opcode == Opcode::LSH || opcode == Opcode::RSH) {
				if (y->getType() != ValueType::U32)
					throw InvalidOperandsError("Binary operation with incompatible types");
			} else if (_x->getType() != y->getType())
				throw InvalidOperandsError("Binary operation with incompatible types");
			switch (opcode) {
				case Opcode::ADD:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() + ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::SUB:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() - ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::MUL:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() * ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::DIV:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() / ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::MOD: {
					T result;
					if constexpr (std::is_same<T, float>::value)
						result = std::fmodf(_x->getValue(), ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
					else if constexpr (std::is_same<T, double>::value)
						result = std::fmod(_x->getValue(), ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
					else
						result = _x->getValue() % ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue();
					return new LiteralObject<T, ValueType::OBJECT>(result);
				}
				case Opcode::AND:
					if constexpr (std::is_integral<T>::value)
						return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() & ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::OR:
					if constexpr (std::is_integral<T>::value)
						return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() | ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::XOR:
					if constexpr (std::is_integral<T>::value)
						return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() ^ ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
					else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::LAND:
					return new BoolObject(_x->getValue() && ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::LOR:
					return new BoolObject(_x->getValue() || ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::EQ:
					return new BoolObject(_x->getValue() == ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::NEQ:
					return new BoolObject(_x->getValue() != ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::LT:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() < ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::GT:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() > ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::LTEQ:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() <= ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::GTEQ:
					return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() >= ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
				case Opcode::LSH:
					if constexpr (std::is_integral<T>::value)
						return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() << ((U32Object*)y)->getValue());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(std::uint32_t*)(&_x->getValue())) << ((U32Object*)y)->getValue();
						return new LiteralObject<T, ValueType::OBJECT>(*(float*)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(std::uint64_t*)(&_x->getValue())) << ((U32Object*)y)->getValue();
						return new LiteralObject<T, ValueType::OBJECT>(*(double*)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				case Opcode::RSH:
					if constexpr (std::is_integral<T>::value)
						return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() >> ((U32Object*)y)->getValue());
					else if constexpr (std::is_same<T, float>::value) {
						auto result = (*(std::uint32_t*)(&_x->getValue())) >> ((U32Object*)y)->getValue();
						return new LiteralObject<T, ValueType::OBJECT>(*(float*)(&result));
					} else if constexpr (std::is_same<T, double>::value) {
						auto result = (*(std::uint64_t*)(&_x->getValue())) >> ((U32Object*)y)->getValue();
						return new LiteralObject<T, ValueType::OBJECT>(*(double*)(&result));
					} else
						throw InvalidOperandsError("Binary operation with incompatible types");
				default:
					throw InvalidOperandsError("Binary operation with incompatible types");
			}
		}
	} else if constexpr (std::is_same<T, std::string>::value) {
		switch (opcode) {
			case Opcode::ADD:
				return new LiteralObject<T, ValueType::OBJECT>(_x->getValue() + ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
			case Opcode::EQ:
				return new BoolObject(_x->getValue() == ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
			case Opcode::NEQ:
				return new BoolObject(_x->getValue() != ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	} else {
		switch (opcode) {
			case Opcode::EQ:
				return new BoolObject(_x->getValue() == ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
			case Opcode::NEQ:
				return new BoolObject(_x->getValue() != ((LiteralObject<T, ValueType::OBJECT>*)y)->getValue());
			default:
				throw InvalidOperandsError("Binary operation with incompatible types");
		}
	}
}

void Slake::Runtime::execIns(Context* context, Instruction* ins) {
	switch (ins->opcode) {
		case Opcode::NOP:
			break;
		case Opcode::PUSH:
			_checkOperandCount(ins, 1);
			context->dataStack.push_back(ins->operands[0]);
			break;
		case Opcode::POP:
			_checkOperandCount(ins, 0);
			context->dataStack.pop_back();
			break;
		case Opcode::LLOAD:
		case Opcode::LOAD:
			_checkOperandCount(ins, 0, 1);
			if (ins->opcode == Opcode::LLOAD) {
				auto off = _getOperandAsAddress(ins->getOperandCount() ? ins->operands[0] : *context->pop());
				context->push(context->lload(off));
			} else {
				_checkOperandType(ins, ValueType::SCOPE_REF);
			}
			break;
		case Opcode::LSTORE:
		case Opcode::STORE:
			_checkOperandCount(ins, 0, 1);
			_checkOperandType(ins, ValueType::I32);
			context->dataStack.pop_back();
			break;
		case Opcode::EXPAND: {
			_checkOperandCount(ins, 0, 1);
			std::uint32_t n = _getOperandAsAddress(ins->operands[0]);
			context->expand(n);
			break;
		}
		case Opcode::SHRINK: {
			_checkOperandCount(ins, 0, 1);
			std::uint32_t n = _getOperandAsAddress(ins->operands[0]);
			context->shrink(n);
			break;
		}
		case Opcode::ENTER: {
			_checkOperandCount(ins, 1);
			context->frames.push_back(Frame(context->stackBase, _getOperandAsAddress(ins->operands[0])));
		}
		case Opcode::LEAVE: {
			_checkOperandCount(ins, 0);
			if (context->frames.size() < 2)
				throw FrameError("Attempting to leave the only frame");
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
			ObjectRef<> x(nullptr), y(nullptr);
			if (!ins->getOperandCount())
				x = ObjectRef<>(context->pop()), y = ObjectRef<>(context->pop());
			else
				x = ObjectRef<>(ins->operands[0]), y = ObjectRef<>(ins->operands[1]);
			switch (x->getType()) {
				case ValueType::I8:
					context->push(_execBinaryOp<std::int8_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::I16:
					context->push(_execBinaryOp<std::int16_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::I32:
					context->push(_execBinaryOp<std::int32_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::I64:
					context->push(_execBinaryOp<std::int64_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::U8:
					context->push(_execBinaryOp<std::uint8_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::U16:
					context->push(_execBinaryOp<std::uint16_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::U32:
					context->push(_execBinaryOp<std::uint32_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::U64:
					context->push(_execBinaryOp<std::uint64_t>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::FLOAT:
					context->push(_execBinaryOp<float>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::DOUBLE:
					context->push(_execBinaryOp<double>(x.get(), y.get(), ins->opcode));
					break;
				case ValueType::STRING:
					context->push(_execBinaryOp<std::string>(x.get(), y.get(), ins->opcode));
					break;
				default:
					throw InvalidOperandsError("Invalid binary operation for operand types");
			}
		}
		case Opcode::REV:
		case Opcode::NOT:
		case Opcode::INC:
		case Opcode::DEC:
		case Opcode::NEG: {
			_checkOperandCount(ins, 1);
			ObjectRef<> x(nullptr);
			_checkOperandCount(ins, 0, 2);
		}
		case Opcode::JMP:
		case Opcode::JT:
		case Opcode::JF: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::CALL: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::ACALL: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::RET: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::SYSCALL: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::ASYSCALL: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::THROW: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::PUSHXH: {
			_checkOperandCount(ins, 0, 1);
		}
		case Opcode::ABORT:
			throw UncaughtExceptionError("Uncaught exception");
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((std::uint8_t)ins->opcode));
	}
}

Object* Slake::Runtime::resolveRef(RefObject* ref, Module* mod) {
	if (mod) {
		for (auto i = ref; i; i = i->next) {
			{
				auto fn = mod->getFn(i->name);
				if (fn) {
				}
			}
		}
	}
	return nullptr;
}
