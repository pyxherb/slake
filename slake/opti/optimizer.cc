#include "optimizer.h"
#include <cmath>
#include <algorithm>

using namespace slake;

OptimizerContext::OptimizerContext(std::vector<Instruction> &instructions)
	: instructions(instructions) {}

bool slake::extractConstantValue(
	OptimizerContext &optiContext,
	const Value &value,
	Value &valueOut) {
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
			valueOut = value;
			return true;
		case ValueType::RegRef:
			if (auto it = optiContext.constRegValues.find(value.getRegIndex());
				it != optiContext.constRegValues.end()) {
				valueOut = it->second;
				return true;
			}
			return false;
		default:
			return false;
	}
}

bool slake::isFnOptimizable(std::vector<Instruction> &instructions) {
	{
		std::set<uint32_t> assignedRegisters;
		for (auto i : instructions) {
			if (i.output.valueType == ValueType::RegRef) {
				uint32_t regIndex = i.output.getRegIndex();

				// The function is not SSA-compatible, return false.
				if (assignedRegisters.count(regIndex))
					return false;

				for (auto &j : i.operands) {
					if (j.valueType == ValueType::RegRef) {
						// The function has instructions that reference
						// unassigned registers, return false.
						if (!assignedRegisters.count(j.getRegIndex()))
							return false;
					}
				}

				assignedRegisters.insert(regIndex);
			}
		}
	}
	return false;
}

static void _genControlFlowGraph(
	OptimizerContext &optiContext) {
	std::set<uint32_t> programBlockBoundaries;

	// Find the program block borders.
	for (size_t i = 0; i < optiContext.instructions.size(); ++i) {
		const Instruction &curIns = optiContext.instructions[i];

		switch (curIns.opcode) {
			case Opcode::JMP:
			case Opcode::JT:
			case Opcode::JF: {
				uint32_t destOff = curIns.operands[0].getU32();

				programBlockBoundaries.insert(destOff);
				programBlockBoundaries.insert(i);
				break;
			}
			case Opcode::RET:
				programBlockBoundaries.insert(i);
				break;
			default:;
		}
	}

	// Divide the instructions into program blocks.
	{
		std::unique_ptr<ProgBlock> curProgBlock = std::make_unique<ProgBlock>();

		size_t prevOffset = 0;
		auto pushProgBlock = [&optiContext](
								 std::unique_ptr<ProgBlock> &&curProgBlock,
								 size_t prevOffset,
								 size_t i) {
			size_t nInstructions = i - prevOffset;

			if (nInstructions) {
				curProgBlock->instructions.resize(nInstructions);
				std::copy_n(
					optiContext.instructions.begin() + prevOffset,
					nInstructions,
					curProgBlock->instructions.begin());
				optiContext.programBlocks.push_back(curProgBlock.get());
				optiContext.managedProgBlocks.insert(std::move(curProgBlock));
			}

			optiContext.programBlocksIndices.byOffset[i] = optiContext.managedProgBlocks.size();
		};

		// Because std::set is sorted,
		// we don't need to sort offset of the borders manually.
		for (auto i : programBlockBoundaries) {
			pushProgBlock(std::move(curProgBlock), prevOffset, i);

			curProgBlock = std::make_unique<ProgBlock>();
			prevOffset = i;
		}

		pushProgBlock(std::move(curProgBlock), prevOffset, optiContext.instructions.size());
	}

	// Link program blocks.
	for (size_t i = 0; i < optiContext.programBlocks.size(); ++i) {
		ProgBlock *curProgBlock = optiContext.programBlocks[i];
		Instruction &lastIns = curProgBlock->instructions.back();
		switch (lastIns.opcode) {
			case Opcode::JMP:
				curProgBlock->exitKind = ProgBlockExitKind::Jump;
				curProgBlock->exitDests.direct = optiContext.programBlocks[optiContext.programBlocksIndices.byOffset[lastIns.operands[0].getU32()]];
				break;
			case Opcode::JT:
				curProgBlock->exitKind = ProgBlockExitKind::JumpIfTrue;
				curProgBlock->exitDests.conditional.trueBranchDest = optiContext.programBlocks[optiContext.programBlocksIndices.byOffset[lastIns.operands[0].getU32()]];
				curProgBlock->exitDests.conditional.falseBranchDest = optiContext.programBlocks[i + 1];
				break;
			case Opcode::JF:
				curProgBlock->exitKind = ProgBlockExitKind::JumpIfFalse;
				curProgBlock->exitDests.conditional.trueBranchDest = optiContext.programBlocks[optiContext.programBlocksIndices.byOffset[lastIns.operands[0].getU32()]];
				curProgBlock->exitDests.conditional.falseBranchDest = optiContext.programBlocks[i + 1];
				break;
			case Opcode::RET:
				curProgBlock->exitKind = ProgBlockExitKind::Return;
				break;
			default:
				curProgBlock->exitKind = ProgBlockExitKind::Fallthrough;
		}
	}
}

