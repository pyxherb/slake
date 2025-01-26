#include "store.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

InternalExceptionPointer slake::jit::x86_64::compileStoreInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) {
	opti::RegAnalyzedInfo &regAnalyzedInfo = analyzedInfo.analyzedRegInfo.at(curIns.operands[0].getRegIndex());
	Value rhs = curIns.operands[1];

	switch (regAnalyzedInfo.storageType) {
		case opti::RegStorageType::GlobalVar:
			// TODO: Implement it.
			break;
		case opti::RegStorageType::LocalVar: {
			LocalVarState &localVarState = compileContext.localVarStates.at(regAnalyzedInfo.storageInfo.asLocalVar.off);

			switch (localVarState.type.typeId) {
				case TypeId::Value: {
					if (rhs.valueType == ValueType::RegRef) {
						switch (localVarState.type.getValueTypeExData()) {
							case ValueType::I8:
							case ValueType::U8:
							case ValueType::Bool: {
								uint32_t regOff = rhs.getRegIndex();
								VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

								if (vregState.saveOffset != INT32_MIN) {
									RegisterId tmpGpRegId = compileContext.allocGpReg();
									int32_t tmpGpRegOff = INT32_MIN;
									size_t tmpGpRegSize;

									if (compileContext.isRegInUse(tmpGpRegId)) {
										compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}

									compileContext.pushIns(emitMovMemToReg8Ins(
										tmpGpRegId,
										MemoryLocation{
											REG_RBP, vregState.saveOffset,
											REG_MAX, 0 }));
									compileContext.pushIns(emitMovReg8ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										tmpGpRegId));

									if (tmpGpRegOff != INT32_MIN) {
										compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}
								} else {
									compileContext.pushIns(emitMovReg8ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										vregState.phyReg));
								}
								break;
							}
							case ValueType::I16:
							case ValueType::U16: {
								uint32_t regOff = rhs.getRegIndex();
								VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

								if (vregState.saveOffset != INT32_MIN) {
									RegisterId tmpGpRegId = compileContext.allocGpReg();
									int32_t tmpGpRegOff = INT32_MIN;
									size_t tmpGpRegSize;

									if (compileContext.isRegInUse(tmpGpRegId)) {
										compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}

									compileContext.pushIns(emitMovMemToReg16Ins(
										tmpGpRegId,
										MemoryLocation{
											REG_RBP, vregState.saveOffset,
											REG_MAX, 0 }));
									compileContext.pushIns(emitMovReg16ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										tmpGpRegId));

									if (tmpGpRegOff != INT32_MIN) {
										compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}
								} else {
									compileContext.pushIns(emitMovReg16ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										vregState.phyReg));
								}
								break;
							}
							case ValueType::I32:
							case ValueType::U32:
							case ValueType::F32: {
								uint32_t regOff = rhs.getRegIndex();
								VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

								if (vregState.saveOffset != INT32_MIN) {
									RegisterId tmpGpRegId = compileContext.allocGpReg();
									int32_t tmpGpRegOff = INT32_MIN;
									size_t tmpGpRegSize;

									if (compileContext.isRegInUse(tmpGpRegId)) {
										compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}

									compileContext.pushIns(emitMovMemToReg32Ins(
										tmpGpRegId,
										MemoryLocation{
											REG_RBP, vregState.saveOffset,
											REG_MAX, 0 }));
									compileContext.pushIns(emitMovReg32ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										tmpGpRegId));

									if (tmpGpRegOff != INT32_MIN) {
										compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}
								} else {
									compileContext.pushIns(emitMovReg32ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										vregState.phyReg));
								}
								break;
							}
							case ValueType::I64:
							case ValueType::U64:
							case ValueType::F64: {
								uint32_t regOff = rhs.getRegIndex();
								VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

								if (vregState.saveOffset != INT32_MIN) {
									RegisterId tmpGpRegId = compileContext.allocGpReg();
									int32_t tmpGpRegOff = INT32_MIN;
									size_t tmpGpRegSize;

									if (compileContext.isRegInUse(tmpGpRegId)) {
										compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}

									compileContext.pushIns(emitMovMemToReg64Ins(
										tmpGpRegId,
										MemoryLocation{
											REG_RBP, vregState.saveOffset,
											REG_MAX, 0 }));
									compileContext.pushIns(emitMovReg64ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										tmpGpRegId));

									if (tmpGpRegOff != INT32_MIN) {
										compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
									}
								} else {
									compileContext.pushIns(emitMovReg64ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										vregState.phyReg));
								}
								break;
							}
						}
					} else {
						switch (localVarState.type.getValueTypeExData()) {
							case ValueType::I8: {
								int8_t imm0 = rhs.getI8();
								compileContext.pushIns(
									emitMovImm8ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::I16: {
								int16_t imm0 = rhs.getI16();
								compileContext.pushIns(
									emitMovImm16ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::I32: {
								int32_t imm0 = rhs.getI32();
								compileContext.pushIns(
									emitMovImm32ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::I64: {
								int64_t imm0 = rhs.getI64();
								compileContext.pushIns(
									emitMovImm64ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::U8: {
								uint8_t imm0 = rhs.getU8();
								compileContext.pushIns(
									emitMovImm8ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::U16: {
								uint16_t imm0 = rhs.getU16();
								compileContext.pushIns(
									emitMovImm16ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::U32: {
								uint16_t imm0 = rhs.getU16();
								compileContext.pushIns(
									emitMovImm16ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::U64: {
								uint64_t imm0 = rhs.getU64();
								compileContext.pushIns(
									emitMovImm64ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::Bool: {
								bool imm0 = rhs.getBool();
								compileContext.pushIns(
									emitMovImm8ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::F32: {
								float imm0 = rhs.getF32();
								compileContext.pushIns(
									emitMovImm32ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
							case ValueType::F64: {
								double imm0 = rhs.getF64();
								compileContext.pushIns(
									emitMovImm64ToMemIns(
										MemoryLocation{
											REG_RBP, localVarState.stackOff,
											REG_MAX, 0 },
										(uint8_t *)&imm0));
								break;
							}
						}
					}
				}
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::FnDelegate: {
					if (rhs.valueType == ValueType::RegRef) {
						uint32_t regOff = rhs.getRegIndex();
						VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

						if (vregState.saveOffset != INT32_MIN) {
							RegisterId tmpGpRegId = compileContext.allocGpReg();
							int32_t tmpGpRegOff = INT32_MIN;
							size_t tmpGpRegSize;

							if (compileContext.isRegInUse(tmpGpRegId)) {
								compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
							}

							compileContext.pushIns(emitMovMemToReg64Ins(
								tmpGpRegId,
								MemoryLocation{
									REG_RBP, vregState.saveOffset,
									REG_MAX, 0 }));
							compileContext.pushIns(emitMovReg64ToMemIns(
								MemoryLocation{
									REG_RBP, localVarState.stackOff,
									REG_MAX, 0 },
								tmpGpRegId));

							if (tmpGpRegOff != INT32_MIN) {
								compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize);
							}
						} else {
							compileContext.pushIns(emitMovReg64ToMemIns(
								MemoryLocation{
									REG_RBP, localVarState.stackOff,
									REG_MAX, 0 },
								vregState.phyReg));
						}
					} else {
						Object *imm0 = rhs.getObjectRef();
						compileContext.pushIns(
							emitMovImm64ToMemIns(
								MemoryLocation{
									REG_RBP, localVarState.stackOff,
									REG_MAX, 0 },
								(uint8_t *)&imm0));
					}
					break;
				}
			}
			break;
		}
		case opti::RegStorageType::ArgRef:
			// TODO: Implement it.
			break;
	}

	return {};
}
