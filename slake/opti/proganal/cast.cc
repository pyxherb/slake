#include "../proganal.h"

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::analyzeCastIns(
	ProgramAnalyzeContext &analyzeContext,
	size_t regIndex) {
	Instruction &curIns = analyzeContext.fnObject->instructions[analyzeContext.idxCurIns];

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

	Type destType = curIns.operands[0].getTypeName();
	Value src, valueOut = ValueType::Undefined;
	Type srcType;
	SLAKE_RETURN_IF_EXCEPT(evalConstValue(analyzeContext, curIns.operands[1], src));
	SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], srcType));

	if (src.valueType != ValueType::Undefined) {
		switch (srcType.typeId) {
			case TypeId::Value: {
				switch (srcType.getValueTypeExData()) {
					case ValueType::I8: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getI8());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getI8());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getI8());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getI8());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getI8());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getI8());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getI8());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getI8());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getI8());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getI8());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getI8());
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
						break;
					}
					case ValueType::I16: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getI16());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getI16());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getI16());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getI16());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getI16());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getI16());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getI16());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getI16());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getI16());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getI16());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getI16());
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
						break;
					}
					case ValueType::I32: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getI32());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getI32());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getI32());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getI32());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getI32());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getI32());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getI32());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getI32());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getI32());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getI32());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getI32());
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
						break;
					}
					case ValueType::I64: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getI64());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getI64());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getI64());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getI64());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getI64());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getI64());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getI64());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getI64());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getI64());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getI64());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getI64());
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
						break;
					}
					case ValueType::U8: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getU8());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getU8());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getU8());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getU8());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getU8());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getU8());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getU8());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getU8());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getU8());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getU8());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getU8());
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
						break;
					}
					case ValueType::U16: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getU16());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getU16());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getU16());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getU16());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getU16());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getU16());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getU16());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getU16());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getU16());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getU16());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getU16());
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
						break;
					}
					case ValueType::U32: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getU32());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getU32());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getU32());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getU32());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getU32());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getU32());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getU32());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getU32());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getU32());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getU32());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getU32());
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
						break;
					}
					case ValueType::U64: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getU64());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getU64());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getU64());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getU64());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getU64());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getU64());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getU64());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getU64());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getU64());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getU64());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getU64());
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
						break;
					}
					case ValueType::F32: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getF32());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getF32());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getF32());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getF32());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getF32());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getF32());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getF32());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getF32());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getF32());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getF32());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getF32());
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
						break;
					}
					case ValueType::F64: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getF64());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getF64());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getF64());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getF64());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getF64());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getF64());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getF64());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getF64());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getF64());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getF64());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getF64());
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
						break;
					}
					case ValueType::Bool: {
						switch (destType.typeId) {
							case TypeId::Value: {
								switch (destType.getValueTypeExData()) {
									case ValueType::I8:
										valueOut = Value((int8_t)src.getBool());
										break;
									case ValueType::I16:
										valueOut = Value((int16_t)src.getBool());
										break;
									case ValueType::I32:
										valueOut = Value((int32_t)src.getBool());
										break;
									case ValueType::I64:
										valueOut = Value((int64_t)src.getBool());
										break;
									case ValueType::U8:
										valueOut = Value((uint8_t)src.getBool());
										break;
									case ValueType::U16:
										valueOut = Value((uint16_t)src.getBool());
										break;
									case ValueType::U32:
										valueOut = Value((uint32_t)src.getBool());
										break;
									case ValueType::U64:
										valueOut = Value((uint64_t)src.getBool());
										break;
									case ValueType::F32:
										valueOut = Value((float)src.getBool());
										break;
									case ValueType::F64:
										valueOut = Value((double)src.getBool());
										break;
									case ValueType::Bool:
										valueOut = Value((bool)src.getBool());
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
						break;
					}
				}
				break;
			}
			case TypeId::Instance: {
				// stub.
				break;
			}
			default: {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}
		}
		analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = valueOut;
	}

	analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = destType;

	return {};
}
