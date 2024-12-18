#include "ctrl.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitJmpWithOff32(size_t &offDestOut) {
	DEF_INS_BUFFER(ins, 0xe9, 0x00, 0x00, 0x00, 0x00);
	offDestOut = 1;
	return emitRawIns(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCallWithOff32(size_t &offDestOut) {
	DEF_INS_BUFFER(ins, 0xe8, 0x00, 0x00, 0x00, 0x00);
	offDestOut = 1;
	return emitRawIns(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitJmpWithReg64Ins(RegisterId registerId) {
	uint8_t modRm = MODRM_BYTE(0b11, 4, 0);
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

	DEF_INS_BUFFER(ins, rexPrefix, 0xff, modRm);
	return emitRawIns(sizeof(ins), ins);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitCallWithReg64Ins(RegisterId registerId) {
	uint8_t modRm = MODRM_BYTE(0b11, 2, 0);
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

	DEF_INS_BUFFER(ins, rexPrefix, 0xff, modRm);
	return emitRawIns(sizeof(ins), ins);
}
