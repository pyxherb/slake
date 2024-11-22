#include "proganal.h"

using namespace slake;
using namespace slake::opti;

InternalExceptionPointer slake::opti::wrapIntoRefType(
	Runtime *runtime,
	Type type,
	HostRefHolder &hostRefHolder,
	Type &typeOut) {
	HostObjectRef<TypeDefObject> typeDef = TypeDefObject::alloc(
		runtime,
		type);
	hostRefHolder.addObject(typeDef.get());
	typeOut = Type(TypeId::Ref, typeDef.get());

	return {};
}

InternalExceptionPointer slake::opti::wrapIntoArrayType(
	Runtime *runtime,
	Type type,
	HostRefHolder &hostRefHolder,
	Type &typeOut) {
	HostObjectRef<TypeDefObject> typeDef = TypeDefObject::alloc(
		runtime,
		type);
	hostRefHolder.addObject(typeDef.get());
	typeOut = Type(TypeId::Ref, typeDef.get());

	return {};
}

InternalExceptionPointer slake::opti::evalObjectType(
	ProgramAnalyzeContext &analyzeContext,
	const VarRefContext &varRefContext,
	Object *object,
	Type &typeOut) {
	switch (object->getKind()) {
		case ObjectKind::String: {
			typeOut = Type(TypeId::String);
			break;
		}
		case ObjectKind::Array: {
			SLAKE_RETURN_IF_EXCEPT(wrapIntoArrayType(
				analyzeContext.runtime,
				((ArrayObject *)object)->elementType,
				analyzeContext.hostRefHolder,
				typeOut));
			break;
		}
		case ObjectKind::Var: {
			Type varType = (((VarObject *)object)->getVarType(varRefContext));

			SLAKE_RETURN_IF_EXCEPT(
				wrapIntoRefType(
					analyzeContext.runtime,
					varType,
					analyzeContext.hostRefHolder,
					typeOut));
			break;
		}
		case ObjectKind::FnOverloading: {
			FnOverloadingObject *fnOverloadingObject = (FnOverloadingObject *)object;

			auto paramTypes = fnOverloadingObject->paramTypes;
			auto typeDef = FnTypeDefObject::alloc(
				analyzeContext.runtime,
				fnOverloadingObject->returnType,
				std::move(paramTypes));
			analyzeContext.hostRefHolder.addObject(typeDef.get());
			typeOut = Type(TypeId::FnDelegate, typeDef.get());
			break;
		}
		default:
			return ErrorEvaluatingObjectTypeError::alloc(
				object->associatedRuntime,
				object);
	}

	return {};
}

InternalExceptionPointer slake::opti::evalValueType(
	ProgramAnalyzeContext &analyzeContext,
	const Value &value,
	Type &typeOut) {
	switch (value.valueType) {
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
		case ValueType::Bool: {
			typeOut = Type(value.valueType);
			break;
		}
		case ValueType::ObjectRef: {
			Type objectType;

			SLAKE_RETURN_IF_EXCEPT(evalObjectType(
				analyzeContext,
				VarRefContext{},
				value.getObjectRef(),
				typeOut));
			break;
		}
		case ValueType::RegRef: {
			uint32_t regIndex = value.getRegIndex();

			if (!analyzeContext.analyzedInfoOut.analyzedRegInfo.count(regIndex)) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			typeOut = analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type;
			break;
		}
		case ValueType::VarRef: {
			VarRef varRef = value.getVarRef();

			typeOut = varRef.varPtr->getVarType(varRef.context);
			break;
		}
		default: {
			return MalformedProgramError::alloc(
				analyzeContext.runtime,
				analyzeContext.fnObject,
				analyzeContext.idxCurIns);
		}
	}
	return {};
}