static void _scanForOmittableRegs(OptimizerContext &optiContext) {
	bool needsRescan = false;

	for (auto i : optiContext.programBlocks) {
		for (auto &j : i->instructions) {
			switch (j.opcode) {
				case Opcode::MOV: {
					// Scan and collect constant and alias registers.
					uint32_t destReg = j.output.getRegIndex();
					Value value = j.operands[0];

					if (!extractConstantValue(
							optiContext,
							value,
							optiContext.constRegValues[destReg])) {
						optiContext.constRegValues[destReg] = value;
					}

					optiContext.omittableRegs.insert(destReg);

					break;
				}
				case Opcode::ADD:
				case Opcode::SUB:
				case Opcode::MUL:
				case Opcode::DIV:
				case Opcode::MOD:
				case Opcode::AND:
				case Opcode::OR:
				case Opcode::XOR:
				case Opcode::LAND:
				case Opcode::LOR:
				case Opcode::EQ:
				case Opcode::NEQ:
				case Opcode::LT:
				case Opcode::GT:
				case Opcode::LTEQ:
				case Opcode::GTEQ:
				case Opcode::LSH:
				case Opcode::RSH: {
					uint32_t destReg = j.output.getRegIndex();

					Value result;
					Value lhs = j.operands[0], rhs = j.operands[1];

					switch (lhs.valueType) {
						case ValueType::I8:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() + rhs.getI8());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() - rhs.getI8());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() * rhs.getI8());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() / rhs.getI8());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() % rhs.getI8());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() & rhs.getI8());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() | rhs.getI8());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value((int8_t)lhs.getI8() ^ rhs.getI8());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() && rhs.getI8());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() || rhs.getI8());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() == rhs.getI8());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() != rhs.getI8());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() < rhs.getI8());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() > rhs.getI8());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() <= rhs.getI8());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::I8)
										break;
									result = Value(lhs.getI8() >= rhs.getI8());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI8() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI8() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::I16:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() + rhs.getI16());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() - rhs.getI16());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() * rhs.getI16());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() / rhs.getI16());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() % rhs.getI16());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() & rhs.getI16());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() | rhs.getI16());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value((int16_t)lhs.getI16() ^ rhs.getI16());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() && rhs.getI16());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() || rhs.getI16());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() == rhs.getI16());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() != rhs.getI16());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() < rhs.getI16());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() > rhs.getI16());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() <= rhs.getI16());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::I16)
										break;
									result = Value(lhs.getI16() >= rhs.getI16());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI16() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI16() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::I32:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() + rhs.getI32());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() - rhs.getI32());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() * rhs.getI32());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() / rhs.getI32());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() % rhs.getI32());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() & rhs.getI32());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() | rhs.getI32());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value((int32_t)lhs.getI32() ^ rhs.getI32());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() && rhs.getI32());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() || rhs.getI32());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() == rhs.getI32());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() != rhs.getI32());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() < rhs.getI32());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() > rhs.getI32());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() <= rhs.getI32());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::I32)
										break;
									result = Value(lhs.getI32() >= rhs.getI32());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI32() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI32() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::I64:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() + rhs.getI64());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() - rhs.getI64());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() * rhs.getI64());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() / rhs.getI64());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() % rhs.getI64());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() & rhs.getI64());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() | rhs.getI64());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value((int64_t)lhs.getI64() ^ rhs.getI64());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() && rhs.getI64());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() || rhs.getI64());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() == rhs.getI64());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() != rhs.getI64());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() < rhs.getI64());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() > rhs.getI64());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() <= rhs.getI64());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::I64)
										break;
									result = Value(lhs.getI64() >= rhs.getI64());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI64() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getI64() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::U8:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() + rhs.getU8());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() - rhs.getU8());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() * rhs.getU8());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() / rhs.getU8());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() % rhs.getU8());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() & rhs.getU8());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() | rhs.getU8());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value((uint8_t)lhs.getU8() ^ rhs.getU8());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() && rhs.getU8());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() || rhs.getU8());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() == rhs.getU8());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() != rhs.getU8());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() < rhs.getU8());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() > rhs.getU8());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() <= rhs.getU8());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::U8)
										break;
									result = Value(lhs.getU8() >= rhs.getU8());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU8() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU8() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::U16:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() + rhs.getU16());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() - rhs.getU16());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() * rhs.getU16());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() / rhs.getU16());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() % rhs.getU16());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() & rhs.getU16());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() | rhs.getU16());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value((uint16_t)lhs.getU16() ^ rhs.getU16());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() && rhs.getU16());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() || rhs.getU16());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() == rhs.getU16());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() != rhs.getU16());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() < rhs.getU16());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() > rhs.getU16());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() <= rhs.getU16());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::U16)
										break;
									result = Value(lhs.getU16() >= rhs.getU16());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU16() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU16() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::U32:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() + rhs.getU32());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() - rhs.getU32());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() * rhs.getU32());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() / rhs.getU32());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() % rhs.getU32());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() & rhs.getU32());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() | rhs.getU32());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value((uint32_t)lhs.getU32() ^ rhs.getU32());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() && rhs.getU32());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() || rhs.getU32());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() == rhs.getU32());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() != rhs.getU32());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() < rhs.getU32());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() > rhs.getU32());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() <= rhs.getU32());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() >= rhs.getU32());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU32() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::U64:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() + rhs.getU64());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() - rhs.getU64());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() * rhs.getU64());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() / rhs.getU64());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() % rhs.getU64());
									break;
								case Opcode::AND:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() & rhs.getU64());
									break;
								case Opcode::OR:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() | rhs.getU64());
									break;
								case Opcode::XOR:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value((uint64_t)lhs.getU64() ^ rhs.getU64());
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() && rhs.getU64());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() || rhs.getU64());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() == rhs.getU64());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() != rhs.getU64());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() < rhs.getU64());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() > rhs.getU64());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() <= rhs.getU64());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::U64)
										break;
									result = Value(lhs.getU64() >= rhs.getU64());
									break;
								case Opcode::LSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU64() << rhs.getU32());
									break;
								case Opcode::RSH:
									if (rhs.valueType != ValueType::U32)
										break;
									result = Value(lhs.getU64() >> rhs.getU32());
									break;
							}
							break;
						case ValueType::F32:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value((float)lhs.getF32() + rhs.getF32());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value((float)lhs.getF32() - rhs.getF32());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value((float)lhs.getF32() * rhs.getF32());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value((float)lhs.getF32() / rhs.getF32());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(fmodf(lhs.getF32(), rhs.getF32()));
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() && rhs.getF32());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() || rhs.getF32());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() == rhs.getF32());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() != rhs.getF32());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() < rhs.getF32());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() > rhs.getF32());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() <= rhs.getF32());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::F32)
										break;
									result = Value(lhs.getF32() >= rhs.getF32());
									break;
							}
							break;
						case ValueType::F64:
							switch (j.opcode) {
								case Opcode::ADD:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value((double)lhs.getF64() + rhs.getF64());
									break;
								case Opcode::SUB:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value((double)lhs.getF64() - rhs.getF64());
									break;
								case Opcode::MUL:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value((double)lhs.getF64() * rhs.getF64());
									break;
								case Opcode::DIV:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value((double)lhs.getF64() / rhs.getF64());
									break;
								case Opcode::MOD:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(fmod(lhs.getF64(), rhs.getF64()));
									break;
								case Opcode::LAND:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() && rhs.getF64());
									break;
								case Opcode::LOR:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() || rhs.getF64());
									break;
								case Opcode::EQ:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() == rhs.getF64());
									break;
								case Opcode::NEQ:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() != rhs.getF64());
									break;
								case Opcode::LT:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() < rhs.getF64());
									break;
								case Opcode::GT:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() > rhs.getF64());
									break;
								case Opcode::LTEQ:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() <= rhs.getF64());
									break;
								case Opcode::GTEQ:
									if (rhs.valueType != ValueType::F64)
										break;
									result = Value(lhs.getF64() >= rhs.getF64());
									break;
							}
							break;
					}

					if (result.valueType != ValueType::Undefined) {
						optiContext.constRegValues[destReg] = result;
						optiContext.omittableRegs.insert(destReg);
					}
				}
			}
		}
	}
}

