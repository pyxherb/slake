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
