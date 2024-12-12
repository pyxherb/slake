#include "sub.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]) {
	switch (registerId) {
		case REG_RAX: {
			DEF_INS_BUFFER(insBuf, 0x2c,
				imm0[0]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0x80, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0x80, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0x80, MODRM_BYTE(0b11, 0b101, (registerId - REG_R8)),
				imm0[0]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]) {
	switch (registerId) {
		case REG_RAX: {
			DEF_INS_BUFFER(insBuf, 0x66, 0x2d,
				imm0[0], imm0[1]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0x66, 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0], imm0[1]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 0), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0], imm0[1]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_R8)),
				imm0[0], imm0[1]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
		case REG_RAX: {
			DEF_INS_BUFFER(insBuf, 0x2d,
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_R8)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
		case REG_RAX: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0x2d,
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), REX_PREFIX(0, 0, 0, 0), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_RAX)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), 0x81, MODRM_BYTE(0b11, 0b101, (registerId - REG_R8)),
				imm0[0], imm0[1], imm0[2], imm0[3]);
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0;

	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 0, 0, 1);
			modRm |= (registerId - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (registerId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rexPrefix) {
		DEF_INS_BUFFER(ins, rexPrefix, 0x28, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x28, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0;

	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 0, 0, 1);
			modRm |= (registerId - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (registerId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rexPrefix) {
		DEF_INS_BUFFER(ins, 0x66, rexPrefix, 0x29, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, 0x29, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0;

	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 0, 0, 1);
			modRm |= (registerId - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (registerId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	if (rexPrefix) {
		DEF_INS_BUFFER(ins, rexPrefix, 0x29, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x29, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = REX_PREFIX(1, 0, 0, 0);
	uint8_t rexPrefix = 0;

	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= 0b01000001;
			modRm |= (registerId - REG_R8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= registerId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= 0b01001000;
			modRm |= (registerId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	DEF_INS_BUFFER(ins, rexPrefix, 0x29, modRm);
	return emitRawIns(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= srcRegisterId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (srcRegisterId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0x28, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= srcRegisterId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (srcRegisterId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0x29, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= srcRegisterId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (srcRegisterId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0x29, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	switch (srcRegisterId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI:
			modRm |= srcRegisterId << 3;
			break;
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15:
			rexPrefix |= REX_PREFIX(0, 1, 0, 0);
			modRm |= (srcRegisterId - REG_R8) << 3;
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, 0x29, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
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
		DEF_INS_BUFFER(ins, 0xf3, rexPrefix, 0x0f, 0x5c, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf3, 0x0f, 0x5c, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
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
		DEF_INS_BUFFER(insBody, 0x0f, 0x5c, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
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
		DEF_INS_BUFFER(ins, 0xf2, rexPrefix, 0x0f, 0x5c, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf2, 0x0f, 0x5c, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
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
		DEF_INS_BUFFER(insBody, 0x0f, 0x5c, modRm);
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
