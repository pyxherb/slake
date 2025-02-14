#include "regular.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

DiscreteInstruction slake::jit::x86_64::emitInsWithImm8AndAlReg(uint8_t opcode, uint8_t imm0[1]) {
	DEF_INS_BUFFER(insBuf, opcode,
		imm0[0]);
	return emitRawIns(sizeof(insBuf), insBuf);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm8AndReg8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[1]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX: {
		DEF_INS_BUFFER(insBuf, majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)),
			imm0[0]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0x80, MODRM_BYTE(0b11, 0, (registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0x80, MODRM_BYTE(0b11, 0, (registerId - REG_R8)),
			imm0[0]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm16AndAxReg(uint8_t opcode, uint8_t imm0[2]) {
	DEF_INS_BUFFER(insBuf, 0x66, opcode,
		imm0[0], imm0[1]);
	return emitRawIns(sizeof(insBuf), insBuf);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm16AndReg16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[2]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, 0x66, majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)),
			imm0[0], imm0[1]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndEaxReg(uint8_t opcode, uint8_t imm0[4]) {
	DEF_INS_BUFFER(insBuf, opcode, imm0[0], imm0[1], imm0[2], imm0[3]);
	return emitRawIns(sizeof(insBuf), insBuf);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndReg32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)),
			imm0[0]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndRaxReg(uint8_t opcode, uint8_t imm0[4]) {
	DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), opcode,
		imm0[0], imm0[1], imm0[2], imm0[3]);
	return emitRawIns(sizeof(insBuf), insBuf);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndReg64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)),
			imm0[0], imm0[1], imm0[2], imm0[3]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg8AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = MODRM_BYTE(0b11, 0, 0);
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
		DEF_INS_BUFFER(ins, rexPrefix, majorOpcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, majorOpcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg16AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = MODRM_BYTE(0b11, 0, 0);
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
		DEF_INS_BUFFER(ins, 0x66, rexPrefix, majorOpcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, 0x66, majorOpcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg32AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = MODRM_BYTE(0b11, 0, 0);
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
		DEF_INS_BUFFER(ins, rexPrefix, majorOpcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	} else {
		DEF_INS_BUFFER(ins, majorOpcode, modRm);
		return emitRawIns(sizeof(ins), ins);
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg64AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId) {
	uint8_t modRm = MODRM_BYTE(0b11, 0, 0);
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

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

	DEF_INS_BUFFER(ins, rexPrefix, majorOpcode, modRm);
	return emitRawIns(sizeof(ins), ins);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX: {
		DEF_INS_BUFFER(insBuf, majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, 0x66, majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), majorOpcode, MODRM_BYTE(0b11, minorOpcode, (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithMem8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithMem16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithMem32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithMem64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithReg8AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, 0, 0);
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
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithReg16AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, 0, 0);
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
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithReg32AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, 0, 0);
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
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithReg64AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem) {
	uint8_t modRm = MODRM_BYTE(0b00, 0, 0);
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
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

DiscreteInstruction slake::jit::x86_64::emitInsWithImm8AndMem8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[1]) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

	ins[off++] = imm0[0];

	return emitRawIns(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm16AndMem16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[2]) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = 0x66;
	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];

	return emitRawIns(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndMem32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[4]) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];

	return emitRawIns(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndMem64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[4]) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];
	ins[off++] = imm0[2];
	ins[off++] = imm0[3];

	return emitRawIns(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm64AndMem64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[8]) {
	uint8_t modRm = MODRM_BYTE(0b00, minorOpcode, 0);
	uint8_t sib = 0b00000000;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, majorOpcode, modRm);
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

	ins[off++] = imm0[0];
	ins[off++] = imm0[1];
	ins[off++] = imm0[2];
	ins[off++] = imm0[3];
	ins[off++] = imm0[4];
	ins[off++] = imm0[5];
	ins[off++] = imm0[6];
	ins[off++] = imm0[7];

	return emitRawIns(off, ins);
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg8Increment(uint8_t majorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX: {
		DEF_INS_BUFFER(insBuf, (uint8_t)(majorOpcode + (registerId - REG_RAX)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), (uint8_t)(majorOpcode + (uint8_t)(registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(majorOpcode + (uint8_t)(registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg16Increment(uint8_t majorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, 0x66, (uint8_t)(majorOpcode + (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), (uint8_t)(majorOpcode + (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg32Increment(uint8_t majorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, (uint8_t)(majorOpcode + (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(majorOpcode + (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithReg64Increment(uint8_t majorOpcode, RegisterId registerId) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), (uint8_t)(majorOpcode + (registerId - REG_RAX)));
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), (uint8_t)(majorOpcode + (registerId - REG_R8)));
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm8AndReg8Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[1]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX: {
		DEF_INS_BUFFER(insBuf, (uint8_t)(majorOpcode + (registerId - REG_RAX)),
			imm0[0]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), (uint8_t)(majorOpcode + (uint8_t)(registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(majorOpcode + (uint8_t)(registerId - REG_R8)),
			imm0[0]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm16AndReg16Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[2]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, 0x66, (uint8_t)(majorOpcode + (registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), (uint8_t)(majorOpcode + (registerId - REG_R8)),
			imm0[0], imm0[1]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm32AndReg32Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, (uint8_t)(majorOpcode + (registerId - REG_RAX)),
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), (uint8_t)(majorOpcode + (registerId - REG_R8)),
			imm0[0], imm0[1], imm0[2], imm0[3]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}

DiscreteInstruction slake::jit::x86_64::emitInsWithImm64AndReg64Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[8]) {
	switch (registerId) {
	case REG_RAX:
	case REG_RCX:
	case REG_RDX:
	case REG_RBX:
	case REG_RSP:
	case REG_RBP:
	case REG_RSI:
	case REG_RDI: {
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), (uint8_t)(majorOpcode + (registerId - REG_RAX)),
			imm0[0], imm0[1], imm0[2], imm0[3], imm0[4], imm0[5], imm0[6], imm0[7]);
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
		DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), (uint8_t)(majorOpcode + (registerId - REG_R8)),
			imm0[0], imm0[1], imm0[2], imm0[3], imm0[4], imm0[5], imm0[6], imm0[7]);
		return emitRawIns(sizeof(insBuf), insBuf);
	}
	default:
		throw std::logic_error("Invalid register ID");
	}
}
