#include "bitshift.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg8WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg8WithMinorOpcode(0xd0, 4, registerId);
	} else {
		return emitInsWithReg8WithMinorOpcode(0xc0, 4, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg16WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg16WithMinorOpcode(0xd1, 4, registerId);
	} else {
		return emitInsWithReg16WithMinorOpcode(0xc1, 4, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg32WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg32WithMinorOpcode(0xd1, 4, registerId);
	} else {
		return emitInsWithReg32WithMinorOpcode(0xc1, 4, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg64WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg64WithMinorOpcode(0xd1, 4, registerId);
	} else {
		return emitInsWithReg64WithMinorOpcode(0xc1, 4, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg8WithClIns(RegisterId registerId) {
	return emitInsWithReg8WithMinorOpcode(0xd2, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg16WithClIns(RegisterId registerId) {
	return emitInsWithReg16WithMinorOpcode(0xd3, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg32WithClIns(RegisterId registerId) {
	return emitInsWithReg32WithMinorOpcode(0xd3, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShlReg64WithClIns(RegisterId registerId) {
	return emitInsWithReg64WithMinorOpcode(0xd3, 4, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg8WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg8WithMinorOpcode(0xd0, 5, registerId);
	} else {
		return emitInsWithReg8WithMinorOpcode(0xc0, 5, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg16WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg16WithMinorOpcode(0xd1, 5, registerId);
	} else {
		return emitInsWithReg16WithMinorOpcode(0xc1, 5, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg32WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg32WithMinorOpcode(0xd1, 5, registerId);
	} else {
		return emitInsWithReg32WithMinorOpcode(0xc1, 5, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg64WithImm8Ins(RegisterId registerId, uint8_t times) {
	if (times == 1) {
		return emitInsWithReg64WithMinorOpcode(0xd1, 5, registerId);
	} else {
		return emitInsWithReg64WithMinorOpcode(0xc1, 5, registerId);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg8WithClIns(RegisterId registerId) {
	return emitInsWithReg8WithMinorOpcode(0xd2, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg16WithClIns(RegisterId registerId) {
	return emitInsWithReg16WithMinorOpcode(0xd3, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg32WithClIns(RegisterId registerId) {
	return emitInsWithReg32WithMinorOpcode(0xd3, 5, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitShrReg64WithClIns(RegisterId registerId) {
	return emitInsWithReg64WithMinorOpcode(0xd3, 5, registerId);
}
