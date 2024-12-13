#include "sub.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
void compileIntSubInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) {
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		// The RHS is an expectable value so we can just simply add it with a register.
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsRegId = compileContext.allocGpReg();

		if (compileContext.isRegInUse(lhsRegId)) {
			int32_t off;
			size_t size;
			compileContext.pushReg(lhsRegId, off, size);
		}

		VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
		if (lhsVregState.saveOffset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				compileContext.pushIns(
					emitMovMemToReg8Ins(
						lhsRegId,
						MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				compileContext.pushIns(
					emitMovMemToReg16Ins(
						lhsRegId,
						MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				compileContext.pushIns(
					emitMovMemToReg32Ins(
						lhsRegId,
						MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				compileContext.pushIns(
					emitMovMemToReg64Ins(
						lhsRegId,
						MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
			} else {
				static_assert((false, "Invalid operand size"));
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				compileContext.pushIns(
					emitMovReg8ToReg8Ins(
						lhsRegId,
						lhsVregState.phyReg));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				compileContext.pushIns(
					emitMovReg16ToReg16Ins(
						lhsRegId,
						lhsVregState.phyReg));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				compileContext.pushIns(
					emitMovReg32ToReg32Ins(
						lhsRegId,
						lhsVregState.phyReg));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				compileContext.pushIns(
					emitMovReg64ToReg64Ins(
						lhsRegId,
						lhsVregState.phyReg));
			} else {
				static_assert((false, "Invalid operand size"));
			}
		}

		VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));

		if constexpr (std::is_same_v<T, int8_t>) {
			int8_t rhsData = curIns.operands[1].getI8();
			compileContext.pushIns(emitSubImm8ToReg8Ins(lhsRegId, (uint8_t *)&rhsData));
		} else if constexpr (std::is_same_v<T, int16_t>) {
			int16_t rhsData = curIns.operands[1].getI16();
			compileContext.pushIns(emitSubImm16ToReg16Ins(lhsRegId, (uint8_t *)&rhsData));
		} else if constexpr (std::is_same_v<T, int32_t>) {
			int32_t rhsData = curIns.operands[1].getI32();
			compileContext.pushIns(emitSubImm32ToReg32Ins(lhsRegId, (uint8_t *)&rhsData));
		} else if constexpr (std::is_same_v<T, int64_t>) {
			int64_t rhsData = curIns.operands[1].getI64();

			if (*((uint64_t *)&rhsData) & 0xffffffff00000000) {
				compileContext.pushIns(emitAddImm32ToReg64Ins(lhsRegId, (uint8_t *)&rhsData));
			} else {
				RegisterId tmpGpRegId = compileContext.allocGpReg();

				int32_t tmpGpRegSavedOff = INT32_MIN;
				size_t tmpGpRegSavedSize;
				if (compileContext.isRegInUse(tmpGpRegId)) {
					compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
				}

				compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitSubReg64ToReg64Ins(lhsRegId, tmpGpRegId));

				if (tmpGpRegSavedOff != INT32_MIN) {
					compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
				}
			}
		} else if constexpr (std::is_same_v<T, uint8_t>) {
			uint8_t rhsData = curIns.operands[1].getU8();
			compileContext.pushIns(emitSubImm8ToReg8Ins(lhsRegId, (uint8_t *)&rhsData));
		} else if constexpr (std::is_same_v<T, uint16_t>) {
			uint16_t rhsData = curIns.operands[1].getU16();
			compileContext.pushIns(emitSubImm16ToReg16Ins(lhsRegId, (uint8_t *)&rhsData));
		} else if constexpr (std::is_same_v<T, uint32_t>) {
			uint32_t rhsData = curIns.operands[1].getU32();
			compileContext.pushIns(emitSubImm32ToReg32Ins(lhsRegId, (uint8_t *)&rhsData));
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			uint64_t rhsData = curIns.operands[1].getU64();

			if (rhsData <= UINT32_MAX) {
				compileContext.pushIns(emitAddImm32ToReg64Ins(lhsRegId, (uint8_t *)&rhsData));
			} else {
				RegisterId tmpGpRegId = compileContext.allocGpReg();

				int32_t tmpGpRegSavedOff = INT32_MIN;
				size_t tmpGpRegSavedSize;
				if (compileContext.isRegInUse(tmpGpRegId)) {
					compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
				}

				compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitSubReg64ToReg64Ins(lhsRegId, tmpGpRegId));

				if (tmpGpRegSavedOff != INT32_MIN) {
					compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
				}
			}
		} else {
			static_assert((false, "Invalid operand type"));
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			// The RHS is an expectable value so we can just simply add it with a register.
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			const RegisterId rhsRegId = compileContext.allocGpReg();

			if (compileContext.isRegInUse(rhsRegId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(rhsRegId, off, size);
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					compileContext.pushIns(
						emitMovMemToReg8Ins(
							rhsRegId,
							MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					compileContext.pushIns(
						emitMovMemToReg16Ins(
							rhsRegId,
							MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					compileContext.pushIns(
						emitMovMemToReg32Ins(
							rhsRegId,
							MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					compileContext.pushIns(
						emitMovMemToReg64Ins(
							rhsRegId,
							MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else {
					static_assert((false, "Invalid operand size"));
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					compileContext.pushIns(
						emitMovReg8ToReg8Ins(
							rhsRegId,
							rhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					compileContext.pushIns(
						emitMovReg16ToReg16Ins(
							rhsRegId,
							rhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					compileContext.pushIns(
						emitMovReg32ToReg32Ins(
							rhsRegId,
							rhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					compileContext.pushIns(
						emitMovReg64ToReg64Ins(
							rhsRegId,
							rhsVregState.phyReg));
				} else {
					static_assert((false, "Invalid operand size"));
				}
			}

			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));

			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t lhsData = curIns.operands[0].getI8();
				compileContext.pushIns(emitSubImm8ToReg8Ins(rhsRegId, (uint8_t *)&lhsData));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t lhsData = curIns.operands[0].getI16();
				compileContext.pushIns(emitSubImm16ToReg16Ins(rhsRegId, (uint8_t *)&lhsData));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t lhsData = curIns.operands[0].getI32();
				compileContext.pushIns(emitSubImm32ToReg32Ins(rhsRegId, (uint8_t *)&lhsData));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t lhsData = curIns.operands[0].getI64();

				if (*((uint64_t *)&lhsData) & 0xffffffff00000000) {
					compileContext.pushIns(emitAddImm32ToReg64Ins(rhsRegId, (uint8_t *)&lhsData));
				} else {
					RegisterId tmpGpRegId = compileContext.allocGpReg();

					int32_t tmpGpRegSavedOff = INT32_MIN;
					size_t tmpGpRegSavedSize;
					if (compileContext.isRegInUse(tmpGpRegId)) {
						compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
					}

					compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitSubReg64ToReg64Ins(rhsRegId, tmpGpRegId));

					if (tmpGpRegSavedOff != INT32_MIN) {
						compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
					}
				}
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t lhsData = curIns.operands[0].getU8();
				compileContext.pushIns(emitSubImm8ToReg8Ins(rhsRegId, (uint8_t *)&lhsData));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t lhsData = curIns.operands[0].getU16();
				compileContext.pushIns(emitSubImm16ToReg16Ins(rhsRegId, (uint8_t *)&lhsData));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t lhsData = curIns.operands[0].getU32();
				compileContext.pushIns(emitSubImm32ToReg32Ins(rhsRegId, (uint8_t *)&lhsData));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t lhsData = curIns.operands[0].getU64();

				if (lhsData <= UINT32_MAX) {
					compileContext.pushIns(emitAddImm32ToReg64Ins(rhsRegId, (uint8_t *)&lhsData));
				} else {
					RegisterId tmpGpRegId = compileContext.allocGpReg();

					int32_t tmpGpRegSavedOff = INT32_MIN;
					size_t tmpGpRegSavedSize;
					if (compileContext.isRegInUse(tmpGpRegId)) {
						compileContext.pushReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
					}

					compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitSubReg64ToReg64Ins(rhsRegId, tmpGpRegId));

					if (tmpGpRegSavedOff != INT32_MIN) {
						compileContext.popReg(tmpGpRegId, tmpGpRegSavedOff, tmpGpRegSavedSize);
					}
				}
			} else {
				static_assert((false, "Invalid operand type"));
			}
		} else {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();

			const RegisterId lhsRegId = compileContext.allocGpReg();

			if (compileContext.isRegInUse(lhsRegId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(lhsRegId, off, size);
			}

			VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegId);
			if (lhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					compileContext.pushIns(
						emitMovMemToReg8Ins(
							lhsRegId,
							MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					compileContext.pushIns(
						emitMovMemToReg16Ins(
							lhsRegId,
							MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					compileContext.pushIns(
						emitMovMemToReg32Ins(
							lhsRegId,
							MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					compileContext.pushIns(
						emitMovMemToReg64Ins(
							lhsRegId,
							MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
				} else {
					static_assert((false, "Invalid operand size"));
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					compileContext.pushIns(
						emitMovReg8ToReg8Ins(
							lhsRegId,
							lhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					compileContext.pushIns(
						emitMovReg16ToReg16Ins(
							lhsRegId,
							lhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					compileContext.pushIns(
						emitMovReg32ToReg32Ins(
							lhsRegId,
							lhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					compileContext.pushIns(
						emitMovReg64ToReg64Ins(
							lhsRegId,
							lhsVregState.phyReg));
				} else {
					static_assert((false, "Invalid operand size"));
				}
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					compileContext.pushIns(
						emitSubMemToReg8Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					compileContext.pushIns(
						emitSubMemToReg16Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					compileContext.pushIns(
						emitSubMemToReg32Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					compileContext.pushIns(
						emitSubMemToReg64Ins(lhsRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else {
					static_assert((false, "Invalid operand type"));
				}
			} else {
				if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
					compileContext.pushIns(
						emitSubReg8ToReg8Ins(lhsRegId, rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t>) {
					compileContext.pushIns(
						emitSubReg16ToReg16Ins(lhsRegId, rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
					compileContext.pushIns(
						emitSubReg32ToReg32Ins(lhsRegId, rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
					compileContext.pushIns(
						emitSubReg64ToReg64Ins(lhsRegId, rhsVregState.phyReg));
				} else {
					static_assert((false, "Invalid operand type"));
				}
			}

			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
		}
	}
}

template <typename T>
void compileFpSubInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) {
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		// The RHS is an expectable value so we can just simply add it with a register.
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		const RegisterId lhsXmmRegId = compileContext.allocXmmReg();

		if (compileContext.isRegInUse(lhsXmmRegId)) {
			int32_t off;
			size_t size;
			compileContext.pushRegXmm(lhsXmmRegId, off, size);
		}

		VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
		if (lhsVregState.saveOffset != INT32_MIN) {
			if constexpr (sizeof(T) == sizeof(float)) {
				compileContext.pushIns(
					emitMovdMemToRegXmmIns(
						lhsXmmRegId,
						MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
			} else if constexpr (sizeof(T) == sizeof(double)) {
				compileContext.pushIns(
					emitMovqMemToRegXmmIns(
						lhsXmmRegId,
						MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));
			} else {
				static_assert((false, "Invalid operand size"));
			}
		} else {
			if constexpr (sizeof(T) == sizeof(float)) {
				compileContext.pushIns(
					emitMovqRegXmmToRegXmmIns(
						lhsXmmRegId,
						lhsVregState.phyReg));
			} else if constexpr (sizeof(T) == sizeof(double)) {
				compileContext.pushIns(
					emitMovqRegXmmToRegXmmIns(
						lhsXmmRegId,
						lhsVregState.phyReg));
			} else {
				static_assert((false, "Invalid operand size"));
			}
		}

		VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsXmmRegId, sizeof(T));

		if constexpr (std::is_same_v<T, float>) {
			float rhsData = curIns.operands[1].getF32();

			const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
			int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
			size_t tmpXmmSize, tmpGpSize;
			if (compileContext.isRegInUse(tmpXmmRegId)) {
				compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
			}
			if (compileContext.isRegInUse(tmpGpRegId)) {
				compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize);
			}

			compileContext.pushIns(emitMovImm32ToReg32Ins(tmpGpRegId, (uint8_t *)&rhsData));
			compileContext.pushIns(emitMovdReg32ToRegXmmIns(tmpXmmRegId, tmpGpRegId));
			compileContext.pushIns(emitSubssRegXmmToRegXmmIns(lhsXmmRegId, tmpXmmRegId));

			if (tmpGpOff != INT32_MIN) {
				compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize);
			}
			if (tmpXmmOff != INT32_MIN) {
				compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
			}
		} else if constexpr (std::is_same_v<T, double>) {
			double rhsData = curIns.operands[1].getF64();

			const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
			int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
			size_t tmpXmmSize, tmpGpSize;
			if (compileContext.isRegInUse(tmpXmmRegId)) {
				compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
			}
			if (compileContext.isRegInUse(tmpGpRegId)) {
				compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize);
			}

			compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&rhsData));
			compileContext.pushIns(emitMovqReg64ToRegXmmIns(tmpXmmRegId, tmpGpRegId));
			compileContext.pushIns(emitSubsdRegXmmToRegXmmIns(lhsXmmRegId, tmpXmmRegId));

			if (tmpGpOff != INT32_MIN) {
				compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize);
			}
			if (tmpXmmOff != INT32_MIN) {
				compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
			}
		} else {
			static_assert((false, "Invalid operand type"));
		}
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {
			uint32_t rhsRegIndex = curIns.operands[0].getRegIndex();
			const RegisterId rhsXmmRegId = compileContext.allocXmmReg();

			if (compileContext.isRegInUse(rhsXmmRegId)) {
				int32_t off;
				size_t size;
				compileContext.pushRegXmm(rhsXmmRegId, off, size);
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (sizeof(T) == sizeof(float)) {
					compileContext.pushIns(
						emitMovdMemToRegXmmIns(
							rhsXmmRegId,
							MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (sizeof(T) == sizeof(double)) {
					compileContext.pushIns(
						emitMovqMemToRegXmmIns(
							rhsXmmRegId,
							MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else {
					static_assert((false, "Invalid operand size"));
				}
			} else {
				if constexpr (sizeof(T) == sizeof(float)) {
					compileContext.pushIns(
						emitMovqRegXmmToRegXmmIns(
							rhsXmmRegId,
							rhsVregState.phyReg));
				} else if constexpr (sizeof(T) == sizeof(double)) {
					compileContext.pushIns(
						emitMovqRegXmmToRegXmmIns(
							rhsXmmRegId,
							rhsVregState.phyReg));
				} else {
					static_assert((false, "Invalid operand size"));
				}
			}

			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsXmmRegId, sizeof(T));

			if constexpr (std::is_same_v<T, float>) {
				float lhsData = curIns.operands[1].getF32();

				const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
				int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
				size_t tmpXmmSize, tmpGpSize;
				if (compileContext.isRegInUse(tmpXmmRegId)) {
					compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
				}
				if (compileContext.isRegInUse(tmpGpRegId)) {
					compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize);
				}

				compileContext.pushIns(emitMovImm32ToReg32Ins(tmpGpRegId, (uint8_t *)&lhsData));
				compileContext.pushIns(emitMovdReg32ToRegXmmIns(tmpXmmRegId, tmpGpRegId));
				compileContext.pushIns(emitSubssRegXmmToRegXmmIns(rhsXmmRegId, tmpXmmRegId));

				if (tmpGpOff != INT32_MIN) {
					compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize);
				}
				if (tmpXmmOff != INT32_MIN) {
					compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
				}
			} else if constexpr (std::is_same_v<T, double>) {
				double lhsData = curIns.operands[1].getF64();

				const RegisterId tmpXmmRegId = compileContext.allocXmmReg(), tmpGpRegId = compileContext.allocGpReg();
				int32_t tmpXmmOff = INT32_MIN, tmpGpOff = INT32_MIN;
				size_t tmpXmmSize, tmpGpSize;
				if (compileContext.isRegInUse(tmpXmmRegId)) {
					compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
				}
				if (compileContext.isRegInUse(tmpGpRegId)) {
					compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize);
				}

				compileContext.pushIns(emitMovImm64ToReg64Ins(tmpGpRegId, (uint8_t *)&lhsData));
				compileContext.pushIns(emitMovqReg64ToRegXmmIns(tmpXmmRegId, tmpGpRegId));
				compileContext.pushIns(emitSubsdRegXmmToRegXmmIns(rhsXmmRegId, tmpXmmRegId));

				if (tmpGpOff != INT32_MIN) {
					compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize);
				}
				if (tmpXmmOff != INT32_MIN) {
					compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
				}
			} else {
				static_assert((false, "Invalid operand type"));
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
				compileContext.pushRegXmm(lhsXmmRegId, off, size);
			}

			VirtualRegState &outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsXmmRegId, sizeof(T));

			if constexpr (std::is_same_v<T, float>) {
				if (lhsVregState.saveOffset != INT32_MIN) {
					compileContext.pushIns(emitMovdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitSubssMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitSubssRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				} else {
					compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitSubssMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitSubssRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				}
			} else if constexpr (std::is_same_v<T, double>) {
				if (lhsVregState.saveOffset != INT32_MIN) {
					compileContext.pushIns(emitMovqMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitSubsdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitSubsdRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				} else {
					compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitSubsdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitSubsdRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				}
			} else {
				static_assert((false, "Invalid operand type"));
			}
		}
	}
}

InternalExceptionPointer slake::jit::x86_64::compileSubInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) {
	uint32_t outputRegIndex = UINT32_MAX;
	auto &outputRegInfo = analyzedInfo.analyzedRegInfo.at(outputRegIndex);

	Value expectedValue = analyzedInfo.analyzedRegInfo.at(outputRegIndex).expectedValue;
	if (expectedValue.valueType == ValueType::Undefined) {
		Value lhs = curIns.operands[0], rhs = curIns.operands[1];
		Value lhsExpectedValue, rhsExpectedValue;

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
						compileIntSubInstruction<int8_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::I16: {
						compileIntSubInstruction<int16_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::I32: {
						compileIntSubInstruction<int32_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::I64: {
						compileIntSubInstruction<int64_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U8: {
						compileIntSubInstruction<uint8_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U16: {
						compileIntSubInstruction<uint16_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U32: {
						compileIntSubInstruction<uint32_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U64: {
						compileIntSubInstruction<uint64_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::F32: {
						compileFpSubInstruction<float>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::F64: {
						compileFpSubInstruction<double>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
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
	} else {
		// The instruction is omitttable, do nothing.
	}

	return {};
}
