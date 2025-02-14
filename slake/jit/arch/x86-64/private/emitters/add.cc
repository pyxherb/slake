#include "add.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]) {
	switch (registerId) {
	case REG_RAX:
		return emitInsWithImm8AndAlReg(0x04, imm0);
	default:
		return emitInsWithImm8AndReg8WithMinorOpcode(0x80, 0, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]) {
	switch (registerId) {
	case REG_RAX:
		return emitInsWithImm16AndAxReg(0x05, imm0);
	default:
		return emitInsWithImm16AndReg16WithMinorOpcode(0x81, 0, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
	case REG_RAX:
		return emitInsWithImm32AndEaxReg(0x05, imm0);
	default:
		return emitInsWithImm32AndReg32WithMinorOpcode(0x81, 0, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]) {
	switch (registerId) {
	case REG_RAX:
		return emitInsWithImm32AndRaxReg(0x05, imm0);
	default:
		return emitInsWithImm32AndReg64WithMinorOpcode(0x81, 0, registerId, imm0);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg8AndModRMReg(0x88, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg16AndModRMReg(0x89, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg32AndModRMReg(0x89, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId) {
	return emitInsWithReg64AndModRMReg(0x89, registerId, srcRegisterId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg8AndMem(0x02, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg16AndMem(0x03, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg32AndMem(0x03, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem) {
	return emitInsWithReg64AndMem(0x03, srcRegisterId, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESsArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x58);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESsArithmMemToRegXmmIns(registerId, mem, 0x58);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESdArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x58);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitAddsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESdArithmMemToRegXmmIns(registerId, mem, 0x58);
}
