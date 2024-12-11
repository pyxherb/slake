#include "stackop.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPushReg16Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RBX:
		case REG_RCX:
		case REG_RDX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins, 0x66, (uint8_t)(0x50 + (registerId - REG_RAX)));
			return emitRawIns(sizeof(ins), ins);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins, 0x66, REX_PREFIX(0, 0, 0, 1), (uint8_t)(0x50 + (registerId - REG_R8)));
			return emitRawIns(sizeof(ins), ins);
		}
		default:;
	}

	throw std::logic_error("Invalid register ID");
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPushReg64Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RBX:
		case REG_RCX:
		case REG_RDX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins, (uint8_t)(0x50 + (registerId - REG_RAX)));
			return emitRawIns(sizeof(ins), ins);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins, REX_PREFIX(0, 0, 0, 1), (uint8_t)(0x50 + (registerId - REG_R8)));
			return emitRawIns(sizeof(ins), ins);
		}
		default:;
	}

	throw std::logic_error("Invalid register ID");
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPopReg16Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RBX:
		case REG_RCX:
		case REG_RDX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(ins, 0x66, (uint8_t)(0x58 + (registerId - REG_RAX)));
			return emitRawIns(sizeof(ins), ins);
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			DEF_INS_BUFFER(ins, 0x66, REX_PREFIX(0, 0, 0, 1), (uint8_t)(0x58 + (registerId - REG_R8)));
			return emitRawIns(sizeof(ins), ins);
		}
		default:;
	}

	throw std::logic_error("Invalid register ID");
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPopReg64Ins(RegisterId registerId) {
	switch (registerId) {
		switch (registerId) {
			case REG_RAX:
			case REG_RBX:
			case REG_RCX:
			case REG_RDX:
			case REG_RSP:
			case REG_RBP:
			case REG_RSI:
			case REG_RDI: {
				DEF_INS_BUFFER(ins, REX_PREFIX(1, 0, 0, 0), (uint8_t)(0x58 + (registerId - REG_RAX)));
				return emitRawIns(sizeof(ins), ins);
			}
			case REG_R8:
			case REG_R9:
			case REG_R10:
			case REG_R11:
			case REG_R12:
			case REG_R13:
			case REG_R14:
			case REG_R15: {
				DEF_INS_BUFFER(ins, REX_PREFIX(1, 0, 0, 1), (uint8_t)(0x58 + (registerId - REG_R8)));
				return emitRawIns(sizeof(ins), ins);
			}
			default:;
		}

		throw std::logic_error("Invalid register ID");
		default:;
	}

	throw std::logic_error("Invalid register ID");
}
