#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"

namespace slake {
	namespace opti {
		struct BasicBlock final {
			peff::DynArray<Instruction> instructions;
			uint32_t linksToLabel = UINT32_MAX;

			SLAKE_API BasicBlock(peff::Alloc *allocator);
			SLAKE_API BasicBlock(BasicBlock &&rhs);
			SLAKE_API ~BasicBlock();

			SLAKE_FORCEINLINE BasicBlock &operator=(BasicBlock&& rhs) noexcept {
				instructions = std::move(rhs.instructions);
				return *this;
			}
		};

		struct ControlFlowGraph final {
			peff::DynArray<BasicBlock> basicBlocks;

			SLAKE_API ControlFlowGraph(peff::Alloc *allocator);
			SLAKE_API ~ControlFlowGraph();
		};

		SLAKE_API InternalExceptionPointer divideInstructionsIntoBasicBlocks(peff::Alloc *intermediateAllocator, RegularFnOverloadingObject *fnOverloading, peff::Alloc *outputAllocator, ControlFlowGraph &controlFlowGraphOut);
	}
}

#endif
