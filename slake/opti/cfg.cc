#include "cfg.h"

using namespace slake;
using namespace slake::opti;

SLAKE_API BasicBlock::BasicBlock(peff::Alloc *allocator) : instructions(allocator) {
}

SLAKE_API BasicBlock::BasicBlock(BasicBlock &&rhs) : instructions(std::move(rhs.instructions)) {
}

SLAKE_API BasicBlock::~BasicBlock() {
}

SLAKE_API ControlFlowGraph::ControlFlowGraph(peff::Alloc *allocator) : basicBlocks(allocator) {
}

SLAKE_API ControlFlowGraph::~ControlFlowGraph() {
}

SLAKE_API InternalExceptionPointer slake::opti::checkTerminalInstructions(peff::Alloc *exceptAllocator, const ControlFlowGraph &controlFlowGraph) {
	for (size_t i = 0; i < controlFlowGraph.basicBlocks.size(); ++i) {
		const BasicBlock &basicBlock = controlFlowGraph.basicBlocks.at(i);
		const Instruction &ins = basicBlock.instructions.back();

		switch (ins.opcode) {
			case Opcode::JMP:
			case Opcode::BR:
			case Opcode::RET:
				break;
			default:
				return MalformedCfgError::alloc(exceptAllocator, &controlFlowGraph, i, basicBlock.instructions.size() - 1);
		}
	}

	return {};
}
