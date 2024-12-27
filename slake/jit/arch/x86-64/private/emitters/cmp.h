#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CMP_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_CMP_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitCmpImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emitCmpImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emitCmpImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitCmpImm32ToReg64Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitCmpImm8ToMemIns(const MemoryLocation &mem, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emitCmpImm16ToMemIns(const MemoryLocation &mem, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emitCmpImm32ToMemIns(const MemoryLocation &mem, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitCmpImm64ToMemIns(const MemoryLocation &mem, uint8_t imm0[8]);
			SLAKE_API DiscreteInstruction emitCmpReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitCmpMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitCmpMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitCmpMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitCmpReg8ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpReg16ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpReg32ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpReg64ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitCmpssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t imm0);
			SLAKE_API DiscreteInstruction emitCmpsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem, uint8_t imm0);
			SLAKE_API DiscreteInstruction emitCmpssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t imm0);
			SLAKE_API DiscreteInstruction emitCmpsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId, uint8_t imm0);
		}
	}
}

#endif
