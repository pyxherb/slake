#include "div.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
InternalExceptionPointer compileIntDivInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) {
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		// The RHS is an expectable value so we can just simply add it with a register.
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		int32_t savedRdxOff = INT32_MIN;
		size_t savedRdxSize;
		const RegisterId lhsRegId = REG_RAX;

		if (compileContext.isRegInUse(REG_RAX)) {
			int32_t off;
			size_t size;
			compileContext.pushReg(REG_RAX, off, size);
		}
		if constexpr (sizeof(T) > sizeof(uint8_t)) {
			if (compileContext.isRegInUse(REG_RDX)) {
				compileContext.pushReg(REG_RDX, savedRdxOff, savedRdxSize);
			}
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
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
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
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		// Try to allocate a new temporary register to store the right operand.
		const RegisterId tmpRegId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(tmpRegId)) {
			int32_t off;
			SLAKE_RETURN_IF_EXCEPT(compileContext.stackAllocAligned(sizeof(T), sizeof(T), off));
			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t rhsData = curIns.operands[1].getI8();
				compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t rhsData = curIns.operands[1].getI16();
				compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t rhsData = curIns.operands[1].getI32();
				compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t rhsData = curIns.operands[1].getI64();
				compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t rhsData = curIns.operands[1].getU8();
				compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t rhsData = curIns.operands[1].getU16();
				compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t rhsData = curIns.operands[1].getU32();
				compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t rhsData = curIns.operands[1].getU64();
				compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}

			compileContext.stackFree(off, sizeof(T));
		} else {
			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t rhsData = curIns.operands[1].getI8();
				compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv8WithReg8Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t rhsData = curIns.operands[1].getI16();
				compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv16WithReg16Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t rhsData = curIns.operands[1].getI32();
				compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv32WithReg32Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t rhsData = curIns.operands[1].getI64();
				compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitDiv64WithReg64Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t rhsData = curIns.operands[1].getU8();
				compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv8WithReg8Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t rhsData = curIns.operands[1].getU16();
				compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv16WithReg16Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t rhsData = curIns.operands[1].getU32();
				compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv32WithReg32Ins(tmpRegId));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t rhsData = curIns.operands[1].getU64();
				compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&rhsData));
				compileContext.pushIns(emitIdiv64WithReg64Ins(tmpRegId));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
			compileContext.unallocReg(tmpRegId);
		}

		if constexpr (sizeof(T) > sizeof(uint8_t)) {
			if (savedRdxOff != INT32_MIN) {
				compileContext.popReg(REG_RDX, savedRdxOff, savedRdxSize);
			}
		}

		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {  // The RHS is an expectable value so we can just simply add it with a register.
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			int32_t savedRdxOff = INT32_MIN;
			size_t savedRdxSize;
			const RegisterId rhsRegId = REG_RAX;

			if (compileContext.isRegInUse(REG_RAX)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(REG_RAX, off, size);
			}
			if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (compileContext.isRegInUse(REG_RDX)) {
					compileContext.pushReg(REG_RDX, savedRdxOff, savedRdxSize);
				}
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
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
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
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			// Try to allocate a new temporary register to store the right operand.
			const RegisterId tmpRegId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(tmpRegId)) {
				int32_t off;
				SLAKE_RETURN_IF_EXCEPT(compileContext.stackAllocAligned(sizeof(T), sizeof(T), off));
				if constexpr (std::is_same_v<T, int8_t>) {
					int8_t lhsData = curIns.operands[0].getI8();
					compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					int16_t lhsData = curIns.operands[0].getI16();
					compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					int32_t lhsData = curIns.operands[0].getI32();
					compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					int64_t lhsData = curIns.operands[0].getI64();
					compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					uint8_t lhsData = curIns.operands[0].getU8();
					compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					uint16_t lhsData = curIns.operands[0].getU16();
					compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					uint32_t lhsData = curIns.operands[0].getU32();
					compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					uint64_t lhsData = curIns.operands[0].getU64();
					compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}

				compileContext.stackFree(off, sizeof(T));
			} else {
				if constexpr (std::is_same_v<T, int8_t>) {
					int8_t lhsData = curIns.operands[1].getI8();
					compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv8WithReg8Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					int16_t lhsData = curIns.operands[1].getI16();
					compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv16WithReg16Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					int32_t lhsData = curIns.operands[1].getI32();
					compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv32WithReg32Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					int64_t lhsData = curIns.operands[1].getI64();
					compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitDiv64WithReg64Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					uint8_t lhsData = curIns.operands[1].getU8();
					compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv8WithReg8Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					uint16_t lhsData = curIns.operands[1].getU16();
					compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv16WithReg16Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					uint32_t lhsData = curIns.operands[1].getU32();
					compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv32WithReg32Ins(tmpRegId));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					uint64_t lhsData = curIns.operands[1].getU64();
					compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&lhsData));
					compileContext.pushIns(emitIdiv64WithReg64Ins(tmpRegId));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
				compileContext.unallocReg(tmpRegId);
			}

			if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (savedRdxOff != INT32_MIN) {
					compileContext.popReg(REG_RDX, savedRdxOff, savedRdxSize);
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();
		} else {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			int32_t savedRdxOff = INT32_MIN;
			size_t savedRdxSize;
			const RegisterId lhsRegId = REG_RAX;

			if (compileContext.isRegInUse(lhsRegId)) {
				int32_t off;
				size_t size;
				compileContext.pushReg(lhsRegId, off, size);
			}
			if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (compileContext.isRegInUse(REG_RDX)) {
					compileContext.pushReg(REG_RDX, savedRdxOff, savedRdxSize);
				}
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
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
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
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (std::is_same_v<T, int8_t>) {
					compileContext.pushIns(
						emitIdiv8WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					compileContext.pushIns(
						emitIdiv16WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					compileContext.pushIns(
						emitIdiv32WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					compileContext.pushIns(
						emitIdiv64WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					compileContext.pushIns(
						emitDiv8WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					compileContext.pushIns(
						emitDiv16WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					compileContext.pushIns(
						emitDiv32WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					compileContext.pushIns(
						emitDiv64WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if constexpr (std::is_same_v<T, int8_t>) {
					compileContext.pushIns(
						emitIdiv8WithReg8Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					compileContext.pushIns(
						emitIdiv16WithReg16Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					compileContext.pushIns(
						emitIdiv32WithReg32Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					compileContext.pushIns(
						emitIdiv64WithReg64Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					compileContext.pushIns(
						emitDiv8WithReg8Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					compileContext.pushIns(
						emitDiv16WithReg16Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					compileContext.pushIns(
						emitDiv32WithReg32Ins(rhsVregState.phyReg));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					compileContext.pushIns(
						emitDiv64WithReg64Ins(rhsVregState.phyReg));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (savedRdxOff != INT32_MIN) {
					compileContext.popReg(REG_RDX, savedRdxOff, savedRdxSize);
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
InternalExceptionPointer compileFpDivInstruction(
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
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
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
				compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
			}
			if (compileContext.isRegInUse(tmpGpRegId)) {
				compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize);
			}

			compileContext.pushIns(emitMovImm32ToReg32Ins(tmpGpRegId, (uint8_t *)&rhsData));
			compileContext.pushIns(emitMovdReg32ToRegXmmIns(tmpXmmRegId, tmpGpRegId));
			compileContext.pushIns(emitDivssRegXmmToRegXmmIns(lhsXmmRegId, tmpXmmRegId));

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
			compileContext.pushIns(emitDivsdRegXmmToRegXmmIns(lhsXmmRegId, tmpXmmRegId));

			if (tmpGpOff != INT32_MIN) {
				compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize);
			}
			if (tmpXmmOff != INT32_MIN) {
				compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
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
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
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
					compileContext.pushRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
				}
				if (compileContext.isRegInUse(tmpGpRegId)) {
					compileContext.pushReg(tmpGpRegId, tmpGpOff, tmpGpSize);
				}

				compileContext.pushIns(emitMovImm32ToReg32Ins(tmpGpRegId, (uint8_t *)&lhsData));
				compileContext.pushIns(emitMovdReg32ToRegXmmIns(tmpXmmRegId, tmpGpRegId));
				compileContext.pushIns(emitDivssRegXmmToRegXmmIns(rhsXmmRegId, tmpXmmRegId));

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
				compileContext.pushIns(emitDivsdRegXmmToRegXmmIns(rhsXmmRegId, tmpXmmRegId));

				if (tmpGpOff != INT32_MIN) {
					compileContext.popReg(tmpGpRegId, tmpGpOff, tmpGpSize);
				}
				if (tmpXmmOff != INT32_MIN) {
					compileContext.popRegXmm(tmpXmmRegId, tmpXmmOff, tmpXmmSize);
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
				compileContext.pushRegXmm(lhsXmmRegId, off, size);
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsXmmRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			if constexpr (std::is_same_v<T, float>) {
				if (lhsVregState.saveOffset != INT32_MIN) {
					compileContext.pushIns(emitMovdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitDivssMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitDivssRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				} else {
					compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitDivssMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitDivssRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				}
			} else if constexpr (std::is_same_v<T, double>) {
				if (lhsVregState.saveOffset != INT32_MIN) {
					compileContext.pushIns(emitMovqMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 }));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitDivsdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitDivsdRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				} else {
					compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg));

					if (rhsVregState.saveOffset != INT32_MIN) {
						compileContext.pushIns(emitDivsdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 }));
					} else {
						RegisterId rhsXmmRegId = rhsVregState.phyReg;

						compileContext.pushIns(emitDivsdRegXmmToRegXmmIns(lhsXmmRegId, rhsXmmRegId));
					}
				}
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
		}
	}
}

InternalExceptionPointer slake::jit::x86_64::compileDivInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) {
	uint32_t outputRegIndex = UINT32_MAX;
	auto &outputRegInfo = analyzedInfo.analyzedRegInfo.at(outputRegIndex);

	Value expectedValue = analyzedInfo.analyzedRegInfo.at(outputRegIndex).expectedValue;
	if (expectedValue.valueType == ValueType::Undefined) {
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
						compileIntDivInstruction<int8_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::I16: {
						compileIntDivInstruction<int16_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::I32: {
						compileIntDivInstruction<int32_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::I64: {
						compileIntDivInstruction<int64_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U8: {
						compileIntDivInstruction<uint8_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U16: {
						compileIntDivInstruction<uint16_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U32: {
						compileIntDivInstruction<uint32_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::U64: {
						compileIntDivInstruction<uint64_t>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::F32: {
						compileFpDivInstruction<float>(
							compileContext,
							curIns,
							lhsExpectedValue,
							rhsExpectedValue);
						break;
					}
					case ValueType::F64: {
						compileFpDivInstruction<double>(
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
