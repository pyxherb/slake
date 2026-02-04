#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_COPY_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_COPY_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitMovImm8ToReg8Ins(RegisterId registerId, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emitMovImm16ToReg16Ins(RegisterId registerId, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emitMovImm32ToReg32Ins(RegisterId registerId, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitMovImm64ToReg64Ins(RegisterId registerId, uint8_t imm0[8]);
			SLAKE_API DiscreteInstruction emitMovImm8ToMemIns(const MemoryLocation &mem, uint8_t imm0[1]);
			SLAKE_API DiscreteInstruction emitMovImm16ToMemIns(const MemoryLocation &mem, uint8_t imm0[2]);
			SLAKE_API DiscreteInstruction emitMovImm32ToMemIns(const MemoryLocation &mem, uint8_t imm0[4]);
			SLAKE_API DiscreteInstruction emitMovImm64ToMemIns(const MemoryLocation &mem, uint8_t imm0[8]);
			SLAKE_API DiscreteInstruction emitMovReg8ToReg8Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovReg16ToReg16Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovReg32ToReg32Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovReg64ToReg64Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovMemToReg8Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMovMemToReg16Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMovMemToReg32Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMovMemToReg64Ins(RegisterId srcRegisterId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMovReg8ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovReg16ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovReg32ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovReg64ToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovdReg32ToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovqReg64ToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovdRegXmmToReg32Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovqRegXmmToReg64Ins(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMovqMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMovdRegXmmToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovqRegXmmToMemIns(const MemoryLocation &mem, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitMovqRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
		}
	}
}

#endif
