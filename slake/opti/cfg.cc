#include "cfg.h"

using namespace slake;
using namespace slake::opti;

SLAKE_API BasicBlock::BasicBlock(peff::Alloc *allocator) : instructions(allocator) {
}

SLAKE_API BasicBlock::BasicBlock(BasicBlock &&rhs) : instructions(std::move(rhs.instructions)) {
}

SLAKE_API BasicBlock::~BasicBlock() {
}

SLAKE_API ControlFlowGraph::ControlFlowGraph(peff::Alloc *allocator) : basic_blocks(allocator) {
}

SLAKE_API ControlFlowGraph::~ControlFlowGraph() {
}

SLAKE_API InternalExceptionPointer slake::opti::check_terminal_instructions(peff::Alloc *except_allocator, const ControlFlowGraph &control_flow_graph) {
	for (size_t i = 0; i < control_flow_graph.basic_blocks.size(); ++i) {
		const BasicBlock &basic_block = control_flow_graph.basic_blocks.at(i);
		const Instruction &ins = basic_block.instructions.back();

		switch (ins.opcode) {
			case Opcode::JMP:
			case Opcode::BR:
			case Opcode::RET:
				break;
			default:
				return MalformedCfgError::alloc(except_allocator, &control_flow_graph, i, basic_block.instructions.size() - 1);
		}
	}

	return {};
}
