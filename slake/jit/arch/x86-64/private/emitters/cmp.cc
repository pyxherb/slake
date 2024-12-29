#include "cmp.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm8AndAlReg(0x3c, imm0);
		default:
			return emitInsWithImm8AndReg8WithMinorOpcode(0x80, 7, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm16AndAxReg(0x3d, imm0);
		default:
			return emitInsWithImm16AndReg16WithMinorOpcode(0x81, 7, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm32AndEaxReg(0x3d, imm0);
		default:
			return emitInsWithImm32AndReg32WithMinorOpcode(0x81, 7, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm32AndRaxReg(0x3d, imm0);
		default:
			return emitInsWithImm32AndReg64WithMinorOpcode(0x81, 7, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm8ToMemIns(const MemoryLocation &mem, uint8_t imm0[1]) {
	return emitInsWithImm8AndMem8WithMinorOpcode(0x80, 7, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm16ToMemIns(const MemoryLocation &mem, uint8_t imm0[2]) {
	return emitInsWithImm16AndMem16WithMinorOpcode(0x81, 7, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm32ToMemIns(const MemoryLocation &mem, uint8_t imm0[4]) {
	return emitInsWithImm32AndMem32WithMinorOpcode(0x81, 7, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpImm64ToMemIns(const MemoryLocation &mem, uint8_t imm0[8]) {
	return emitInsWithImm32AndMem32WithMinorOpcode(0x81, 7, mem, imm0);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg8AndModRMReg(0x38, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg16AndModRMReg(0x39, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg32AndModRMReg(0x39, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg64AndModRMReg(0x39, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg8AndMem(0x3a, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg16AndMem(0x3b, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg32AndMem(0x3b, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg64AndMem(0x3b, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg8ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg8AndMem(0x38, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg16ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg16AndMem(0x39, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg32ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg32AndMem(0x39, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpReg64ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId) {
	return emitInsWithReg64AndMem(0x39, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t imm0) {
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
		DEF_INS_BUFFER(insBody, 0x0f, 0xc2, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t imm0) {
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
		DEF_INS_BUFFER(insBody, 0x0f, 0xc2, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t imm0) {
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
		DEF_INS_BUFFER(ins, 0xf3, rexPrefix, 0x0f, 0xc2, modRm, imm0);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf3, 0x0f, 0xc2, modRm, imm0);
		return emitRawIns(sizeof(ins), ins);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCmpsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t imm0) {
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
		DEF_INS_BUFFER(ins, 0xf2, rexPrefix, 0x0f, 0xc2, modRm, imm0);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0xf2, 0x0f, 0xc2, modRm, imm0);
		return emitRawIns(sizeof(ins), ins);
	}
}
