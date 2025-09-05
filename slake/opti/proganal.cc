#include "proganal.h"

using namespace slake;
using namespace slake::opti;

void slake::opti::markRegAsForOutput(ProgramAnalyzeContext &analyzeContext, uint32_t i) {
	switch (auto &regInfo = analyzeContext.analyzedInfoOut.analyzedRegInfo.at(i); regInfo.storageType) {
		case opti::RegStorageType::None:
			break;
		case opti::RegStorageType::FieldVar:
			regInfo.storageInfo.asFieldVar.isUsedForOutput = true;
			break;
		case opti::RegStorageType::LocalVar:
			regInfo.storageInfo.asLocalVar.isUsedForOutput = true;
			break;
		case opti::RegStorageType::ArgRef:
			regInfo.storageInfo.asArgRef.isUsedForOutput = true;
			break;
	}
}

InternalExceptionPointer slake::opti::wrapIntoRefType(
	Runtime *runtime,
	Type type,
	HostRefHolder &hostRefHolder,
	TypeRef &typeOut) {
	HostObjectRef<TypeDefObject> typeDef = TypeDefObject::alloc(runtime);

	typeDef->type = type;

	if (!hostRefHolder.addObject(typeDef.get()))
		return OutOfMemoryError::alloc();
	typeOut = Type(TypeId::Ref, typeDef.get());

	return {};
}

InternalExceptionPointer slake::opti::wrapIntoArrayType(
	Runtime *runtime,
	Type type,
	HostRefHolder &hostRefHolder,
	TypeRef &typeOut) {
	HostObjectRef<TypeDefObject> typeDef = TypeDefObject::alloc(
		runtime);

	typeDef->type = type;

	if (!hostRefHolder.addObject(typeDef.get()))
		return OutOfMemoryError::alloc();
	typeOut = Type(TypeId::Array, typeDef.get());

	return {};
}

InternalExceptionPointer slake::opti::evalObjectType(
	ProgramAnalyzeContext &analyzeContext,
	const EntityRef &entityRef,
	TypeRef &typeOut) {
	switch (entityRef.kind) {
		case ObjectRefKind::FieldRef:
		case ObjectRefKind::ArrayElementRef:
		case ObjectRefKind::LocalVarRef:
		case ObjectRefKind::ArgRef:
		case ObjectRefKind::InstanceFieldRef: {
			Type varType;
			SLAKE_RETURN_IF_EXCEPT(analyzeContext.runtime->typeofVar(entityRef, varType));

			SLAKE_RETURN_IF_EXCEPT(
				wrapIntoRefType(
					analyzeContext.runtime,
					varType,
					analyzeContext.hostRefHolder,
					typeOut));
			break;
		}
		case ObjectRefKind::ObjectRef: {
			Object *object = entityRef.asObject.instanceObject;
			switch (object->getObjectKind()) {
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
				case ObjectKind::FnOverloading: {
					FnOverloadingObject *fnOverloadingObject = (FnOverloadingObject *)object;

					auto typeDef = FnTypeDefObject::alloc(
						analyzeContext.runtime);

					typeDef->returnType = fnOverloadingObject->returnType;

					if (!typeDef->paramTypes.resize(fnOverloadingObject->paramTypes.size())) {
						return OutOfMemoryError::alloc();
					}

					for (size_t i = 0; i < fnOverloadingObject->paramTypes.size(); ++i) {
						typeDef->paramTypes.at(i) = TypeId::Void;
					}

					for (size_t i = 0; i < fnOverloadingObject->paramTypes.size(); ++i) {
						typeDef->paramTypes.at(i) = fnOverloadingObject->paramTypes.at(i);
					}

					if (!analyzeContext.hostRefHolder.addObject(typeDef.get()))
						return OutOfMemoryError::alloc();
					typeOut = Type(TypeId::Fn, typeDef.get());
					break;
				}
				default:
					return allocOutOfMemoryErrorIfAllocFailed(
						ErrorEvaluatingObjectTypeError::alloc(
							object->associatedRuntime->getFixedAlloc(),
							object));
			}
			break;
		}
	}

	return {};
}

