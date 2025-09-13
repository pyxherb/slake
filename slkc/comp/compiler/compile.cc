#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileTypeName(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<TypeNameNode> typeName,
	slake::TypeRef &typeOut) {
	switch (typeName->typeNameKind) {
		case TypeNameKind::Void:
			typeOut = slake::TypeRef(slake::TypeId::Void);
			break;
		case TypeNameKind::I8:
			typeOut = slake::TypeRef(slake::TypeId::I8);
			break;
		case TypeNameKind::I16:
			typeOut = slake::TypeRef(slake::TypeId::I16);
			break;
		case TypeNameKind::I32:
			typeOut = slake::TypeRef(slake::TypeId::I32);
			break;
		case TypeNameKind::I64:
			typeOut = slake::TypeRef(slake::TypeId::I64);
			break;
		case TypeNameKind::U8:
			typeOut = slake::TypeRef(slake::TypeId::U8);
			break;
		case TypeNameKind::U16:
			typeOut = slake::TypeRef(slake::TypeId::U16);
			break;
		case TypeNameKind::U32:
			typeOut = slake::TypeRef(slake::TypeId::U32);
			break;
		case TypeNameKind::U64:
			typeOut = slake::TypeRef(slake::TypeId::U64);
			break;
		case TypeNameKind::F32:
			typeOut = slake::TypeRef(slake::TypeId::F32);
			break;
		case TypeNameKind::F64:
			typeOut = slake::TypeRef(slake::TypeId::F64);
			break;
		case TypeNameKind::String:
			typeOut = slake::TypeRef(slake::TypeId::String);
			break;
		case TypeNameKind::Bool:
			typeOut = slake::TypeRef(slake::TypeId::Bool);
			break;
		case TypeNameKind::Object:
			typeOut = slake::TypeRef(slake::TypeId::Instance, nullptr);
			break;
		case TypeNameKind::Any:
			typeOut = slake::TypeRef(slake::TypeId::Any);
			break;
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode> t = typeName.castTo<CustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(doc, t, m));

			if (!m) {
				return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			switch (m->astNodeType) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					slake::HostObjectRef<slake::CustomTypeDefObject> typeDef;

					if (!(typeDef = slake::CustomTypeDefObject::alloc(compileEnv->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					IdRefPtr fullName;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(doc->allocator.get(), m, fullName));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, false, obj));

					if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeDef->typeObject = obj.get();

					typeOut = slake::TypeRef(slake::TypeId::Instance, typeDef.get());
					break;
				}
				case AstNodeType::GenericParam: {
					slake::HostObjectRef<slake::GenericArgTypeDefObject> typeDef;

					if (!(typeDef = slake::GenericArgTypeDefObject::alloc(compileEnv->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					slake::HostObjectRef<slake::StringObject> obj;

					if (!(obj = slake::StringObject::alloc(compileEnv->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!obj->data.build(m->name)) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeDef->ownerObject = nullptr;
					typeDef->nameObject = obj.get();

					typeOut = slake::TypeRef(slake::TypeId::GenericArg, typeDef.get());
					break;
				}
				default:
					return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			break;
		}
		case TypeNameKind::Array: {
			AstNodePtr<ArrayTypeNameNode> t = typeName.castTo<ArrayTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::ArrayTypeDefObject> typeDef;

			if (!(typeDef = slake::ArrayTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			slake::HostObjectRef<slake::HeapTypeObject> obj;

			if (!(obj = slake::HeapTypeObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, t->elementType, obj->typeRef));

			if (!(compileEnv->hostRefHolder.addObject(typeDef.get()))) {
				return genOutOfMemoryCompError();
			}

			typeDef->elementType = obj.get();

			typeOut = slake::TypeRef(slake::TypeId::Array, typeDef.get());
			break;
		}
		case TypeNameKind::Ref: {
			AstNodePtr<RefTypeNameNode> t = typeName.castTo<RefTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::RefTypeDefObject> typeDef;

			if (!(typeDef = slake::RefTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!(compileEnv->hostRefHolder.addObject(typeDef.get()))) {
				return genOutOfMemoryCompError();
			}

			slake::HostObjectRef<slake::HeapTypeObject> obj;

			if (!(obj = slake::HeapTypeObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, t->referencedType, obj->typeRef));

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeDef->referencedType = obj.get();

			typeOut = slake::TypeRef(slake::TypeId::Ref, typeDef.get());
			break;
		}
		case TypeNameKind::Tuple: {
			AstNodePtr<TupleTypeNameNode> t = typeName.castTo<TupleTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::TupleTypeDefObject> obj;

			if (!(obj = slake::TupleTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!obj->elementTypes.resize(t->elementTypes.size())) {
				return genOutOfRuntimeMemoryCompError();
			}

			for (size_t i = 0; i < t->elementTypes.size(); ++i) {
				slake::HostObjectRef<slake::HeapTypeObject> heapType;

				if (!(heapType = slake::HeapTypeObject::alloc(compileEnv->runtime)))
					return genOutOfRuntimeMemoryCompError();

				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, t->elementTypes.at(i), heapType->typeRef));

				if (!(compileEnv->hostRefHolder.addObject(heapType.get())))
					return genOutOfMemoryCompError();

				obj->elementTypes.at(i) = heapType.get();
			}

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::TypeRef(slake::TypeId::Tuple, obj.get());
			break;
		}
		case TypeNameKind::SIMD: {
			AstNodePtr<SIMDTypeNameNode> t = typeName.castTo<SIMDTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::SIMDTypeDefObject> obj;

			if (!(obj = slake::SIMDTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			slake::HostObjectRef<slake::HeapTypeObject> heapType;

			if (!(heapType = slake::HeapTypeObject::alloc(compileEnv->runtime)))
				return genOutOfRuntimeMemoryCompError();

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, t->elementType, heapType->typeRef));

			if (!(compileEnv->hostRefHolder.addObject(heapType.get())))
				return genOutOfMemoryCompError();

			AstNodePtr<ExprNode> width;

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, t->width, width));

			if (!width) {
				return CompilationError(t->width->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);
			}

			if (width->exprKind != ExprKind::U32) {
				AstNodePtr<CastExprNode> ce;

				if (!(ce = makeAstNode<CastExprNode>(t->document->allocator.get(), t->document->allocator.get(), t->document->sharedFromThis()))) {
					return genOutOfMemoryCompError();
				}

				AstNodePtr<U32TypeNameNode> u32Type;

				if (!(u32Type = makeAstNode<U32TypeNameNode>(t->document->allocator.get(), t->document->allocator.get(), t->document->sharedFromThis()))) {
					return genOutOfMemoryCompError();
				}

				ce->source = width;
				ce->targetType = u32Type.castTo<TypeNameNode>();

				SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, ce.castTo<ExprNode>(), width));

				if (!width) {
					return CompilationError(t->width->tokenRange, CompilationErrorKind::TypeArgTypeMismatched);
				}
			}

			obj->type = heapType.get();
			obj->width = width.castTo<U32LiteralExprNode>()->data;

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::TypeRef(slake::TypeId::SIMD, obj.get());
			break;
		}
		case TypeNameKind::ParamTypeList: {
			AstNodePtr<ParamTypeListTypeNameNode> t = typeName.castTo<ParamTypeListTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::ParamTypeListTypeDefObject> obj;

			if (!(obj = slake::ParamTypeListTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!obj->paramTypes.resize(t->paramTypes.size())) {
				return genOutOfRuntimeMemoryCompError();
			}
			for (size_t i = 0; i < obj->paramTypes.size(); ++i) {
				slake::HostObjectRef<slake::HeapTypeObject> heapType;

				if (!(heapType = slake::HeapTypeObject::alloc(compileEnv->runtime)))
					return genOutOfRuntimeMemoryCompError();

				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, t->paramTypes.at(i), heapType->typeRef));

				if (!(compileEnv->hostRefHolder.addObject(heapType.get())))
					return genOutOfMemoryCompError();

				obj->paramTypes.at(i) = heapType.get();
			}

			obj->hasVarArg = t->hasVarArgs;

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::TypeRef(slake::TypeId::ParamTypeList, obj.get());
			break;
		}
		case TypeNameKind::Unpacking: {
			AstNodePtr<UnpackingTypeNameNode> t = typeName.castTo<UnpackingTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::UnpackingTypeDefObject> obj;

			if (!(obj = slake::UnpackingTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			slake::HostObjectRef<slake::HeapTypeObject> heapType;

			if (!(heapType = slake::HeapTypeObject::alloc(compileEnv->runtime)))
				return genOutOfRuntimeMemoryCompError();

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, t->innerTypeName, heapType->typeRef));

			if (!(compileEnv->hostRefHolder.addObject(heapType.get())))
				return genOutOfMemoryCompError();

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::TypeRef(slake::TypeId::Unpacking, obj.get());
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileIdRef(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	const IdRefEntry *entries,
	size_t nEntries,
	AstNodePtr<TypeNameNode> *paramTypes,
	size_t nParams,
	bool hasVarArgs,
	slake::HostObjectRef<slake::IdRefObject> &idRefOut) {
	slake::HostObjectRef<slake::IdRefObject> id;
	assert(nEntries);

	if (!(id = slake::IdRefObject::alloc(compileEnv->runtime))) {
		return genOutOfRuntimeMemoryCompError();
	}
	if (!id->entries.resizeUninitialized(nEntries)) {
		return genOutOfRuntimeMemoryCompError();
	}

	for (size_t i = 0; i < id->entries.size(); ++i) {
		peff::constructAt<slake::IdRefEntry>(&id->entries.at(i), slake::IdRefEntry(compileEnv->runtime->getCurGenAlloc()));
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
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, ce.genericArgs.at(i), e.genericArgs.at(i)));
		}
	}

	if (paramTypes) {
		id->paramTypes = peff::DynArray<slake::TypeRef>(compileEnv->runtime->getCurGenAlloc());

		if (!id->paramTypes->resize(nParams)) {
			return genOutOfMemoryCompError();
		}

		for (size_t i = 0; i < nParams; ++i) {
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, paramTypes[i], id->paramTypes->at(i)));
		}
	}

	id->hasVarArgs = hasVarArgs;
	idRefOut = id;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileValueExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ExprNode> expr,
	slake::Value &valueOut) {
	switch (expr->exprKind) {
		case ExprKind::IdRef: {
			AstNodePtr<IdRefExprNode> e = expr.castTo<IdRefExprNode>();
			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(
				compileIdRef(
					compileEnv,
					compilationContext,
					e->idRefPtr->entries.data(),
					e->idRefPtr->entries.size(),
					nullptr,
					0,
					false,
					id));

			if (!compileEnv->hostRefHolder.addObject(id.get())) {
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
			AstNodePtr<StringLiteralExprNode> e = expr.castTo<StringLiteralExprNode>();

			slake::HostObjectRef<slake::StringObject> s;

			if (!(s = slake::StringObject::alloc(compileEnv->runtime))) {
				return genOutOfMemoryCompError();
			}

			if (!s->data.build(e->data)) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!compileEnv->hostRefHolder.addObject(s.get())) {
				return genOutOfMemoryCompError();
			}

			slake::EntityRef entityRef = slake::EntityRef::makeObjectRef(s.get());

			valueOut = slake::Value(entityRef);
			break;
		}
		case ExprKind::Bool: {
			AstNodePtr<BoolLiteralExprNode> e = expr.castTo<BoolLiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::Null: {
			AstNodePtr<NullLiteralExprNode> e = expr.castTo<NullLiteralExprNode>();

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
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ModuleNode> mod,
	AstNodePtr<GenericParamNode> *genericParams,
	size_t nGenericParams,
	slake::GenericParamList &genericParamListOut) {
	std::optional<CompilationError> e;

	for (size_t j = 0; j < nGenericParams; ++j) {
		auto gpNode = genericParams[j];
		slake::GenericParam gp(compileEnv->runtime->getCurGenAlloc());

		if (!gp.name.build(gpNode->name)) {
			return genOutOfMemoryCompError();
		}

		if (gpNode->isParamTypeList) {
			// TODO: Implement it.
		} else {
			if (gpNode->genericConstraint->baseType) {
				if ((e = compileTypeName(compileEnv, compilationContext, gpNode->genericConstraint->baseType, gp.baseType))) {
					if (e->errorKind == CompilationErrorKind::OutOfMemory)
						return e;
					if (!compileEnv->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
				}
			}

			if (!gp.interfaces.resize(gpNode->genericConstraint->implTypes.size())) {
				return genOutOfRuntimeMemoryCompError();
			}

			for (size_t k = 0; k < gpNode->genericConstraint->implTypes.size(); ++k) {
				if ((e = compileTypeName(compileEnv, compilationContext, gpNode->genericConstraint->implTypes.at(k), gp.interfaces.at(k)))) {
					if (e->errorKind == CompilationErrorKind::OutOfMemory)
						return e;
					if (!compileEnv->errors.pushBack(std::move(*e))) {
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
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> mod,
	slake::ModuleObject *modOut) {
	std::optional<CompilationError> compilationError;
	for (auto i : mod->anonymousImports) {
		NormalCompilationContext compilationContext(compileEnv, nullptr);

		slake::HostObjectRef<slake::IdRefObject> id;

		for (auto &j : compileEnv->document->externalModuleProviders) {
			SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileEnv, i->idRef.get()));
		}

		SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, &compilationContext, i->idRef->entries.data(), i->idRef->entries.size(), nullptr, 0, false, id));

		if (!modOut->unnamedImports.pushBack(id.get())) {
			return genOutOfRuntimeMemoryCompError();
		}
	}

	for (auto [k, v] : mod->memberIndices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		if (m->astNodeType == AstNodeType::Import) {
			AstNodePtr<ImportNode> importNode = m.castTo<ImportNode>();

			for (auto &j : compileEnv->document->externalModuleProviders) {
				SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileEnv, importNode->idRef.get()));
			}

			NormalCompilationContext compilationContext(compileEnv, nullptr);

			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, &compilationContext, importNode->idRef->entries.data(), importNode->idRef->entries.size(), nullptr, 0, false, id));

			if (!modOut->unnamedImports.pushBack(id.get())) {
				return genOutOfMemoryCompError();
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(indexModuleMembers(compileEnv, compileEnv->document->rootModule));

	for (auto [k, v] : mod->memberIndices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		NormalCompilationContext compilationContext(compileEnv, nullptr);

		switch (m->astNodeType) {
			case AstNodeType::Var: {
				AstNodePtr<VarNode> varNode = m.castTo<VarNode>();

				slake::FieldRecord fr(compileEnv->runtime->getCurGenAlloc());

				if (!fr.name.build(k)) {
					return genOutOfRuntimeMemoryCompError();
				}

				fr.accessModifier = m->accessModifier;
				fr.offset = modOut->localFieldStorage.size();

				slake::TypeRef type;

				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, &compilationContext, varNode->type, type));

				fr.type = type;

				if (!modOut->appendFieldRecord(std::move(fr))) {
					return genOutOfRuntimeMemoryCompError();
				}

				slake::Value defaultValue;

				if (varNode->initialValue) {
					SLKC_RETURN_IF_COMP_ERROR(compileValueExpr(compileEnv, &compilationContext, varNode->initialValue, defaultValue));
				} else {
					defaultValue = modOut->associatedRuntime->defaultValueOf(type);
				}
				modOut->associatedRuntime->writeVar(slake::EntityRef::makeFieldRef(modOut, modOut->fieldRecords.size() - 1), defaultValue).unwrap();

				break;
			}
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> clsNode = m.castTo<ClassNode>();

				slake::HostObjectRef<slake::ClassObject> cls;

				if (!(cls = slake::ClassObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				if (clsNode->baseType) {
					AstNodePtr<MemberNode> baseTypeNode;

					if (clsNode->baseType->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), clsNode->baseType.castTo<CustomTypeNameNode>(), baseTypeNode))) {
							if (baseTypeNode) {
								if (baseTypeNode->astNodeType != AstNodeType::Class) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
								}

								bool isCyclicInherited = false;
								SLKC_RETURN_IF_COMP_ERROR(isBaseOf(clsNode->document->sharedFromThis(), clsNode, clsNode, isCyclicInherited));

								if (isCyclicInherited) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::CyclicInheritedClass)));
									continue;
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(std::move(compilationError.value())));
							compilationError.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
					}

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, &compilationContext, clsNode->baseType, cls->baseType));
				}

				peff::Set<AstNodePtr<InterfaceNode>> involvedInterfaces(compileEnv->allocator.get());

				for (auto &i : clsNode->implTypes) {
					AstNodePtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->astNodeType != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collectInvolvedInterfaces(compileEnv->document, implementedTypeNode.castTo<InterfaceNode>(), involvedInterfaces, true); e) {
										if (e->errorKind != CompilationErrorKind::CyclicInheritedInterface)
											return e;
									}

									bool isCyclicInherited = false;
									SLKC_RETURN_IF_COMP_ERROR(clsNode->isCyclicInherited(isCyclicInherited));

									if (isCyclicInherited) {
										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::CyclicInheritedClass)));
										continue;
									}
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(std::move(compilationError.value())));
							compilationError.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
					}

					slake::TypeRef t;

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, &compilationContext, i, t));

					if (!cls->implTypes.pushBack(std::move(t))) {
						return genOutOfRuntimeMemoryCompError();
					}
				}

				for (auto &i : involvedInterfaces) {
					for (auto &j : i->members) {
						if (j->astNodeType == AstNodeType::FnSlot) {
							AstNodePtr<FnNode> method = j.castTo<FnNode>();

							if (auto it = clsNode->memberIndices.find(j->name); it != clsNode->memberIndices.end()) {
								AstNodePtr<MemberNode> correspondingMember = clsNode->members.at(it.value());

								if (correspondingMember->astNodeType != AstNodeType::FnSlot) {
									for (auto &k : method->overloadings) {
										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));
									}
								} else {
									AstNodePtr<FnNode> correspondingMethod = correspondingMember.castTo<FnNode>();

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

										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));

									matched:;
									}
								}
							} else {
								for (auto &k : method->overloadings) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));
								}
							}
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileEnv, clsNode.castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> clsNode = m.castTo<InterfaceNode>();

				slake::HostObjectRef<slake::InterfaceObject> cls;

				if (!(cls = slake::InterfaceObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				for (auto &i : clsNode->implTypes) {
					AstNodePtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->astNodeType != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								}
								bool isCyclicInherited = false;
								SLKC_RETURN_IF_COMP_ERROR(clsNode->isCyclicInherited(isCyclicInherited));

								if (isCyclicInherited) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::CyclicInheritedInterface)));
									continue;
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(std::move(compilationError.value())));
							compilationError.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
					}

					slake::TypeRef t;

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, &compilationContext, i, t));

					if (!cls->implTypes.pushBack(std::move(t))) {
						return genOutOfRuntimeMemoryCompError();
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileEnv, clsNode.castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::FnSlot: {
				AstNodePtr<FnNode> slotNode = m.castTo<FnNode>();
				slake::HostObjectRef<slake::FnObject> slotObject;

				if (!(slotObject = slake::FnObject::alloc(compileEnv->runtime))) {
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
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::FunctionOverloadingDuplicated)));
							break;
						}
					}

					compileEnv->reset();

					switch (mod->astNodeType) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
							if (!(i->accessModifier & slake::ACCESS_STATIC)) {
								if (!(compileEnv->thisNode = makeAstNode<ThisNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
									return genOutOfMemoryCompError();
								compileEnv->thisNode->thisType = i->parent->parent->sharedFromThis().castTo<MemberNode>();
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

					compileEnv->curOverloading = i;

					if (!fnObject->paramTypes.resize(i->params.size())) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (i->returnType) {
						if ((e = compileTypeName(compileEnv, &compilationContext, i->returnType, fnObject->returnType))) {
							if (e->errorKind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compileEnv->errors.pushBack(std::move(*e))) {
								return genOutOfMemoryCompError();
							}
							e.reset();
						}
					} else {
						fnObject->returnType = slake::TypeId::Void;
					}

					for (size_t j = 0; j < i->params.size(); ++j) {
						if ((e = compileTypeName(compileEnv, &compilationContext, i->params.at(j)->type, fnObject->paramTypes.at(j)))) {
							if (e->errorKind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compileEnv->errors.pushBack(std::move(*e))) {
								return genOutOfMemoryCompError();
							}
							e.reset();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, i->genericParams.data(), i->genericParams.size(), fnObject->genericParams));

					if (i->body) {
						NormalCompilationContext compContext(compileEnv, nullptr);

						for (auto j : i->body->body) {
							if ((e = compileStmt(compileEnv, &compContext, j))) {
								if (e->errorKind == CompilationErrorKind::OutOfMemory)
									return e;
								if (!compileEnv->errors.pushBack(std::move(*e))) {
									return genOutOfMemoryCompError();
								}
								e.reset();
							}
						}
						if (!fnObject->instructions.resize(compContext.generatedInstructions.size())) {
							return genOutOfRuntimeMemoryCompError();
						}
						for (size_t i = 0; i < compContext.generatedInstructions.size(); ++i) {
							fnObject->instructions.at(i) = std::move(compContext.generatedInstructions.at(i));
						}
						compContext.generatedInstructions.clear();

						for (auto &j : fnObject->instructions) {
							for (size_t k = 0; k < j.nOperands; ++k) {
								if (j.operands[k].valueType == slake::ValueType::Label) {
									j.operands[k] = slake::Value(compContext.getLabelOffset(j.operands[k].getLabel()));
								}
							}
						}

						fnObject->nRegisters = compContext.nTotalRegs;
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
