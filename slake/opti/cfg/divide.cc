#include "../cfg.h"

using namespace slake;
using namespace slake::opti;

SLAKE_API InternalExceptionPointer slake::opti::divideInstructionsIntoBasicBlocks(peff::Alloc *intermediateAllocator, RegularFnOverloadingObject *fnOverloading, peff::Alloc *outputAllocator, ControlFlowGraph &controlFlowGraphOut) {
	Runtime *const rt = fnOverloading->associatedRuntime;
	const RegularFnOverloadingObject *ol = const_cast<const RegularFnOverloadingObject *>(fnOverloading);
	peff::Set<size_t> basicBlockBoundaries(intermediateAllocator);	 // Basic block boundaries
	peff::Map<size_t, size_t> basicBlockMap(intermediateAllocator);	 // Basic block offset-to-label-id map

	for (size_t i = 0; i < ol->instructions.size(); ++i) {
		const Instruction &ins = ol->instructions.at(i);

		switch (ins.opcode) {
			case Opcode::JMP: {
				if (ins.nOperands != 1)
					return MalformedProgramError::alloc(rt->getFixedAlloc(), fnOverloading, i);
				if (ins.operands[0].valueType != ValueType::U32)
					return MalformedProgramError::alloc(rt->getFixedAlloc(), fnOverloading, i);
				if (!basicBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!basicBlockBoundaries.insert(ins.operands[0].getU32()))
					return OutOfMemoryError::alloc();
				break;
			}
			case Opcode::BR: {
				if (ins.nOperands != 3)
					return MalformedProgramError::alloc(rt->getFixedAlloc(), fnOverloading, i);
				if (ins.operands[1].valueType != ValueType::U32)
					return MalformedProgramError::alloc(rt->getFixedAlloc(), fnOverloading, i);
				if (ins.operands[2].valueType != ValueType::U32)
					return MalformedProgramError::alloc(rt->getFixedAlloc(), fnOverloading, i);
				if (!basicBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!basicBlockBoundaries.insert(ins.operands[1].getU32()))
					return OutOfMemoryError::alloc();
				if (!basicBlockBoundaries.insert(ins.operands[2].getU32()))
					return OutOfMemoryError::alloc();
				break;
			}
			case Opcode::RET: {
				if (ins.nOperands > 1)
					return MalformedProgramError::alloc(rt->getFixedAlloc(), fnOverloading, i);
				if (i + 1 < ol->instructions.size())
					if (!basicBlockBoundaries.insert(i + 1))
						return OutOfMemoryError::alloc();
				break;
			}
			default:
				break;
		}
	}

	if (!basicBlockMap.insert(0, 0))
		return OutOfMemoryError::alloc();
	{
		auto it = basicBlockBoundaries.begin();
		for (size_t i = 0; i <= basicBlockBoundaries.size(); ++i) {
			size_t blockBegin;
			if (it == basicBlockBoundaries.begin())
				blockBegin = 0;
			else
				blockBegin = *it.prev();
			if (!basicBlockMap.insert(+blockBegin, basicBlockMap.size()))
				return OutOfMemoryError::alloc();
			if (i < basicBlockBoundaries.size())
				++it;
		}
	}

	if (!controlFlowGraphOut.basicBlocks.resizeUninitialized(basicBlockBoundaries.size() + 1))
		return OutOfMemoryError::alloc();
	for (size_t i = 0; i < controlFlowGraphOut.basicBlocks.size(); ++i)
		peff::constructAt<BasicBlock>(&controlFlowGraphOut.basicBlocks.at(i), outputAllocator);

	{
		auto it = basicBlockBoundaries.begin();
		for (size_t i = 0; i <= basicBlockBoundaries.size(); ++i) {
			const size_t blockBegin = i ? *it.prev() : 0,
						 blockEnd = i < basicBlockBoundaries.size() ? *it++ : ol->instructions.size();

			BasicBlock curBlock(outputAllocator);

			if (!curBlock.instructions.resize(blockEnd - blockBegin))
				return OutOfMemoryError::alloc();

			for (size_t j = blockBegin, k = 0; j < blockEnd; ++j, ++k) {
				const Instruction &originalInstruction = fnOverloading->instructions.at(j);

				Instruction &newInstruction = curBlock.instructions.at(k);

				newInstruction.offSourceLocDesc = originalInstruction.offSourceLocDesc;
				newInstruction.setOpcode(originalInstruction.opcode);
				newInstruction.setOutput(originalInstruction.output);
				if (!newInstruction.reserveOperands(outputAllocator, originalInstruction.nOperands))
					return OutOfMemoryError::alloc();
				memcpy(newInstruction.operands, originalInstruction.operands, sizeof(Value) * newInstruction.nOperands);

				switch (newInstruction.opcode) {
					case Opcode::JMP: {
						const uint32_t dest = newInstruction.operands[0].getU32();
						newInstruction.operands[0] = Value(ValueType::Label, basicBlockMap.at(dest));
						break;
					}
					case Opcode::BR: {
						newInstruction.operands[1] = Value(ValueType::Label, basicBlockMap.at(newInstruction.operands[1].getU32()));
						newInstruction.operands[2] = Value(ValueType::Label, basicBlockMap.at(newInstruction.operands[2].getU32()));
						break;
					}
					default:
						break;
				}
			}

			controlFlowGraphOut.basicBlocks.at(i) = std::move(curBlock);
		}
	}

	return {};
}
