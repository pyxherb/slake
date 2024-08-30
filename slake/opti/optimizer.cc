#include "optimizer.h"
#include <algorithm>

using namespace slake;

struct ProgramBlock {
	std::vector<Instruction> instructions;
};

struct OptimizerContext {
	std::vector<std::shared_ptr<ProgramBlock>> programBlocks;
	struct {
		std::unordered_map<std::string, size_t> byLabel;
		std::map<uint32_t, size_t> byOffset;
	} programBlocksIndices;
	std::map<uint32_t, Value> constantRegs;
	std::set<uint32_t> eliminatableRegs;
	std::map<size_t, uint32_t> programBlocksOffsets;
	std::map<uint32_t, uint32_t> regRemappingMap;
};

bool slake::isConstantValue(const Value &value) {
	switch (value.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
		case ValueType::ObjectRef:
		case ValueType::TypeName:
			return true;
		default:
			return false;
	}
}

bool slake::isFnSSACompatible(RegularFnOverloadingObject *fn) {
	return false;
}

void slake::trimFnInstructions(RegularFnOverloadingObject *fn) {
	std::vector<Instruction> expandedInstructions;
	{
		OptimizerContext optimizerContext;

		{
			std::set<uint32_t> programBlockBorders;

			// Divide the instructions into program blocks.
			for (size_t i = 0; i < fn->instructions.size(); ++i) {
				Instruction &curIns = fn->instructions[i];

				switch (curIns.opcode) {
					case Opcode::JMP:
					case Opcode::JT:
					case Opcode::JF: {
						uint32_t destOff = curIns.operands[0].getU32();

						programBlockBorders.insert(destOff);
						programBlockBorders.insert(i);
						break;
					}
					case Opcode::RET:
						programBlockBorders.insert(i);

						break;
					case Opcode::MOV: {
						// Eliminate useless MOV assignments.
						uint32_t destReg = curIns.output.getRegIndex();
						Value constant = curIns.operands[0];

						optimizerContext.constantRegs[destReg] = curIns.operands[0];

						if (constant.valueType == ValueType::RegRef) {
							uint32_t regIndex = constant.getRegIndex();

							if (auto it = optimizerContext.constantRegs.find(regIndex);
								it != optimizerContext.constantRegs.end()) {
								optimizerContext.constantRegs[destReg] = it->second;
							}
						}

						optimizerContext.eliminatableRegs.insert(destReg);
						break;
					}
					default:;
				}
			}

			std::shared_ptr<ProgramBlock> curProgramBlock = std::make_shared<ProgramBlock>();

			size_t prevOffset = 0;
			auto pushProgramBlock = [&optimizerContext, fn](
				std::shared_ptr<ProgramBlock> curProgramBlock,
				size_t prevOffset,
				size_t i) {
				size_t nInstructions = i - prevOffset;

				curProgramBlock->instructions.resize(nInstructions);
				std::copy_n(
					fn->instructions.begin() + prevOffset,
					nInstructions,
					curProgramBlock->instructions.begin());
				optimizerContext.programBlocks.push_back(curProgramBlock);

				optimizerContext.programBlocksIndices.byOffset[i] = optimizerContext.programBlocks.size();
			};

			// Because std::set is sorted,
			// we don't need to sort offset of the borders manually.
			for (auto i : programBlockBorders) {
				pushProgramBlock(curProgramBlock, prevOffset, i);

				curProgramBlock = std::make_shared<ProgramBlock>();
				prevOffset = i;
			}

			pushProgramBlock(curProgramBlock, prevOffset, fn->instructions.size());
		}

		// Eliminate constant-valued registers.
		for (auto i : optimizerContext.programBlocks) {
			std::vector<Instruction> newInstructions;

			for (size_t j = 0; j < i->instructions.size(); ++j) {
				auto curIns = i->instructions[j];

				// Skip assignment of eliminatable registers
				if (curIns.output.valueType == ValueType::RegRef) {
					if (optimizerContext.eliminatableRegs.count(curIns.output.getRegIndex())) {
						continue;
					}
				}

				// Eliminate register allocations for eliminated registers.
				if (curIns.opcode == Opcode::REG) {
					if (optimizerContext.eliminatableRegs.count(curIns.operands[0].getU32()))
						continue;
				}

				// Replace references to eliminated registers with the constant value.
				for (auto &k : curIns.operands) {
					if (k.valueType == ValueType::RegRef) {
						uint32_t regIndex = k.getRegIndex();

						if (auto it = optimizerContext.constantRegs.find(regIndex);
							it != optimizerContext.constantRegs.end())
							k = it->second;
					}
				}

				newInstructions.push_back(curIns);
			}

			i->instructions = std::move(newInstructions);
		}

		// Expand program blocks.
		for (size_t i = 0; i < optimizerContext.programBlocks.size(); ++i) {
			auto curProgramBlock = optimizerContext.programBlocks[i];
			size_t index = expandedInstructions.size();

			expandedInstructions.resize(
				expandedInstructions.size() +
				curProgramBlock->instructions.size());

			std::copy_n(
				curProgramBlock->instructions.begin(),
				curProgramBlock->instructions.size(),
				expandedInstructions.begin() + index);

			optimizerContext.programBlocksOffsets[i] = index;
		}

		// Scan and reallocate registers.
		for (auto &i : expandedInstructions) {
			switch (i.opcode) {
				case Opcode::REG: {
					uint32_t regIndex = i.operands[0].getU32();
					uint32_t newIndex = optimizerContext.regRemappingMap.size();

					optimizerContext.regRemappingMap[regIndex] = newIndex;

					i.operands[0] = Value(newIndex);

					break;
				}
			}
		}

		// Replace old register indices with the news.
		for (auto &i : expandedInstructions) {
			if (i.output.valueType == ValueType::RegRef) {
				uint32_t regIndex = i.output.getRegIndex();

				i.output = Value(ValueType::RegRef, optimizerContext.regRemappingMap.at(regIndex));
			}

			for (auto &j : i.operands) {
				if (j.valueType == ValueType::RegRef) {
					uint32_t regIndex = j.getRegIndex();

					j = Value(ValueType::RegRef, optimizerContext.regRemappingMap.at(regIndex));
				}
			}
		}

		// Replace jump destinations with new offsets.
		for (auto &i : expandedInstructions) {
			switch (i.opcode) {
				case Opcode::JMP:
				case Opcode::JT:
				case Opcode::JF: {
					size_t index = optimizerContext.programBlocksIndices.byOffset[i.operands[0].getU32()];
					i.operands[0] = Value(
						optimizerContext.programBlocksOffsets[index]);
					break;
				}
			}
		}
	}

	fn->instructions = std::move(expandedInstructions);
}
