#include "../proganal.h"

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::analyzeExprIns(
	ProgramAnalyzeContext &analyzeContext,
	uint32_t regIndex) {
	Instruction &curIns = analyzeContext.fnObject->instructions[analyzeContext.idxCurIns];

	switch (curIns.opcode) {
		case Opcode::ADD: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() + evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() + evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() + evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() + evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() + evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() + evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() + evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() + evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							resultType = ValueType::F32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((float)evaluatedLhs.getF32() + evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							resultType = ValueType::F64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((double)evaluatedLhs.getF64() + evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::SUB: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() - evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() - evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() - evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() - evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() - evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() - evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() - evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() - evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							resultType = ValueType::F32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((float)evaluatedLhs.getF32() - evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							resultType = ValueType::F64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((double)evaluatedLhs.getF64() - evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::MUL: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() * evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() * evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() * evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() * evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() * evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() * evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() * evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() * evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							resultType = ValueType::F32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((float)evaluatedLhs.getF32() * evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							resultType = ValueType::F64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((double)evaluatedLhs.getF64() * evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::DIV: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() / evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() / evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() / evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() / evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() / evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() / evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() / evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() / evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							resultType = ValueType::F32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((float)evaluatedLhs.getF32() / evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							resultType = ValueType::F64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((double)evaluatedLhs.getF64() / evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::AND: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() & evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() & evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() & evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() & evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() & evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() & evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() & evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() & evaluatedRhs.getU64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::OR: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() | evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() | evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() | evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() | evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() | evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() | evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() | evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() | evaluatedRhs.getU64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::XOR: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() ^ evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() ^ evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() ^ evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() ^ evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() ^ evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() ^ evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() ^ evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() ^ evaluatedRhs.getU64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::LAND: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != ValueType::Bool) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}
			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::Bool:
							resultType = ValueType::Bool;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((bool)evaluatedLhs.getBool() && evaluatedRhs.getBool());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::LOR: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != ValueType::Bool) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}
			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::Bool:
							resultType = ValueType::Bool;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((bool)evaluatedLhs.getBool() || evaluatedRhs.getBool());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::EQ: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI8() == evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI16() == evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI32() == evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI64() == evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU8() == evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU16() == evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU32() == evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU64() == evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF32() == evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF64() == evaluatedRhs.getF64());
							}
							break;
						case ValueType::Bool:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getBool() == evaluatedRhs.getBool());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::NEQ: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI8() != evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI16() != evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI32() != evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI64() != evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU8() != evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU16() != evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU32() != evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU64() != evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF32() != evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF64() != evaluatedRhs.getF64());
							}
							break;
						case ValueType::Bool:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getBool() != evaluatedRhs.getBool());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::LT: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI8() < evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI16() < evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI32() < evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI64() < evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU8() < evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU16() < evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU32() < evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU64() < evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF32() < evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF64() < evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::GT: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI8() > evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI16() > evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI32() > evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI64() > evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU8() > evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU16() > evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU32() > evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU64() > evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF32() > evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF64() > evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::LTEQ: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI8() <= evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI16() <= evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI32() <= evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI64() <= evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU8() <= evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU16() <= evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU32() <= evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU64() <= evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF32() <= evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF64() <= evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::GTEQ: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (lhsType != rhsType) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI8() >= evaluatedRhs.getI8());
							}
							break;
						case ValueType::I16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI16() >= evaluatedRhs.getI16());
							}
							break;
						case ValueType::I32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI32() >= evaluatedRhs.getI32());
							}
							break;
						case ValueType::I64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getI64() >= evaluatedRhs.getI64());
							}
							break;
						case ValueType::U8:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU8() >= evaluatedRhs.getU8());
							}
							break;
						case ValueType::U16:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU16() >= evaluatedRhs.getU16());
							}
							break;
						case ValueType::U32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU32() >= evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getU64() >= evaluatedRhs.getU64());
							}
							break;
						case ValueType::F32:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF32() >= evaluatedRhs.getF32());
							}
							break;
						case ValueType::F64:
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value(evaluatedLhs.getF64() >= evaluatedRhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::LSH: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (rhsType != ValueType::U32) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() << evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() << evaluatedRhs.getU32());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::RSH: {
			if (curIns.operands.size() != 2) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			Value lhs = curIns.operands[0],
				  rhs = curIns.operands[1],
				  evaluatedLhs(ValueType::Undefined),
				  evaluatedRhs(ValueType::Undefined),
				  result = (ValueType::Undefined);
			Type lhsType, rhsType, resultType;

			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], lhsType));
			SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], rhsType));

			if (rhsType != ValueType::U32) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, lhs, lhs));
			SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, rhs, rhs));

			switch (lhsType.typeId) {
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)evaluatedLhs.getI8() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)evaluatedLhs.getI16() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)evaluatedLhs.getI32() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)evaluatedLhs.getI64() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32() >> evaluatedRhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined &&
								evaluatedRhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64() >> evaluatedRhs.getU32());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = resultType;
			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::NOT: {
			if (curIns.operands.size() != 1) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
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
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)~evaluatedLhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)~evaluatedLhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)~evaluatedLhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)~evaluatedLhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)~evaluatedLhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)~evaluatedLhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)~evaluatedLhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)~evaluatedLhs.getU64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::LNOT: {
			if (curIns.operands.size() != 1) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
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
				case TypeId::Value: {
					resultType = ValueType::Bool;
					switch (lhsType.getValueTypeExData()) {
						case ValueType::Bool:
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((bool)!evaluatedLhs.getBool());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		case Opcode::NEG: {
			if (curIns.operands.size() != 1) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
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
				case TypeId::Value: {
					switch (lhsType.getValueTypeExData()) {
						case ValueType::I8:
							resultType = ValueType::I8;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int8_t)-evaluatedLhs.getI8());
							}
							break;
						case ValueType::I16:
							resultType = ValueType::I16;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int16_t)-evaluatedLhs.getI16());
							}
							break;
						case ValueType::I32:
							resultType = ValueType::I32;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int32_t)-evaluatedLhs.getI32());
							}
							break;
						case ValueType::I64:
							resultType = ValueType::I64;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((int64_t)-evaluatedLhs.getI64());
							}
							break;
						case ValueType::U8:
							resultType = ValueType::U8;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint8_t)evaluatedLhs.getU8());
							}
							break;
						case ValueType::U16:
							resultType = ValueType::U16;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint16_t)evaluatedLhs.getU16());
							}
							break;
						case ValueType::U32:
							resultType = ValueType::U32;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint32_t)evaluatedLhs.getU32());
							}
							break;
						case ValueType::U64:
							resultType = ValueType::U64;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value((uint64_t)evaluatedLhs.getU64());
							}
							break;
						case ValueType::F32:
							resultType = ValueType::F32;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value(-evaluatedLhs.getF32());
							}
							break;
						case ValueType::F64:
							resultType = ValueType::F64;
							if (evaluatedLhs.valueType != ValueType::Undefined) {
								result = Value(-evaluatedLhs.getF64());
							}
							break;
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				}
				default: {
					return MalformedProgramError::alloc(
						analyzeContext.runtime,
						analyzeContext.fnObject,
						analyzeContext.idxCurIns);
				}
			}

			if (result.valueType != ValueType::Undefined) {
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = result;
			}
			break;
		}
		default:
			throw std::logic_error("Unhandled opcode");
	}

	return {};
}
