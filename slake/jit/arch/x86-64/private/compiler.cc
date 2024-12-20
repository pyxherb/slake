#include <cstdint>
#include <cstring>

#include "common.h"
#include <slake/opti/proganal.h>

#include "emitters.h"
#include "comp/add.h"
#include "comp/sub.h"
#include "comp/mul.h"
#include "comp/div.h"
#include "comp/mod.h"
#include "comp/mov.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

InternalExceptionPointer compileInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) {
	uint32_t outputRegIndex = UINT32_MAX;
	if (curIns.output.valueType != ValueType::Undefined) {
		outputRegIndex = curIns.output.getRegIndex();
		size_t offTimelineEnd = analyzedInfo.analyzedRegInfo.at(outputRegIndex).lifetime.offEndIns;

		compileContext.regRecycleBoundaries[offTimelineEnd].push_back(outputRegIndex);
	}

	if (auto it = compileContext.regRecycleBoundaries.find(offIns);
		it != compileContext.regRecycleBoundaries.end()) {
		for (auto i : it->second) {
			VirtualRegState &vregState = compileContext.virtualRegStates.at(i);

			if (vregState.saveOffset != INT32_MIN) {
				compileContext.stackFree(vregState.saveOffset, vregState.size);
			} else {
				compileContext.regAllocFlags.reset(vregState.phyReg);
			}
		}
		compileContext.regRecycleBoundaries.erase(it);
	}

	switch (curIns.opcode) {
		case Opcode::NOP:
			break;
		case Opcode::LOAD: {
			uint32_t outputRegIndex = curIns.output.getRegIndex();

			{
				Value expectedValue = analyzedInfo.analyzedRegInfo.at(outputRegIndex).expectedValue;

				if (expectedValue.valueType != ValueType::Undefined) {
					Instruction ins = { Opcode::MOV, curIns.output, { expectedValue } };
					compileInstruction(compileContext, analyzedInfo, SIZE_MAX, ins);
					return {};
				}
			}

			CallingRegSavingInfo callingRegSavingInfo;

			compileContext.saveCallingRegs(callingRegSavingInfo);

			IdRefObject *refObj = (IdRefObject *)curIns.operands[0].getObjectRef();

			compileContext.pushIns(emitMovImm64ToReg64Ins(REG_RDX, (uint8_t *)&refObj));

			compileContext.restoreCallingRegs(callingRegSavingInfo);

			break;
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT(compileAddInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT(compileSubInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT(compileMulInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT(compileDivInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT(compileDivInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
		case Opcode::MOV: {
			SLAKE_RETURN_IF_EXCEPT(compileMovInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
	}

	return {};
}

InternalExceptionPointer slake::compileRegularFn(RegularFnOverloadingObject *fn, const JITCompilerOptions &options) {
	slake::CodePage *codePage;
	size_t size;

	JITCompileContext compileContext;
	size_t nIns = fn->instructions.size();

	opti::ProgramAnalyzedInfo analyzedInfo;
	HostRefHolder hostRefHolder;

	InternalExceptionPointer exceptionPtr;

	SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptionPtr, opti::analyzeProgramInfo(fn->associatedRuntime, fn, analyzedInfo, hostRefHolder));

	for (size_t i = 0; i < nIns; ++i) {
		const Instruction &curIns = fn->instructions[i];
	}

	return {};
}
