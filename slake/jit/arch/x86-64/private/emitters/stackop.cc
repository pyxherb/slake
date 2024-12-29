#include "stackop.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPushReg16Ins(RegisterId registerId) {
	return emitInsWithReg16Increment(0x50, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPushReg64Ins(RegisterId registerId) {
	return emitInsWithReg64Increment(0x50, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPopReg16Ins(RegisterId registerId) {
	return emitInsWithReg16Increment(0x58, registerId);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPopReg64Ins(RegisterId registerId) {
	return emitInsWithReg64Increment(0x58, registerId);
}
