#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_ADD_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_ADD_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitAddImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emitAddImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emitAddImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitAddImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitAddReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitAddReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitAddReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitAddReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitAddMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitAddMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitAddMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitAddMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitAddssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitAddssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitAddsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitAddsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
		}
	}
}

#endif
