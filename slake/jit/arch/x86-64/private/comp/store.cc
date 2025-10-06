#include "store.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <size_t size>
InternalExceptionPointer compileRegToFieldVarStoreInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns,
	FieldRecord &fieldRecord,
	char *rawDataPtr,
	uint32_t regOff) noexcept {
	InternalExceptionPointer exception;
	VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

	if (vregState.saveOffset != INT32_MIN) {
		RegisterId tmpGpRegId = compileContext.allocGpReg();
		int32_t tmpGpRegOff = INT32_MIN;
		size_t tmpGpRegSize;

		if (compileContext.isRegInUse(tmpGpRegId)) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
		}

		if constexpr (size == 1) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg8Ins(
															tmpGpRegId,
															MemoryLocation{
																REG_RBP, vregState.saveOffset,
																REG_MAX, 0 })));
		} else if constexpr (size == 2) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg16Ins(
															tmpGpRegId,
															MemoryLocation{
																REG_RBP, vregState.saveOffset,
																REG_MAX, 0 })));
		} else if constexpr (size == 4) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg32Ins(
															tmpGpRegId,
															MemoryLocation{
																REG_RBP, vregState.saveOffset,
																REG_MAX, 0 })));
		} else if constexpr (size == 8) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg64Ins(
															tmpGpRegId,
															MemoryLocation{
																REG_RBP, vregState.saveOffset,
																REG_MAX, 0 })));
		} else {
			static_assert(size != size, "Invalid operand size");
		}
		if (((uintptr_t)rawDataPtr) < INT32_MAX) {
			if constexpr (size == 1) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																tmpGpRegId)));
			} else if constexpr (size == 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																tmpGpRegId)));
			} else if constexpr (size == 4) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																tmpGpRegId)));
			} else if constexpr (size == 8) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																tmpGpRegId)));
			} else {
				static_assert(size != size, "Invalid operand size");
			}
		} else {
			RegisterId baseRegId = compileContext.allocGpReg();
			int32_t baseRegOff = INT32_MIN;
			size_t baseRegSize;

			if (compileContext.isRegInUse(baseRegId)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
			}

			uint64_t imm0 = (uint64_t)rawDataPtr;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
															baseRegId,
															(uint8_t *)&imm0)));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
															MemoryLocation{
																baseRegId, 0,
																REG_MAX, 0 },
															tmpGpRegId)));

			if (baseRegOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(baseRegId, baseRegOff, baseRegSize));
			}
		}

		if (tmpGpRegOff != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
		}
	} else {
		if (((uintptr_t)rawDataPtr) < INT32_MAX) {
			if constexpr (size == 1) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																vregState.phyReg)));
			} else if constexpr (size == 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																vregState.phyReg)));
			} else if constexpr (size == 4) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																vregState.phyReg)));
			} else if constexpr (size == 8) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																MemoryLocation{
																	REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																	REG_MAX, 0 },
																vregState.phyReg)));
			}
		} else {
			RegisterId baseRegId = compileContext.allocGpReg();

			if (baseRegId == vregState.phyReg) {
				int32_t vregRegOff = INT32_MIN;
				size_t vregRegSize;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(vregState.phyReg, vregRegOff, vregRegSize));

				uint64_t imm0 = (uint64_t)rawDataPtr;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																baseRegId,
																(uint8_t *)&imm0)));

				RegisterId tmpGpRegId = compileContext.allocGpReg();
				int32_t tmpGpRegOff = INT32_MIN;
				size_t tmpGpRegSize;

				if (compileContext.isRegInUse(tmpGpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
				}

				if constexpr (size == 1) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg8Ins(
																	tmpGpRegId,
																	MemoryLocation{
																		REG_RBP, vregState.saveOffset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	tmpGpRegId)));
				} else if constexpr (size == 2) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg16Ins(
																	tmpGpRegId,
																	MemoryLocation{
																		REG_RBP, vregState.saveOffset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	tmpGpRegId)));
				} else if constexpr (size == 4) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg32Ins(
																	tmpGpRegId,
																	MemoryLocation{
																		REG_RBP, vregState.saveOffset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	tmpGpRegId)));
				} else if constexpr (size == 8) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg64Ins(
																	tmpGpRegId,
																	MemoryLocation{
																		REG_RBP, vregState.saveOffset,
																		REG_MAX, 0 })));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	tmpGpRegId)));
				}

				if (tmpGpRegOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
				}

			} else {
				int32_t baseRegOff = INT32_MIN;
				size_t baseRegSize;

				if (compileContext.isRegInUse(baseRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
				}

				uint64_t imm0 = (uint64_t)rawDataPtr;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																baseRegId,
																(uint8_t *)&imm0)));

				if constexpr (size == 1) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	vregState.phyReg)));
				} else if constexpr (size == 2) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	vregState.phyReg)));
				} else if constexpr (size == 4) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	vregState.phyReg)));
				} else if constexpr (size == 8) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																	MemoryLocation{
																		baseRegId, 0,
																		REG_MAX, 0 },
																	vregState.phyReg)));
				}

				if (baseRegOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(baseRegId, baseRegOff, baseRegSize));
				}
			}

			compileContext.unallocReg(baseRegId);
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compileStoreInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) noexcept {
	InternalExceptionPointer exception;
	opti::RegAnalyzedInfo &regAnalyzedInfo = analyzedInfo.analyzedRegInfo.at(curIns.operands[0].getRegIndex());
	Value rhs = curIns.operands[1];

	switch (regAnalyzedInfo.storageType) {
		case opti::RegStorageType::None:
			std::terminate();
		case opti::RegStorageType::FieldVar: {
			const EntityRef &entityRef = regAnalyzedInfo.expectedValue.getEntityRef();
			FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);
			char *rawDataPtr = entityRef.asField.moduleObject->localFieldStorage.data() + fieldRecord.offset;

			switch (fieldRecord.type.typeId) {
				case TypeId::I8:
				case TypeId::I16:
				case TypeId::I32:
				case TypeId::I64:
				case TypeId::U8:
				case TypeId::U16:
				case TypeId::U32:
				case TypeId::U64:
				case TypeId::F32:
				case TypeId::F64:
				case TypeId::Bool: {
					if (rhs.valueType == ValueType::RegRef) {
						switch (fieldRecord.type.typeId) {
							case TypeId::I8:
							case TypeId::U8:
							case TypeId::Bool: {
								compileRegToFieldVarStoreInstruction<1>(compileContext, analyzedInfo, offIns, curIns, fieldRecord, rawDataPtr, rhs.getRegIndex());
								break;
							}
							case TypeId::I16:
							case TypeId::U16: {
								compileRegToFieldVarStoreInstruction<2>(compileContext, analyzedInfo, offIns, curIns, fieldRecord, rawDataPtr, rhs.getRegIndex());
								break;
							}
							case TypeId::I32:
							case TypeId::U32:
							case TypeId::F32: {
								compileRegToFieldVarStoreInstruction<4>(compileContext, analyzedInfo, offIns, curIns, fieldRecord, rawDataPtr, rhs.getRegIndex());
								break;
							}
							case TypeId::I64:
							case TypeId::U64:
							case TypeId::F64: {
								compileRegToFieldVarStoreInstruction<8>(compileContext, analyzedInfo, offIns, curIns, fieldRecord, rawDataPtr, rhs.getRegIndex());
								break;
							}
							default:
								std::terminate();
						}
					} else {
						switch (fieldRecord.type.typeId) {
							case TypeId::I8: {
								int8_t imm0 = rhs.getI8();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm8ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::I16: {
								int16_t imm0 = rhs.getI16();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm16ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::I32: {
								int32_t imm0 = rhs.getI32();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm32ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::I64: {
								int64_t imm0 = rhs.getI64();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm64ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::U8: {
								uint8_t imm0 = rhs.getU8();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm8ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::U16: {
								uint16_t imm0 = rhs.getU16();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm16ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::U32: {
								uint16_t imm0 = rhs.getU16();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm32ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::U64: {
								uint64_t imm0 = rhs.getU64();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm64ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::Bool: {
								bool imm0 = rhs.getBool();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm8ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::F32: {
								float imm0 = rhs.getF32();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm32ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
							case TypeId::F64: {
								double imm0 = rhs.getF64();
								if (((uintptr_t)rawDataPtr) < INT32_MAX) {
									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																					emitMovImm64ToMemIns(
																						MemoryLocation{
																							REG_MAX, (int32_t)(uintptr_t)rawDataPtr,
																							REG_MAX, 0 },
																						(uint8_t *)&imm0)));
								} else {
									RegisterId baseRegId = compileContext.allocGpReg();

									int32_t baseRegOff = INT32_MIN;
									size_t baseRegSize;

									uint64_t baseRegImm0 = (uint64_t)rawDataPtr;

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(
																					baseRegId,
																					(uint8_t *)&baseRegImm0)));

									SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(
																					MemoryLocation{
																						baseRegId, 0,
																						REG_MAX, 0 },
																					(uint8_t *)&imm0)));

									if (compileContext.isRegInUse(baseRegId)) {
										SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(baseRegId, baseRegOff, baseRegSize));
									}

									compileContext.unallocReg(baseRegId);
								}
								break;
							}
						}
					}
				}
			}
			break;
		}
		case opti::RegStorageType::LocalVar: {
			/*LocalVarState &localVarState = compileContext.localVarStates.at(regAnalyzedInfo.expectedValue.getEntityRef().asLocalVar.localVarIndex);

			switch (localVarState.type.typeId) {
			case TypeId::Value: {
				if (rhs.valueType == ValueType::RegRef) {
					switch (localVarState.type.getValueTypeExData()) {
					case TypeId::I8:
					case TypeId::U8:
					case TypeId::Bool: {
						uint32_t regOff = rhs.getRegIndex();
						VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

						if (vregState.saveOffset != INT32_MIN) {
							RegisterId tmpGpRegId = compileContext.allocGpReg();
							int32_t tmpGpRegOff = INT32_MIN;
							size_t tmpGpRegSize;

							if (compileContext.isRegInUse(tmpGpRegId)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg8Ins(
																			tmpGpRegId,
																			MemoryLocation{
																				REG_RBP, vregState.saveOffset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			tmpGpRegId)));

							if (tmpGpRegOff != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			vregState.phyReg)));
						}
						break;
					}
					case TypeId::I16:
					case TypeId::U16: {
						uint32_t regOff = rhs.getRegIndex();
						VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

						if (vregState.saveOffset != INT32_MIN) {
							RegisterId tmpGpRegId = compileContext.allocGpReg();
							int32_t tmpGpRegOff = INT32_MIN;
							size_t tmpGpRegSize;

							if (compileContext.isRegInUse(tmpGpRegId)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg16Ins(
																			tmpGpRegId,
																			MemoryLocation{
																				REG_RBP, vregState.saveOffset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			tmpGpRegId)));

							if (tmpGpRegOff != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			vregState.phyReg)));
						}
						break;
					}
					case TypeId::I32:
					case TypeId::U32:
					case TypeId::F32: {
						uint32_t regOff = rhs.getRegIndex();
						VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

						if (vregState.saveOffset != INT32_MIN) {
							RegisterId tmpGpRegId = compileContext.allocGpReg();
							int32_t tmpGpRegOff = INT32_MIN;
							size_t tmpGpRegSize;

							if (compileContext.isRegInUse(tmpGpRegId)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg32Ins(
																			tmpGpRegId,
																			MemoryLocation{
																				REG_RBP, vregState.saveOffset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			tmpGpRegId)));

							if (tmpGpRegOff != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			vregState.phyReg)));
						}
						break;
					}
					case TypeId::I64:
					case TypeId::U64:
					case TypeId::F64: {
						uint32_t regOff = rhs.getRegIndex();
						VirtualRegState &vregState = compileContext.virtualRegStates.at(regOff);

						if (vregState.saveOffset != INT32_MIN) {
							RegisterId tmpGpRegId = compileContext.allocGpReg();
							int32_t tmpGpRegOff = INT32_MIN;
							size_t tmpGpRegSize;

							if (compileContext.isRegInUse(tmpGpRegId)) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}

							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg64Ins(
																			tmpGpRegId,
																			MemoryLocation{
																				REG_RBP, vregState.saveOffset,
																				REG_MAX, 0 })));
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			tmpGpRegId)));

							if (tmpGpRegOff != INT32_MIN) {
								SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
							}
						} else {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			vregState.phyReg)));
						}
						break;
					}
					}
				} else {
					switch (localVarState.type.getValueTypeExData()) {
					case TypeId::I8: {
						int8_t imm0 = rhs.getI8();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm8ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::I16: {
						int16_t imm0 = rhs.getI16();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm16ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::I32: {
						int32_t imm0 = rhs.getI32();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm32ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::I64: {
						int64_t imm0 = rhs.getI64();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm64ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U8: {
						uint8_t imm0 = rhs.getU8();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm8ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U16: {
						uint16_t imm0 = rhs.getU16();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm16ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U32: {
						uint16_t imm0 = rhs.getU16();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm16ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::U64: {
						uint64_t imm0 = rhs.getU64();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm64ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::Bool: {
						bool imm0 = rhs.getBool();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm8ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::F32: {
						float imm0 = rhs.getF32();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm32ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					case TypeId::F64: {
						double imm0 = rhs.getF64();
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																		emitMovImm64ToMemIns(
																			MemoryLocation{
																				REG_RBP, localVarState.stackOff,
																				REG_MAX, 0 },
																			(uint8_t *)&imm0)));
						break;
					}
					}
				}
				break;
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
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
						}

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovMemToReg64Ins(
																		tmpGpRegId,
																		MemoryLocation{
																			REG_RBP, vregState.saveOffset,
																			REG_MAX, 0 })));
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																		MemoryLocation{
																			REG_RBP, localVarState.stackOff,
																			REG_MAX, 0 },
																		tmpGpRegId)));

						if (tmpGpRegOff != INT32_MIN) {
							SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegOff, tmpGpRegSize));
						}
					} else {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToMemIns(
																		MemoryLocation{
																			REG_RBP, localVarState.stackOff,
																			REG_MAX, 0 },
																		vregState.phyReg)));
					}
				} else {
					Object *imm0 = rhs.getEntityRef().asObject;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm64ToMemIns(
																		MemoryLocation{
																			REG_RBP, localVarState.stackOff,
																			REG_MAX, 0 },
																		(uint8_t *)&imm0)));
				}
				break;
			}
			}
			break;*/
		}
		case opti::RegStorageType::ArgRef:
			// TODO: Implement it.
			break;
	}

	return {};
}
