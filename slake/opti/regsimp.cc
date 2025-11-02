#include "regsimp.h"

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer opti::simplifyRegularFnOverloading(
	Runtime *runtime,
	peff::Alloc *resourceAllocator,
	RegularFnOverloadingObject *fnObject,
	const ProgramAnalyzedInfo &analyzedInfoOut,
	HostRefHolder &hostRefHolder) {
	peff::Option<ProgramAnalyzedInfo> prevNewAnalyzedInfo;
	const ProgramAnalyzedInfo *pAnalyzedInfo = &analyzedInfoOut;
	bool deadRegistersFound;

	do {
		deadRegistersFound = false;
		peff::Map<uint32_t, uint32_t> originalLabelToNewLabelMap(resourceAllocator);
		peff::Set<uint32_t> insMarkedForRemoval(resourceAllocator);

		for (auto i : pAnalyzedInfo->codeBlockBoundaries) {
			originalLabelToNewLabelMap.insert(+i, +i);
		}

		for (uint32_t i = 0; i < fnObject->instructions.size(); ++i) {
			const Instruction &curIns = fnObject->instructions.at(i);

			bool isSimplifiable = false;

			if (curIns.output != UINT32_MAX) {
				const RegAnalyzedInfo &info = pAnalyzedInfo->analyzedRegInfo.at(curIns.output);

				if (info.lifetime.offBeginIns == info.lifetime.offEndIns)
					isSimplifiable = true;
			} else
				isSimplifiable = true;

			if (isSimplifiable) {
				if (isInstructionSimplifiable(curIns.opcode)) {
					for (auto it = originalLabelToNewLabelMap.findMaxLteq(i); it != originalLabelToNewLabelMap.end(); ++it) {
						--it.value();
					}

					if (!insMarkedForRemoval.insert(+i))
						return OutOfMemoryError::alloc();
				}
			}

			for (size_t j = 0; j < curIns.nOperands; ++j) {
				Value &curValue = curIns.operands[j];

				if (curValue.valueType == ValueType::RegIndex) {
					uint32_t regIndex = curValue.getRegIndex();

					if (Value v = pAnalyzedInfo->analyzedRegInfo.at(regIndex).expectedValue;
						(v.valueType != ValueType::Undefined) &&
						(v.valueType != ValueType::Reference)) {
						curValue = v;
					}
				}
			}
		}

		peff::DynArray<Instruction> newBody(fnObject->instructions.allocator());

		for (uint32_t i = 0; i < fnObject->instructions.size(); ++i) {
			if (insMarkedForRemoval.contains(i))
				continue;
			if (!newBody.pushBack(std::move(fnObject->instructions.at(i))))
				return OutOfMemoryError::alloc();
		}

		fnObject->instructions = std::move(newBody);

		for (uint32_t i = 0; i < fnObject->instructions.size(); ++i) {
			Instruction &curIns = fnObject->instructions.at(i);

			switch (curIns.opcode) {
				case Opcode::JMP:
					curIns.operands[0] = Value((uint32_t)originalLabelToNewLabelMap.at(curIns.operands[0].getU32()));
					break;
				case Opcode::JT:
					curIns.operands[0] = Value((uint32_t)originalLabelToNewLabelMap.at(curIns.operands[0].getU32()));
					break;
				case Opcode::JF:
					curIns.operands[0] = Value((uint32_t)originalLabelToNewLabelMap.at(curIns.operands[0].getU32()));
					break;
				case Opcode::PHI:
					for (size_t i = 0; i < curIns.nOperands; i += 2) {
						if (curIns.operands[i].getU32() != UINT32_MAX)
							curIns.operands[i] = Value((uint32_t)originalLabelToNewLabelMap.at(curIns.operands[i].getU32()));
					}
					break;
			}
		}

		ProgramAnalyzedInfo newAnalyzedInfo(runtime, resourceAllocator);
		SLAKE_RETURN_IF_EXCEPT(analyzeProgramInfo(runtime, resourceAllocator, fnObject, newAnalyzedInfo, hostRefHolder));

		for (auto i : newAnalyzedInfo.analyzedRegInfo) {
			if (i.second.lifetime.offEndIns == i.second.lifetime.offBeginIns) {
				deadRegistersFound = true;
				break;
			}
		}

		prevNewAnalyzedInfo = std::move(newAnalyzedInfo);
		pAnalyzedInfo = &prevNewAnalyzedInfo.value();
	} while (deadRegistersFound);
	return {};
}
