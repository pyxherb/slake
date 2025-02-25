#include "mod.h"
#include <slake/flib/math/fmod.h>

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

template <typename T>
[[nodiscard]] InternalExceptionPointer compileIntModInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	if (rhsExpectedValue.valueType != ValueType::Undefined) {
		uint32_t lhsRegIndex = curIns.operands[0].getRegIndex();
		int32_t savedRaxOff = INT32_MIN;
		size_t savedRaxSize;
		RegisterId lhsRegId;

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			lhsRegId = REG_RAX;
		} else {
			lhsRegId = REG_RDX;
			if (compileContext.isRegInUse(REG_RAX)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(REG_RAX, savedRaxOff, savedRaxSize));
			}
		}
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
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg16Ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg32Ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg64Ins(
																	REG_RAX,
																	MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		} else {
			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg8ToReg8Ins(
																	REG_RAX,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg16ToReg16Ins(
																	REG_RAX,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg32ToReg32Ins(
																	REG_RAX,
																	lhsVregState.phyReg)));
			} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovReg64ToReg64Ins(
																	REG_RAX,
																	lhsVregState.phyReg)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand size");
			}
		}

		// Try to allocate a new temporary register to store the right operand.
		const RegisterId tmpRegId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(tmpRegId)) {
			int32_t off;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.stackAllocAligned(sizeof(T), sizeof(T), off));
			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t rhsData = curIns.operands[1].getI8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t rhsData = curIns.operands[1].getI16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t rhsData = curIns.operands[1].getI32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t rhsData = curIns.operands[1].getI64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t rhsData = curIns.operands[1].getU8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t rhsData = curIns.operands[1].getU16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t rhsData = curIns.operands[1].getU32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t rhsData = curIns.operands[1].getU64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}

			compileContext.stackFree(off, sizeof(T));
		} else {
			if constexpr (std::is_same_v<T, int8_t>) {
				int8_t rhsData = curIns.operands[1].getI8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv8WithReg8Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, int16_t>) {
				int16_t rhsData = curIns.operands[1].getI16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv16WithReg16Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, int32_t>) {
				int32_t rhsData = curIns.operands[1].getI32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv32WithReg32Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, int64_t>) {
				int64_t rhsData = curIns.operands[1].getI64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv64WithReg64Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, uint8_t>) {
				uint8_t rhsData = curIns.operands[1].getU8();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv8WithReg8Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, uint16_t>) {
				uint16_t rhsData = curIns.operands[1].getU16();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv16WithReg16Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, uint32_t>) {
				uint32_t rhsData = curIns.operands[1].getU32();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv32WithReg32Ins(tmpRegId)));
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				uint64_t rhsData = curIns.operands[1].getU64();
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&rhsData)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv64WithReg64Ins(tmpRegId)));
			} else {
				static_assert(!std::is_same_v<T, T>, "Invalid operand type");
			}
			compileContext.unallocReg(tmpRegId);
		}

		if constexpr (sizeof(T) == sizeof(uint8_t)) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg16WithImm8Ins(REG_RAX, 8)));
		} else if constexpr (sizeof(T) > sizeof(uint8_t)) {
			if (savedRaxOff != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(REG_RAX, savedRaxOff, savedRaxSize));
			}
		}

		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsRegId, sizeof(T));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
	} else {
		if (lhsExpectedValue.valueType != ValueType::Undefined) {  // The RHS is an expectable value so we can just simply add it with a register.
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			int32_t savedRaxOff = INT32_MIN;
			size_t savedRaxSize;
			RegisterId rhsRegId;

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				rhsRegId = REG_RAX;
			} else {
				rhsRegId = REG_RDX;
				if (compileContext.isRegInUse(REG_RAX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(REG_RAX, savedRaxOff, savedRaxSize));
				}
			}
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
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		REG_RAX,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		REG_RAX,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		REG_RAX,
																		rhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		REG_RAX,
																		rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			// Try to allocate a new temporary register to store the right operand.
			const RegisterId tmpRegId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(tmpRegId)) {
				int32_t off;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.stackAllocAligned(sizeof(T), sizeof(T), off));
				if constexpr (std::is_same_v<T, int8_t>) {
					int8_t lhsData = curIns.operands[0].getI8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					int16_t lhsData = curIns.operands[0].getI16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					int32_t lhsData = curIns.operands[0].getI32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					int64_t lhsData = curIns.operands[0].getI64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					uint8_t lhsData = curIns.operands[0].getU8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv8WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					uint16_t lhsData = curIns.operands[0].getU16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv16WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					uint32_t lhsData = curIns.operands[0].getU32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv32WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					uint64_t lhsData = curIns.operands[0].getU64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 }, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv64WithMemIns(MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}

				compileContext.stackFree(off, sizeof(T));
			} else {
				if constexpr (std::is_same_v<T, int8_t>) {
					int8_t lhsData = curIns.operands[1].getI8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv8WithReg8Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					int16_t lhsData = curIns.operands[1].getI16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv16WithReg16Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					int32_t lhsData = curIns.operands[1].getI32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv32WithReg32Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					int64_t lhsData = curIns.operands[1].getI64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitDiv64WithReg64Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					uint8_t lhsData = curIns.operands[1].getU8();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv8WithReg8Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					uint16_t lhsData = curIns.operands[1].getU16();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToReg16Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv16WithReg16Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					uint32_t lhsData = curIns.operands[1].getU32();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv32WithReg32Ins(tmpRegId)));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					uint64_t lhsData = curIns.operands[1].getU64();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(tmpRegId, (uint8_t *)&lhsData)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitIdiv64WithReg64Ins(tmpRegId)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
				compileContext.unallocReg(tmpRegId);
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg16WithImm8Ins(REG_RAX, 8)));
			} else if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (savedRaxOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(REG_RAX, savedRaxOff, savedRaxSize));
				}
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, rhsRegId, sizeof(T));
			if (!outputVregState)
				return OutOfMemoryError::alloc();
		} else {
			uint32_t rhsRegIndex = curIns.operands[1].getRegIndex();
			int32_t savedRaxOff = INT32_MIN;
			size_t savedRaxSize;
			RegisterId lhsRegId;

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				lhsRegId = REG_RAX;
			} else {
				lhsRegId = REG_RDX;
				if (compileContext.isRegInUse(REG_RAX)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(REG_RAX, savedRaxOff, savedRaxSize));
				}
			}
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
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		REG_RAX,
																		MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			} else {
				if constexpr (sizeof(T) == sizeof(uint8_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg8ToReg8Ins(
																		REG_RAX,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint16_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg16ToReg16Ins(
																		REG_RAX,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint32_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg32ToReg32Ins(
																		REG_RAX,
																		lhsVregState.phyReg)));
				} else if constexpr (sizeof(T) == sizeof(uint64_t)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovReg64ToReg64Ins(
																		REG_RAX,
																		lhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand size");
				}
			}

			VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
			if (rhsVregState.saveOffset != INT32_MIN) {
				if constexpr (std::is_same_v<T, int8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv8WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv16WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv32WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv64WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv8WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv16WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv32WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv64WithMemIns(MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			} else {
				if constexpr (std::is_same_v<T, int8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv8WithReg8Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, int16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv16WithReg16Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, int32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv32WithReg32Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, int64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitIdiv64WithReg64Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, uint8_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv8WithReg8Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, uint16_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv16WithReg16Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, uint32_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv32WithReg32Ins(rhsVregState.phyReg)));
				} else if constexpr (std::is_same_v<T, uint64_t>) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitDiv64WithReg64Ins(rhsVregState.phyReg)));
				} else {
					static_assert(!std::is_same_v<T, T>, "Invalid operand type");
				}
			}

			if constexpr (sizeof(T) == sizeof(uint8_t)) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitShrReg16WithImm8Ins(REG_RAX, 8)));
			} else if constexpr (sizeof(T) > sizeof(uint8_t)) {
				if (savedRaxOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(REG_RAX, savedRaxOff, savedRaxSize));
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
[[nodiscard]] InternalExceptionPointer compileFpModInstruction(
	JITCompileContext &compileContext,
	const Instruction &curIns,
	const Value &lhsExpectedValue,
	const Value &rhsExpectedValue) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	uint32_t lhsRegIndex = curIns.operands[0].getRegIndex(),
			 rhsRegIndex = curIns.operands[1].getRegIndex();

	VirtualRegState &lhsVregState = compileContext.virtualRegStates.at(lhsRegIndex);
	VirtualRegState &rhsVregState = compileContext.virtualRegStates.at(rhsRegIndex);
	const RegisterId lhsXmmRegId = REG_XMM0, rhsXmmRegId = REG_XMM1;
	int32_t rhsOff = INT32_MIN;
	size_t rhsSize;

	if (compileContext.isRegInUse(lhsXmmRegId)) {
		int32_t off;
		size_t size;
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(lhsXmmRegId, off, size));
	}

	if (compileContext.isRegInUse(rhsXmmRegId)) {
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(rhsXmmRegId, rhsOff, rhsSize));
	}

	VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, lhsXmmRegId, sizeof(T));
	if (!outputVregState)
		return OutOfMemoryError::alloc();

	if constexpr (std::is_same_v<T, float>) {
		if (lhsVregState.saveOffset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg)));
		}

		if (rhsVregState.saveOffset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdMemToRegXmmIns(rhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(rhsXmmRegId, rhsVregState.phyReg)));
		}

		size_t padding = compileContext.curStackSize % 16;
		int32_t paddingOff;

		if (padding) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.stackAllocAligned(16 - padding, 1, paddingOff));
		}

		CallingRegSavingInfo callingRegSavingInfo;

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.saveCallingRegs(callingRegSavingInfo));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitCallIns((void *)fmodfWrapper)));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.restoreCallingRegs(callingRegSavingInfo));

		if (padding) {
			compileContext.stackFree(paddingOff, 16 - padding);
		}
	} else if constexpr (std::is_same_v<T, double>) {
		if (lhsVregState.saveOffset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqMemToRegXmmIns(lhsXmmRegId, MemoryLocation{ REG_RBP, lhsVregState.saveOffset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(lhsXmmRegId, lhsVregState.phyReg)));
		}

		if (rhsVregState.saveOffset != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqMemToRegXmmIns(rhsXmmRegId, MemoryLocation{ REG_RBP, rhsVregState.saveOffset, REG_MAX, 0 })));
		} else {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(rhsXmmRegId, rhsVregState.phyReg)));
		}

		uint32_t padding = (uint32_t)compileContext.curStackSize % 16;

		if (padding) {
			uint32_t szDiff = 16 - padding;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.checkAndPushStackPointer(szDiff));
		}

		CallingRegSavingInfo callingRegSavingInfo;

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.saveCallingRegs(callingRegSavingInfo));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitCallIns((void *)fmodWrapper)));

		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.restoreCallingRegs(callingRegSavingInfo));

		if (padding) {
			uint32_t szDiff = 16 - padding;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(REG_RSP, (uint8_t *)&szDiff)));
			compileContext.subStackPtr(16 - padding);
		}
	} else {
		static_assert(!std::is_same_v<T, T>, "Invalid operand type");
	}

	if (rhsOff != INT32_MIN) {
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popRegXmm(rhsXmmRegId, rhsOff, rhsSize));
	}

	return {};
}

InternalExceptionPointer slake::jit::x86_64::compileModInstruction(
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
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
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
		case TypeId::I8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<int8_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::I16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<int16_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::I32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<int32_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::I64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<int64_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::U8: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<uint8_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::U16: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<uint16_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::U32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<uint32_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::U64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileIntModInstruction<uint64_t>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::F32: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileFpModInstruction<float>(
															compileContext,
															curIns,
															lhsExpectedValue,
															rhsExpectedValue));
			break;
		}
		case TypeId::F64: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileFpModInstruction<double>(
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

	return {};
}

SLAKE_API float slake::jit::x86_64::fmodfWrapper(float n, float d) {
	return flib::fmodf(n, d);
}

SLAKE_API double slake::jit::x86_64::fmodWrapper(double n, double d) {
	return flib::fmod(n, d);
}