InternalExceptionPointer slake::opti::evalValueType(
	ProgramAnalyzeContext &analyzeContext,
	const Value &value,
	TypeRef &typeOut) {
	switch (value.valueType) {
		case ValueType::I8:
			typeOut = TypeId::I8;
			break;
		case ValueType::I16:
			typeOut = TypeId::I16;
			break;
		case ValueType::I32:
			typeOut = TypeId::I32;
			break;
		case ValueType::I64:
			typeOut = TypeId::I64;
			break;
		case ValueType::U8:
			typeOut = TypeId::U8;
			break;
		case ValueType::U16:
			typeOut = TypeId::U16;
			break;
		case ValueType::U32:
			typeOut = TypeId::U32;
			break;
		case ValueType::U64:
			typeOut = TypeId::U64;
			break;
		case ValueType::F32:
			typeOut = TypeId::F32;
			break;
		case ValueType::F64:
			typeOut = TypeId::F64;
			break;
		case ValueType::Bool:
			typeOut = TypeId::Bool;
			break;
		case ValueType::EntityRef: {
			const EntityRef &entityRef = value.getEntityRef();

			SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, entityRef, typeOut));
			break;
		}
		case ValueType::RegRef: {
			uint32_t regIndex = value.getRegIndex();

			if (!analyzeContext.analyzedInfoOut.analyzedRegInfo.contains(regIndex)) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			typeOut = analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type;
			break;
		}
		default: {
			return allocOutOfMemoryErrorIfAllocFailed(
				MalformedProgramError::alloc(
					analyzeContext.runtime->getFixedAlloc(),
					analyzeContext.fnObject,
					analyzeContext.idxCurIns));
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

			if (!analyzeContext.analyzedInfoOut.analyzedRegInfo.contains(idxReg)) {
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						analyzeContext.runtime->getFixedAlloc(),
						analyzeContext.fnObject,
						analyzeContext.idxCurIns));
			}

			constValueOut = analyzeContext.analyzedInfoOut.analyzedRegInfo.at(idxReg).expectedValue;
			break;
		} /*
		case ValueType::VarRef: {
			VarRef varRef = value.getVarRef();

			SLAKE_RETURN_IF_EXCEPT(varRef.varPtr->getData(varRef.context, constValueOut));
			break;
		}*/
		default:
			constValueOut = Value(ValueType::Undefined);
	}
	return {};
}

