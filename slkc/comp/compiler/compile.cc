#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileTypeName(
	CompileContext *compileContext,
	peff::SharedPtr<TypeNameNode> typeName,
	slake::Type &typeOut) {
	switch (typeName->typeNameKind) {
		case TypeNameKind::Void:
			typeOut = slake::Type(slake::TypeId::None);
			break;
		case TypeNameKind::I8:
			typeOut = slake::Type(slake::TypeId::I8);
			break;
		case TypeNameKind::I16:
			typeOut = slake::Type(slake::TypeId::I16);
			break;
		case TypeNameKind::I32:
			typeOut = slake::Type(slake::TypeId::I32);
			break;
		case TypeNameKind::I64:
			typeOut = slake::Type(slake::TypeId::I64);
			break;
		case TypeNameKind::U8:
			typeOut = slake::Type(slake::TypeId::U8);
			break;
		case TypeNameKind::U16:
			typeOut = slake::Type(slake::TypeId::U16);
			break;
		case TypeNameKind::U32:
			typeOut = slake::Type(slake::TypeId::U32);
			break;
		case TypeNameKind::U64:
			typeOut = slake::Type(slake::TypeId::U64);
			break;
		case TypeNameKind::F32:
			typeOut = slake::Type(slake::TypeId::F32);
			break;
		case TypeNameKind::F64:
			typeOut = slake::Type(slake::TypeId::F64);
			break;
		case TypeNameKind::String:
			typeOut = slake::Type(slake::TypeId::String);
			break;
		case TypeNameKind::Bool:
			typeOut = slake::Type(slake::TypeId::Bool);
			break;
		case TypeNameKind::Object:
			typeOut = slake::Type(slake::TypeId::Instance, nullptr);
			break;
		case TypeNameKind::Any:
			typeOut = slake::Type(slake::TypeId::Any);
			break;
		case TypeNameKind::Custom: {
			peff::SharedPtr<CustomTypeNameNode> t = typeName.castTo<CustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();
			peff::SharedPtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(doc, t, m));

			if (!m) {
				return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			switch (m->astNodeType) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					IdRefPtr fullName;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(doc->allocator.get(), m, fullName));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, false, obj));

					if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeOut = slake::Type(slake::TypeId::Instance, obj.get());
					break;
				}
				case AstNodeType::GenericParam: {
					slake::HostObjectRef<slake::StringObject> obj;

					if (!(obj = slake::StringObject::alloc(compileContext->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!obj->data.build(m->name)) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeOut = slake::Type(obj.get(), /* Placeholder!!! */ nullptr);
					break;
				}
				default:
					return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			break;
		}
		case TypeNameKind::Array: {
			peff::SharedPtr<ArrayTypeNameNode> t = typeName.castTo<ArrayTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(compileContext->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, t->elementType, obj->type));

			if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Array, obj.get());
			break;
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode> t = typeName.castTo<RefTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(compileContext->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, t->referencedType, obj->type));

			if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Ref, obj.get());
			break;
		}
		case TypeNameKind::ParamTypeList: {
			peff::SharedPtr<ParamTypeListTypeNameNode> t = typeName.castTo<ParamTypeListTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::ParamTypeListTypeDefObject> obj;

			if (!(obj = slake::ParamTypeListTypeDefObject::alloc(compileContext->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!obj->paramTypes.resize(t->paramTypes.size())) {
				return genOutOfRuntimeMemoryCompError();
			}
			for (size_t i = 0; i < obj->paramTypes.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, t->paramTypes.at(i), obj->paramTypes.at(i)));
			}

			obj->hasVarArg = t->hasVarArgs;

			if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::ParamTypeList, obj.get());
			break;
		}
		case TypeNameKind::Unpacking: {
			peff::SharedPtr<UnpackingTypeNameNode> t = typeName.castTo<UnpackingTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(compileContext->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, t->innerTypeName, obj->type));

			if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Unpacking, obj.get());
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileIdRef(
	CompileContext *compileContext,
	const IdRefEntry *entries,
	size_t nEntries,
	peff::SharedPtr<TypeNameNode> *paramTypes,
	size_t nParams,
	bool hasVarArgs,
	slake::HostObjectRef<slake::IdRefObject> &idRefOut) {
	slake::HostObjectRef<slake::IdRefObject> id;
	assert(nEntries);

	if (!(id = slake::IdRefObject::alloc(compileContext->runtime))) {
		return genOutOfRuntimeMemoryCompError();
	}
	if (!id->entries.resizeUninitialized(nEntries)) {
		return genOutOfRuntimeMemoryCompError();
	}

	for (size_t i = 0; i < id->entries.size(); ++i) {
		peff::constructAt<slake::IdRefEntry>(&id->entries.at(i), slake::IdRefEntry(compileContext->runtime->getCurGenAlloc()));
	}

	for (size_t i = 0; i < nEntries; ++i) {
		const IdRefEntry &ce = entries[i];
		slake::IdRefEntry &e = id->entries.at(i);

		if (!e.name.build(ce.name)) {
			return genOutOfRuntimeMemoryCompError();
		}

		if (!e.genericArgs.resize(ce.genericArgs.size())) {
			return genOutOfRuntimeMemoryCompError();
		}

		for (size_t i = 0; i < ce.genericArgs.size(); ++i) {
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, ce.genericArgs.at(i), e.genericArgs.at(i)));
		}
	}

	if (paramTypes) {
		id->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

		if (!id->paramTypes->resize(nParams)) {
			return genOutOfMemoryCompError();
		}

		for (size_t i = 0; i < nParams; ++i) {
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, paramTypes[i], id->paramTypes->at(i)));
		}
	}

	id->hasVarArgs = hasVarArgs;
	idRefOut = id;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileValueExpr(
	CompileContext *compileContext,
	peff::SharedPtr<ExprNode> expr,
	slake::Value &valueOut) {
	switch (expr->exprKind) {
		case ExprKind::IdRef: {
			peff::SharedPtr<IdRefExprNode> e = expr.castTo<IdRefExprNode>();
			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(
				compileIdRef(
					compileContext,
					e->idRefPtr->entries.data(),
					e->idRefPtr->entries.size(),
					nullptr,
					0,
					false,
					id));

			if (!compileContext->hostRefHolder.addObject(id.get())) {
				return genOutOfMemoryCompError();
			}

			slake::EntityRef entityRef = slake::EntityRef::makeObjectRef(id.get());

			valueOut = slake::Value(entityRef);
			break;
		}
		case ExprKind::I8: {
			peff::SharedPtr<I8LiteralExprNode> e = expr.castTo<I8LiteralExprNode>();

			valueOut = slake::Value((int8_t)e->data);
			break;
		}
		case ExprKind::I16: {
			peff::SharedPtr<I16LiteralExprNode> e = expr.castTo<I16LiteralExprNode>();

			valueOut = slake::Value((int16_t)e->data);
			break;
		}
		case ExprKind::I32: {
			peff::SharedPtr<I32LiteralExprNode> e = expr.castTo<I32LiteralExprNode>();

			valueOut = slake::Value((int32_t)e->data);
			break;
		}
		case ExprKind::I64: {
			peff::SharedPtr<I64LiteralExprNode> e = expr.castTo<I64LiteralExprNode>();

			valueOut = slake::Value((int64_t)e->data);
			break;
		}
		case ExprKind::U8: {
			peff::SharedPtr<U8LiteralExprNode> e = expr.castTo<U8LiteralExprNode>();

			valueOut = slake::Value((uint8_t)e->data);
			break;
		}
		case ExprKind::U16: {
			peff::SharedPtr<U16LiteralExprNode> e = expr.castTo<U16LiteralExprNode>();

			valueOut = slake::Value((uint16_t)e->data);
			break;
		}
		case ExprKind::U32: {
			peff::SharedPtr<U32LiteralExprNode> e = expr.castTo<U32LiteralExprNode>();

			valueOut = slake::Value((uint32_t)e->data);
			break;
		}
		case ExprKind::U64: {
			peff::SharedPtr<U64LiteralExprNode> e = expr.castTo<U64LiteralExprNode>();

			valueOut = slake::Value((uint64_t)e->data);
			break;
		}
		case ExprKind::F32: {
			peff::SharedPtr<F32LiteralExprNode> e = expr.castTo<F32LiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::F64: {
			peff::SharedPtr<F64LiteralExprNode> e = expr.castTo<F64LiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::String: {
			peff::SharedPtr<StringLiteralExprNode> e = expr.castTo<StringLiteralExprNode>();

			slake::HostObjectRef<slake::StringObject> s;

			if (!(s = slake::StringObject::alloc(compileContext->runtime))) {
				return genOutOfMemoryCompError();
			}

			if (!s->data.build(e->data)) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!compileContext->hostRefHolder.addObject(s.get())) {
				return genOutOfMemoryCompError();
			}

			slake::EntityRef entityRef = slake::EntityRef::makeObjectRef(s.get());

			valueOut = slake::Value(entityRef);
			break;
		}
		case ExprKind::Bool: {
			peff::SharedPtr<BoolLiteralExprNode> e = expr.castTo<BoolLiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::Null: {
			peff::SharedPtr<NullLiteralExprNode> e = expr.castTo<NullLiteralExprNode>();

			slake::EntityRef entityRef = slake::EntityRef::makeObjectRef(nullptr);

			valueOut = slake::Value(entityRef);
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileGenericParams(
	CompileContext *compileContext,
	peff::SharedPtr<ModuleNode> mod,
	peff::SharedPtr<GenericParamNode> *genericParams,
	size_t nGenericParams,
	slake::GenericParamList &genericParamListOut) {
	std::optional<CompilationError> e;

	for (size_t j = 0; j < nGenericParams; ++j) {
		auto gpNode = genericParams[j];
		slake::GenericParam gp(compileContext->runtime->getCurGenAlloc());

		if (!gp.name.build(gpNode->name)) {
			return genOutOfMemoryCompError();
		}

		if (gpNode->isParamTypeList) {
			// TODO: Implement it.
		} else {
			if (gpNode->genericConstraint->baseType) {
				if ((e = compileTypeName(compileContext, gpNode->genericConstraint->baseType, gp.baseType))) {
					if (e->errorKind == CompilationErrorKind::OutOfMemory)
						return e;
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
				}
			}

			if (!gp.interfaces.resize(gpNode->genericConstraint->implTypes.size())) {
				return genOutOfRuntimeMemoryCompError();
			}

			for (size_t k = 0; k < gpNode->genericConstraint->implTypes.size(); ++k) {
				if ((e = compileTypeName(compileContext, gpNode->genericConstraint->implTypes.at(k), gp.interfaces.at(k)))) {
					if (e->errorKind == CompilationErrorKind::OutOfMemory)
						return e;
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
				}
			}
		}

		if (!genericParamListOut.pushBack(std::move(gp))) {
			return genOutOfRuntimeMemoryCompError();
		}
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileModule(
	CompileContext *compileContext,
	peff::SharedPtr<ModuleNode> mod,
	slake::ModuleObject *modOut) {
	std::optional<CompilationError> compilationError;
	for (auto i : mod->anonymousImports) {
		slake::HostObjectRef<slake::IdRefObject> id;

		for (auto &j : compileContext->document->externalModuleProviders) {
			SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileContext, i->idRef.get()));
		}

		SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, i->idRef->entries.data(), i->idRef->entries.size(), nullptr, 0, false, id));

		if (!modOut->unnamedImports.pushBack(id.get())) {
			return genOutOfRuntimeMemoryCompError();
		}
	}

	for (auto [k, v] : mod->memberIndices) {
		peff::SharedPtr<MemberNode> m = mod->members.at(v);

		if (m->astNodeType == AstNodeType::Import) {
			peff::SharedPtr<ImportNode> importNode = m.castTo<ImportNode>();

			for (auto &j : compileContext->document->externalModuleProviders) {
				SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileContext, importNode->idRef.get()));
			}

			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, importNode->idRef->entries.data(), importNode->idRef->entries.size(), nullptr, 0, false, id));

			if (!modOut->unnamedImports.pushBack(id.get())) {
				return genOutOfMemoryCompError();
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(indexModuleMembers(compileContext, compileContext->document->rootModule));

	for (auto [k, v] : mod->memberIndices) {
		peff::SharedPtr<MemberNode> m = mod->members.at(v);

		switch (m->astNodeType) {
			case AstNodeType::Var: {
				peff::SharedPtr<VarNode> varNode = m.castTo<VarNode>();

				slake::FieldRecord fr(compileContext->runtime->getCurGenAlloc());

				if (!fr.name.build(k)) {
					return genOutOfRuntimeMemoryCompError();
				}

				fr.accessModifier = m->accessModifier;
				fr.offset = modOut->localFieldStorage.size();

				slake::Type type;

				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, varNode->type, type));

				fr.type = type;

				if (!modOut->appendFieldRecord(std::move(fr))) {
					return genOutOfRuntimeMemoryCompError();
				}

				slake::Value defaultValue;

				if (varNode->initialValue) {
					SLKC_RETURN_IF_COMP_ERROR(compileValueExpr(compileContext, varNode->initialValue, defaultValue));
				} else {
					defaultValue = modOut->associatedRuntime->defaultValueOf(type);
				}
				modOut->associatedRuntime->writeVar(slake::EntityRef::makeFieldRef(modOut, modOut->fieldRecords.size() - 1), defaultValue).unwrap();

				break;
			}
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> clsNode = m.castTo<ClassNode>();

				slake::HostObjectRef<slake::ClassObject> cls;

				if (!(cls = slake::ClassObject::alloc(compileContext->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				if (clsNode->baseType) {
					peff::SharedPtr<MemberNode> baseTypeNode;

					if (clsNode->baseType->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), clsNode->baseType.castTo<CustomTypeNameNode>(), baseTypeNode))) {
							if (baseTypeNode) {
								if (baseTypeNode->astNodeType != AstNodeType::Class) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
								}

								bool isCyclicInherited = false;
								SLKC_RETURN_IF_COMP_ERROR(isBaseOf(clsNode->document->sharedFromThis(), clsNode, clsNode, isCyclicInherited));

								if (isCyclicInherited) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::CyclicInheritedClass)));
									continue;
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(std::move(compilationError.value())));
							compilationError.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
					}

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, clsNode->baseType, cls->baseType));
				}

				peff::Set<peff::SharedPtr<InterfaceNode>> involvedInterfaces(compileContext->allocator.get());

				for (auto &i : clsNode->implTypes) {
					peff::SharedPtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->astNodeType != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									SLKC_RETURN_IF_COMP_ERROR(collectInvolvedInterfaces(compileContext->document, implementedTypeNode.castTo<InterfaceNode>(), involvedInterfaces, true));
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(std::move(compilationError.value())));
							compilationError.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
					}

					slake::Type t;

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, i, t));

					if (!cls->implTypes.pushBack(std::move(t))) {
						return genOutOfRuntimeMemoryCompError();
					}
				}

				for (auto &i : involvedInterfaces) {
					for (auto &j : i->members) {
						if (j->astNodeType == AstNodeType::FnSlot) {
							peff::SharedPtr<FnNode> method = j.castTo<FnNode>();

							if (auto it = clsNode->memberIndices.find(j->name); it != clsNode->memberIndices.end()) {
								peff::SharedPtr<MemberNode> correspondingMember = clsNode->members.at(it.value());

								if (correspondingMember->astNodeType != AstNodeType::FnSlot) {
									for (auto &k : method->overloadings) {
										SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));
									}
								} else {
									peff::SharedPtr<FnNode> correspondingMethod = correspondingMember.castTo<FnNode>();

									for (auto &k : method->overloadings) {
										bool b = false;

										for (auto &l : correspondingMethod->overloadings) {
											if (k->params.size() != l->params.size()) {
												continue;
											}

											SLKC_RETURN_IF_COMP_ERROR(isFnSignatureSame(k->params.data(), l->params.data(), k->params.size(), b));

											if (b) {
												goto matched;
											}
										}

										SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));

									matched:;
									}
								}
							} else {
								for (auto &k : method->overloadings) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));
								}
							}
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileContext, clsNode.castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Interface: {
				peff::SharedPtr<InterfaceNode> clsNode = m.castTo<InterfaceNode>();

				slake::HostObjectRef<slake::InterfaceObject> cls;

				if (!(cls = slake::InterfaceObject::alloc(compileContext->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				for (auto &i : clsNode->implTypes) {
					peff::SharedPtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->astNodeType != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								}
								bool isCyclicInherited = false;
								SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(clsNode->document->sharedFromThis(), clsNode, clsNode, isCyclicInherited));

								if (isCyclicInherited) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::CyclicInheritedClass)));
									continue;
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(std::move(compilationError.value())));
							compilationError.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileContext, clsNode.castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnNode> slotNode = m.castTo<FnNode>();
				slake::HostObjectRef<slake::FnObject> slotObject;

				if (!(slotObject = slake::FnObject::alloc(compileContext->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (!slotObject->name.build(slotNode->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				for (auto &i : slotNode->overloadings) {
					std::optional<CompilationError> e;

					for (size_t j = &i - slotNode->overloadings.data() + 1; j < slotNode->overloadings.size(); ++j) {
						bool whether;
						SLKC_RETURN_IF_COMP_ERROR(isFnSignatureDuplicated(i, slotNode->overloadings.at(j), whether));
						if (whether) {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::FunctionOverloadingDuplicated)));
							break;
						}
					}

					compileContext->reset();

					switch (mod->astNodeType) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
							if (!(i->accessModifier & slake::ACCESS_STATIC)) {
								if (!(compileContext->thisNode = peff::makeSharedWithControlBlock<ThisNode, AstNodeControlBlock<ThisNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document)))
									return genOutOfMemoryCompError();
								compileContext->thisNode->thisType = i->parent->parent->sharedFromThis().castTo<MemberNode>();
							}
							break;
						default:
							break;
					}

					slake::HostObjectRef<slake::RegularFnOverloadingObject> fnObject;

					if (!(fnObject = slake::RegularFnOverloadingObject::alloc(slotObject.get()))) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (i->fnFlags & FN_VARG) {
						fnObject->setVarArgs();
					}
					if (i->fnFlags & FN_VIRTUAL) {
						fnObject->setVirtualFlag();
					}

					fnObject->setAccess(i->accessModifier);

					compileContext->curOverloading = i;

					if (!fnObject->paramTypes.resize(i->params.size())) {
						return genOutOfRuntimeMemoryCompError();
					}
					for (size_t j = 0; j < i->params.size(); ++j) {
						if ((e = compileTypeName(compileContext, i->params.at(j)->type, fnObject->paramTypes.at(j)))) {
							if (e->errorKind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compileContext->errors.pushBack(std::move(*e))) {
								return genOutOfMemoryCompError();
							}
							e.reset();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileContext, mod, i->genericParams.data(), i->genericParams.size(), fnObject->genericParams));

					if (i->body) {
						NormalCompilationContext compilationContext(compileContext, nullptr);

						for (auto j : i->body->body) {
							if ((e = compileStmt(compileContext, &compilationContext, j))) {
								if (e->errorKind == CompilationErrorKind::OutOfMemory)
									return e;
								if (!compileContext->errors.pushBack(std::move(*e))) {
									return genOutOfMemoryCompError();
								}
								e.reset();
							}
						}
						if (!fnObject->instructions.resize(compilationContext.generatedInstructions.size())) {
							return genOutOfRuntimeMemoryCompError();
						}
						for (size_t i = 0; i < compilationContext.generatedInstructions.size(); ++i) {
							fnObject->instructions.at(i) = std::move(compilationContext.generatedInstructions.at(i));
						}
						compilationContext.generatedInstructions.clear();

						for (auto &j : fnObject->instructions) {
							for (size_t k = 0; k < j.nOperands; ++k) {
								if (j.operands[k].valueType == slake::ValueType::Label) {
									j.operands[k] = slake::Value(compilationContext.getLabelOffset(j.operands[k].getLabel()));
								}
							}
						}

						fnObject->nRegisters = compilationContext.nTotalRegs;
					}

					if (!slotObject->overloadings.insert(fnObject.get())) {
						return genOutOfRuntimeMemoryCompError();
					}
				}

				if (!modOut->addMember(slotObject.get())) {
					return genOutOfRuntimeMemoryCompError();
				}
				break;
			}
			default:
				break;
		}
	}

	return {};
}