static bool _simplifyCommonSubexpr(OptimizerContext &optiContext) {
	// Key: outputless calculation instruction
	// Value: Index to register that stores the calculated result
	std::map<Instruction, uint32_t> calculatedSubexpressions;

	bool foundCommonSubexpr = false;

	for (auto i : optiContext.programBlocks) {
		for (auto &j : i->instructions) {
			auto outputlessIns = j;

			outputlessIns.output = {};

			if (j.output.valueType != ValueType::Undefined) {
				if (auto it = calculatedSubexpressions.find(outputlessIns);
					it != calculatedSubexpressions.end()) {
					j.opcode = Opcode::MOV;
					j.operands = { Value(ValueType::RegRef, it->second) };

					foundCommonSubexpr = true;
				} else {
					switch (j.opcode) {
						case Opcode::LOAD:
						case Opcode::ADD:
						case Opcode::SUB:
						case Opcode::MUL:
						case Opcode::DIV:
						case Opcode::MOD:
						case Opcode::AND:
						case Opcode::OR:
						case Opcode::XOR:
						case Opcode::LAND:
						case Opcode::LOR:
						case Opcode::EQ:
						case Opcode::NEQ:
						case Opcode::LT:
						case Opcode::GT:
						case Opcode::LTEQ:
						case Opcode::GTEQ:
						case Opcode::LSH:
						case Opcode::RSH:
						case Opcode::NOT:
						case Opcode::LNOT:
						case Opcode::NEG:
						case Opcode::AT:
							calculatedSubexpressions[outputlessIns] = j.output.getRegIndex();
							continue;
					}
				}
			}
		}
	}

	return foundCommonSubexpr;
}

