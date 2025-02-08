#include "add.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
[[nodiscard]] InternalExceptionPointer compileIntAddInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsRegId = compileContext.allocGpReg();

		if (compileContext.isRegInUse(lhsRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(lhsRegId, off, size));
		}

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

		if constexpr (std::is_same_v<T, int8_t>) {
			int8_t rhsData = curIns.operands[1].getI8();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm8ToReg8Ins(lhsRegId, (uint8_t *)&rhsData)));
		} else if constexpr (std::is_same_v<T, int16_t>) {
			int16_t rhsData = curIns.operands[1].getI16();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm16ToReg16Ins(lhsRegId, (uint8_t *)&rhsData)));
		} else if constexpr (std::is_same_v<T, int32_t>) {
			int32_t rhsData = curIns.operands[1].getI32();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg32Ins(lhsRegId, (uint8_t *)&rhsData)));
		} else if constexpr (std::is_same_v<T, int64_t>) {
			int64_t rhsData = curIns.operands[1].getI64();

			if (*((uint64_t *)&rhsData) & 0xffffffff00000000) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(lhsRegId, (uint8_t *)&rhsData)));
			} else {
				RegisterId tmpGpRegId = compileContext.allocGpReg();

				int32_t tmpGpRegSavedOff = INT32_MIN;
				size_t tmpGpRegSavedSize;
				if (compileContext.isRegInUse(tmpGpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
				}

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddReg64ToReg64Ins(lhsRegId, tmpGpRegId)));

				if (tmpGpRegSavedOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
				}
			}
		} else if constexpr (std::is_same_v<T, uint8_t>) {
			uint8_t rhsData = curIns.operands[1].getU8();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm8ToReg8Ins(lhsRegId, (uint8_t *)&rhsData)));
		} else if constexpr (std::is_same_v<T, uint16_t>) {
			uint16_t rhsData = curIns.operands[1].getU16();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm16ToReg16Ins(lhsRegId, (uint8_t *)&rhsData)));
		} else if constexpr (std::is_same_v<T, uint32_t>) {
			uint32_t rhsData = curIns.operands[1].getU32();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg32Ins(lhsRegId, (uint8_t *)&rhsData)));
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			uint64_t rhsData = curIns.operands[1].getU64();

			if (rhsData <= UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(lhsRegId, (uint8_t *)&rhsData)));
			} else {
				RegisterId tmpGpRegId = compileContext.allocGpReg();

				int32_t tmpGpRegSavedOff = INT32_MIN;
				size_t tmpGpRegSavedSize;
				if (compileContext.isRegInUse(tmpGpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
				}

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddReg64ToReg64Ins(lhsRegId, tmpGpRegId)));

				if (tmpGpRegSavedOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
				}
			}
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			const RegisterId rhsRegId = compileContext.allocGpReg();

			if (compileContext.isRegInUse(rhsRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(rhsRegId, off, size));
			}

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

			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t lhsData = curIns.operands[0].getI8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm8ToReg8Ins(rhsRegId, (uint8_t *)&lhsData)));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t lhsData = curIns.operands[0].getI16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm16ToReg16Ins(rhsRegId, (uint8_t *)&lhsData)));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t lhsData = curIns.operands[0].getI32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg32Ins(rhsRegId, (uint8_t *)&lhsData)));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t lhsData = curIns.operands[0].getI64();

				if (*((uint64_t *)&lhsData) & 0xffffffff00000000) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(rhsRegId, (uint8_t *)&lhsData)));
				} else {
					RegisterId tmpGpRegId = compileContext.allocGpReg();

					int32_t tmpGpRegSavedOff = INT32_MIN;
					size_t tmpGpRegSavedSize;
					if (compileContext.isRegInUse(tmpGpRegId)) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
					}

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddReg64ToReg64Ins(rhsRegId, tmpGpRegId)));

					if (tmpGpRegSavedOff != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
					}
				}
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t lhsData = curIns.operands[0].getU8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm8ToReg8Ins(rhsRegId, (uint8_t *)&lhsData)));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t lhsData = curIns.operands[0].getU16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm16ToReg16Ins(rhsRegId, (uint8_t *)&lhsData)));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t lhsData = curIns.operands[0].getU32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg32Ins(rhsRegId, (uint8_t *)&lhsData)));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t lhsData = curIns.operands[0].getU64();

				if (lhsData <= UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(rhsRegId, (uint8_t *)&lhsData)));
				} else {
					RegisterId tmpGpRegId = compileContext.allocGpReg();

					int32_t tmpGpRegSavedOff = INT32_MIN;
					size_t tmpGpRegSavedSize;
					if (compileContext.isRegInUse(tmpGpRegId)) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
					}

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddReg64ToReg64Ins(rhsRegId, tmpGpRegId)));

					if (tmpGpRegSavedOff != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize));
					}
				}
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
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddMemToReg8Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddMemToReg16Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddMemToReg32Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddMemToReg64Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddReg8ToReg8Ins(lhsRegId, rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddReg16ToReg16Ins(lhsRegId, rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddReg32ToReg32Ins(lhsRegId, rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitAddReg64ToReg64Ins(lhsRegId, rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();
		}
	}

	return {};
}

template <typename T>
[[nodiscard]] InternalExceptionPointer compileFpAddInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsXmmRegId = compileContext.allocXmmReg();

		if (compileContext.isRegInUse(lhsXmmRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(lhsXmmRegId, off, size));
		}

		VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
		if (lhsVregState.saveOffset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(float)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovdMemToRegXmmIns(
																	lhsXmmRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(double)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovqMemToRegXmmIns(
																	lhsXmmRegId,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(float)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovqRegXmmToRegXmmIns(
																	lhsXmmRegId,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(double)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovqRegXmmToRegXmmIns(
																	lhsXmmRegId,
																	lhsVregState.phyReg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsXmmRegId, sizeof(T));
		if (!outputVregState)
			return OutOfMemoryError::alloc();

		if constexpr (std::is_same_v<T, float>) {
			float rhsData = curIns.operands[1].getF32();

			const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
			int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
			size_t tmpXmmSize, tmpGpSize;
			if (compileContext.isRegInUse(tmpXmmRegId)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
			}
			if (compileContext.isRegInUse(tmpGpRegId)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpGpRegId, (uint8_t *)&rhsData)));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdReg32ToRegXmmIns(tmpXmmRegId, tmpGpRegId)));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddssRegXmmToRegXmmIns(lhsXmmRegId, tmpXmmRegId)));

			if (tmpGpOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize));
			}
			if (tmpXmmOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
			}
		} else if constexpr (std::is_same_v<T, double>) {
			double rhsData = curIns.operands[1].getF64();

			const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
			int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
			size_t tmpXmmSize, tmpGpSize;
			if (compileContext.isRegInUse(tmpXmmRegId)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
			}
			if (compileContext.isRegInUse(tmpGpRegId)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&rhsData)));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqReg64ToRegXmmIns(tmpXmmRegId, tmpGpRegId)));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddsdRegXmmToRegXmmIns(lhsXmmRegId, tmpXmmRegId)));

			if (tmpGpOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize));
			}
			if (tmpXmmOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
			}
		} else {
			static_assert(!std::is_same_v<T, T>, "Invalid operand type");
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			uint32_t rhsRegIndex = curIns.operands[0].getRegIndex();
			const RegisterId rhsXmmRegId = compileContext.allocXmmReg();

			if (compileContext.isRegInUse(rhsXmmRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(rhsXmmRegId, off, size));
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(float)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovdMemToRegXmmIns(
																		rhsXmmRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(double)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovqMemToRegXmmIns(
																		rhsXmmRegId,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(float)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovqRegXmmToRegXmmIns(
																		rhsXmmRegId,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(double)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovqRegXmmToRegXmmIns(
																		rhsXmmRegId,
																		rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsXmmRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, float>) {
				float lhsData = curIns.operands[1].getF32();

				const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
				int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
				size_t tmpXmmSize, tmpGpSize;
				if (compileContext.isRegInUse(tmpXmmRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
				}
				if (compileContext.isRegInUse(tmpGpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize));
				}

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpGpRegId, (uint8_t *)&lhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdReg32ToRegXmmIns(tmpXmmRegId, tmpGpRegId)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddssRegXmmToRegXmmIns(rhsXmmRegId, tmpXmmRegId)));

				if (tmpGpOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize));
				}
				if (tmpXmmOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
				}
			} else if constexpr (std::is_same_v<T, double>) {
				double lhsData = curIns.operands[1].getF64();

				const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
				int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
				size_t tmpXmmSize, tmpGpSize;
				if (compileContext.isRegInUse(tmpXmmRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
				}
				if (compileContext.isRegInUse(tmpGpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize));
				}

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&lhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqReg64ToRegXmmIns(tmpXmmRegId, tmpGpRegId)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddsdRegXmmToRegXmmIns(rhsXmmRegId, tmpXmmRegId)));

				if (tmpGpOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize));
				}
				if (tmpXmmOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize));
				}
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		} else {
			uint32_t lhsRegIndex = curIns.operands[0].getRegIndex(),
					 rhsRegIndex = curIns.operands[1].getRegIndex();

			VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			const RegisterId lhsXmmRegId = compileContext.allocXmmReg();
			int32_t rhsOff = INT32_MIN;
			size_t rhsSize;

			if (compileContext.isRegInUse(lhsXmmRegId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(lhsXmmRegId, off, size));
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsXmmRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, float>) {
				if (lhsVregState.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));

					if (rhsVregState.saveOffset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddssMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddssRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId)));
					}
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg)));

					if (rhsVregState.saveOffset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddssMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddssRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId)));
					}
				}
			} else if constexpr (std::is_same_v<T, double>) {
				if (lhsVregState.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));

					if (rhsVregState.saveOffset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddsdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddsdRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId)));
					}
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg)));

					if (rhsVregState.saveOffset != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddsdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddsdRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId)));
					}
				}
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		}
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compileAddInstruction(
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
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
			lhsExpectedValue = lhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(lhs.getRegIndex()).expectedValue;
			break;
		default:
			assert(("Malformed function", false));
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
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
			lhsExpectedValue = rhs;
			break;
		case ValueType::RegRef:
			lhsExpectedValue = analyzedInfo.analyzedRegInfo.at(rhs.getRegIndex()).expectedValue;
			break;
		default:
			assert(("Malformed function", false));
	}

	switch (outputRegInfo.type.typeId) {
		case TypeId::Value: {
			switch (outputRegInfo.type.getValueTypeExData()) {
				case ValueType::I8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<int8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<int16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<int32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::I64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<int64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U8: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<uint8_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U16: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<uint16_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<uint32_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::U64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntAddInstruction<uint64_t>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::F32: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileFpAddInstruction<float>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				case ValueType::F64: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileFpAddInstruction<double>(
																	compileContext,
																	curIns,
																	lhsExpectedValue,
																	rhsExpectedValue));
					break;
				}
				default:
					assert(("The function is malformed", false));
			}
			break;
		}
		default:
			assert(("The function is malformed", false));
	}

	return {};
}