InternalExceptionPointer slake::opti::analyzeProgramInfo(
	Runtime *runtime,
	peff::Alloc *resourceAllocator,
	RegularFnOverloadingObject *fnObject,
	ProgramAnalyzedInfo &analyzedInfoOut,
	HostRefHolder &hostRefHolder) {
	if (fnObject->instructions.size() > UINT32_MAX) {
		// TODO: Deal with this case elegently.
		std::terminate();
	}
	uint32_t nIns = (uint32_t)fnObject->instructions.size();
	analyzedInfoOut.contextObject = ContextObject::alloc(runtime);
	MajorFramePtr pseudoMajorFrame;
	{
		SLAKE_RETURN_IF_EXCEPT(runtime->_createNewMajorFrame(&analyzedInfoOut.contextObject->_context, nullptr, nullptr, nullptr, 0, UINT32_MAX));
		pseudoMajorFrame = MajorFramePtr(MajorFrame::alloc(runtime, &analyzedInfoOut.contextObject->_context));
	}

	ProgramAnalyzeContext analyzeContext = {
		runtime,
		resourceAllocator,
		fnObject,
		analyzedInfoOut,
		hostRefHolder
	};

	if (!(pseudoMajorFrame->resumable = ResumableObject::alloc(runtime)))
		return OutOfMemoryError::alloc();

	pseudoMajorFrame->resumable->argStack.resizeWith(fnObject->paramTypes.size(), ArgRecord{});
	for (size_t i = 0; i < fnObject->paramTypes.size(); ++i) {
		pseudoMajorFrame->resumable->argStack.at(i) = { Value(), fnObject->paramTypes.at(i) };
	}

	if (fnObject->overloadingFlags & OL_VARG) {
		auto varArgTypeDefObject = TypeDefObject::alloc(runtime);

		varArgTypeDefObject->type = Type(TypeId::Any);

		if(!hostRefHolder.addObject(varArgTypeDefObject.get()))
			return OutOfMemoryError::alloc();

		if(!pseudoMajorFrame->resumable->argStack.pushBack({ Value(), Type(TypeId::Array, varArgTypeDefObject.get()) }))
			return OutOfMemoryError::alloc();
	}

	// Analyze lifetime of virtual registers.
	for (uint32_t &i = analyzeContext.idxCurIns; i < nIns; ++i) {
		const Instruction &curIns = fnObject->instructions.at(i);

		uint32_t regIndex = UINT32_MAX;

		if (curIns.output != UINT32_MAX) {
			regIndex = curIns.output;

			if (analyzedInfoOut.analyzedRegInfo.contains(regIndex)) {
				// Malformed program, return.
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						runtime->getFixedAlloc(),
						fnObject,
						i));
			}

			if (!analyzedInfoOut.analyzedRegInfo.insert(+regIndex, {}))
				return OutOfMemoryError::alloc();
			analyzedInfoOut.analyzedRegInfo.at(regIndex).lifetime = { i, i };
		}

		for (size_t j = 0; j < curIns.nOperands; ++j) {
			if (curIns.operands[j].valueType == ValueType::RegRef) {
				uint32_t index = curIns.operands[j].getRegIndex();

				if (!analyzedInfoOut.analyzedRegInfo.contains(index)) {
					// Malformed program, return.
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				analyzedInfoOut.analyzedRegInfo.at(index).lifetime.offEndIns = i;
			}
		}

		switch (curIns.opcode) {
			case Opcode::NOP:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}
				break;
			case Opcode::LOAD: {
				if (regIndex != UINT32_MAX) {
					if (curIns.nOperands != 1) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					if (curIns.operands[0].valueType != ValueType::EntityRef) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					IdRefObject *idRef;
					{
						EntityRef object = curIns.operands[0].getEntityRef();

						if ((object.kind != ObjectRefKind::ObjectRef) ||
							(object.asObject.instanceObject->getObjectKind() != ObjectKind::IdRef)) {
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						}
						idRef = (IdRefObject *)object.asObject.instanceObject;
					}

					EntityRef entityRef;
					SLAKE_RETURN_IF_EXCEPT(
						runtime->resolveIdRef(
							idRef,
							entityRef));

					InternalExceptionPointer e = evalObjectType(
						analyzeContext,
						entityRef,
						analyzedInfoOut.analyzedRegInfo.at(regIndex).type);
					if (e) {
						if (e->kind != ErrorKind::OptimizerError) {
							return e;
						} else {
							e.reset();
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						}
					}

					switch (entityRef.kind) {
						case ObjectRefKind::FieldRef:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::FieldVar;
							break;
						case ObjectRefKind::InstanceFieldRef:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::InstanceFieldVar;
							break;
						case ObjectRefKind::ObjectRef: {
							break;
						}
						default:
							std::terminate();
					}

					analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = entityRef;
					SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, entityRef, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
				}

				break;
			}
			case Opcode::RLOAD: {
				if (regIndex != UINT32_MAX) {
					if (curIns.nOperands != 2) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					if (curIns.operands[0].valueType != ValueType::RegRef) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					if (curIns.operands[1].valueType != ValueType::EntityRef) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					IdRefObject *idRef;
					{
						const EntityRef &object = curIns.operands[1].getEntityRef();

						if ((object.kind != ObjectRefKind::ObjectRef) ||
							(object.asObject.instanceObject->getObjectKind() != ObjectKind::IdRef)) {
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						}
						idRef = (IdRefObject *)object.asObject.instanceObject;
					}

					uint32_t callTargetRegIndex = curIns.operands[0].getRegIndex();
					Type type = analyzeContext.analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).type;

					switch (type.typeId) {
						case TypeId::Instance: {
							SLAKE_RETURN_IF_EXCEPT(type.loadDeferredType(runtime));

							EntityRef entityRef;

							SLAKE_RETURN_IF_EXCEPT(
								runtime->resolveIdRef(
									idRef,
									entityRef,
									(MemberObject *)type.getCustomTypeExData()));

							switch (entityRef.kind) {
								case ObjectRefKind::ObjectRef: {
									Object *object = entityRef.asObject.instanceObject;
									switch (object->getObjectKind()) {
										case ObjectKind::FnOverloading:
											if (((FnOverloadingObject *)object)->access & ACCESS_STATIC) {
												return allocOutOfMemoryErrorIfAllocFailed(
													MalformedProgramError::alloc(
														runtime->getFixedAlloc(),
														fnObject,
														i));
											}
											break;
										default: {
											return allocOutOfMemoryErrorIfAllocFailed(
												MalformedProgramError::alloc(
													runtime->getFixedAlloc(),
													fnObject,
													i));
										}
									}
								}
								case ObjectRefKind::InstanceFieldRef:
									analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::InstanceFieldVar;
									break;
								case ObjectRefKind::FieldRef:
									analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::FieldVar;
									break;
								default: {
									return allocOutOfMemoryErrorIfAllocFailed(
										MalformedProgramError::alloc(
											runtime->getFixedAlloc(),
											fnObject,
											i));
								}
							}

							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = entityRef;
							SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, entityRef, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
							break;
						}
						default: {
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						}
					}
				}

				break;
			}
			case Opcode::STORE: {
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				break;
			}
			case Opcode::MOV: {
				if (regIndex != UINT32_MAX) {
					switch (curIns.operands[0].valueType) {
						case ValueType::I8:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::I8;
							break;
						case ValueType::I16:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::I16;
							break;
						case ValueType::I32:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::I32;
							break;
						case ValueType::I64:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::I64;
							break;
						case ValueType::U8:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::U8;
							break;
						case ValueType::U16:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::U16;
							break;
						case ValueType::U32:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::U32;
							break;
						case ValueType::U64:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::U64;
							break;
						case ValueType::F32:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::F32;
							break;
						case ValueType::F64:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::F64;
							break;
						case ValueType::Bool:
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = TypeId::Bool;
							break;
						case ValueType::EntityRef: {
							analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = curIns.operands[0];
							SLAKE_RETURN_IF_EXCEPT(evalObjectType(
								analyzeContext,
								curIns.operands[0].getEntityRef(),
								analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
							break;
						}
						case ValueType::RegRef:
							analyzedInfoOut.analyzedRegInfo.at(regIndex) = analyzedInfoOut.analyzedRegInfo.at(curIns.operands[0].getRegIndex());
							break;
						default: {
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						}
					}
				}

				break;
			}
			case Opcode::LARG: {
				if (regIndex != UINT32_MAX) {
					if (curIns.nOperands != 1) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					if (curIns.operands[0].valueType != ValueType::U32) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					uint32_t index = curIns.operands[0].getU32();
					Type type;

					if (fnObject->overloadingFlags & OL_VARG) {
						if (index > fnObject->paramTypes.size()) {
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						} else if (index == fnObject->paramTypes.size()) {
							HostObjectRef<TypeDefObject> typeDef = TypeDefObject::alloc(
								runtime);

							typeDef->type = TypeId::Any;

							if(!hostRefHolder.addObject(typeDef.get()))
								return OutOfMemoryError::alloc();
							SLAKE_RETURN_IF_EXCEPT(
								wrapIntoRefType(
									runtime,
									Type(TypeId::Array, typeDef.get()),
									hostRefHolder,
									type));
						} else {
							SLAKE_RETURN_IF_EXCEPT(
								wrapIntoRefType(
									runtime,
									fnObject->paramTypes.at(index),
									hostRefHolder,
									type));
						}
					} else {
						if (index >= fnObject->paramTypes.size()) {
							return allocOutOfMemoryErrorIfAllocFailed(
								MalformedProgramError::alloc(
									runtime->getFixedAlloc(),
									fnObject,
									i));
						} else {
							SLAKE_RETURN_IF_EXCEPT(
								wrapIntoRefType(
									runtime,
									fnObject->paramTypes.at(index),
									hostRefHolder,
									type));
						}
					}

					analyzedInfoOut.analyzedRegInfo.at(regIndex).type = type;

					analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::ArgRef;
					analyzedInfoOut.analyzedRegInfo.at(regIndex).storageInfo.asArgRef = {};
					analyzedInfoOut.analyzedRegInfo.at(regIndex).storageInfo.asArgRef.idxArg = index;
					analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value(EntityRef::makeArgRef(pseudoMajorFrame.get(), index));
				}
				break;
			}
			case Opcode::LVAR: {
				if (regIndex == UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (curIns.nOperands != 1) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				Type typeName = curIns.operands[0].getTypeName();

				SLAKE_RETURN_IF_EXCEPT(typeName.loadDeferredType(runtime));

				EntityRef entityRef;
				SLAKE_RETURN_IF_EXCEPT(runtime->_addLocalVar(&analyzedInfoOut.contextObject->_context, pseudoMajorFrame.get(), typeName, entityRef));

				SLAKE_RETURN_IF_EXCEPT(
					wrapIntoRefType(
						runtime,
						typeName,
						hostRefHolder,
						analyzedInfoOut.analyzedRegInfo.at(regIndex).type));

				analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::LocalVar;
				analyzedInfoOut.analyzedRegInfo.at(regIndex).storageInfo.asLocalVar.definitionReg = curIns.output;
				analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value(entityRef);
				break;
			}
			case Opcode::LVALUE: {
				if (regIndex != UINT32_MAX) {
					if (curIns.nOperands != 1) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					if (curIns.operands[0].valueType != ValueType::RegRef) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					uint32_t index = curIns.operands[0].getRegIndex();
					Type type = analyzedInfoOut.analyzedRegInfo.at(index).type;

					if (type.typeId != TypeId::Ref) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					Type unwrappedType = type.getRefExData();
					SLAKE_RETURN_IF_EXCEPT(unwrappedType.loadDeferredType(runtime));
					analyzedInfoOut.analyzedRegInfo.at(regIndex).type = unwrappedType;
				}
				break;
			}
			case Opcode::ENTER: {
				break;
			}
			case Opcode::LEAVE: {
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
			case Opcode::CMP:
			case Opcode::NOT:
			case Opcode::LNOT:
			case Opcode::NEG:
				SLAKE_RETURN_IF_EXCEPT(analyzeArithmeticIns(analyzeContext, regIndex));
				break;
			case Opcode::AT: {
				if (regIndex != UINT32_MAX) {
					if (curIns.nOperands != 2) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					if (curIns.operands[0].valueType != ValueType::RegRef) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					uint32_t index = curIns.operands[0].getRegIndex();
					Type type = analyzedInfoOut.analyzedRegInfo.at(index).type;

					if (type.typeId != TypeId::Array) {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}

					Type unwrappedType = type.getArrayExData();
					SLAKE_RETURN_IF_EXCEPT(unwrappedType.loadDeferredType(runtime));
					SLAKE_RETURN_IF_EXCEPT(wrapIntoRefType(
						analyzeContext.runtime,
						unwrappedType,
						analyzeContext.hostRefHolder,
						analyzedInfoOut.analyzedRegInfo.at(regIndex).type));

					analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::ArrayElement;
				}
				break;
			}
			case Opcode::JMP:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(+regIndex))
					return OutOfMemoryError::alloc();
				break;
			case Opcode::JT:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(+regIndex))
					return OutOfMemoryError::alloc();
				break;
			case Opcode::JF:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(+regIndex))
					return OutOfMemoryError::alloc();
				break;
			case Opcode::PUSHARG: {
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}
				if (curIns.nOperands != 1) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
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
					case ValueType::Bool:
					case ValueType::F32:
					case ValueType::F64:
					case ValueType::EntityRef:
						break;
					case ValueType::RegRef:
						markRegAsForOutput(analyzeContext, curIns.operands[0].getRegIndex());
						break;
					default:
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
				}
				if (!analyzeContext.argPushInsOffs.pushBack(+i))
					return OutOfMemoryError::alloc();
				break;
			}
			case Opcode::CALL: {
				if (curIns.nOperands != 1) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				Value callTarget = curIns.operands[0];
				if (callTarget.valueType != ValueType::RegRef) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				uint32_t callTargetRegIndex = callTarget.getRegIndex();
				if (!analyzedInfoOut.analyzedRegInfo.contains(callTargetRegIndex)) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				Type callTargetType = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).type;

				switch (callTargetType.typeId) {
					case TypeId::Fn:
						if (regIndex != UINT32_MAX) {
							FnTypeDefObject *typeDef = (FnTypeDefObject *)callTargetType.getCustomTypeExData();
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = typeDef->returnType;
						}
						break;
					default: {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}
				}

				analyzeContext.analyzedInfoOut.analyzedFnCallInfo.insert(+i, FnCallAnalyzedInfo(analyzeContext.resourceAllocator.get()));
				analyzeContext.analyzedInfoOut.analyzedFnCallInfo.at(i).argPushInsOffs = std::move(analyzeContext.argPushInsOffs);
				analyzeContext.argPushInsOffs = peff::DynArray<uint32_t>(analyzeContext.resourceAllocator.get());

				Value expectedFnValue = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).expectedValue;

				if (expectedFnValue.valueType != ValueType::Undefined) {
					FnOverloadingObject *expectedFnObject = (FnOverloadingObject *)expectedFnValue.getEntityRef().asObject.instanceObject;
					if (!analyzeContext.analyzedInfoOut.fnCallMap.contains(expectedFnObject)) {
						analyzeContext.analyzedInfoOut.fnCallMap.insert(+expectedFnObject, peff::DynArray<uint32_t>(analyzeContext.runtime->getFixedAlloc()));
					}
					if (!analyzeContext.analyzedInfoOut.fnCallMap.at(expectedFnObject).pushBack(+i))
						return OutOfMemoryError::alloc();
				}

				break;
			}
			case Opcode::MCALL:
			case Opcode::CTORCALL: {
				if (curIns.nOperands != 2) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				Value callTarget = curIns.operands[0];
				if (callTarget.valueType != ValueType::RegRef) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				uint32_t callTargetRegIndex = callTarget.getRegIndex();
				if (!analyzedInfoOut.analyzedRegInfo.contains(callTargetRegIndex)) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				Type callTargetType = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).type;

				switch (callTargetType.typeId) {
					case TypeId::Fn:
						if (regIndex != UINT32_MAX) {
							FnTypeDefObject *typeDef = (FnTypeDefObject *)callTargetType.getCustomTypeExData();
							analyzedInfoOut.analyzedRegInfo.at(regIndex).type = typeDef->returnType;
						}
						break;
					default: {
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
					}
				}
				analyzeContext.analyzedInfoOut.analyzedFnCallInfo.insert(+i, FnCallAnalyzedInfo(analyzeContext.resourceAllocator.get()));
				analyzeContext.analyzedInfoOut.analyzedFnCallInfo.at(i).argPushInsOffs = std::move(analyzeContext.argPushInsOffs);
				analyzeContext.argPushInsOffs = peff::DynArray<uint32_t>(analyzeContext.resourceAllocator.get());

				Value expectedFnValue = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).expectedValue;

				if (expectedFnValue.valueType != ValueType::Undefined) {
					FnOverloadingObject *expectedFnObject = (FnOverloadingObject *)expectedFnValue.getEntityRef().asObject.instanceObject;
					if (!analyzeContext.analyzedInfoOut.fnCallMap.contains(expectedFnObject)) {
						analyzeContext.analyzedInfoOut.fnCallMap.insert(+expectedFnObject, peff::DynArray<uint32_t>(analyzeContext.resourceAllocator.get()));
					}
					if (!analyzeContext.analyzedInfoOut.fnCallMap.at(expectedFnObject).pushBack(+i))
						return OutOfMemoryError::alloc();
				}

				break;
			}
			case Opcode::RET:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}
				if (curIns.nOperands != 1) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
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
					case ValueType::Bool:
					case ValueType::F32:
					case ValueType::F64:
					case ValueType::EntityRef:
						break;
					case ValueType::RegRef:
						markRegAsForOutput(analyzeContext, curIns.operands[0].getRegIndex());
						break;
					default:
						return allocOutOfMemoryErrorIfAllocFailed(
							MalformedProgramError::alloc(
								runtime->getFixedAlloc(),
								fnObject,
								i));
				}
				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				break;
			case Opcode::YIELD:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}
				if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
					return OutOfMemoryError::alloc();
				break;
			case Opcode::LTHIS:
				if (regIndex == UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (analyzeContext.fnObject->thisType.typeId == TypeId::Void) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}
				analyzedInfoOut.analyzedRegInfo.at(regIndex).type = analyzeContext.fnObject->thisType;
				break;
			case Opcode::NEW:
				if (regIndex == UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (curIns.nOperands != 1) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				analyzedInfoOut.analyzedRegInfo.at(regIndex).type = curIns.operands[0].getTypeName();
				break;
			case Opcode::ARRNEW: {
				if (regIndex == UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (curIns.nOperands != 2) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				if (curIns.operands[0].valueType != ValueType::TypeName) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				Type lengthType;
				SLAKE_RETURN_IF_EXCEPT(evalValueType(analyzeContext, curIns.operands[1], lengthType));
				if (!isValueTypeCompatibleTypeId(lengthType.typeId)) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				int cmpResult;

				SLAKE_RETURN_IF_EXCEPT(runtime->compareTypes(analyzeContext.resourceAllocator.get(), lengthType, TypeId::U32, cmpResult));

				if (cmpResult) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}

				SLAKE_RETURN_IF_EXCEPT(wrapIntoArrayType(
					runtime,
					curIns.operands[0].getTypeName(),
					hostRefHolder,
					analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
				break;
			}
			case Opcode::THROW:
			case Opcode::PUSHXH:
				if (regIndex != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(
						MalformedProgramError::alloc(
							runtime->getFixedAlloc(),
							fnObject,
							i));
				}
				break;
			case Opcode::LEXCEPT:
				// stub
				break;
			case Opcode::CAST: {
				SLAKE_RETURN_IF_EXCEPT(analyzeCastIns(analyzeContext, regIndex));
				break;
			}
			default: {
				// Malformed program, return.
				return allocOutOfMemoryErrorIfAllocFailed(
					MalformedProgramError::alloc(
						runtime->getFixedAlloc(),
						fnObject,
						i));
			}
		}
	}

	// A well-formed program should not have unused argument pushing instructions.
	if (analyzeContext.argPushInsOffs.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(
			MalformedProgramError::alloc(
				runtime->getFixedAlloc(),
				fnObject,
				nIns - 1));
	}

	return {};
}
