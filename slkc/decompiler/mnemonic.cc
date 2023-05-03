#include "mnemonic.hh"

using namespace Slake;

std::unordered_map<Opcode, const char *> Decompiler::mnemonics = {
	{ Opcode::NOP, "NOP" },

	{ Opcode::PUSH, "PUSH" },
	{ Opcode::POP, "POP" },

	{ Opcode::LOAD, "LOAD" },
	{ Opcode::RLOAD, "RLOAD" },
	{ Opcode::STORE, "STORE" },

	{ Opcode::LLOAD, "LLOAD" },
	{ Opcode::LSTORE, "LSTORE" },

	{ Opcode::LVALUE, "LVALUE" },

	{ Opcode::EXPAND, "EXPAND" },
	{ Opcode::SHRINK, "SHRINK" },

	{ Opcode::ENTER, "ENTER" },
	{ Opcode::LEAVE, "LEAVE" },

	{ Opcode::ADD, "ADD" },
	{ Opcode::SUB, "SUB" },
	{ Opcode::MUL, "MUL" },
	{ Opcode::DIV, "DIV" },
	{ Opcode::MOD, "MOD" },
	{ Opcode::AND, "AND" },
	{ Opcode::OR, "OR" },
	{ Opcode::XOR, "XOR" },
	{ Opcode::LAND, "LAND" },
	{ Opcode::LOR, "LOR" },
	{ Opcode::EQ, "EQ" },
	{ Opcode::NEQ, "NEQ" },
	{ Opcode::LT, "LT" },
	{ Opcode::GT, "GT" },
	{ Opcode::LTEQ, "LTEQ" },
	{ Opcode::GTEQ, "GTEQ" },
	{ Opcode::LSH, "LSH" },
	{ Opcode::RSH, "RSH" },
	
	{ Opcode::REV, "REV" },
	{ Opcode::NOT, "NOT" },
	{ Opcode::INCF, "INCF" },
	{ Opcode::DECF, "DECF" },
	{ Opcode::INCB, "INCB" },
	{ Opcode::DECB, "DECB" },
	{ Opcode::NEG, "NEG" },

	{ Opcode::JMP, "JMP" },
	{ Opcode::JT, "JT" },
	{ Opcode::JF, "JF" },

	{ Opcode::CAST, "CAST" },

	{ Opcode::SARG, "SARG" },
	{ Opcode::LARG, "LARG" },

	{ Opcode::LTHIS, "LTHIS" },
	{ Opcode::STHIS, "STHIS" },

	{ Opcode::CALL, "CALL" },
	{ Opcode::ACALL, "ACALL" },
	{ Opcode::RET, "RET" },

	{ Opcode::NEW, "NEW" },

	{ Opcode::LRET, "LRET" },

	{ Opcode::THROW, "THROW" },
	{ Opcode::PUSHXH, "PUSHXH" },

	{ Opcode::ABORT, "ABORT" }
};
