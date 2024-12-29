#include "div.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv8WithReg8Ins(RegisterId registerId) {
	return emitInsWithReg8WithMinorOpcode(0xf6, 6, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv16WithReg16Ins(RegisterId registerId) {
	return emitInsWithReg16WithMinorOpcode(0xf7, 6, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv32WithReg32Ins(RegisterId registerId) {
	return emitInsWithReg32WithMinorOpcode(0xf7, 6, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv64WithReg64Ins(RegisterId registerId) {
	return emitInsWithReg64WithMinorOpcode(0xf7, 6, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv8WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem8WithMinorOpcode(0xf6, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv16WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem16WithMinorOpcode(0xf7, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv32WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem32WithMinorOpcode(0xf7, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDiv64WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem64WithMinorOpcode(0xf7, 6, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv8WithReg8Ins(RegisterId registerId) {
	return emitInsWithReg8WithMinorOpcode(0xf6, 7, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv16WithReg16Ins(RegisterId registerId) {
	return emitInsWithReg16WithMinorOpcode(0xf7, 7, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv32WithReg32Ins(RegisterId registerId) {
	return emitInsWithReg32WithMinorOpcode(0xf7, 7, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv64WithReg64Ins(RegisterId registerId) {
	return emitInsWithReg64WithMinorOpcode(0xf7, 7, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv8WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem8WithMinorOpcode(0xf6, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv16WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem16WithMinorOpcode(0xf7, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv32WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem32WithMinorOpcode(0xf7, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitIdiv64WithMemIns(const MemoryLocation &mem) {
	return emitInsWithMem64WithMinorOpcode(0xf7, 7, mem);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDivssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESsArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x5e);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDivssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESsArithmMemToRegXmmIns(registerId, mem, 0x5e);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDivsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId) {
	return emitSSESdArithmRegXmmToRegXmmIns(registerId, srcRegisterId, 0x5e);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitDivsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem) {
	return emitSSESdArithmMemToRegXmmIns(registerId, mem, 0x5e);
}
