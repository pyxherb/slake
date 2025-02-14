#include "sse.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSSESsArithmRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t opcode) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0;

	switch (registerId) {
	case REG_XMM0:
	case REG_XMM1:
	case REG_XMM2:
	case REG_XMM3:
	case REG_XMM4:
	case REG_XMM5:
	case REG_XMM6:
	case REG_XMM7:
		modRm |= registerId;
		break;
	case REG_XMM8:
	case REG_XMM9:
	case REG_XMM10:
	case REG_XMM11:
	case REG_XMM12:
	case REG_XMM13:
	case REG_XMM14:
	case REG_XMM15:
		rexPrefix |= REX_PREFIX(0, 0, 0, 1);
		modRm |= (registerId - REG_XMM8);
		break;
	default:
		throw std::logic_error("Invalid register ID");
	}

	switch (srcRegisterId) {
	case REG_XMM0:
	case REG_XMM1:
	case REG_XMM2:
	case REG_XMM3:
	case REG_XMM4:
	case REG_XMM5:
	case REG_XMM6:
	case REG_XMM7:
		modRm |= registerId << 3;
		break;
	case REG_XMM8:
	case REG_XMM9:
	case REG_XMM10:
	case REG_XMM11:
	case REG_XMM12:
	case REG_XMM13:
	case REG_XMM14:
	case REG_XMM15:
		rexPrefix |= REX_PREFIX(0, 1, 0, 0);
		modRm |= (registerId - REG_XMM8) << 3;
		break;
	default:
		throw std::logic_error("Invalid register ID");
	}

	if (rexPrefix) {
		DEF_INS_BUFFER(ins, 0xf3, rexPrefix, 0x0f, opcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf3, 0x0f, opcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSSESsArithmMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t opcode) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0, sib = 0;

	switch (registerId) {
	case REG_XMM0:
	case REG_XMM1:
	case REG_XMM2:
	case REG_XMM3:
	case REG_XMM4:
	case REG_XMM5:
	case REG_XMM6:
	case REG_XMM7:
		modRm |= registerId;
		break;
	case REG_XMM8:
	case REG_XMM9:
	case REG_XMM10:
	case REG_XMM11:
	case REG_XMM12:
	case REG_XMM13:
	case REG_XMM14:
	case REG_XMM15:
		rexPrefix |= REX_PREFIX(0, 0, 0, 1);
		modRm |= (registerId - REG_XMM8);
		break;
	default:
		throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);
	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0xf3;
	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0x0f, opcode, modRm);
		memcpy(ins + off, insBody, sizeof(insBody));
		off += sizeof(insBody);
	}

	if (isSibValid) {
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

	return emitRawIns(off, ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSSESdArithmRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t opcode) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0;

	switch (registerId) {
	case REG_XMM0:
	case REG_XMM1:
	case REG_XMM2:
	case REG_XMM3:
	case REG_XMM4:
	case REG_XMM5:
	case REG_XMM6:
	case REG_XMM7:
		modRm |= registerId;
		break;
	case REG_XMM8:
	case REG_XMM9:
	case REG_XMM10:
	case REG_XMM11:
	case REG_XMM12:
	case REG_XMM13:
	case REG_XMM14:
	case REG_XMM15:
		rexPrefix |= REX_PREFIX(0, 0, 0, 1);
		modRm |= (registerId - REG_XMM8);
		break;
	default:
		throw std::logic_error("Invalid register ID");
	}

	switch (srcRegisterId) {
	case REG_XMM0:
	case REG_XMM1:
	case REG_XMM2:
	case REG_XMM3:
	case REG_XMM4:
	case REG_XMM5:
	case REG_XMM6:
	case REG_XMM7:
		modRm |= registerId << 3;
		break;
	case REG_XMM8:
	case REG_XMM9:
	case REG_XMM10:
	case REG_XMM11:
	case REG_XMM12:
	case REG_XMM13:
	case REG_XMM14:
	case REG_XMM15:
		rexPrefix |= REX_PREFIX(0, 1, 0, 0);
		modRm |= (registerId - REG_XMM8) << 3;
		break;
	default:
		throw std::logic_error("Invalid register ID");
	}

	if (rexPrefix) {
		DEF_INS_BUFFER(ins, 0xf2, rexPrefix, 0x0f, opcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf2, 0x0f, opcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSSESdArithmMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t opcode) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0, sib = 0;

	switch (registerId) {
	case REG_XMM0:
	case REG_XMM1:
	case REG_XMM2:
	case REG_XMM3:
	case REG_XMM4:
	case REG_XMM5:
	case REG_XMM6:
	case REG_XMM7:
		modRm |= registerId;
		break;
	case REG_XMM8:
	case REG_XMM9:
	case REG_XMM10:
	case REG_XMM11:
	case REG_XMM12:
	case REG_XMM13:
	case REG_XMM14:
	case REG_XMM15:
		rexPrefix |= REX_PREFIX(0, 0, 0, 1);
		modRm |= (registerId - REG_XMM8);
		break;
	default:
		throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0xf2;
	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0x0f, opcode, modRm);
		memcpy(ins + off, insBody, sizeof(insBody));
		off += sizeof(insBody);
	}

	if (isSibValid) {
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

	return emitRawIns(off, ins);
}
