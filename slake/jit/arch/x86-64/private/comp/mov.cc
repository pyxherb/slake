#include "mov.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

InternalExceptionPointer slake::jit::x86_64::compileMovInstruction(
	JITCompileContext &compileContext,
	opti::ProgramAnalyzedInfo &analyzedInfo,
	size_t offIns,
	const Instruction &curIns) noexcept {
	InternalExceptionPointer exception;
	uint32_t outputRegIndex = curIns.output.getRegIndex();

	Value src = curIns.operands[1];

	switch (src.valueType) {
	case ValueType::I8: {
		int8_t imm0 = src.getI8();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int8_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::I16: {
		int16_t imm0 = src.getI16();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int16_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::I32: {
		int32_t imm0 = src.getI32();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int32_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::I64: {
		int64_t imm0 = src.getI64();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(int64_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::U8: {
		uint8_t imm0 = src.getU8();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint8_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::U16: {
		uint16_t imm0 = src.getU16();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm16ToReg16Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint16_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::U32: {
		uint32_t imm0 = src.getU32();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint32_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::U64: {
		uint64_t imm0 = src.getU64();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint64_t));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::Bool: {
		bool imm0 = src.getBool();

		RegisterId regId = compileContext.allocGpReg();
		if (compileContext.isRegInUse(regId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm8ToReg8Ins(regId, (uint8_t *)&imm0)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(bool));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		break;
	}
	case ValueType::F32: {
		float imm0 = src.getF32();

		RegisterId tmpRegId = compileContext.allocGpReg();
		int32_t tmpRegOff = INT32_MIN;
		size_t tmpRegSize;
		if (compileContext.isRegInUse(tmpRegId)) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&imm0)));

		RegisterId xmmRegId = compileContext.allocXmmReg();
		if (compileContext.isRegInUse(xmmRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(xmmRegId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdReg32ToRegXmmIns(xmmRegId, tmpRegId)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, xmmRegId, sizeof(float));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		if (tmpRegOff != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize));
		}
		break;
	}
	case ValueType::F64: {
		double imm0 = src.getF64();

		RegisterId tmpRegId = compileContext.allocGpReg();
		int32_t tmpRegOff = INT32_MIN;
		size_t tmpRegSize;
		if (compileContext.isRegInUse(tmpRegId)) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm32ToReg32Ins(tmpRegId, (uint8_t *)&imm0)));

		RegisterId xmmRegId = compileContext.allocXmmReg();
		if (compileContext.isRegInUse(xmmRegId)) {
			int32_t off;
			size_t size;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(xmmRegId, off, size));
		}
		SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovdReg32ToRegXmmIns(xmmRegId, tmpRegId)));
		VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, xmmRegId, sizeof(double));
		if (!outputVregState)
			return OutOfMemoryError::alloc();
		if (tmpRegOff != INT32_MIN) {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize));
		}
		break;
	}
	case ValueType::EntityRef: {
		EntityRef entityRef = src.getEntityRef();

		switch (entityRef.kind) {
		case ObjectRefKind::InstanceRef: {
			Object *imm0 = entityRef.asInstance.instanceObject;

			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
			}
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(regId, (uint8_t *)&imm0)));
			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(void *));
			if (!outputVregState)
				return OutOfMemoryError::alloc();
			break;
		}
		default:
			std::terminate();
		}
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
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
				}
				if (srcVregInfo.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg8Ins(
																		regId,
																		MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg8ToReg8Ins(regId, srcVregInfo.phyReg)));
				}

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint8_t));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			case ValueType::I16:
			case ValueType::U16: {
				RegisterId regId = compileContext.allocGpReg();
				if (compileContext.isRegInUse(regId)) {
					int32_t off;
					size_t size;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
				}
				if (srcVregInfo.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg16Ins(
																		regId,
																		MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg16ToReg16Ins(regId, srcVregInfo.phyReg)));
				}

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint16_t));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			case ValueType::I32:
			case ValueType::U32: {
				RegisterId regId = compileContext.allocGpReg();
				if (compileContext.isRegInUse(regId)) {
					int32_t off;
					size_t size;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
				}
				if (srcVregInfo.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg32Ins(
																		regId,
																		MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg32ToReg32Ins(regId, srcVregInfo.phyReg)));
				}

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint32_t));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			case ValueType::I64:
			case ValueType::U64: {
				RegisterId regId = compileContext.allocGpReg();
				if (compileContext.isRegInUse(regId)) {
					int32_t off;
					size_t size;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
				}
				if (srcVregInfo.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovMemToReg64Ins(
																		regId,
																		MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(regId, srcVregInfo.phyReg)));
				}

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint64_t));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			case ValueType::F32: {
				RegisterId regId = compileContext.allocXmmReg();
				if (compileContext.isRegInUse(regId)) {
					int32_t off;
					size_t size;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(regId, off, size));
				}
				if (srcVregInfo.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovdMemToRegXmmIns(
																		regId,
																		MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(regId, srcVregInfo.phyReg)));
				}

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(float));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			case ValueType::F64: {
				RegisterId regId = compileContext.allocXmmReg();
				if (compileContext.isRegInUse(regId)) {
					int32_t off;
					size_t size;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushRegXmm(regId, off, size));
				}
				if (srcVregInfo.saveOffset != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																	emitMovqMemToRegXmmIns(
																		regId,
																		MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
				} else {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovqRegXmmToRegXmmIns(regId, srcVregInfo.phyReg)));
				}

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(double));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			case ValueType::EntityRef: {
				int32_t off;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.stackAllocAligned(sizeof(EntityRef), sizeof(EntityRef), off));

				{
					RegisterId tmpRegId = compileContext.allocGpReg();
					int32_t tmpRegOff = INT32_MIN;
					size_t tmpRegSize;
					if (compileContext.isRegInUse(tmpRegId)) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize));
					}

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(tmpRegId, REG_RBP)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(tmpRegId, (uint8_t *)&srcVregInfo.saveOffset)));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(REG_RCX, tmpRegId)));

					if (tmpRegOff != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize));
					}
				}
				{
					RegisterId tmpRegId = compileContext.allocGpReg();
					int32_t tmpRegOff = INT32_MIN;
					size_t tmpRegSize;
					if (compileContext.isRegInUse(tmpRegId)) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize));
					}

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(tmpRegId, REG_RBP)));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(tmpRegId, (uint8_t *)&off)));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(REG_RDX, tmpRegId)));

					if (tmpRegOff != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize));
					}
				}
				{
					uint64_t size = sizeof(EntityRef);
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(REG_R8, (uint8_t *)&size)));
				}

				CallingRegSavingInfo callingRegSavingInfo;

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.saveCallingRegs(callingRegSavingInfo));

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitCallIns((void *)memcpyWrapper)));

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.restoreCallingRegs(callingRegSavingInfo));

				VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, off, sizeof(double));
				if (!outputVregState)
					return OutOfMemoryError::alloc();
				break;
			}
			}
			break;
		}
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::FnDelegate: {
			RegisterId regId = compileContext.allocGpReg();
			if (compileContext.isRegInUse(regId)) {
				int32_t off;
				size_t size;
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(regId, off, size));
			}
			if (srcVregInfo.saveOffset != INT32_MIN) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(
																emitMovMemToReg64Ins(
																	regId,
																	MemoryLocation{ REG_RBP, srcVregInfo.saveOffset, REG_MAX, 0 })));
			} else {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(regId, srcVregInfo.phyReg)));
			}

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, regId, sizeof(uint64_t));
			if (!outputVregState)
				return OutOfMemoryError::alloc();
			break;
		}
		case TypeId::Ref: {
			switch (srcRegInfo.storageType) {
			case opti::RegStorageType::None:
				break;
			case opti::RegStorageType::FieldVar:
				if (srcRegInfo.storageInfo.asFieldVar.isUsedForOutput) {
				} else {
					VirtualRegState *outputVregState = compileContext.defDummyVirtualReg(outputRegIndex);
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
				break;
			case opti::RegStorageType::LocalVar:
				if (srcRegInfo.storageInfo.asLocalVar.isUsedForOutput) {
				} else {
					VirtualRegState *outputVregState = compileContext.defDummyVirtualReg(outputRegIndex);
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
				break;
			case opti::RegStorageType::ArgRef:
				if (srcRegInfo.storageInfo.asArgRef.isUsedForOutput) {
				} else {
					VirtualRegState *outputVregState = compileContext.defDummyVirtualReg(outputRegIndex);
					if (!outputVregState)
						return OutOfMemoryError::alloc();
				}
				break;
			}
			break;
		}
		case TypeId::Any: {
			int32_t off;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.stackAllocAligned(sizeof(Value), sizeof(Value), off));

			{
				RegisterId tmpRegId = compileContext.allocGpReg();
				int32_t tmpRegOff = INT32_MIN;
				size_t tmpRegSize;
				if (compileContext.isRegInUse(tmpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize));
				}

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(tmpRegId, REG_RBP)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(tmpRegId, (uint8_t *)&srcVregInfo.saveOffset)));

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(REG_RCX, tmpRegId)));

				if (tmpRegOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize));
				}
			}
			{
				RegisterId tmpRegId = compileContext.allocGpReg();
				int32_t tmpRegOff = INT32_MIN;
				size_t tmpRegSize;
				if (compileContext.isRegInUse(tmpRegId)) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushReg(tmpRegId, tmpRegOff, tmpRegSize));
				}

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(tmpRegId, REG_RBP)));
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitAddImm32ToReg64Ins(tmpRegId, (uint8_t *)&off)));

				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovReg64ToReg64Ins(REG_RDX, tmpRegId)));

				if (tmpRegOff != INT32_MIN) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.popReg(tmpRegId, tmpRegOff, tmpRegSize));
				}
			}
			{
				uint64_t size = sizeof(Value);
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitMovImm64ToReg64Ins(REG_R8, (uint8_t *)&size)));
			}

			CallingRegSavingInfo callingRegSavingInfo;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.saveCallingRegs(callingRegSavingInfo));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.pushIns(emitCallIns((void *)memcpyWrapper)));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exception, compileContext.restoreCallingRegs(callingRegSavingInfo));

			VirtualRegState *outputVregState = compileContext.defVirtualReg(outputRegIndex, off, sizeof(double));
			if (!outputVregState)
				return OutOfMemoryError::alloc();

			break;
		} break;
		}
		break;
	}
	default:
		// Unhandled value type
		std::terminate();
	}

	return {};
}
