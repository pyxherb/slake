#include "bitshift.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
[[nodiscard]] InternalExceptionPointer compileIntShlInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsRegId = compileContext.allocGpReg();

		uint32_t rhsData = curIns.operands[1].getU32();

		if (compileContext.isRegInUse(lhsRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			if (rhsData >= 8) {
				uint8_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm8ToReg8Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
			if (rhsData >= 16) {
				uint16_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm16ToReg16Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
			if (rhsData >= 32) {
				uint32_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm32ToReg32Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
			if (rhsData >= 64) {
				uint64_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm64ToReg64Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		}

		uint8_t narrowedRhsData = (uint8_t)rhsData;

		VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
		if (lhsVregState.saveOffset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg8Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg16Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg64Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg8ToReg8Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg16ToReg16Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg32ToReg32Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg64ToReg64Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
		if (!outputVregState)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg8WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg16WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg32WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg64WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			const RegisterId rhsRegId = compileContext.allocGpReg();

			uint32_t lhsData = curIns.operands[1].getU32();

			if (compileContext.isRegInUse(rhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(rhsRegId, off, size));
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				if (lhsData >= 8) {
					uint8_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm8ToReg8Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				if (lhsData >= 16) {
					uint16_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm16ToReg16Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				if (lhsData >= 32) {
					uint32_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm32ToReg32Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				if (lhsData >= 64) {
					uint64_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm64ToReg64Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			}

			uint8_t narrowedLhsData = (uint8_t)lhsData;

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg8WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg16WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg32WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShlReg64WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();

			const RegisterId lhsRegId = compileContext.allocGpReg();

			if (compileContext.isRegInUse(lhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
			}

			VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegId);
			if (lhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			int32_t cxOff = INT32_MIN;
			size_t cxSize;

			// TODO: Handle RCX >= 8, 16, 32, 64.
			if (rhsVregState.saveOffset != INT32_MIN) {
				if (compileContext.isRegInUse(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg32(REG_RCX, cxOff, cxSize));
				}
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(REG_RCX, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));

				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg8WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg16WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg32WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg64WithClIns(lhsRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if (rhsVregState.phyReg != REG_RCX && compileContext.isRegInUse(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg8(REG_RCX, cxOff, cxSize));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(REG_RCX, rhsVregState.phyReg)));
				}
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg8WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg16WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg32WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShlReg64WithClIns(lhsRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if (cxOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT(compileContext.popReg32(REG_RCX, cxOff));
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compileShlInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = UINT32_MAX;
	auto &outputRegInfo = analyzedInfo.analyzedRegInfo.at(outputRegIndex);

	Value lhs = curIns.operands[0], rhs = curIns.operands[1];
	Value lhsExpectedValue(ValueType::Undefined), rhsExpectedValue(ValueType::Undefined);

	switch (lhs.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhsExpectedValue = lhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(lhs.getRegIndex()).expectedValue;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (rhs.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhsExpectedValue = rhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(rhs.getRegIndex()).expectedValue;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (outputRegInfo.type.typeId) {
		case TypeId::Value: {
			switch (outputRegInfo.type.getValueTypeExData()) {
				case ValueType::I8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<int8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<int16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<int32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<int64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<uint8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<uint16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<uint32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShlInstruction<uint64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				default:
					// The function is malformed
					std::terminate();
			}
			break;
		}
		default:
			// The function is malformed
			std::terminate();
	}

	return {};
}

template <typename T>
[[nodiscard]] InternalExceptionPointer compileIntShrInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsRegId = compileContext.allocGpReg();

		uint32_t rhsData = curIns.operands[1].getU32();

		if (compileContext.isRegInUse(lhsRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			if (rhsData >= 8) {
				uint8_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm8ToReg8Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
			if (rhsData >= 16) {
				uint16_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm16ToReg16Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
			if (rhsData >= 32) {
				uint32_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm32ToReg32Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
			if (rhsData >= 64) {
				uint64_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm64ToReg64Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		}

		uint8_t narrowedRhsData = (uint8_t)rhsData;

		VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
		if (lhsVregState.saveOffset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg8Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg16Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg64Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg8ToReg8Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg16ToReg16Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg32ToReg32Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg64ToReg64Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
		if (!outputVregState)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg8WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg16WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg32WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg64WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			const RegisterId rhsRegId = compileContext.allocGpReg();

			uint32_t lhsData = curIns.operands[1].getU32();

			if (compileContext.isRegInUse(rhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(rhsRegId, off, size));
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				if (lhsData >= 8) {
					uint8_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm8ToReg8Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				if (lhsData >= 16) {
					uint16_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm16ToReg16Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				if (lhsData >= 32) {
					uint32_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm32ToReg32Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				if (lhsData >= 64) {
					uint64_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm64ToReg64Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			}

			uint8_t narrowedLhsData = (uint8_t)lhsData;

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg8WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg16WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg32WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg64WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();

			const RegisterId lhsRegId = compileContext.allocGpReg();

			if (compileContext.isRegInUse(lhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
			}

			VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegId);
			if (lhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			int32_t cxOff = INT32_MIN;
			size_t cxSize;

			// TODO: Handle RCX >= 8, 16, 32, 64.
			if (rhsVregState.saveOffset != INT32_MIN) {
				if (compileContext.isRegInUse(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg32(REG_RCX, cxOff, cxSize));
				}
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(REG_RCX, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));

				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg8WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg16WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg32WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg64WithClIns(lhsRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if (rhsVregState.phyReg != REG_RCX && compileContext.isRegInUse(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg8(REG_RCX, cxOff, cxSize));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(REG_RCX, rhsVregState.phyReg)));
				}
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg8WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg16WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg32WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitShrReg64WithClIns(lhsRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if (cxOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT(compileContext.popReg32(REG_RCX, cxOff));
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compileShrInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = UINT32_MAX;
	auto &outputRegInfo = analyzedInfo.analyzedRegInfo.at(outputRegIndex);

	Value lhs = curIns.operands[0], rhs = curIns.operands[1];
	Value lhsExpectedValue(ValueType::Undefined), rhsExpectedValue(ValueType::Undefined);

	switch (lhs.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhsExpectedValue = lhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(lhs.getRegIndex()).expectedValue;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (rhs.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhsExpectedValue = rhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(rhs.getRegIndex()).expectedValue;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (outputRegInfo.type.typeId) {
		case TypeId::Value: {
			switch (outputRegInfo.type.getValueTypeExData()) {
				case ValueType::I8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<int8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<int16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<int32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<int64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<uint8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<uint16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<uint32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntShrInstruction<uint64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				default:
					// The function is malformed
					std::terminate();
			}
			break;
		}
		default:
			// The function is malformed
			std::terminate();
	}

	return {};
}

template <typename T>
[[nodiscard]] InternalExceptionPointer compileIntSarInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsRegId = compileContext.allocGpReg();

		uint32_t rhsData = curIns.operands[1].getU32();

		if (compileContext.isRegInUse(lhsRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			if (rhsData >= 8) {
				uint8_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm8ToReg8Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
			if (rhsData >= 16) {
				uint16_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm16ToReg16Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
			if (rhsData >= 32) {
				uint32_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm32ToReg32Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
			if (rhsData >= 64) {
				uint64_t imm0 = 0;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovImm64ToReg64Ins(
																	lhsRegId,
																	(uint8_t *)&imm0)));
				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
			}
		}

		uint8_t narrowedRhsData = (uint8_t)rhsData;

		VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
		if (lhsVregState.saveOffset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg8Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg16Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg64Ins(
																	lhsRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg8ToReg8Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg16ToReg16Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg32ToReg32Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg64ToReg64Ins(
																	lhsRegId,
																	lhsVregState.phyReg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
		if (!outputVregState)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg8WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg16WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg32WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg64WithImm8Ins(lhsRegId, narrowedRhsData)));
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			const RegisterId rhsRegId = compileContext.allocGpReg();

			uint32_t lhsData = curIns.operands[1].getU32();

			if (compileContext.isRegInUse(rhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(rhsRegId, off, size));
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				if (lhsData >= 8) {
					uint8_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm8ToReg8Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				if (lhsData >= 16) {
					uint16_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm16ToReg16Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				if (lhsData >= 32) {
					uint32_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm32ToReg32Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				if (lhsData >= 64) {
					uint64_t imm0 = 0;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovImm64ToReg64Ins(
																		rhsRegId,
																		(uint8_t *)&imm0)));
					VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
			}

			uint8_t narrowedLhsData = (uint8_t)lhsData;

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		rhsRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		rhsRegId,
																		rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg8WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg16WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg32WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitSarReg64WithImm8Ins(rhsRegId, narrowedLhsData)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();

			const RegisterId lhsRegId = compileContext.allocGpReg();

			if (compileContext.isRegInUse(lhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
			}

			VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegId);
			if (lhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		lhsRegId,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		lhsRegId,
																		lhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			int32_t cxOff = INT32_MIN;
			size_t cxSize;

			// TODO: Handle RCX >= 8, 16, 32, 64.
			if (rhsVregState.saveOffset != INT32_MIN) {
				if (compileContext.isRegInUse(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg32(REG_RCX, cxOff, cxSize));
				}
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(REG_RCX, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));

				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg8WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg16WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg32WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg64WithClIns(lhsRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if (rhsVregState.phyReg != REG_RCX && compileContext.isRegInUse(REG_RCX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg8(REG_RCX, cxOff, cxSize));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(REG_RCX, rhsVregState.phyReg)));
				}
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg8WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg16WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg32WithClIns(lhsRegId)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitSarReg64WithClIns(lhsRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if (cxOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT(compileContext.popReg32(REG_RCX, cxOff));
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compileSarInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = UINT32_MAX;
	auto &outputRegInfo = analyzedInfo.analyzedRegInfo.at(outputRegIndex);

	Value lhs = curIns.operands[0], rhs = curIns.operands[1];
	Value lhsExpectedValue(ValueType::Undefined), rhsExpectedValue(ValueType::Undefined);

	switch (lhs.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhsExpectedValue = lhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(lhs.getRegIndex()).expectedValue;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (rhs.valueType) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
			lhsExpectedValue = rhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(rhs.getRegIndex()).expectedValue;
			break;
		default:
			// Malformed function
			std::terminate();
	}

	switch (outputRegInfo.type.typeId) {
		case TypeId::Value: {
			switch (outputRegInfo.type.getValueTypeExData()) {
				case ValueType::I8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<int8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<int16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<int32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<int64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<uint8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<uint16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<uint32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntSarInstruction<uint64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				default:
					// The function is malformed
					std::terminate();
			}
			break;
		}
		default:
			// The function is malformed
			std::terminate();
	}

	return {};
}
