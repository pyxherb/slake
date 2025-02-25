#include "mov.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]) {
	return emitInsWithImm8AndReg8Increment(0xb0, registerId, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]) {
	return emitInsWithImm16AndReg16Increment(0xb8, registerId, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]) {
	return emitInsWithImm32AndReg32Increment(0xb8, registerId, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm64ToReg64Ins(RegisterId registerId, uint8_t imm0[8]) {
	return emitInsWithImm64AndReg64Increment(0xb8, registerId, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm8ToMemIns(const MemoryLocation &mem, uint8_t imm0[1]) {
	return emitInsWithImm8AndMem8WithMinorOpcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm16ToMemIns(const MemoryLocation &mem, uint8_t imm0[2]) {
	return emitInsWithImm16AndMem16WithMinorOpcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm32ToMemIns(const MemoryLocation &mem, uint8_t imm0[4]) {
	return emitInsWithImm32AndMem32WithMinorOpcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovImm64ToMemIns(const MemoryLocation &mem, uint8_t imm0[8]) {
	return emitInsWithImm64AndMem64WithMinorOpcode(0xc6, 0, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg8AndModRMReg(0x88, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg16AndModRMReg(0x89, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg32AndModRMReg(0x89, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg64AndModRMReg(0x89, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg8AndMem(0x8a, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg16AndMem(0x8b, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg32AndMem(0x8b, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg64AndMem(0x8b, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg8ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg8AndMem(0x88, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg16ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg16AndMem(0x89, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg32ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg32AndMem(0x89, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovReg64ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg64AndMem(0x89, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovdReg32ToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
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
		DEF_INS_BUFFER(ins, 0x66, rexPrefix, 0x0f, 0x6e, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, 0x0f, 0x6e, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovqReg64ToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

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

	DEF_INS_BUFFER(ins, 0x66, rexPrefix, 0x0f, 0x6e, modRm);
	return emitRawIns(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovdRegXmmToReg32Ins(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0;

	switch (srcRegisterId) {
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

	switch (registerId) {
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
		DEF_INS_BUFFER(ins, 0x66, rexPrefix, 0x0f, 0x7e, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, 0x0f, 0x7e, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovqRegXmmToReg64Ins(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	switch (srcRegisterId) {
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

	switch (registerId) {
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

	DEF_INS_BUFFER(ins, 0x66, rexPrefix, 0x0f, 0x7e, modRm);
	return emitRawIns(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
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
			modRm |= (registerId - REG_R8) << 3;
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
		DEF_INS_BUFFER(insBody, 0x0f, 0x6e, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovqMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	switch (registerId) {
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

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, 0x0f, 0x6e, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovdRegXmmToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	switch (srcRegisterId) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			modRm |= srcRegisterId;
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
			modRm |= (srcRegisterId - REG_XMM8);
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
		DEF_INS_BUFFER(insBody, 0x0f, 0x7e, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovqRegXmmToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	uint8_t modRm = 0b00000000;
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	switch (srcRegisterId) {
		case REG_XMM0:
		case REG_XMM1:
		case REG_XMM2:
		case REG_XMM3:
		case REG_XMM4:
		case REG_XMM5:
		case REG_XMM6:
		case REG_XMM7:
			modRm |= srcRegisterId;
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
			modRm |= (srcRegisterId - REG_XMM8);
			break;
		default:
			throw std::logic_error("Invalid register ID");
	}

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, 0x0f, 0x7e, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMovqRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = 0b11000000;
	uint8_t rexPrefix = 0b00000000;

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
		DEF_INS_BUFFER(ins, 0xf3, rexPrefix, 0x0f, 0x7e, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf3, 0x0f, 0x7e, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}
