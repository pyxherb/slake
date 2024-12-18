#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_DIV_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_DIV_H_

#include "sse.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitDiv8WithReg8Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitDiv16WithReg16Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitDiv32WithReg32Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitDiv64WithReg64Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitDiv8WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitDiv16WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitDiv32WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitDiv64WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitIdiv8WithReg8Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitIdiv16WithReg16Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitIdiv32WithReg32Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitIdiv64WithReg64Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitIdiv8WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitIdiv16WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitIdiv32WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitIdiv64WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitDivssRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitDivssMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitDivsdRegXmmToRegXmmIns(RegisterId registerId, RegisterId srcRegisterId);
			SLAKE_API DiscreteInstruction emitDivsdMemToRegXmmIns(RegisterId registerId, const MemoryLocation &mem);
		}
	}
}

#endif
