#include "mov.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm8_to_reg8_ins(RegisterId register_id, uint8_t imm0[1]) {
	return emit_ins_with_imm8_and_reg8_increment(0xb0, register_id, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm16_to_reg16_ins(RegisterId register_id, uint8_t imm0[2]) {
	return emit_ins_with_imm16_and_reg16_increment(0xb8, register_id, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm32_to_reg32_ins(RegisterId register_id, uint8_t imm0[4]) {
	return emit_ins_with_imm32_and_reg32_increment(0xb8, register_id, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm64_to_reg64_ins(RegisterId register_id, uint8_t imm0[8]) {
	return emit_ins_with_imm64_and_reg64_increment(0xb8, register_id, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm8_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[1]) {
	return emit_ins_with_imm8_and_mem8_with_minor_opcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm16_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[2]) {
	return emit_ins_with_imm16_and_mem16_with_minor_opcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm32_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[4]) {
	return emit_ins_with_imm32_and_mem32_with_minor_opcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_imm64_to_mem_ins(const MemoryLocation &mem, uint8_t imm0[8]) {
	return emit_ins_with_imm64_and_mem64_with_minor_opcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg8_to_reg8_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg8_and_mod_rmreg(0x88, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg16_to_reg16_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg16_and_mod_rmreg(0x89, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg32_to_reg32_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg32_and_mod_rmreg(0x89, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg64_to_reg64_ins(RegisterId register_id, RegisterId src_register_id) {
	return emit_ins_with_reg64_and_mod_rmreg(0x89, register_id, src_register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_mem_to_reg8_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg8_and_mem(0x8a, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_mem_to_reg16_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg16_and_mem(0x8b, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_mem_to_reg32_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg32_and_mem(0x8b, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_mem_to_reg64_ins(RegisterId src_register_id, const MemoryLocation &mem) {
	return emit_ins_with_reg64_and_mem(0x8b, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg8_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id) {
	return emit_ins_with_reg8_and_mem(0x88, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg16_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id) {
	return emit_ins_with_reg16_and_mem(0x89, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg32_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id) {
	return emit_ins_with_reg32_and_mem(0x89, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_mov_reg64_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id) {
	return emit_ins_with_reg64_and_mem(0x89, src_register_id, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movd_reg32_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = 0;

	switch (register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_XMM8);
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
		DEF_INS_BUFFER(ins, 0x66, rex_prefix, 0x0f, 0x6e, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, 0x0f, 0x6e, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movq_reg64_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	switch (register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_XMM8);
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

	DEF_INS_BUFFER(ins, 0x66, rex_prefix, 0x0f, 0x6e, mod_rm);
	return emit_raw_ins(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movd_reg_xmm_to_reg32_ins(RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = 0;

	switch (src_register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_XMM8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (register_id) {
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
		DEF_INS_BUFFER(ins, 0x66, rex_prefix, 0x0f, 0x7e, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, 0x0f, 0x7e, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movq_reg_xmm_to_reg64_ins(RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	switch (src_register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_XMM8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (register_id) {
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

	DEF_INS_BUFFER(ins, 0x66, rex_prefix, 0x0f, 0x7e, mod_rm);
	return emit_raw_ins(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movd_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	uint8_t mod_rm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	switch (register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id << 3;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (register_id - REG_R8) << 3;
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
		DEF_INS_BUFFER(ins_body, 0x0f, 0x6e, mod_rm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movq_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem) {
	uint8_t mod_rm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	switch (register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id << 3;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (register_id - REG_XMM8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	ins[off++] = rex_prefix;

	{
		DEF_INS_BUFFER(ins_body, 0x0f, 0x6e, mod_rm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movd_reg_xmm_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id) {
	uint8_t mod_rm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = 0;

	switch (src_register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= src_register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (src_register_id - REG_XMM8);
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
		DEF_INS_BUFFER(ins_body, 0x0f, 0x7e, mod_rm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movq_reg_xmm_to_mem_ins(const MemoryLocation &mem, RegisterId src_register_id) {
	uint8_t mod_rm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rex_prefix = REX_PREFIX(1, 0, 0, 0);

	switch (src_register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= src_register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (src_register_id - REG_XMM8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	ins[off++] = rex_prefix;

	{
		DEF_INS_BUFFER(ins_body, 0x0f, 0x7e, mod_rm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_movq_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = 0b00000000;

	switch (register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 0, 0, 1);
			mod_rm |= (register_id - REG_XMM8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (src_register_id) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			mod_rm |= register_id << 3;
			break;
		case REG_XMM8:
		case REG_XMM9:
		case REG_XMM10:
		case REG_XMM11:
		case REG_XMM12:
		case REG_XMM13:
		case REG_XMM14:
		case REG_XMM15:
			rex_prefix |= REX_PREFIX(0, 1, 0, 0);
			mod_rm |= (register_id - REG_XMM8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rex_prefix) {
		DEF_INS_BUFFER(ins, 0xf3, rex_prefix, 0x0f, 0x7e, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf3, 0x0f, 0x7e, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}
