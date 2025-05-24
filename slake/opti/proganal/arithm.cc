#include "../proganal.h"
#include <slake/flib/math/fmod.h>

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::analyzeArithmeticIns(
	ProgramAnalyzeContext &analyzeContext,
	uint32_t regIndex) noexcept {
	Instruction &curIns = analyzeContext.fnObject->instructions.at(analyzeContext.idxCurIns);

	Value lhs = curIns.operands[0],
		  rhs = curIns.operands[1],
		  evaluatedLhs(ValueType::Undefined),
		  evaluatedRhs(ValueType::Undefined),
		  result = (ValueType::Undefined);
	Type lhsType, rhsType, resultType;

	switch (curIns.opcode) {
		case Opcode::ADD: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() + evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() + evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() + evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() + evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() + evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() + evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() + evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() + evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::F32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((float)evaluatedLhs.getF32() + evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::F64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((double)evaluatedLhs.getF64() + evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::SUB: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() - evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() - evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() - evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() - evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() - evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() - evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() - evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() - evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::F32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((float)evaluatedLhs.getF32() - evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::F64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((double)evaluatedLhs.getF64() - evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::MUL: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() * evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() * evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() * evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() * evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() * evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() * evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() * evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() * evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::F32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((float)evaluatedLhs.getF32() * evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::F64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((double)evaluatedLhs.getF64() * evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::DIV: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() / evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() / evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() / evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() / evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() / evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() / evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() / evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() / evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::F32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((float)evaluatedLhs.getF32() / evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::F64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((double)evaluatedLhs.getF64() / evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::MOD: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() % evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() % evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() % evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() % evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() % evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() % evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() % evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() % evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::F32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((float)flib::fmodf(evaluatedLhs.getF32(), evaluatedRhs.getF32()));
					}
					break;
				case TypeId::F64:
					resultType = TypeId::F64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((double)flib::fmod(evaluatedLhs.getF64(), evaluatedRhs.getF64()));
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::AND: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() & evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() & evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() & evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() & evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() & evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() & evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() & evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() & evaluatedRhs.getU64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::OR: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() | evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() | evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() | evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() | evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() | evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() | evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() | evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() | evaluatedRhs.getU64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::XOR: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() ^ evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() ^ evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() ^ evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() ^ evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() ^ evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() ^ evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() ^ evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() ^ evaluatedRhs.getU64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::LAND: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != TypeId::Bool) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}
			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Bool:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((bool)evaluatedLhs.getBool() && evaluatedRhs.getBool());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::LOR: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != TypeId::Bool) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}
			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Bool:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((bool)evaluatedLhs.getBool() || evaluatedRhs.getBool());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::EQ: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI8() == evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI16() == evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI32() == evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI64() == evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU8() == evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU16() == evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU32() == evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU64() == evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF32() == evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF64() == evaluatedRhs.getF64());
					}
					break;
				case TypeId::Bool:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getBool() == evaluatedRhs.getBool());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::NEQ: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI8() != evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI16() != evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI32() != evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI64() != evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU8() != evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU16() != evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU32() != evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU64() != evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF32() != evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF64() != evaluatedRhs.getF64());
					}
					break;
				case TypeId::Bool:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getBool() != evaluatedRhs.getBool());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::LT: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI8() < evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI16() < evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI32() < evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI64() < evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU8() < evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU16() < evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU32() < evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU64() < evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF32() < evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF64() < evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::GT: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI8() > evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI16() > evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI32() > evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI64() > evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU8() > evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU16() > evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU32() > evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU64() > evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF32() > evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF64() > evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::LTEQ: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI8() <= evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI16() <= evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI32() <= evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI64() <= evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU8() <= evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU16() <= evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU32() <= evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU64() <= evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF32() <= evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF64() <= evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::GTEQ: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI8() >= evaluatedRhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI16() >= evaluatedRhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI32() >= evaluatedRhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getI64() >= evaluatedRhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU8() >= evaluatedRhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU16() >= evaluatedRhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU32() >= evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getU64() >= evaluatedRhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF32() >= evaluatedRhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value(evaluatedLhs.getF64() >= evaluatedRhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::CMP: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						int8_t lhsData = evaluatedLhs.getI8(), rhsData = evaluatedRhs.getI8();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						int16_t lhsData = evaluatedLhs.getI16(), rhsData = evaluatedRhs.getI16();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						int32_t lhsData = evaluatedLhs.getI32(), rhsData = evaluatedRhs.getI32();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						int64_t lhsData = evaluatedLhs.getI64(), rhsData = evaluatedRhs.getI64();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U8:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						uint8_t lhsData = evaluatedLhs.getU8(), rhsData = evaluatedRhs.getU8();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U16:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						uint16_t lhsData = evaluatedLhs.getU16(), rhsData = evaluatedRhs.getU16();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						uint32_t lhsData = evaluatedLhs.getU32(), rhsData = evaluatedRhs.getU32();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::U64:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						uint64_t lhsData = evaluatedLhs.getU64(), rhsData = evaluatedRhs.getU64();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::F32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						float lhsData = evaluatedLhs.getF32(), rhsData = evaluatedRhs.getF32();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				case TypeId::F64:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						double lhsData = evaluatedLhs.getF64(), rhsData = evaluatedRhs.getF64();
						if (lhsData < rhsData) {
							result = Value((int32_t)-1);
						} else if (lhsData > rhsData) {
							result = Value((int32_t)1);
						} else {
							result = Value((int32_t)0);
						}
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::LSH: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (rhsType != TypeId::U32) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() << evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() << evaluatedRhs.getU32());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::RSH: {
			if (curIns.nOperands != 2) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (rhsType != TypeId::U32) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)evaluatedLhs.getI8() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)evaluatedLhs.getI16() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)evaluatedLhs.getI32() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)evaluatedLhs.getI64() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32() >> evaluatedRhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined &&
						evaluatedRhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64() >> evaluatedRhs.getU32());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::NOT: {
			if (curIns.nOperands != 1) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, lhs, lhsType));

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)~evaluatedLhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)~evaluatedLhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)~evaluatedLhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)~evaluatedLhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)~evaluatedLhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)~evaluatedLhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)~evaluatedLhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)~evaluatedLhs.getU64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::LNOT: {
			if (curIns.nOperands != 1) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, lhs, lhsType));

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));

			switch (lhsType.typeId) {
				case TypeId::Bool:
					resultType = TypeId::Bool;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((bool)!evaluatedLhs.getBool());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		case Opcode::NEG: {
			if (curIns.nOperands != 1) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, lhs, lhsType));

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));

			switch (lhsType.typeId) {
				case TypeId::I8:
					resultType = TypeId::I8;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int8_t)-evaluatedLhs.getI8());
					}
					break;
				case TypeId::I16:
					resultType = TypeId::I16;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int16_t)-evaluatedLhs.getI16());
					}
					break;
				case TypeId::I32:
					resultType = TypeId::I32;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int32_t)-evaluatedLhs.getI32());
					}
					break;
				case TypeId::I64:
					resultType = TypeId::I64;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((int64_t)-evaluatedLhs.getI64());
					}
					break;
				case TypeId::U8:
					resultType = TypeId::U8;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint8_t)evaluatedLhs.getU8());
					}
					break;
				case TypeId::U16:
					resultType = TypeId::U16;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint16_t)evaluatedLhs.getU16());
					}
					break;
				case TypeId::U32:
					resultType = TypeId::U32;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint32_t)evaluatedLhs.getU32());
					}
					break;
				case TypeId::U64:
					resultType = TypeId::U64;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value((uint64_t)evaluatedLhs.getU64());
					}
					break;
				case TypeId::F32:
					resultType = TypeId::F32;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value(-evaluatedLhs.getF32());
					}
					break;
				case TypeId::F64:
					resultType = TypeId::F64;
					if (evaluatedLhs.valueType != ValueType::Undefined) {
						result = Value(-evaluatedLhs.getF64());
					}
					break;
				default: {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							analyzeContext.runtime->getFixedAlloc(),
							analyzeContext.fnObject,
							analyzeContext.idxCurIns));
				}
			}

			if (regIndex != UINT32_MAX) {
				if (result.valueType != ValueType::Undefined) {
					analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
				}
			}
			break;
		}
		default:
			throw std::logic_error("Unhandled opcode");
	}

	return {};
}
