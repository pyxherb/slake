#include <cstdint>
#include <cstring>

#include "common.h"
#include <slake/opti/proganal.h>

#include "emitters.h"
#include "comp/arithm.h"

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

			int32_t offSavedRax = INT32_MIN,
					offSavedR10 = INT32_MIN,
					offSavedR11 = INT32_MIN;
			size_t szSavedRax,
				szSavedR10,
				szSavedR11;
			int32_t offSavedRcx, offSavedRdx, offSavedR8, offSavedR9;
			size_t szSavedRcx, szSavedRdx, szSavedR8, szSavedR9;

			// Save parameter registers.
			compileContext.pushReg(REG_RCX, offSavedRcx, szSavedRcx);
			compileContext.pushReg(REG_RDX, offSavedRdx, szSavedRdx);
			compileContext.pushReg(REG_R8, offSavedR8, szSavedR8);
			compileContext.pushReg(REG_R9, offSavedR9, szSavedR9);

			// Save scratch registers.
			if (compileContext.isRegInUse(REG_RAX) > 0) {
				compileContext.pushReg(REG_RAX, offSavedRax, szSavedRax);
			}
			if (compileContext.isRegInUse(REG_R10) > 0) {
				compileContext.pushReg(REG_R10, offSavedR10, szSavedR10);
			}
			if (compileContext.isRegInUse(REG_R11) > 0) {
				compileContext.pushReg(REG_R11, offSavedR11, szSavedR11);
			}

			IdRefObject *refObj = (IdRefObject *)curIns.operands[0].getObjectRef();

			{
				{
					DEF_INS_BUFFER(ins, 0x48, 0x89, 0xd9);	// mov rcx, rbx
					compileContext.pushIns(emitRawIns(sizeof(ins), ins));
				}

				{
					uintptr_t loadInsWrapperAddr = (uintptr_t)loadInsWrapper;
					compileContext.pushIns(emitMovImm64ToReg64Ins(REG_RAX, (uint8_t *)&loadInsWrapperAddr));

					DEF_INS_BUFFER(ins, 0xff, 0xd0);  // call rax
					compileContext.pushIns(emitRawIns(sizeof(ins), ins));
				}
			}

			// Restore scratch registers.
			if (offSavedR11 != INT32_MIN) {
				compileContext.popReg(REG_R11, offSavedR11, szSavedR11);
			}
			if (offSavedR10 != INT32_MIN) {
				compileContext.popReg(REG_R10, offSavedR10, szSavedR10);
			}
			if (offSavedRax != INT32_MIN) {
				compileContext.popReg(REG_RAX, offSavedRax, szSavedRax);
			}

			// Restore parameter registers.
			compileContext.popReg(REG_R9, offSavedR9, szSavedR9);
			compileContext.popReg(REG_R8, offSavedR8, szSavedR8);
			compileContext.popReg(REG_RDX, offSavedRdx, szSavedRdx);
			compileContext.popReg(REG_RCX, offSavedRcx, szSavedRcx);
			break;
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT(compileAddInstruction(compileContext, analyzedInfo, offIns, curIns));
			break;
		}
		case Opcode::MOV: {
			uint32_t outputRegIndex = curIns.output.getRegIndex();

			Value expectedValue = analyzedInfo.analyzedRegInfo.at(outputRegIndex).expectedValue;

			// Check if the instruction is pre-evaluatable.
			if (expectedValue.valueType != ValueType::Undefined) {
				Value src = curIns.operands[0];
				RegisterId regId = compileContext.allocGpReg();

				switch (src.valueType) {
					case ValueType::I8: {
						int8_t imm0 = src.getI8();

						compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::I16: {
						int16_t imm0 = src.getI16();

						compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::I32: {
						int32_t imm0 = src.getI32();

						compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::I64: {
						int64_t imm0 = src.getI64();

						compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::U8: {
						uint8_t imm0 = src.getU8();

						compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::U16: {
						uint16_t imm0 = src.getU16();

						compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::U32: {
						uint32_t imm0 = src.getU32();

						compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::U64: {
						uint64_t imm0 = src.getU64();

						compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::Bool: {
						uint8_t imm0 = src.getBool();

						compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0));
						break;
					}
					case ValueType::RegRef: {
						uint32_t srcRegIndex = src.getRegIndex();
						auto &srcRegInfo = analyzedInfo.analyzedRegInfo.at(srcRegIndex);
						auto &srcVregInfo = compileContext.virtualRegStates.at(srcRegIndex);
						Type &srcRegType = srcRegInfo.type;
						switch (srcRegType.typeId) {
							case TypeId::Value: {
								switch (srcRegType.getValueTypeExData()) {
									case ValueType::I8:
									case ValueType::U8:
									case ValueType::Bool: {
										if (srcVregInfo.saveOffset != INT32_MIN) {
											compileContext.pushIns(
												emitMovMemToReg8Ins(
													regId,
													MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 }));
										} else {
											compileContext.pushIns(emitMovReg8ToReg8Ins(regId, srcVregInfo.phyReg));
										}

										VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint8_t));
										break;
									}
									case ValueType::I16:
									case ValueType::U16: {
										if (srcVregInfo.saveOffset != INT32_MIN) {
											compileContext.pushIns(
												emitMovMemToReg16Ins(
													regId,
													MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 }));
										} else {
											compileContext.pushIns(emitMovReg16ToReg16Ins(regId, srcVregInfo.phyReg));
										}

										VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint16_t));
										break;
									}
									case ValueType::I32:
									case ValueType::U32: {
										if (srcVregInfo.saveOffset != INT32_MIN) {
											compileContext.pushIns(
												emitMovMemToReg32Ins(
													regId,
													MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 }));
										} else {
											compileContext.pushIns(emitMovReg32ToReg32Ins(regId, srcVregInfo.phyReg));
										}

										VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint32_t));
										break;
									}
									case ValueType::I64:
									case ValueType::U64: {
										if (srcVregInfo.saveOffset != INT32_MIN) {
											compileContext.pushIns(
												emitMovMemToReg64Ins(
													regId,
													MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 }));
										} else {
											compileContext.pushIns(emitMovReg64ToReg64Ins(regId, srcVregInfo.phyReg));
										}

										VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint64_t));
										break;
									}
								}
								break;
							}
						}
						break;
					}
					default:
						assert(("Unhandled value type", false));
				}
			} else {
			}
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