InternalExceptionPointer slake::opti::evalConstValue(
	ProgramAnalyzeContext &analyzeContext,
	const Value &value,
	Value &constValueOut) {
	switch (value.valueType) {
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
			constValueOut = value;
			break;
		case ValueType::RegRef: {
			uint32_t idxReg = value.getRegIndex();

			if (!analyzeContext.analyzedInfoOut.analyzedRegInfo.count(idxReg)) {
				return MalformedProgramError::alloc(
					analyzeContext.runtime,
					analyzeContext.fnObject,
					analyzeContext.idxCurIns);
			}

			constValueOut = analyzeContext.analyzedInfoOut.analyzedRegInfo[idxReg].expectedValue;
			break;
		}
		case ValueType::VarRef: {
			VarRef varRef = value.getVarRef();

			SLAKE_RETURN_IF_EXCEPT(varRef.varPtr->getData(varRef.context, constValueOut));
			break;
		}
		default:
			constValueOut = Value();
	}
	return {};
}

InternalExceptionPointer slake::opti::analyzeProgramInfo(
	Runtime *runtime,
	RegularFnOverloadingObject *fnObject,
	ProgramAnalyzedInfo &analyzedInfoOut,
	HostRefHolder &hostRefHolder) {
	size_t nIns = fnObject->instructions.size();

	StackFrameState stackFrameState;
	ProgramAnalyzeContext analyzeContext = {
		runtime,
		fnObject,
		analyzedInfoOut,
		hostRefHolder
	};

	// Analyze lifetime of virtual registers.
	for (size_t &i = analyzeContext.idxCurIns; i < nIns; ++i) {
		const Instruction &curIns = fnObject->instructions[i];

		uint32_t regIndex = UINT32_MAX;

		if (curIns.output.valueType == ValueType::RegRef) {
			regIndex = curIns.output.getRegIndex();

			if (analyzedInfoOut.analyzedRegInfo.count(regIndex)) {
				// Malformed program, return.
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			analyzedInfoOut.analyzedRegInfo[regIndex].lifetime = { i, i };
		}

		for (auto &j : curIns.operands) {
			if (j.valueType == ValueType::RegRef) {
				uint32_t index = j.getRegIndex();

				if (!analyzedInfoOut.analyzedRegInfo.count(index)) {
					// Malformed program, return.
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				analyzedInfoOut.analyzedRegInfo.at(index).lifetime.offEndIns = i;
			}
		}

		switch (curIns.opcode) {
			case Opcode::NOP:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::LOAD: {
				if (regIndex != UINT32_MAX) {
					if (curIns.operands.size() != 1) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[0].valueType != ValueType::ObjectRef) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					IdRefObject *idRef;
					{
						Object *object = curIns.operands[0].getObjectRef();

						if (object->getKind() != ObjectKind::IdRef) {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						}
						idRef = (IdRefObject *)object;
					}

					VarRefContext varRefContext;
					Object *object;
					SLAKE_RETURN_IF_EXCEPT(
						runtime->resolveIdRef(
							idRef,
							&varRefContext,
							object));

					InternalExceptionPointer e = evalObjectType(
						analyzeContext,
						varRefContext,
						object,
						analyzedInfoOut.analyzedRegInfo.at(regIndex).type);
					if (e) {
						if (e->kind != ErrorKind::OptimizerError) {
							return e;
						} else {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						}
					}

					analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = object;
					SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, varRefContext, object, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
				}

				break;
			}
			case Opcode::RLOAD: {
				if (regIndex != UINT32_MAX) {
					if (curIns.operands.size() != 2) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[0].valueType != ValueType::RegRef) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[1].valueType != ValueType::ObjectRef) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					IdRefObject *idRef;
					{
						Object *object = curIns.operands[1].getObjectRef();

						if (object->getKind() != ObjectKind::IdRef) {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						}
						idRef = (IdRefObject *)object;
					}

					uint32_t callTargetRegIndex = curIns.operands[0].getRegIndex();
					Type type = analyzeContext.analyzedInfoOut.analyzedRegInfo[callTargetRegIndex].type;

					switch (type.typeId) {
						case TypeId::Instance: {
							SLAKE_RETURN_IF_EXCEPT(type.loadDeferredType(runtime));

							VarRefContext varRefContext;
							Object *object;

							SLAKE_RETURN_IF_EXCEPT(
								runtime->resolveIdRef(
									idRef,
									&varRefContext,
									object,
									type.getCustomTypeExData()));

							switch (object->getKind()) {
								case ObjectKind::FnOverloading:
									if (((FnOverloadingObject *)object)->access & ACCESS_STATIC) {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
									break;
								case ObjectKind::Var:
									if (((MemberObject *)object)->accessModifier & ACCESS_STATIC) {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
									break;
								default: {
									return MalformedProgramError::alloc(
										runtime,
										fnObject,
										i);
								}
							}

							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = object;
							SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, varRefContext, object, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
							break;
						}
						default: {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						}
					}
				}

				break;
			}
			case Opcode::STORE: {
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				break;
			}
			case Opcode::MOV: {
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				switch (curIns.operands[0].valueType) {
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
						analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
						analyzedInfoOut.analyzedRegInfo.at(regIndex).type = curIns.operands[0].valueType;
						break;
					case ValueType::ObjectRef: {
						analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
						SLAKE_RETURN_IF_EXCEPT(evalObjectType(
							analyzeContext,
							VarRefContext{},
							curIns.operands[0].getObjectRef(),
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
						break;
					}
					case ValueType::RegRef:
						analyzedInfoOut.analyzedRegInfo.at(regIndex) = analyzedInfoOut.analyzedRegInfo.at(curIns.operands[0].getRegIndex());
						break;
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				break;
			}
			case Opcode::LLOAD: {
				if (regIndex != UINT32_MAX) {
					if (curIns.operands.size() != 1) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[0].valueType != ValueType::U32) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					uint32_t index = curIns.operands[0].getU32();

					if (index >= stackFrameState.analyzedLocalVarInfo.size()) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					SLAKE_RETURN_IF_EXCEPT(
						wrapIntoRefType(
							runtime,
							stackFrameState.analyzedLocalVarInfo[index].type,
							hostRefHolder,
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
				}
				break;
			}
			case Opcode::LARG: {
				if (regIndex != UINT32_MAX) {
					if (curIns.operands.size() != 1) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[0].valueType != ValueType::U32) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					uint32_t index = curIns.operands[0].getU32();
					Type type;

					if (fnObject->overloadingFlags & OL_VARG) {
						if (index > fnObject->paramTypes.size()) {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						} else if (index == fnObject->paramTypes.size()) {
							HostObjectRef<TypeDefObject> typeDef = TypeDefObject::alloc(
								runtime,
								type);
							hostRefHolder.addObject(typeDef.get());
							SLAKE_RETURN_IF_EXCEPT(
								wrapIntoRefType(
									runtime,
									Type(TypeId::Ref, typeDef.get()),
									hostRefHolder,
									type));
						} else {
							SLAKE_RETURN_IF_EXCEPT(
								wrapIntoRefType(
									runtime,
									fnObject->paramTypes[i],
									hostRefHolder,
									type));
						}
					} else {
						if (index >= fnObject->paramTypes.size()) {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						} else {
							SLAKE_RETURN_IF_EXCEPT(
								wrapIntoRefType(
									runtime,
									fnObject->paramTypes[i],
									hostRefHolder,
									type));
						}
					}

					analyzedInfoOut.analyzedRegInfo.at(regIndex).type = type;
				}
				break;
			}
			case Opcode::LVAR: {
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands.size() != 1) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Type typeName = curIns.operands[0].getTypeName();

				SLAKE_RETURN_IF_EXCEPT(typeName.loadDeferredType(runtime));

				size_t index = stackFrameState.analyzedLocalVarInfo.size();
				stackFrameState.analyzedLocalVarInfo.push_back({ typeName });
				break;
			}
			case Opcode::REG: {
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands.size() != 1) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands[0].valueType != ValueType::U32) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				break;
			}
			case Opcode::LVALUE: {
				if (regIndex != UINT32_MAX) {
					if (curIns.operands.size() != 1) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[0].valueType != ValueType::RegRef) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					uint32_t index = curIns.operands[0].getRegIndex();
					Type type = analyzedInfoOut.analyzedRegInfo.at(index).type;

					if (type.typeId != TypeId::Ref) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					Type unwrappedType = type.getRefExData();
					SLAKE_RETURN_IF_EXCEPT(unwrappedType.loadDeferredType(runtime));
					analyzedInfoOut.analyzedRegInfo.at(regIndex).type = unwrappedType;
				}
				break;
			}
			case Opcode::ENTER: {
				stackFrameState.stackBases.push_back(stackFrameState.analyzedLocalVarInfo.size());
				break;
			}
			case Opcode::LEAVE: {
				if (stackFrameState.stackBases.empty()) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				stackFrameState.stackBases.pop_back();
				break;
			}
			case Opcode::ADD:
			case Opcode::SUB:
			case Opcode::MUL:
			case Opcode::DIV:
			case Opcode::AND:
			case Opcode::OR:
			case Opcode::XOR:
			case Opcode::LAND:
			case Opcode::LOR:
			case Opcode::EQ:
			case Opcode::NEQ:
			case Opcode::LT:
			case Opcode::GT:
			case Opcode::LTEQ:
			case Opcode::GTEQ:
			case Opcode::LSH:
			case Opcode::RSH:
			case Opcode::NOT:
			case Opcode::LNOT:
			case Opcode::NEG:
				SLAKE_RETURN_IF_EXCEPT(analyzeExprIns(analyzeContext, regIndex));
				break;
			case Opcode::AT: {
				if (regIndex != UINT32_MAX) {
					if (curIns.operands.size() != 2) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					if (curIns.operands[0].valueType != ValueType::RegRef) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					uint32_t index = curIns.operands[0].getRegIndex();
					Type type = analyzedInfoOut.analyzedRegInfo.at(index).type;

					if (type.typeId != TypeId::Array) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}

					Type unwrappedType = type.getArrayExData();
					SLAKE_RETURN_IF_EXCEPT(unwrappedType.loadDeferredType(runtime));
					SLAKE_RETURN_IF_EXCEPT(wrapIntoRefType(
						analyzeContext.runtime,
						unwrappedType,
						analyzeContext.hostRefHolder,
						analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
				}
				break;
			}
			case Opcode::JMP:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::JT:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::JF:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::PUSHARG:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::CALL: {
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				if (curIns.operands.size() != 1) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Value callTarget = curIns.operands[0];
				if (callTarget.valueType != ValueType::RegRef) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				uint32_t callTargetRegIndex = callTarget.getRegIndex();
				if (!analyzedInfoOut.analyzedRegInfo.count(callTargetRegIndex)) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Type callTargetType = analyzedInfoOut.analyzedRegInfo[callTargetRegIndex].type;

				switch (callTargetType.typeId) {
					case TypeId::FnDelegate:
						analyzeContext.lastCallTargetType = callTargetType;
						break;
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				break;
			}
			case Opcode::MCALL:
			case Opcode::CTORCALL: {
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				if (curIns.operands.size() != 2) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Value callTarget = curIns.operands[0];
				if (callTarget.valueType != ValueType::RegRef) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				uint32_t callTargetRegIndex = callTarget.getRegIndex();
				if (!analyzedInfoOut.analyzedRegInfo.count(callTargetRegIndex)) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Type callTargetType = analyzedInfoOut.analyzedRegInfo[callTargetRegIndex].type;

				switch (callTargetType.typeId) {
					case TypeId::FnDelegate:
						analyzeContext.lastCallTargetType = callTargetType;
						break;
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				break;
			}
			case Opcode::RET:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::LRET:
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				switch (analyzeContext.lastCallTargetType.typeId) {
					case TypeId::FnDelegate: {
						Type returnType = ((FnTypeDefObject *)analyzeContext.lastCallTargetType.getCustomTypeExData())->returnType;
						if (returnType.typeId == TypeId::None) {
							return MalformedProgramError::alloc(
								runtime,
								fnObject,
								i);
						}
						analyzedInfoOut.analyzedRegInfo[regIndex].type = ((FnTypeDefObject *)analyzeContext.lastCallTargetType.getCustomTypeExData())->returnType;
						break;
					}
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				break;
			case Opcode::YIELD:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::LTHIS:
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (analyzeContext.fnObject->thisObjectType.typeId == TypeId::None) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				analyzedInfoOut.analyzedRegInfo[regIndex].type = analyzeContext.fnObject->thisObjectType;
				break;
			case Opcode::NEW:
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands.size() != 1) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				analyzedInfoOut.analyzedRegInfo[regIndex].type = curIns.operands[0].getTypeName();
				break;
			case Opcode::ARRNEW: {
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands.size() != 2) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Type lengthType;
				SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], lengthType));
				if (lengthType.typeId != TypeId::Value) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				if (lengthType.getValueTypeExData() != ValueType::U32) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				SLAKE_RETURN_IF_EXCEPT(wrapIntoArrayType(
					runtime,
					curIns.operands[0].getTypeName(),
					hostRefHolder,
					analyzedInfoOut.analyzedRegInfo[regIndex].type));
				break;
			}
			case Opcode::THROW:
			case Opcode::PUSHXH:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				break;
			case Opcode::LEXCEPT:
				// stub
				break;
			case Opcode::CAST: {
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands.size() != 2) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				Value constSrc;
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
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getI8());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getI8());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getI8());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getI8());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getI8());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getI8());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getI8());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getI8());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getI8());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getI8());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getI8());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::I16:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getI16());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getI16());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getI16());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getI16());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getI16());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getI16());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getI16());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getI16());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getI16());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getI16());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getI16());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::I32:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getI32());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getI32());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getI32());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getI32());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getI32());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getI32());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getI32());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getI32());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getI32());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getI32());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getI32());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::I64:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getI64());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getI64());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getI64());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getI64());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getI64());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getI64());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getI64());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getI64());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getI64());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getI64());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getI64());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::U8:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getU8());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getU8());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getU8());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getU8());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getU8());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getU8());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getU8());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getU8());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getU8());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getU8());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getU8());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::U16:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getU16());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getU16());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getU16());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getU16());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getU16());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getU16());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getU16());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getU16());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getU16());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getU16());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getU16());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::U32:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getU32());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getU32());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getU32());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getU32());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getU32());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getU32());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getU32());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getU32());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getU32());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getU32());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getU32());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::U64:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getU64());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getU64());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getU64());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getU64());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getU64());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getU64());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getU64());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getU64());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getU64());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getU64());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getU64());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::Bool:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getBool());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getBool());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getBool());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getBool());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getBool());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getBool());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getBool());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getBool());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getBool());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getBool());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getBool());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::F32:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getF32());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getF32());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getF32());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getF32());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getF32());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getF32());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getF32());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getF32());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getF32());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getF32());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getF32());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
									}
								}
								break;
							case ValueType::F64:
								switch (destType.typeId) {
									case TypeId::Value:
										switch (destType.getValueTypeExData()) {
											case ValueType::I8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int8_t)constSrc.getF64());
												}
												break;
											case ValueType::I16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int16_t)constSrc.getF64());
												}
												break;
											case ValueType::I32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int32_t)constSrc.getF64());
												}
												break;
											case ValueType::I64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((int64_t)constSrc.getF64());
												}
												break;
											case ValueType::U8:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint8_t)constSrc.getF64());
												}
												break;
											case ValueType::U16:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint16_t)constSrc.getF64());
												}
												break;
											case ValueType::U32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint32_t)constSrc.getF64());
												}
												break;
											case ValueType::U64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((uint64_t)constSrc.getF64());
												}
												break;
											case ValueType::F32:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((float)constSrc.getF64());
												}
												break;
											case ValueType::F64:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((double)constSrc.getF64());
												}
												break;
											case ValueType::Bool:
												if (constSrc.valueType != ValueType::Undefined) {
													analyzedInfoOut.analyzedRegInfo[regIndex].expectedValue = Value((bool)constSrc.getF64());
												}
												break;
											default: {
												return MalformedProgramError::alloc(
													runtime,
													fnObject,
													i);
											}
										}
										break;
									default: {
										return MalformedProgramError::alloc(
											runtime,
											fnObject,
											i);
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
							runtime,
							fnObject,
							i);
					}
				}

				analyzedInfoOut.analyzedRegInfo[regIndex].type = destType;
				break;
			}
			default: {
				// Malformed program, return.
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
		}
	}

	return {};
}
