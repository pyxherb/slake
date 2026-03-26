#include "regular.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm8_and_al_reg(uint8_t opcode, uint8_t imm0[1]) {
	DEF_INS_BUFFER(ins_buf, opcode,
		imm0[0]);
	return emit_raw_ins(sizeof(ins_buf), ins_buf);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm8_and_reg8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[1]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(ins_buf, major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 0), 0x80, MODRM_BYTE(0b11, 0, (register_id - REG_RAX)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), 0x80, MODRM_BYTE(0b11, 0, (register_id - REG_R8)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm16_and_ax_reg(uint8_t opcode, uint8_t imm0[2]) {
	DEF_INS_BUFFER(ins_buf, 0x66, opcode,
		imm0[0], imm0[1]);
	return emit_raw_ins(sizeof(ins_buf), ins_buf);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm16_and_reg16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[2]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, 0x66, major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)),
				imm0[0], imm0[1]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, 0x66, REX_PREFIX(0, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)),
				imm0[0], imm0[1]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_eax_reg(uint8_t opcode, uint8_t imm0[4]) {
	DEF_INS_BUFFER(ins_buf, opcode, imm0[0], imm0[1], imm0[2], imm0[3]);
	return emit_raw_ins(sizeof(ins_buf), ins_buf);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_reg32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[4]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_rax_reg(uint8_t opcode, uint8_t imm0[4]) {
	DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 0), opcode,
		imm0[0], imm0[1], imm0[2], imm0[3]);
	return emit_raw_ins(sizeof(ins_buf), ins_buf);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_reg64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id, uint8_t imm0[4]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 0), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg8_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = MODRM_BYTE(0b11, 0, 0);
	uint8_t rex_prefix = 0;

	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rex_prefix) {
		DEF_INS_BUFFER(ins, rex_prefix, major_opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, major_opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg16_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = MODRM_BYTE(0b11, 0, 0);
	uint8_t rex_prefix = 0;

	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rex_prefix) {
		DEF_INS_BUFFER(ins, 0x66, rex_prefix, major_opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, major_opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg32_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = MODRM_BYTE(0b11, 0, 0);
	uint8_t rex_prefix = 0;

	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rex_prefix) {
		DEF_INS_BUFFER(ins, rex_prefix, major_opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, major_opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg64_and_mod_rmreg(uint8_t major_opcode, RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = MODRM_BYTE(0b11, 0, 0);
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= 0b01000001;
			mod_rm |= (register_id - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= 0b01001000;
			mod_rm |= (register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	DEF_INS_BUFFER(ins, rex_prefix, major_opcode, mod_rm);
	return emit_raw_ins(sizeof(ins), ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(ins_buf, major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 0), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, 0x66, major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, 0x66, REX_PREFIX(0, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 0), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 1), major_opcode, MODRM_BYTE(0b11, minor_opcode, (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_mem8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib;
	uint8_t rex_prefix = 0;

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_mem16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib;
	uint8_t rex_prefix = 0;

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_mem32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib;
	uint8_t rex_prefix = 0;

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_mem64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rex_prefix;

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg8_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, 0, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= src_register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (src_register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg16_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, 0, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= src_register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (src_register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg32_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, 0, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= src_register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (src_register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg64_and_mem(uint8_t major_opcode, RegisterId src_register_id, const MemoryLocation &mem) {
	uint8_t mod_rm = MODRM_BYTE(0b00, 0, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	switch (src_register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			mod_rm |= src_register_id << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (src_register_id - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rex_prefix;

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm8_and_mem8_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[1]) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	ins[off++] = imm0[0];

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm16_and_mem16_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[2]) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_mem32_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[4]) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_mem64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[4]) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rex_prefix;

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];
	ins[off++] = imm0[2];
	ins[off++] = imm0[3];

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm64_and_mem64_with_minor_opcode(uint8_t major_opcode, uint8_t minor_opcode, const MemoryLocation &mem, uint8_t imm0[8]) {
	uint8_t mod_rm = MODRM_BYTE(0b00, minor_opcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rex_prefix;

	{
		DEF_INS_BUFFER(ins_body, major_opcode, mod_rm);
		memcpy(ins + off, ins_body, sizeof(ins_body));
		off += sizeof(ins_body);
	}

	if (is_sib_valid) {
		ins[off++] = sib;
	}

	if (!mem.disp) {
	} else if (mem.disp <= UINT8_MAX) {
		int8_t disp = (int8_t)mem.disp;
		ins[off++] = disp;
	} else {
		int32_t disp = (int32_t)mem.disp;
		memcpy(ins + off, &disp, sizeof(disp));
		off += sizeof(disp);
	}

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];
	ins[off++] = imm0[2];
	ins[off++] = imm0[3];
	ins[off++] = imm0[4];
	ins[off++] = imm0[5];
	ins[off++] = imm0[6];
	ins[off++] = imm0[7];

	return emit_raw_ins(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg8_increment(uint8_t major_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(ins_buf, (uint8_t)(major_opcode + (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 0), (uint8_t)(major_opcode + (uint8_t)(register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(major_opcode + (uint8_t)(register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg16_increment(uint8_t major_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, 0x66, (uint8_t)(major_opcode + (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, 0x66, REX_PREFIX(0, 0, 0, 1), (uint8_t)(major_opcode + (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg32_increment(uint8_t major_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, (uint8_t)(major_opcode + (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(major_opcode + (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_reg64_increment(uint8_t major_opcode, RegisterId register_id) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 0), (uint8_t)(major_opcode + (register_id - REG_RAX)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 1), (uint8_t)(major_opcode + (register_id - REG_R8)));
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm8_and_reg8_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[1]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(ins_buf, (uint8_t)(major_opcode + (register_id - REG_RAX)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 0), (uint8_t)(major_opcode + (uint8_t)(register_id - REG_RAX)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(major_opcode + (uint8_t)(register_id - REG_R8)),
				imm0[0]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm16_and_reg16_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[2]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, 0x66, (uint8_t)(major_opcode + (register_id - REG_RAX)),
				imm0[0], imm0[1]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, 0x66, REX_PREFIX(0, 0, 0, 1), (uint8_t)(major_opcode + (register_id - REG_R8)),
				imm0[0], imm0[1]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm32_and_reg32_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[4]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, (uint8_t)(major_opcode + (register_id - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(major_opcode + (register_id - REG_R8)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emit_ins_with_imm64_and_reg64_increment(uint8_t major_opcode, RegisterId register_id, uint8_t imm0[8]) {
	switch (register_id) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 0), (uint8_t)(major_opcode + (register_id - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3], imm0[4], imm0[5], imm0[6], imm0[7]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins_buf, REX_PREFIX(1, 0, 0, 1), (uint8_t)(major_opcode + (register_id - REG_R8)),
				imm0[0], imm0[1], imm0[2], imm0[3], imm0[4], imm0[5], imm0[6], imm0[7]);
			return emit_raw_ins(sizeof(ins_buf), ins_buf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}
