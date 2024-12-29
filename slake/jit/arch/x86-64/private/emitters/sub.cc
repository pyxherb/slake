#include "sub.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm8AndAlReg(0x2c, imm0);
		default:
			return emitInsWithImm8AndReg8WithMinorOpcode(0x80, 0b101, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm16AndAxReg(0x2d, imm0);
		default:
			return emitInsWithImm16AndReg16WithMinorOpcode(0x81, 0b101, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm32AndEaxReg(0x2d, imm0);
		default:
			return emitInsWithImm32AndReg32WithMinorOpcode(0x81, 0b101, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
		case REG_RAX:
			return emitInsWithImm32AndRaxReg(0x2d, imm0);
		default:
			return emitInsWithImm32AndReg64WithMinorOpcode(0x81, 0b101, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg8AndModRMReg(0x28, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg16AndModRMReg(0x29, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg32AndModRMReg(0x29, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg64AndModRMReg(0x29, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg8AndMem(0x28, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg16AndMem(0x29, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg32AndMem(0x29, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg64AndMem(0x29, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESsArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x5c);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESsArithmMemToRegXmmIns(registerId, mem, 0x5c);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESdArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x5c);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitSubsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESdArithmMemToRegXmmIns(registerId, mem, 0x5c);
}
