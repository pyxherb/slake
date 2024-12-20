#include "mov.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

InternalExceptionPointer slake::jit::x86_64::compileMovInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) {
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	Value src = curIns.operands[0];

	switch (src.valueType) {
		case ValueType::I8: {
			int8_t imm0 = src.getI8();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int8_t));
			break;
		}
		case ValueType::I16: {
			int16_t imm0 = src.getI16();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int16_t));
			break;
		}
		case ValueType::I32: {
			int32_t imm0 = src.getI32();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm32ToReg32Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int32_t));
			break;
		}
		case ValueType::I64: {
			int64_t imm0 = src.getI64();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm64ToReg64Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int64_t));
			break;
		}
		case ValueType::U8: {
			uint8_t imm0 = src.getU8();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint8_t));
			break;
		}
		case ValueType::U16: {
			uint16_t imm0 = src.getU16();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint16_t));
			break;
		}
		case ValueType::U32: {
			uint32_t imm0 = src.getU32();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm32ToReg32Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint32_t));
			break;
		}
		case ValueType::U64: {
			uint64_t imm0 = src.getU64();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm64ToReg64Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint64_t));
			break;
		}
		case ValueType::Bool: {
			bool imm0 = src.getBool();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(bool));
			break;
		}
		case ValueType::F32: {
			float imm0 = src.getF32();

			RegisterId tmpRegId = compileContext.allocGpReg();
			int32_t tmpRegOff = INT32_MIN;
			size_t tmpRegSize;
			if (compileContext.isRegInUse(tmpRegId)) {
				compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize);
			}
			compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&imm0));

			RegisterId xmmRegId = compileContext.allocXmmReg();
			if (compileContext.isRegInUse(xmmRegId)) {
				int32_t off;
				size_t size;
				compileContext.pushRegXmm(xmmRegId, off, size);
			}
			compileContext.pushIns(emitMovdReg32ToRegXmmIns(xmmRegId, tmpRegId));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, xmmRegId, sizeof(float));

			if (tmpRegOff != INT32_MIN) {
				compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize);
			}
			break;
		}
		case ValueType::F64: {
			double imm0 = src.getF64();

			RegisterId tmpRegId = compileContext.allocGpReg();
			int32_t tmpRegOff = INT32_MIN;
			size_t tmpRegSize;
			if (compileContext.isRegInUse(tmpRegId)) {
				compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize);
			}
			compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&imm0));

			RegisterId xmmRegId = compileContext.allocXmmReg();
			if (compileContext.isRegInUse(xmmRegId)) {
				int32_t off;
				size_t size;
				compileContext.pushRegXmm(xmmRegId, off, size);
			}
			compileContext.pushIns(emitMovdReg32ToRegXmmIns(xmmRegId, tmpRegId));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, xmmRegId, sizeof(double));

			if (tmpRegOff != INT32_MIN) {
				compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize);
			}
			break;
		}
		case ValueType::ObjectRef: {
			Object *imm0 = src.getObjectRef();

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(regId, off, size);
			}
			compileContext.pushIns(emitMovImm64ToReg64Ins(regId, (uint8_t *)&imm0));
			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(Object *));
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
							RegisterId regId = compileContext.allocGpReg();
							if (compileContext.isRegInUse(regId)) {
								int32_t off;
								size_t size;
								compileContext.pushReg(regId, off, size);
							}
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
							RegisterId regId = compileContext.allocGpReg();
							if (compileContext.isRegInUse(regId)) {
								int32_t off;
								size_t size;
								compileContext.pushReg(regId, off, size);
							}
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
							RegisterId regId = compileContext.allocGpReg();
							if (compileContext.isRegInUse(regId)) {
								int32_t off;
								size_t size;
								compileContext.pushReg(regId, off, size);
							}
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
						case ValueType::U64:
						case ValueType::ObjectRef: {
							RegisterId regId = compileContext.allocGpReg();
							if (compileContext.isRegInUse(regId)) {
								int32_t off;
								size_t size;
								compileContext.pushReg(regId, off, size);
							}
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
						case ValueType::F32: {
							RegisterId regId = compileContext.allocXmmReg();
							if (compileContext.isRegInUse(regId)) {
								int32_t off;
								size_t size;
								compileContext.pushRegXmm(regId, off, size);
							}
							if (srcVregInfo.saveOffset != INT32_MIN) {
								compileContext.pushIns(
									emitMovdMemToRegXmmIns(
										regId,
										MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 }));
							} else {
								compileContext.pushIns(emitMovqRegXmmToRegXmmIns(regId, srcVregInfo.phyReg));
							}

							VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(float));
							break;
						}
						case ValueType::F64: {
							RegisterId regId = compileContext.allocXmmReg();
							if (compileContext.isRegInUse(regId)) {
								int32_t off;
								size_t size;
								compileContext.pushRegXmm(regId, off, size);
							}
							if (srcVregInfo.saveOffset != INT32_MIN) {
								compileContext.pushIns(
									emitMovqMemToRegXmmIns(
										regId,
										MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 }));
							} else {
								compileContext.pushIns(emitMovqRegXmmToRegXmmIns(regId, srcVregInfo.phyReg));
							}

							VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(double));
							break;
						}
						case ValueType::VarRef: {
							int32_t off = compileContext.stackAllocAligned(sizeof(VarRef), sizeof(VarRef));

							{
								RegisterId tmpRegId = compileContext.allocGpReg();
								int32_t tmpRegOff = INT32_MIN;
								size_t tmpRegSize;
								if (compileContext.isRegInUse(tmpRegId)) {
									compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize);
								}

								compileContext.pushIns(emitMovReg64ToReg64Ins(tmpRegId, REG_RBP));
								compileContext.pushIns(emitSubImm32ToReg64Ins(tmpRegId, (uint8_t *)&srcVregInfo.saveOffset));

								compileContext.pushIns(emitMovImm64ToReg64Ins(REG_RCX, tmpRegId));

								if (tmpRegOff != INT32_MIN) {
									compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize);
								}
							}
							{
								RegisterId tmpRegId = compileContext.allocGpReg();
								int32_t tmpRegOff = INT32_MIN;
								size_t tmpRegSize;
								if (compileContext.isRegInUse(tmpRegId)) {
									compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize);
								}

								compileContext.pushIns(emitMovReg64ToReg64Ins(tmpRegId, REG_RBP));
								compileContext.pushIns(emitSubImm32ToReg64Ins(tmpRegId, (uint8_t *)&off));

								compileContext.pushIns(emitMovImm64ToReg64Ins(REG_RDX, tmpRegId));

								if (tmpRegOff != INT32_MIN) {
									compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize);
								}
							}
							{
								uint64_t size = sizeof(VarRef);
								compileContext.pushIns(emitMovImm64ToReg64Ins(REG_R8, (uint8_t*)&size));
							}

							CallingRegSavingInfo callingRegSavingInfo;

							compileContext.saveCallingRegs(callingRegSavingInfo);

							compileContext.pushIns(emitCallIns((void *)memcpyWrapper));

							compileContext.restoreCallingRegs(callingRegSavingInfo);

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

	return {};
}
