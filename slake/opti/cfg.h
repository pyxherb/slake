#ifndef _SLAKE_OPTI_REGANAL_H_
#define _SLAKE_OPTI_REGANAL_H_

#include "../runtime.h"

namespace slake {
	namespace opti {
		struct BasicBlock final {
			peff::DynArray<Instruction> instructions;

			SLAKE_API BasicBlock(peff::Alloc *allocator);
			SLAKE_API BasicBlock(BasicBlock &&rhs);
			SLAKE_API ~BasicBlock();

			SLAKE_FORCEINLINE BasicBlock &operator=(BasicBlock&& rhs) noexcept {
				instructions = std::move(rhs.instructions);
				return *this;
			}
		};

		struct ControlFlowGraph final {
			peff::DynArray<BasicBlock> basic_blocks;

			SLAKE_API ControlFlowGraph(peff::Alloc *allocator);
			SLAKE_API ~ControlFlowGraph();
		};

		SLAKE_API InternalExceptionPointer divide_instructions_into_basic_blocks(peff::Alloc *intermediate_allocator, RegularFnOverloadingObject *fn_overloading, peff::Alloc *output_allocator, ControlFlowGraph &control_flow_graph_out);
		SLAKE_API InternalExceptionPointer check_terminal_instructions(peff::Alloc *except_allocator, const ControlFlowGraph &control_flow_graph);
	}
}

#endif
