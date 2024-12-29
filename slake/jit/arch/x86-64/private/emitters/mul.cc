#include "mul.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul8WithReg8Ins(RegisterId registerId) {
	return emitInsWithReg8WithMinorOpcode(0xf6, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul16WithReg16Ins(RegisterId registerId) {
	return emitInsWithReg16WithMinorOpcode(0xf7, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul32WithReg32Ins(RegisterId registerId) {
	return emitInsWithReg32WithMinorOpcode(0xf7, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul64WithReg64Ins(RegisterId registerId) {
	return emitInsWithReg64WithMinorOpcode(0xf7, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul8WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem8WithMinorOpcode(0xf6, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul16WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem16WithMinorOpcode(0xf7, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul32WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem32WithMinorOpcode(0xf7, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMul64WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem64WithMinorOpcode(0xf7, 4, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul8WithReg8Ins(RegisterId registerId) {
	return emitInsWithReg8WithMinorOpcode(0xf6, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul16WithReg16Ins(RegisterId registerId) {
	return emitInsWithReg16WithMinorOpcode(0xf7, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul32WithReg32Ins(RegisterId registerId) {
	return emitInsWithReg32WithMinorOpcode(0xf7, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul64WithReg64Ins(RegisterId registerId) {
	return emitInsWithReg64WithMinorOpcode(0xf7, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul8WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem8WithMinorOpcode(0xf6, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul16WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem16WithMinorOpcode(0xf7, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul32WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem32WithMinorOpcode(0xf7, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitImul64WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem64WithMinorOpcode(0xf7, 5, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMulssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESsArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x59);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMulssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESsArithmMemToRegXmmIns(registerId, mem, 0x59);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMulsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESdArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x59);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitMulsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESdArithmMemToRegXmmIns(registerId, mem, 0x59);
}
