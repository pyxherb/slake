#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_SUB_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_SUB_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitSubImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emitSubImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emitSubImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitSubImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitSubReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitSubReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitSubReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitSubReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitSubMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitSubMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitSubMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitSubMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitSubssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitSubssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitSubsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitSubsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
		}
	}
}

#endif
