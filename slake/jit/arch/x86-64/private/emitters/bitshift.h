#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_BITSHIFT_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_BITSHIFT_H_

#include "regular.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitShlReg8WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShlReg16WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShlReg32WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShlReg64WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShlReg8WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShlReg16WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShlReg32WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShlReg64WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShrReg8WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShrReg16WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShrReg32WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShrReg64WithImm8Ins(RegisterId registerId, uint8_t times);
			SLAKE_API DiscreteInstruction emitShrReg8WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShrReg16WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShrReg32WithClIns(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitShrReg64WithClIns(RegisterId registerId);
		}
	}
}

#endif
