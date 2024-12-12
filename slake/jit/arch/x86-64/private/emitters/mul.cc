#include "mul.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul8WithReg8Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0xf6, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xf6, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xf6, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul16WithReg16Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0x66, 0xf7, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), 0xf7, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul32WithReg32Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0xf7, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xf7, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul64WithReg64Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xf7, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), 0xf7, MODRM_BYTE(0b11, 4, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul8WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 4 << 3;
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0xf6, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul16WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 4 << 3;
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
		DEF_INS_BUFFER(insBody, 0xf7, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul32WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 4 << 3;
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0xf7, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul64WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 4 << 3;
	uint8_t sib;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, 0xf6, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul8WithReg8Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX: {
			DEF_INS_BUFFER(insBuf, 0xf6, MODRM_BYTE(0b11, 5, (registerId - REG_RAX)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 0), 0xf6, MODRM_BYTE(0b11, 4, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xf6, MODRM_BYTE(0b11, 5, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul16WithReg16Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0x66, 0xf7, MODRM_BYTE(0b11, 5, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, 0x66, REX_PREFIX(0, 0, 0, 1), 0xf7, MODRM_BYTE(0b11, 5, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul32WithReg32Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, 0xf7, MODRM_BYTE(0b11, 5, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(0, 0, 0, 1), 0xf7, MODRM_BYTE(0b11, 5, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul64WithReg64Ins(RegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
		case REG_RCX:
		case REG_RDX:
		case REG_RBX:
		case REG_RSP:
		case REG_RBP:
		case REG_RSI:
		case REG_RDI: {
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 0), 0xf7, MODRM_BYTE(0b11, 5, (registerId - REG_RAX)));
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
			DEF_INS_BUFFER(insBuf, REX_PREFIX(1, 0, 0, 1), 0xf7, MODRM_BYTE(0b11, 5, (registerId - REG_R8)));
			return emitRawIns(sizeof(insBuf), insBuf);
		}
		default:
			throw std::logic_error("Invalid register ID");
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul8WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 5 << 3;
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0xf6, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul16WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 5 << 3;
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
		DEF_INS_BUFFER(insBody, 0xf7, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul32WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 5 << 3;
	uint8_t sib;
	uint8_t rexPrefix = 0;

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	if (rexPrefix) {
		ins[off++] = rexPrefix;
	}

	{
		DEF_INS_BUFFER(insBody, 0xf7, modRm);
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

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul64WithMemIns(const MemoryLocation &mem) {
	uint8_t modRm = 5 << 3;
	uint8_t sib;
	uint8_t rexPrefix = REX_PREFIX(1, 0, 0, 0);

	bool isSibValid = memoryToModRmAndSib(mem, modRm, sib, rexPrefix);

	uint8_t ins[16];
	size_t off = 0;

	ins[off++] = rexPrefix;

	{
		DEF_INS_BUFFER(insBody, 0xf6, modRm);
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
