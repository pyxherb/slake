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

	{ Opcode::AT, "AT" },

	{ Opcode::JMP, "JMP" },
	{ Opcode::JT, "JT" },
	{ Opcode::JF, "JF" },

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

	{ Opcode::ABORT, "ABORT" },

	{ Opcode::CASTI8, "CASTI8" },
	{ Opcode::CASTI16, "CASTI16" },
	{ Opcode::CASTI32, "CASTI32" },
	{ Opcode::CASTI64, "CASTI64" },
	{ Opcode::CASTU8, "CASTU8" },
	{ Opcode::CASTU16, "CASTU16" },
	{ Opcode::CASTU32, "CASTU32" },
	{ Opcode::CASTU64, "CASTU64" },
	{ Opcode::CASTF32, "CASTF32" },
	{ Opcode::CASTF64, "CASTF64" },

	{ Opcode::TYPEOF, "TYPEOF" }
};
