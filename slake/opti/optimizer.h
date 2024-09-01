#ifndef _SLAKE_OPTI_OPTIMIZER_H_
#define _SLAKE_OPTI_OPTIMIZER_H_

#include "../runtime.h"

namespace slake {
	constexpr static Opcode PHI_OPCODE = (Opcode)UINT16_MAX;

	enum class ProgBlockExitKind {
		Fallthrough = 0,
		Jump,
		JumpIfTrue,
		JumpIfFalse,
		Return
	};

	struct ProgBlock {
		std::vector<Instruction> instructions;

		ProgBlockExitKind exitKind;
		union {
			struct {
				ProgBlock *trueBranchDest, *falseBranchDest;
			} conditional;
			ProgBlock *direct;
		} exitDests;
	};

	struct OptimizerContext {
		std::vector<Instruction> &instructions;

		std::set<std::unique_ptr<ProgBlock>> managedProgBlocks;

		std::vector<ProgBlock *> programBlocks;
		struct {
			std::unordered_map<std::string, size_t> byLabel;
			std::map<uint32_t, size_t> byOffset;
		} programBlocksIndices;

		std::map<uint32_t, Value> constRegValues;
		std::set<uint32_t> omittableRegs;

		std::map<size_t, uint32_t> programBlocksOffsets;
		std::map<uint32_t, uint32_t> regRemappingMap;

		OptimizerContext(std::vector<Instruction> &instructions);
	};

	bool extractConstantValue(
		OptimizerContext &optiContext,
		const Value &value,
		Value &valueOut);
	bool isFnOptimizable(std::vector<Instruction> &instructions);
	void trimFnInstructions(std::vector<Instruction> &instructions);
}

#endif
