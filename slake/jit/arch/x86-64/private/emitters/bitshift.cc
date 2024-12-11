#include "bitshift.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg8WithImm8Ins(RegisterId registerId, uint8_t times) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, 0xd0, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, 0xc0, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xd0, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xc0, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xd0, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xc0, MODRM_BYTE(0b11, 4, (registerId - REG_R8)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg16WithImm8Ins(RegisterId registerId, uint8_t times) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, 0x66, 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, 0x66, 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0x66, 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0x66, 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0x66, 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0x66, 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_R8)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg32WithImm8Ins(RegisterId registerId, uint8_t times) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_R8)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg64WithImm8Ins(RegisterId registerId, uint8_t times) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		case REG_R8:
		case REG_R9:
		case REG_R10:
		case REG_R11:
		case REG_R12:
		case REG_R13:
		case REG_R14:
		case REG_R15: {
			if (times == 1) {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), 0xd1, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
				return emitRawIns(sizeof(insBuf), insBuf);
			} else {
				DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), 0xc1, MODRM_BYTE(0b11, 4, (registerId - REG_R8)),
					times);
				return emitRawIns(sizeof(insBuf), insBuf);
			}
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg8WithClIns(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0xd2, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xd2, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xd2, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg16WithClIns(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0x66, 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0x66, 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0x66, 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg32WithClIns(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg64WithClIns(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), 0xd3, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}
