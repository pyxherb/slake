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
	const EntityRef &entityRef,
	Type &typeOut) {
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
		case ObjectKind::FnOverloading: {
			FnOverloadingObject *fnOverloadingObject = (FnOverloadingObject *)object;

			peff::DynArray<Type> paramTypes;
			if (!(peff::copy(paramTypes, fnOverloadingObject->paramTypes))) {
				return OutOfMemoryError::alloc();
			}
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
		break;
	}
	}

	return {};
}

InternalExceptionPointer slake::opti::evalValueType(
	ProgramAnalyzeContext &analyzeContext,
	const Value &value,
	Type &typeOut) {
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
			return MalformedProgramError::alloc(
				analyzeContext.runtime,
				analyzeContext.fnObject,
				analyzeContext.idxCurIns);
		}

		typeOut = analyzeContext.analyzedInfoOut.analyzedRegInfo.at(regIndex).type;
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

		if (!analyzeContext.analyzedInfoOut.analyzedRegInfo.contains(idxReg)) {
			return MalformedProgramError::alloc(
				analyzeContext.runtime,
				analyzeContext.fnObject,
				analyzeContext.idxCurIns);
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
	RegularFnOverloadingObject *fnObject,
	ProgramAnalyzedInfo &analyzedInfoOut,
	HostRefHolder &hostRefHolder) {
	size_t nIns = fnObject->instructions.size();
	analyzedInfoOut.contextObject = ContextObject::alloc(runtime);
	MajorFrame *pseudoMajorFrame;
	{
		SLAKE_RETURN_IF_EXCEPT(runtime->_createNewMajorFrame(&analyzedInfoOut.contextObject->_context, nullptr, nullptr, nullptr, 0, UINT32_MAX));
		pseudoMajorFrame = analyzedInfoOut.contextObject->_context.majorFrameList;
	}

	ProgramAnalyzeContext analyzeContext = {
		runtime,
		fnObject,
		analyzedInfoOut,
		hostRefHolder
	};

	pseudoMajorFrame->argStack.resizeWith(fnObject->paramTypes.size(), ArgRecord{});
	for (size_t i = 0; i < fnObject->paramTypes.size(); ++i) {
		pseudoMajorFrame->argStack.at(i) = { Value(), fnObject->paramTypes.at(i) };
	}

	if (fnObject->overloadingFlags & OL_VARG) {
		auto varArgTypeDefObject = TypeDefObject::alloc(runtime, Type(TypeId::Any));
		hostRefHolder.addObject(varArgTypeDefObject.get());

		pseudoMajorFrame->argStack.pushBack({ Value(), Type(TypeId::Array, varArgTypeDefObject.get()) });
	}

	// Analyze lifetime of virtual registers.
	for (size_t &i = analyzeContext.idxCurIns; i < nIns; ++i) {
		const Instruction &curIns = fnObject->instructions.at(i);

		uint32_t regIndex = UINT32_MAX;

		if (curIns.output.valueType == ValueType::RegRef) {
			regIndex = curIns.output.getRegIndex();

			if (analyzedInfoOut.analyzedRegInfo.contains(regIndex)) {
				// Malformed program, return.
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			if (!analyzedInfoOut.analyzedRegInfo.insert(+regIndex, {}))
				return OutOfMemoryError::alloc();
			analyzedInfoOut.analyzedRegInfo.at(regIndex).lifetime = { i, i };
		}

		for (auto &j : curIns.operands) {
			if (j.valueType == ValueType::RegRef) {
				uint32_t index = j.getRegIndex();

				if (!analyzedInfoOut.analyzedRegInfo.contains(index)) {
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
				if (curIns.nOperands != 1) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				if (curIns.operands[0].valueType != ValueType::EntityRef) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				IdRefObject *idRef;
				{
					EntityRef object = curIns.operands[0].getEntityRef();

					if ((object.kind != ObjectRefKind::ObjectRef) ||
						(object.asObject.instanceObject->getKind() != ObjectKind::IdRef)) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
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
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
				}

				switch (entityRef.kind) {
				case ObjectRefKind::FieldRef:
				case ObjectRefKind::ArrayElementRef:
				case ObjectRefKind::LocalVarRef:
				case ObjectRefKind::ArgRef:
				case ObjectRefKind::InstanceFieldRef: {
				}
				case ObjectRefKind::ObjectRef: {
					analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::FieldVar;
				}
				}

				analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = entityRef;
				SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, entityRef, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
			}

			break;
		}
		case Opcode::RLOAD: {
			if (regIndex != UINT32_MAX) {
				if (curIns.nOperands != 2) {
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

				if (curIns.operands[1].valueType != ValueType::EntityRef) {
					return MalformedProgramError::alloc(
						runtime,
						fnObject,
						i);
				}

				IdRefObject *idRef;
				{
					const EntityRef &object = curIns.operands[1].getEntityRef();

					if ((object.kind != ObjectRefKind::ObjectRef) ||
						(object.asObject.instanceObject->getKind() != ObjectKind::IdRef)) {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
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
							type.getCustomTypeExData()));

					switch (entityRef.kind) {
					case ObjectRefKind::ObjectRef: {
						Object *object = entityRef.asObject.instanceObject;
						switch (object->getKind()) {
						case ObjectKind::FnOverloading:
							if (((FnOverloadingObject *)object)->access & ACCESS_STATIC) {
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
					}
					case ObjectRefKind::InstanceFieldRef:
					case ObjectRefKind::FieldRef:
						break;
					default: {
						return MalformedProgramError::alloc(
							runtime,
							fnObject,
							i);
					}
					}

					analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = entityRef;
					SLAKE_RETURN_IF_EXCEPT(evalObjectType(analyzeContext, entityRef, analyzedInfoOut.analyzedRegInfo.at(regIndex).type));
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
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			}

			break;
		}
		case Opcode::LARG: {
			if (regIndex != UINT32_MAX) {
				if (curIns.nOperands != 1) {
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
								fnObject->paramTypes.at(index),
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
								fnObject->paramTypes.at(index),
								hostRefHolder,
								type));
					}
				}

				analyzedInfoOut.analyzedRegInfo.at(regIndex).type = type;

				analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::ArgRef;
				analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value(EntityRef::makeArgRef(pseudoMajorFrame, index));
			}
			break;
		}
		case Opcode::LVAR: {
			if (regIndex == UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			if (curIns.nOperands != 1) {
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

			EntityRef entityRef;
			SLAKE_RETURN_IF_EXCEPT(runtime->_addLocalVar(pseudoMajorFrame, typeName, entityRef));

			SLAKE_RETURN_IF_EXCEPT(
				wrapIntoRefType(
					runtime,
					typeName,
					hostRefHolder,
					analyzedInfoOut.analyzedRegInfo.at(regIndex).type));

			analyzedInfoOut.analyzedRegInfo.at(regIndex).storageType = RegStorageType::LocalVar;
			analyzedInfoOut.analyzedRegInfo.at(regIndex).storageInfo.asLocalVar.definitionReg = curIns.output.getRegIndex();
			analyzedInfoOut.analyzedRegInfo.at(regIndex).expectedValue = Value(entityRef);
			break;
		}
		case Opcode::LVALUE: {
			if (regIndex != UINT32_MAX) {
				if (curIns.nOperands != 1) {
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

			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
				return OutOfMemoryError::alloc();
			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(+regIndex))
				return OutOfMemoryError::alloc();
			break;
		case Opcode::JT:
			if (regIndex != UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
				return OutOfMemoryError::alloc();
			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(+regIndex))
				return OutOfMemoryError::alloc();
			break;
		case Opcode::JF:
			if (regIndex != UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
				return OutOfMemoryError::alloc();
			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(+regIndex))
				return OutOfMemoryError::alloc();
			break;
		case Opcode::PUSHARG: {
			if (regIndex != UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			if (curIns.nOperands != 1) {
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
			case ValueType::Bool:
			case ValueType::F32:
			case ValueType::F64:
			case ValueType::EntityRef:
				break;
			case ValueType::RegRef:
				markRegAsForOutput(analyzeContext, curIns.operands[0].getRegIndex());
				break;
			default:
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			if (!analyzeContext.argPushInsOffs.pushBack(i))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::CALL: {
			if (curIns.nOperands != 1) {
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
			if (!analyzedInfoOut.analyzedRegInfo.contains(callTargetRegIndex)) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			Type callTargetType = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).type;

			switch (callTargetType.typeId) {
			case TypeId::FnDelegate:
				if (regIndex != UINT32_MAX) {
					FnTypeDefObject *typeDef = (FnTypeDefObject *)callTargetType.getCustomTypeExData();
					analyzedInfoOut.analyzedRegInfo.at(regIndex).type = typeDef->returnType;
				}
				break;
			default: {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			}

			analyzeContext.analyzedInfoOut.analyzedFnCallInfo.insert(i, FnCallAnalyzedInfo(&analyzeContext.runtime->globalHeapPoolAlloc));
			analyzeContext.analyzedInfoOut.analyzedFnCallInfo.at(i).argPushInsOffs = std::move(analyzeContext.argPushInsOffs);
			analyzeContext.argPushInsOffs = peff::DynArray<uint32_t>(&analyzeContext.runtime->globalHeapPoolAlloc);

			Value expectedFnValue = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).expectedValue;

			if (expectedFnValue.valueType != ValueType::Undefined) {
				FnOverloadingObject *expectedFnObject = (FnOverloadingObject *)expectedFnValue.getEntityRef().asObject.instanceObject;
				if (!analyzeContext.analyzedInfoOut.fnCallMap.contains(expectedFnObject)) {
					analyzeContext.analyzedInfoOut.fnCallMap.insert(+expectedFnObject, peff::DynArray<uint32_t>(&analyzeContext.runtime->globalHeapPoolAlloc));
				}
				if (!analyzeContext.analyzedInfoOut.fnCallMap.at(expectedFnObject).pushBack(i))
					return OutOfMemoryError::alloc();
			}

			break;
		}
		case Opcode::MCALL:
		case Opcode::CTORCALL: {
			if (curIns.nOperands != 2) {
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
			if (!analyzedInfoOut.analyzedRegInfo.contains(callTargetRegIndex)) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			Type callTargetType = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).type;

			switch (callTargetType.typeId) {
			case TypeId::FnDelegate:
				if (regIndex != UINT32_MAX) {
					FnTypeDefObject *typeDef = (FnTypeDefObject *)callTargetType.getCustomTypeExData();
					analyzedInfoOut.analyzedRegInfo.at(regIndex).type = typeDef->returnType;
				}
				break;
			default: {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			}
			analyzeContext.analyzedInfoOut.analyzedFnCallInfo.insert(i, FnCallAnalyzedInfo(&analyzeContext.runtime->globalHeapPoolAlloc));
			analyzeContext.analyzedInfoOut.analyzedFnCallInfo.at(i).argPushInsOffs = std::move(analyzeContext.argPushInsOffs);
			analyzeContext.argPushInsOffs = peff::DynArray<uint32_t>(&analyzeContext.runtime->globalHeapPoolAlloc);

			Value expectedFnValue = analyzedInfoOut.analyzedRegInfo.at(callTargetRegIndex).expectedValue;

			if (expectedFnValue.valueType != ValueType::Undefined) {
				FnOverloadingObject *expectedFnObject = (FnOverloadingObject *)expectedFnValue.getEntityRef().asObject.instanceObject;
				if (!analyzeContext.analyzedInfoOut.fnCallMap.contains(expectedFnObject)) {
					analyzeContext.analyzedInfoOut.fnCallMap.insert(+expectedFnObject, peff::DynArray<uint32_t>(&analyzeContext.runtime->globalHeapPoolAlloc));
				}
				if (!analyzeContext.analyzedInfoOut.fnCallMap.at(expectedFnObject).pushBack(i))
					return OutOfMemoryError::alloc();
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
			if (curIns.nOperands != 1) {
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
			case ValueType::Bool:
			case ValueType::F32:
			case ValueType::F64:
			case ValueType::EntityRef:
				break;
			case ValueType::RegRef:
				markRegAsForOutput(analyzeContext, curIns.operands[0].getRegIndex());
				break;
			default:
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
				return OutOfMemoryError::alloc();
			break;
		case Opcode::YIELD:
			if (regIndex != UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			if (!analyzeContext.analyzedInfoOut.codeBlockBoundaries.insert(i + 1))
				return OutOfMemoryError::alloc();
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
			analyzedInfoOut.analyzedRegInfo.at(regIndex).type = analyzeContext.fnObject->thisObjectType;
			break;
		case Opcode::NEW:
			if (regIndex == UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			if (curIns.nOperands != 1) {
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

			analyzedInfoOut.analyzedRegInfo.at(regIndex).type = curIns.operands[0].getTypeName();
			break;
		case Opcode::ARRNEW: {
			if (regIndex == UINT32_MAX) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}

			if (curIns.nOperands != 2) {
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
			if (!isValueTypeCompatibleTypeId(lengthType.typeId)) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
			}
			if (lengthType != TypeId::U32) {
				return MalformedProgramError::alloc(
					runtime,
					fnObject,
					i);
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
			SLAKE_RETURN_IF_EXCEPT(analyzeCastIns(analyzeContext, regIndex));
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

	// A well-formed program should not have unused argument pushing instructions.
	if (analyzeContext.argPushInsOffs.size()) {
		return MalformedProgramError::alloc(
			runtime,
			fnObject,
			nIns - 1);
	}

	return {};
}
