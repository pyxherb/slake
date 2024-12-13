#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_MUL_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_MUL_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_API DiscreteInstruction emitMul8WithReg8Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitMul16WithReg16Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitMul32WithReg32Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitMul64WithReg64Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitMul8WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMul16WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMul32WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitMul64WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitImul8WithReg8Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitImul16WithReg16Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitImul32WithReg32Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitImul64WithReg64Ins(RegisterId registerId);
			SLAKE_API DiscreteInstruction emitImul8WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitImul16WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitImul32WithMemIns(const MemoryLocation &mem);
			SLAKE_API DiscreteInstruction emitImul64WithMemIns(const MemoryLocation &mem);
		}
	}
}

#endif
