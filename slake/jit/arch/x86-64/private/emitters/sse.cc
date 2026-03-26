#include "sse.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_ssess_arithm_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id, uint8_t opcode) {
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
		DEF_INS_BUFFER(ins, 0xf3, rex_prefix, 0x0f, opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf3, 0x0f, opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_ssess_arithm_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem, uint8_t opcode) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = 0, sib = 0;

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

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);
	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0xf3;
	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, 0x0f, opcode, mod_rm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_ssesd_arithm_reg_xmm_to_reg_xmm_ins(RegisterId register_id, RegisterId src_register_id, uint8_t opcode) {
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
		DEF_INS_BUFFER(ins, 0xf2, rex_prefix, 0x0f, opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf2, 0x0f, opcode, mod_rm);
		return emit_raw_ins(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_ssesd_arithm_mem_to_reg_xmm_ins(RegisterId register_id, const MemoryLocation &mem, uint8_t opcode) {
	uint8_t mod_rm = 0b11000000;
	uint8_t rex_prefix = 0, sib = 0;

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

	bool is_sib_valid = memory_to_mod_rm_and_sib(mem, mod_rm, sib, rex_prefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0xf2;
	if (rex_prefix) {
		ins[off++] = rex_prefix;
	}

	{
		DEF_INS_BUFFER(ins_body, 0x0f, opcode, mod_rm);
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