static void _eliminateOmittableRegs(OptimizerContext &optiContext) {
	// Eliminate omittable registers.
	for (auto i : optiContext.programBlocks) {
		std::vector<Instruction> newInstructions;

		for (size_t j = 0; j < i->instructions.size(); ++j) {
			auto curIns = i->instructions[j];

			// Skip assignment of eliminatable registers
			if (curIns.output.valueType == ValueType::RegRef) {
				if (optiContext.omittableRegs.count(curIns.output.getRegIndex())) {
					continue;
				}
			}

			// Eliminate register allocations for eliminated registers.
			if (curIns.opcode == Opcode::REG) {
				if (optiContext.omittableRegs.count(curIns.operands[0].getU32()))
					continue;
			}

			// Replace references to eliminated registers with the constant value.
			for (auto &k : curIns.operands) {
				if (k.valueType == ValueType::RegRef) {
					uint32_t regIndex = k.getRegIndex();

					if (auto it = optiContext.constRegValues.find(regIndex);
						it != optiContext.constRegValues.end())
						k = it->second;
				}
			}

			newInstructions.push_back(curIns);
		}

		i->instructions = std::move(newInstructions);
	}
}

static std::vector<Instruction> _expandProgBlocks(OptimizerContext &optiContext) {
	std::vector<Instruction> expandedInstructions;

	// Expand program blocks.
	for (size_t i = 0; i < optiContext.managedProgBlocks.size(); ++i) {
		auto curProgBlock = optiContext.programBlocks[i];
		size_t index = expandedInstructions.size();

		expandedInstructions.resize(
			expandedInstructions.size() +
			curProgBlock->instructions.size());

		std::copy_n(
			curProgBlock->instructions.begin(),
			curProgBlock->instructions.size(),
			expandedInstructions.begin() + index);

		optiContext.programBlocksOffsets[i] = index;
	}

	return expandedInstructions;
}

static void _reindexRegs(
	OptimizerContext &optiContext,
	std::vector<Instruction> &expandedInstructions) {
	// Scan and reallocate registers.
	for (auto &i : expandedInstructions) {
		switch (i.opcode) {
			case Opcode::REG: {
				uint32_t regIndex = i.operands[0].getU32();
				uint32_t newIndex = optiContext.regRemappingMap.size();

				optiContext.regRemappingMap[regIndex] = newIndex;

				i.operands[0] = Value(newIndex);

				break;
			}
		}
	}
	// Replace old register indices with the new indices.
	for (auto &i : expandedInstructions) {
		if (i.output.valueType == ValueType::RegRef) {
			uint32_t regIndex = i.output.getRegIndex();

			i.output = Value(ValueType::RegRef, optiContext.regRemappingMap.at(regIndex));
		}

		for (auto &j : i.operands) {
			if (j.valueType == ValueType::RegRef) {
				uint32_t regIndex = j.getRegIndex();

				j = Value(ValueType::RegRef, optiContext.regRemappingMap.at(regIndex));
			}
		}
	}
}

static void _redirectJumpInstructions(
	OptimizerContext &optiContext,
	std::vector<Instruction> &expandedInstructions) {
	// Replace jump destinations with new offsets.
	for (auto &i : expandedInstructions) {
		switch (i.opcode) {
			case Opcode::JMP:
			case Opcode::JT:
			case Opcode::JF: {
				size_t index = optiContext.programBlocksIndices.byOffset[i.operands[0].getU32()];
				i.operands[0] = Value(
					optiContext.programBlocksOffsets[index]);
				break;
			}
		}
	}
}

void slake::trimFnInstructions(std::vector<Instruction> &instructions) {
	OptimizerContext optiContext(instructions);

	_genControlFlowGraph(optiContext);

	do {
		_scanForOmittableRegs(optiContext);
		_eliminateOmittableRegs(optiContext);
	} while (_simplifyCommonSubexpr(optiContext));

	std::vector<Instruction> expandedInstructions = _expandProgBlocks(optiContext);

	_reindexRegs(optiContext, expandedInstructions);
	_redirectJumpInstructions(optiContext, expandedInstructions);

	instructions = std::move(expandedInstructions);
}
