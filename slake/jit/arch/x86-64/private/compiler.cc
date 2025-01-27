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
#include "comp/store.h"

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

		if(!compileContext.regRecycleBoundaries.contains(offTimelineEnd)) {
			compileContext.regRecycleBoundaries.insert(+offTimelineEnd, peff::List<uint32_t>(&compileContext.runtime->globalHeapPoolAlloc));
		}

		if(!compileContext.regRecycleBoundaries.at(offTimelineEnd).pushBack(+outputRegIndex))
			return OutOfMemoryError::alloc();
	}

	if (auto it = compileContext.regRecycleBoundaries.find(offIns);
		it != compileContext.regRecycleBoundaries.end()) {
		for (auto i : it.value()) {
			VirtualRegState &vregState = compileContext.virtualRegStates.at(i);

			if (vregState.saveOffset != INT32_MIN) {
				compileContext.stackFree(vregState.saveOffset, vregState.size);
			} else {
				compileContext.regAllocFlags.reset(vregState.phyReg);
			}
		}
		compileContext.regRecycleBoundaries.remove(it);
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

			SLAKE_RETURN_IF_EXCEPT(compileContext.saveCallingRegs(callingRegSavingInfo));

			// Pass the first argument.
			{
				SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_RCX, MemoryLocation{ REG_RBP, compileContext.jitContextOff, REG_MAX, 0 })));
			}

			// Pass the second argument.
			{
				IdRefObject *refObj = (IdRefObject *)curIns.operands[0].getObjectRef();

				SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovImm64ToReg64Ins(REG_RDX, (uint8_t *)&refObj)));
			}

			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitCallIns((void *)loadInsWrapper)));

			// Psas the first argument for the memcpy wrapper.
			int32_t stackOff;
			SLAKE_RETURN_IF_EXCEPT(compileContext.stackAllocAligned(sizeof(Value), sizeof(Value), stackOff));
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovReg64ToReg64Ins(REG_RCX, REG_RBP)));
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitAddImm32ToReg64Ins(REG_RCX, (uint8_t *)&stackOff)));

			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_RDX, MemoryLocation{ REG_RBP, compileContext.jitContextOff, REG_MAX, 0 })));

			// Psas the second argument for the memcpy wrapper.
			static int32_t returnValueOff = -(int32_t)offsetof(JITExecContext, returnValue);
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitAddImm32ToReg64Ins(REG_RDX, (uint8_t *)&returnValueOff)));

			// Psas the third argument for the memcpy wrapper.
			static uint64_t size = sizeof(Value);
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovImm64ToReg64Ins(REG_R8, (uint8_t *)&size)));

			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitCallIns((void *)memcpyWrapper)));

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, stackOff, sizeof(Value));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			SLAKE_RETURN_IF_EXCEPT(compileContext.restoreCallingRegs(callingRegSavingInfo));

			break;
		}
		case Opcode::RLOAD: {
			uint32_t outputRegIndex = curIns.output.getRegIndex(),
					 baseObjectRegIndex = curIns.operands[0].getRegIndex();

			{
				Value expectedValue = analyzedInfo.analyzedRegInfo.at(outputRegIndex).expectedValue;

				if (expectedValue.valueType != ValueType::Undefined) {
					Instruction ins = { Opcode::MOV, curIns.output, { expectedValue } };
					compileInstruction(compileContext, analyzedInfo, SIZE_MAX, ins);
					return {};
				}
			}

			CallingRegSavingInfo callingRegSavingInfo;

			SLAKE_RETURN_IF_EXCEPT(compileContext.saveCallingRegs(callingRegSavingInfo));

			// Pass the first argument.
			{
				SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_RCX, MemoryLocation{ REG_RBP, compileContext.jitContextOff, REG_MAX, 0 })));
			}

			// Pass the second argument.
			{
				auto &vregState = compileContext.virtualRegStates.at(baseObjectRegIndex);
				if (vregState.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(
						REG_RDX,
						MemoryLocation{
							REG_RBP,
							compileContext.virtualRegStates.at(baseObjectRegIndex).saveOffset,
							REG_MAX,
							0 })));
				} else {
					if (vregState.phyReg != REG_RDX) {
						SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovReg64ToReg64Ins(vregState.phyReg, REG_RDX)));
					}
				}
			}

			// Pass the third argument.
			{
				IdRefObject *refObj = (IdRefObject *)curIns.operands[1].getObjectRef();

				SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovImm64ToReg64Ins(REG_R8, (uint8_t *)&refObj)));
			}

			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitCallIns((void *)rloadInsWrapper)));

			// Psas the first argument for the memcpy wrapper.
			int32_t stackOff;
			SLAKE_RETURN_IF_EXCEPT(compileContext.stackAllocAligned(sizeof(Value), sizeof(Value), stackOff));
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovReg64ToReg64Ins(REG_RCX, REG_RBP)));
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitAddImm32ToReg64Ins(REG_RCX, (uint8_t *)&stackOff)));

			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_RDX, MemoryLocation{ REG_RBP, compileContext.jitContextOff, REG_MAX, 0 })));

			// Psas the second argument for the memcpy wrapper.
			static int32_t returnValueOff = -(int32_t)offsetof(JITExecContext, returnValue);
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitAddImm32ToReg64Ins(REG_RDX, (uint8_t *)&returnValueOff)));

			// Psas the third argument for the memcpy wrapper.
			static uint64_t size = sizeof(Value);
			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovImm64ToReg64Ins(REG_R8, (uint8_t *)&size)));

			SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitCallIns((void *)memcpyWrapper)));

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, stackOff, sizeof(Value));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			SLAKE_RETURN_IF_EXCEPT(compileContext.restoreCallingRegs(callingRegSavingInfo));

			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT(compileStoreInstruction(compileContext, analyzedInfo, offIns, curIns));
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

	opti::ProgramAnalyzedInfo analyzedInfo(fn->associatedRuntime);
	HostRefHolder hostRefHolder(&fn->associatedRuntime->globalHeapPoolAlloc);

	InternalExceptionPointer exceptionPtr;

	SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptionPtr, opti::analyzeProgramInfo(fn->associatedRuntime, fn, analyzedInfo, hostRefHolder));

	SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_R11, MemoryLocation{ REG_R9, offsetof(JITExecContext, stackLimit), REG_MAX, 0 })));

	SLAKE_RETURN_IF_EXCEPT(compileContext.pushPrologStackOpIns());

	// R11 is used for stack limit checking.
	compileContext.setRegAllocated(REG_R11);

	{
		SLAKE_RETURN_IF_EXCEPT(compileContext.initJITContextStorage());
		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(
			emitMovReg64ToMemIns(
				MemoryLocation{
					REG_RBP, compileContext.jitContextOff,
					REG_MAX, 0 },
				REG_R9)));

		for (size_t i = 0; i < nIns; ++i) {
			const Instruction &curIns = fn->instructions.at(i);

			SLAKE_RETURN_IF_EXCEPT(compileInstruction(compileContext, analyzedInfo, i, curIns));
		}

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushEpilogStackOpIns());

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitNearRetIns()));
	}

	{
		peff::String labelName;
		if(!labelName.build("_report_stack_overflow"))
			return OutOfMemoryError::alloc();
		SLAKE_RETURN_IF_EXCEPT(compileContext.pushLabel(std::move(labelName)));

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushEpilogStackOpIns());

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_R11, MemoryLocation{ REG_R9, offsetof(JITExecContext, stackOverflowError), REG_MAX, 0 })));
		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovReg64ToMemIns(MemoryLocation{ REG_R9, offsetof(JITExecContext, exception), REG_MAX, 0 }, REG_R11)));

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitNearRetIns()));
	}

	{
		peff::String labelName;
		if(!labelName.build("_report_stack_overflow_on_prolog"))
			return OutOfMemoryError::alloc();
		SLAKE_RETURN_IF_EXCEPT(compileContext.pushLabel(std::move(labelName)));

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovMemToReg64Ins(REG_R11, MemoryLocation{ REG_R9, offsetof(JITExecContext, stackOverflowError), REG_MAX, 0 })));
		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitMovReg64ToMemIns(MemoryLocation{ REG_R9, offsetof(JITExecContext, exception), REG_MAX, 0 }, REG_R11)));

		SLAKE_RETURN_IF_EXCEPT(compileContext.pushIns(emitNearRetIns()));
	}

	return {};
}
