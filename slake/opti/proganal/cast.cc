#include "../proganal.h"

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::analyzeCastIns(
	ProgramAnalyzeContext& analyzeContext,
	size_t regIndex) {
	Instruction &curIns = analyzeContext.fnObject->instructions.at(analyzeContext.idxCurIns);

	if (regIndex == UINT32_MAX) {
		return MalformedProgramError::alloc(
			analyzeContext.runtime,
			analyzeContext.fnObject,
			analyzeContext.idxCurIns);
	}

	if (curIns.operands.size() != 2) {
		return MalformedProgramError::alloc(
			analyzeContext.runtime,
			analyzeContext.fnObject,
			analyzeContext.idxCurIns);
	}

	if (curIns.operands[0].valueType != ValueType::TypeName) {
		return MalformedProgramError::alloc(
			analyzeContext.runtime,
			analyzeContext.fnObject,
			analyzeContext.idxCurIns);
	}

	Value constSrc(ValueType::Undefined);
	Type srcType, destType = curIns.operands[0].getTypeName();
	SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, curIns.operands[1], constSrc));
	SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], srcType));

	switch (srcType.typeId) {
		case TypeId::Value: {
			switch (srcType.getValueTypeExData()) {
				case ValueType::I8:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getI8());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getI8());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getI8());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getI8());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getI8());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getI8());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getI8());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getI8());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getI8());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getI8());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getI8());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::I16:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getI16());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getI16());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getI16());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getI16());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getI16());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getI16());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getI16());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getI16());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getI16());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getI16());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getI16());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::I32:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getI32());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getI32());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getI32());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getI32());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getI32());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getI32());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getI32());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getI32());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getI32());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getI32());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getI32());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::I64:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getI64());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getI64());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getI64());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getI64());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getI64());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getI64());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getI64());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getI64());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getI64());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getI64());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getI64());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::U8:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getU8());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getU8());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getU8());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getU8());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getU8());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getU8());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getU8());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getU8());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getU8());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getU8());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getU8());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::U16:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getU16());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getU16());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getU16());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getU16());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getU16());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getU16());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getU16());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getU16());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getU16());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getU16());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getU16());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::U32:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getU32());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getU32());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getU32());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getU32());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getU32());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getU32());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getU32());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getU32());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getU32());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getU32());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getU32());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::U64:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getU64());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getU64());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getU64());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getU64());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getU64());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getU64());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getU64());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getU64());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getU64());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getU64());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getU64());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::Bool:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getBool());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getBool());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getBool());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getBool());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getBool());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getBool());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getBool());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getBool());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getBool());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getBool());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getBool());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::F32:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getF32());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getF32());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getF32());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getF32());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getF32());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getF32());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getF32());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getF32());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getF32());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getF32());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getF32());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
				case ValueType::F64:
					switch (destType.typeId) {
						case TypeId::Value:
							switch (destType.getValueTypeExData()) {
								case ValueType::I8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int8_t)constSrc.getF64());
									}
									break;
								case ValueType::I16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int16_t)constSrc.getF64());
									}
									break;
								case ValueType::I32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int32_t)constSrc.getF64());
									}
									break;
								case ValueType::I64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((int64_t)constSrc.getF64());
									}
									break;
								case ValueType::U8:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint8_t)constSrc.getF64());
									}
									break;
								case ValueType::U16:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint16_t)constSrc.getF64());
									}
									break;
								case ValueType::U32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint32_t)constSrc.getF64());
									}
									break;
								case ValueType::U64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((uint64_t)constSrc.getF64());
									}
									break;
								case ValueType::F32:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((float)constSrc.getF64());
									}
									break;
								case ValueType::F64:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((double)constSrc.getF64());
									}
									break;
								case ValueType::Bool:
									if (constSrc.valueType != ValueType::Undefined) {
										analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value((bool)constSrc.getF64());
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
						default: {
							return MalformedProgramError::alloc(
								analyzeContext.runtime,
								analyzeContext.fnObject,
								analyzeContext.idxCurIns);
						}
					}
					break;
			}
			break;
		}
		case TypeId::Instance: {
			break;
		}
		default: {
			return MalformedProgramError::alloc(
				analyzeContext.runtime,
				analyzeContext.fnObject,
				analyzeContext.idxCurIns);
		}
	}

	analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = destType;

	return {};
}
