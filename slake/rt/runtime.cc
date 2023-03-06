#include "runtime.h"

using namespace Slake;

template <typename TD, typename TS>
static TD _checkOperandRange(void* v) {
	auto value = (LiteralValue<TS, 0>*)v;
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

static void _checkOperandCount(Instruction* ins, std::uint8_t max) {
	if (ins->getOperandCount() > max)
		throw InvalidOperandsError("Invalid operand count");
}

void Slake::RuntimeImpl::execIns(Context* context, Instruction* ins) {
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
			_checkOperandCount(ins, 1);
			if (ins->opcode == Opcode::LLOAD) {
				auto off = _getOperandAsAddress(ins->getOperandCount() ? ins->operands[0] : *context->pop());
				context->push(context->lload(off));
			} else {
				_checkOperandType(ins, ValueType::SCOPE_REF);
			}
			break;
		case Opcode::LSTORE:
		case Opcode::STORE:
			_checkOperandCount(ins, 1);
			_checkOperandType(ins, ValueType::I32);
			context->dataStack.pop_back();
			break;
		case Opcode::EXPAND: {
			_checkOperandCount(ins, 1);
			std::uint32_t n = _getOperandAsAddress(ins->operands[0]);
			context->dataStack.insert(--(context->dataStack.end()), n, nullptr);
			break;
		}
		case Opcode::SHRINK: {
			_checkOperandCount(ins, 1);
			std::uint32_t n = _getOperandAsAddress(ins->operands[0]);
			if ((n > context->dataStack.size()) ||
				(n - context->dataStack.size() < *(context->frameBases.rbegin())))
				throw FrameBoundaryExceededError("Operation exceeds stack frame boundary");
			context->dataStack.resize(context->dataStack.size() - n);
			break;
		}
		case Opcode::ENTER: {
		}
		case Opcode::LEAVE: {
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
		}
		case Opcode::REV:
		case Opcode::NOT:
		case Opcode::INC:
		case Opcode::DEC:
		case Opcode::NEG: {
		}
		case Opcode::JMP:
		case Opcode::JT:
		case Opcode::JF: {
		}
		case Opcode::CALL: {
		}
		case Opcode::ACALL: {
		}
		case Opcode::RET: {
		}
		case Opcode::SYSCALL: {
		}
		case Opcode::ASYSCALL: {
		}
		case Opcode::THROW: {
		}
		case Opcode::PUSHXH: {
		}
		case Opcode::ABORT:
			throw UncaughtExceptionError("Uncaught exception");
		default:
			throw InvalidOpcodeError("Invalid opcode " + std::to_string((std::uint8_t)ins->opcode));
	}
}

Object* Slake::RuntimeImpl::resolveRef(RefValue* ref, Module* mod) {
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
