#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"

namespace slake {
	namespace opti {
		struct BasicBlock {
			peff::DynArray<Instruction> instructions;

			SLAKE_API BasicBlock(peff::Alloc *allocator);
			SLAKE_API ~BasicBlock();

			SLAKE_FORCEINLINE BasicBlock &operator=(BasicBlock&& rhs) {
				instructions = std::move(rhs.instructions);
				return *this;
			}
		};

		struct ControlFlowGraph {
			peff::DynArray<BasicBlock> basicBlocks;
		};

		SLAKE_API InternalExceptionPointer divideInstructionsIntoBasicBlocks(peff::Alloc *intermediateAllocator, RegularFnOverloadingObject *fnOverloading, peff::Alloc *outputAllocator, ControlFlowGraph &controlFlowGraphOut);
	}
}

#endif
