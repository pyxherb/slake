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
	typeOut = Type(TypeId::Array, typeDef.get());

	return {};
}

InternalExceptionPointer slake::opti::evalObjectType(
	ProgramAnalyzeContext &analyzeContext,
	const VarRefContext &varRefContext,
	Object *object,
	Type &typeOut) {
	switch (object->getKind()) {
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
			FnOverloadingObject *fn = (FnOverloadingObject *)object;

			std::pmr::vector<Type> paramTypes(&analyzeContext.runtime->globalHeapPoolResource);
			paramTypes = fn->paramTypes;

			HostObjectRef<FnTypeDefObject> fnTypeDef = FnTypeDefObject::alloc(
				analyzeContext.runtime,
				fn->returnType,
				std::move(paramTypes));

			analyzeContext.hostRefHolder.addObject(fnTypeDef.get());

			typeOut = Type(TypeId::FnDelegate, fnTypeDef.get());
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

			if (regIndex >= analyzeContext.analyzedInfoOut.analyzedRegInfo.size()) {
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

			if (idxReg > analyzeContext.analyzedInfoOut.analyzedRegInfo.size()) {
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

			if (!analyzedInfoOut.analyzedRegInfo.count(regIndex)) {
				// Malformed program, return.
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			analyzedInfoOut.analyzedRegInfo.at(regIndex).lifetime.offEndIns = i;
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

					Type type = analyzeContext.analyzedInfoOut.analyzedRegInfo[curIns.operands[0].getRegIndex()].type;

					switch (type.typeId) {
						case TypeId::Instance: {
							SLAKE_RETURN_IF_EXCEPT(type.loadDeferredType(runtime));

							VarRefContext varRefContext;
							Object *object, *scopeObject = type.getCustomTypeExData();

							switch (scopeObject->getKind()) {
								case ObjectKind::Class: {
									ClassObject *clsObject = (ClassObject *)scopeObject;
									SLAKE_RETURN_IF_EXCEPT(clsObject->parentClass.loadDeferredType(runtime));
									break;
								}
								case ObjectKind::Interface: {
									InterfaceObject *interfaceObject = (InterfaceObject *)scopeObject;
									for (auto &j : interfaceObject->parents) {
										SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(runtime));
									}
									break;
								}
							}

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
						SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[0], analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
						break;
					case ValueType::RegRef:
						analyzedInfoOut.analyzedRegInfo.at(regIndex) = analyzedInfoOut.analyzedRegInfo.at(curIns.operands[0].getRegIndex());
						break;
					case ValueType::ObjectRef: {
						Object *obj = curIns.operands[0].getObjectRef();

						analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
						switch (obj->getKind()) {
							case ObjectKind::String:
								analyzedInfoOut.analyzedRegInfo.at(regIndex).type = Type(TypeId::String);
								break;
							case ObjectKind::Array:
								SLAKE_RETURN_IF_EXCEPT(wrapIntoArrayType(runtime, ((ArrayObject *)obj)->elementType, hostRefHolder, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
								break;
							default:
								return MalformedProgramError::alloc(
									runtime,
									fnObject,
									i);
						}
						break;
					}
					default:
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
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

				uint32_t index = curIns.operands[0].getU32();

				analyzedInfoOut.analyzedRegInfo[index].lifetime = { i, i };

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
					SLAKE_RETURN_IF_EXCEPT(wrapIntoRefType(runtime, unwrappedType, hostRefHolder, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
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
					case TypeId::FnDelegate: {
						FnTypeDefObject *typeDef = (FnTypeDefObject *)callTargetType.getCustomTypeExData();
						analyzeContext.lastCallReturnType = typeDef->returnType;
						break;
					}
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				analyzeContext.lastCallTargetType = callTargetType;
				if (callTarget.valueType != ValueType::Undefined) {
					analyzeContext.lastCallTarget = callTarget;
				}

				break;
			}
			case Opcode::MCALL: {
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
					case TypeId::FnDelegate: {
						FnTypeDefObject *typeDef = (FnTypeDefObject *)callTargetType.getCustomTypeExData();
						analyzeContext.lastCallReturnType = typeDef->returnType;
						break;
					}
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				analyzeContext.lastCallTargetType = callTargetType;
				if (callTarget.valueType != ValueType::Undefined) {
					analyzeContext.lastCallTarget = callTarget;
				}

				break;
			}
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
				// stub
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
				if (curIns.operands.size() != 0) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				if (analyzeContext.lastCallTargetType == TypeId::None) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type = analyzeContext.lastCallReturnType;
				break;
			case Opcode::ACALL:
			case Opcode::AMCALL:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				// stub
				break;
			case Opcode::YIELD:
				if (regIndex != UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}
				// stub
				break;
			case Opcode::AWAIT:
				// stub
				break;
			case Opcode::LTHIS:
				if (regIndex == UINT32_MAX) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				analyzedInfoOut.analyzedRegInfo.at(regIndex).type = fnObject->returnType;
				break;
			case Opcode::NEW: {
				if (regIndex == UINT32_MAX) {
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

				Type type = curIns.operands[0].getTypeName();

				switch (type.typeId) {
					case TypeId::Instance:
						break;
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				analyzedInfoOut.analyzedRegInfo.at(regIndex).type = type;
				break;
			}
			case Opcode::ARRNEW: {
				if (regIndex != UINT32_MAX) {
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

				Type type = curIns.operands[0].getTypeName();

				switch (type.typeId) {
					case TypeId::None: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
					default:;
				}

				Type wrappedType;
				SLAKE_RETURN_IF_EXCEPT(wrapIntoArrayType(runtime, type, hostRefHolder, wrappedType));

				analyzedInfoOut.analyzedRegInfo.at(regIndex).type = wrappedType;
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
			case Opcode::CAST:
				SLAKE_RETURN_IF_EXCEPT(analyzeCastIns(analyzeContext, regIndex));
				break;
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
