#include "ins.h"

using namespace Slake;

InsRegistry insRegistries[] = {
	{ "nop", Opcode::NOP, { OPRD_END } },

	{ "push", Opcode::PUSH, { OPRD_END } },
	{ "push", Opcode::PUSH, { OPRD_VALUE, OPRD_END } },
	{ "pop", Opcode::POP, { OPRD_END } },
	{ "pop", Opcode::POP, { OPRD_DATAREF, OPRD_END } },
	{ "load", Opcode::LOAD, { OPRD_DATAREF, OPRD_END } },
	{ "store", Opcode::STORE, { OPRD_DATAREF, OPRD_END } },
	{ "lload", Opcode::LLOAD, { OPRD_INT, OPRD_END } },
	{ "lstore", Opcode::LSTORE, { OPRD_INT, OPRD_END } },

	{ "enter", Opcode::ENTER, { OPRD_END } },
	{ "leave", Opcode::LEAVE, { OPRD_END } },

	{ "add", Opcode::ADD, { OPRD_END } },
	{ "sub", Opcode::SUB, { OPRD_END } },
	{ "mul", Opcode::MUL, { OPRD_END } },
	{ "div", Opcode::DIV, { OPRD_END } },
	{ "mod", Opcode::MOD, { OPRD_END } },
	{ "and", Opcode::AND, { OPRD_END } },
	{ "or", Opcode::OR, { OPRD_END } },
	{ "xor", Opcode::XOR, { OPRD_END } },
	{ "land", Opcode::LAND, { OPRD_END } },
	{ "lor", Opcode::LOR, { OPRD_END } },
	{ "eq", Opcode::EQ, { OPRD_END } },
	{ "meq", Opcode::NEQ, { OPRD_END } },
	{ "lt", Opcode::LT, { OPRD_END } },
	{ "gt", Opcode::GT, { OPRD_END } },
	{ "lteq", Opcode::LTEQ, { OPRD_END } },
	{ "gteq", Opcode::GTEQ, { OPRD_END } },
	{ "not", Opcode::NOT, { OPRD_END } },
	{ "lnot", Opcode::LNOT, { OPRD_END } },

	{ "jmp", Opcode::JMP, { OPRD_END } },
	{ "jmp", Opcode::JMP, { OPRD_CODEREF, OPRD_END } },
	{ "jt", Opcode::JT, { OPRD_END } },
	{ "jt", Opcode::JT, { OPRD_CODEREF, OPRD_END } },
	{ "jf", Opcode::JF, { OPRD_END } },
	{ "jf", Opcode::JF, { OPRD_CODEREF, OPRD_END } },

	{ "call", Opcode::CALL, { OPRD_END } },
	{ "call", Opcode::CALL, { OPRD_CODEREF, OPRD_END } },
	{ "ret", Opcode::RET, { OPRD_END } },

	{ "syscall", Opcode::SYSCALL, { OPRD_END } },
	{ "syscall", Opcode::SYSCALL, { OPRD_UUID, OPRD_END } },

	{ nullptr, Opcode::NOP, { OPRD_END } }
};
