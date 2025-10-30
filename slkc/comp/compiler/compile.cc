#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::compileTypeName(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<TypeNameNode> typeName,
	slake::TypeRef &typeOut) {
	typeOut = slake::TypeId::Void;

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
			AstNodePtr<CustomTypeNameNode> t = typeName.template castTo<CustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(doc, t, m));

			if (!m) {
				return CompilationError(typeName->tokenRange, CompilationErrorKind::DoesNotReferToATypeName);
			}

			switch (m->getAstNodeType()) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					slake::HostObjectRef<slake::CustomTypeDefObject> typeDef;

					if (!(typeDef = slake::CustomTypeDefObject::alloc(compileEnv->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					IdRefPtr fullName;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(doc->allocator.get(), m, fullName));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeDef->typeObject = obj.get();

					typeOut = slake::TypeRef(slake::TypeId::Instance, typeDef.get());
					break;
				}
				case AstNodeType::Struct: {
					slake::HostObjectRef<slake::CustomTypeDefObject> typeDef;

					if (!(typeDef = slake::CustomTypeDefObject::alloc(compileEnv->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					IdRefPtr fullName;

					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(doc->allocator.get(), m, fullName));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					typeDef->typeObject = obj.get();

					typeOut = slake::TypeRef(slake::TypeId::StructInstance, typeDef.get());
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
			AstNodePtr<ArrayTypeNameNode> t = typeName.template castTo<ArrayTypeNameNode>();
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
			AstNodePtr<RefTypeNameNode> t = typeName.template castTo<RefTypeNameNode>();
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
			AstNodePtr<TupleTypeNameNode> t = typeName.template castTo<TupleTypeNameNode>();
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
			AstNodePtr<SIMDTypeNameNode> t = typeName.template castTo<SIMDTypeNameNode>();
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
				ce->targetType = u32Type.template castTo<TypeNameNode>();

				SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, ce.template castTo<ExprNode>(), width));

				if (!width) {
					return CompilationError(t->width->tokenRange, CompilationErrorKind::TypeArgTypeMismatched);
				}
			}

			obj->type = heapType.get();
			obj->width = width.template castTo<U32LiteralExprNode>()->data;

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::TypeRef(slake::TypeId::SIMD, obj.get());
			break;
		}
		case TypeNameKind::ParamTypeList: {
			AstNodePtr<ParamTypeListTypeNameNode> t = typeName.template castTo<ParamTypeListTypeNameNode>();
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
			AstNodePtr<UnpackingTypeNameNode> t = typeName.template castTo<UnpackingTypeNameNode>();
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

			obj->type = heapType.get();

			typeOut = slake::TypeRef(slake::TypeId::Unpacking, obj.get());
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileIdRef(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	const IdRefEntry *entries,
	size_t nEntries,
	AstNodePtr<TypeNameNode> *paramTypes,
	size_t nParams,
	bool hasVarArgs,
	AstNodePtr<TypeNameNode> overridenType,
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

	id->overridenType = slake::TypeId::Void;

	if (overridenType)
		SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, overridenType, id->overridenType));

	idRefOut = id;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileValueExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ExprNode> expr,
	slake::Value &valueOut) {
	switch (expr->exprKind) {
		case ExprKind::IdRef: {
			AstNodePtr<IdRefExprNode> e = expr.template castTo<IdRefExprNode>();
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
					{},
					id));

			if (!compileEnv->hostRefHolder.addObject(id.get())) {
				return genOutOfMemoryCompError();
			}

			slake::Reference entityRef = slake::Reference::makeObjectRef(id.get());

			valueOut = slake::Value(entityRef);
			break;
		}
		case ExprKind::I8: {
			peff::SharedPtr<I8LiteralExprNode> e = expr.template castTo<I8LiteralExprNode>();

			valueOut = slake::Value((int8_t)e->data);
			break;
		}
		case ExprKind::I16: {
			peff::SharedPtr<I16LiteralExprNode> e = expr.template castTo<I16LiteralExprNode>();

			valueOut = slake::Value((int16_t)e->data);
			break;
		}
		case ExprKind::I32: {
			peff::SharedPtr<I32LiteralExprNode> e = expr.template castTo<I32LiteralExprNode>();

			valueOut = slake::Value((int32_t)e->data);
			break;
		}
		case ExprKind::I64: {
			peff::SharedPtr<I64LiteralExprNode> e = expr.template castTo<I64LiteralExprNode>();

			valueOut = slake::Value((int64_t)e->data);
			break;
		}
		case ExprKind::U8: {
			peff::SharedPtr<U8LiteralExprNode> e = expr.template castTo<U8LiteralExprNode>();

			valueOut = slake::Value((uint8_t)e->data);
			break;
		}
		case ExprKind::U16: {
			peff::SharedPtr<U16LiteralExprNode> e = expr.template castTo<U16LiteralExprNode>();

			valueOut = slake::Value((uint16_t)e->data);
			break;
		}
		case ExprKind::U32: {
			peff::SharedPtr<U32LiteralExprNode> e = expr.template castTo<U32LiteralExprNode>();

			valueOut = slake::Value((uint32_t)e->data);
			break;
		}
		case ExprKind::U64: {
			peff::SharedPtr<U64LiteralExprNode> e = expr.template castTo<U64LiteralExprNode>();

			valueOut = slake::Value((uint64_t)e->data);
			break;
		}
		case ExprKind::F32: {
			peff::SharedPtr<F32LiteralExprNode> e = expr.template castTo<F32LiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::F64: {
			peff::SharedPtr<F64LiteralExprNode> e = expr.template castTo<F64LiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::String: {
			AstNodePtr<StringLiteralExprNode> e = expr.template castTo<StringLiteralExprNode>();

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

			slake::Reference entityRef = slake::Reference::makeObjectRef(s.get());

			valueOut = slake::Value(entityRef);
			break;
		}
		case ExprKind::Bool: {
			AstNodePtr<BoolLiteralExprNode> e = expr.template castTo<BoolLiteralExprNode>();

			valueOut = slake::Value(e->data);
			break;
		}
		case ExprKind::Null: {
			AstNodePtr<NullLiteralExprNode> e = expr.template castTo<NullLiteralExprNode>();

			slake::Reference entityRef = slake::Reference::makeObjectRef(nullptr);

			valueOut = slake::Value(entityRef);
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileGenericParams(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ModuleNode> mod,
	AstNodePtr<GenericParamNode> *genericParams,
	size_t nGenericParams,
	slake::GenericParamList &genericParamListOut) {
	peff::Option<CompilationError> e;

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

SLKC_API peff::Option<CompilationError> slkc::compileModule(
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> mod,
	slake::ModuleObject *modOut) {
	peff::Option<CompilationError> compilationError;
	for (auto i : mod->anonymousImports) {
		NormalCompilationContext compilationContext(compileEnv, nullptr);

		slake::HostObjectRef<slake::IdRefObject> id;

		for (auto &j : compileEnv->document->externalModuleProviders) {
			SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileEnv, i->idRef.get()));
		}

		SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, &compilationContext, i->idRef->entries.data(), i->idRef->entries.size(), nullptr, 0, false, {}, id));

		if (!modOut->unnamedImports.pushBack(id.get())) {
			return genOutOfRuntimeMemoryCompError();
		}
	}

	for (auto [k, v] : mod->memberIndices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		if (m->getAstNodeType() == AstNodeType::Import) {
			AstNodePtr<ImportNode> importNode = m.template castTo<ImportNode>();

			for (auto &j : compileEnv->document->externalModuleProviders) {
				SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileEnv, importNode->idRef.get()));
			}

			NormalCompilationContext compilationContext(compileEnv, nullptr);

			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, &compilationContext, importNode->idRef->entries.data(), importNode->idRef->entries.size(), nullptr, 0, false, {}, id));

			if (!modOut->unnamedImports.pushBack(id.get())) {
				return genOutOfMemoryCompError();
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(indexModuleMembers(compileEnv, compileEnv->document->rootModule));

	for (auto [k, v] : mod->memberIndices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		NormalCompilationContext compilationContext(compileEnv, nullptr);

		switch (m->getAstNodeType()) {
			case AstNodeType::Var: {
				AstNodePtr<VarNode> varNode = m.template castTo<VarNode>();

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
				modOut->associatedRuntime->writeVar(slake::Reference::makeStaticFieldRef(modOut, modOut->fieldRecords.size() - 1), defaultValue).unwrap();

				break;
			}
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> clsNode = m.template castTo<ClassNode>();

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
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), clsNode->baseType.template castTo<CustomTypeNameNode>(), baseTypeNode))) {
							if (baseTypeNode) {
								if (baseTypeNode->getAstNodeType() != AstNodeType::Class) {
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
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.template castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->getAstNodeType() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collectInvolvedInterfaces(compileEnv->document, implementedTypeNode.template castTo<InterfaceNode>(), involvedInterfaces, true); e) {
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

				auto cmp = [](const std::pair<AstNodePtr<InterfaceNode>, AstNodePtr<InterfaceNode>> &lhs,
							   const std::pair<AstNodePtr<InterfaceNode>, AstNodePtr<InterfaceNode>> &rhs) -> int {
					if (lhs.first < rhs.first)
						return -1;
					if (lhs.first > rhs.first)
						return 1;
					if (lhs.second < rhs.second)
						return -1;
					if (lhs.second > rhs.second)
						return 1;
					return 0;
				};

				peff::Set<std::pair<AstNodePtr<InterfaceNode>, AstNodePtr<InterfaceNode>>, decltype(cmp), true> reportedConflictingInterfacesSet(
					compileEnv->allocator.get(),
					std::move(cmp));

				bool conflictedInterfacesDetected = false;
				for (auto lhsIt = clsNode->implTypes.begin(); lhsIt != clsNode->implTypes.end(); ++lhsIt) {
					for (auto rhsIt = lhsIt + 1; rhsIt != clsNode->implTypes.end(); ++rhsIt) {
						AstNodePtr<InterfaceNode> lhsParent, rhsParent;

						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(*lhsIt, lhsParent, nullptr));
						SLKC_RETURN_IF_COMP_ERROR(visitBaseInterface(*rhsIt, rhsParent, nullptr));

						if ((!lhsParent) || (!rhsParent))
							continue;

						if (reportedConflictingInterfacesSet.contains({ lhsParent, rhsParent }) ||
							reportedConflictingInterfacesSet.contains({ rhsParent, lhsParent }))
							continue;
						if (!reportedConflictingInterfacesSet.insert({ lhsParent, rhsParent }))
							return genOutOfMemoryCompError();

						for (auto &lhsMember : lhsParent->members) {
							if (lhsMember->getAstNodeType() == AstNodeType::FnSlot) {
								AstNodePtr<FnNode> lhs = lhsMember.template castTo<FnNode>();

								if (auto rhsMember = rhsParent->memberIndices.find(lhsMember->name); rhsMember != rhsParent->memberIndices.end()) {
									AstNodePtr<MemberNode> correspondingMember = rhsParent->members.at(rhsMember.value());

									if (correspondingMember->getAstNodeType() != AstNodeType::FnSlot) {
										// Corresponding member should not be not a function.
										std::terminate();
									} else {
										AstNodePtr<FnNode> rhs = correspondingMember.template castTo<FnNode>();

										for (auto &curLhsOverloading : lhs->overloadings) {
											bool b = false;

											for (auto &curRhsOverloading : rhs->overloadings) {
												if (curLhsOverloading->params.size() != curRhsOverloading->params.size()) {
													continue;
												}

												SLKC_RETURN_IF_COMP_ERROR(isFnSignatureSame(curLhsOverloading->params.data(), curRhsOverloading->params.data(), curLhsOverloading->params.size(), {}, {}, b));

												// No conflict, continue.
												if (!b)
													continue;

												if (auto overridenIt = clsNode->memberIndices.find(lhsMember->name); overridenIt != clsNode->memberIndices.end()) {
													AstNodePtr<MemberNode> correspondingOverridenMember = clsNode->members.at(overridenIt.value());

													if (correspondingOverridenMember->getAstNodeType() == AstNodeType::FnSlot) {
														AstNodePtr<FnNode> correspondingOverridenMethod = correspondingOverridenMember.castTo<FnNode>();

														bool overridenWhether;

														for (auto &curOverridenOverloading : correspondingOverridenMethod->overloadings) {
															SLKC_RETURN_IF_COMP_ERROR(isFnSignatureSame(
																curLhsOverloading->params.data(),
																curOverridenOverloading->params.data(),
																curLhsOverloading->params.size(),
																curOverridenOverloading->overridenType,
																{},
																overridenWhether));
															// Unspecialized overriden, continue.
															if (overridenWhether) {
																b = false;
																goto checkInterfaceMethodsConflicted;
															}

															/*
															{
																AstNodePtr<CustomTypeNameNode> customOverridenType;

																if (!(customOverridenType = makeAstNode<CustomTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
																	return genOutOfMemoryCompError();

																SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), lhsIt->castTo<MemberNode>(), customOverridenType->idRefPtr));

																customOverridenType->contextNode = lhsIt->castTo<MemberNode>();

																SLKC_RETURN_IF_COMP_ERROR(
																	isFnSignatureSame(
																		curLhsOverloading->params.data(),
																		curOverridenOverloading->params.data(),
																		curLhsOverloading->params.size(),
																		customOverridenType.castTo<TypeNameNode>(),
																		curLhsOverloading->overridenType,
																		overridenWhether));
																// Specialized overriden, continue.
																if (overridenWhether) {
																	b = false;
																	goto checkInterfaceMethodsConflicted;
																}
															}*/
														}
													}
												}

											checkInterfaceMethodsConflicted:
												if (b) {
													conflictedInterfacesDetected = true;
													SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError((*lhsIt)->tokenRange, CompilationErrorKind::InterfaceMethodsConflicted)));
													continue;
												}
											}
										}
									}
								}
							}
						}
					}
				}

				if (!conflictedInterfacesDetected) {
					for (auto &i : involvedInterfaces) {
						for (auto &j : i->members) {
							if (j->getAstNodeType() == AstNodeType::FnSlot) {
								AstNodePtr<FnNode> method = j.template castTo<FnNode>();

								if (auto it = clsNode->memberIndices.find(j->name); it != clsNode->memberIndices.end()) {
									AstNodePtr<MemberNode> correspondingMember = clsNode->members.at(it.value());

									if (correspondingMember->getAstNodeType() != AstNodeType::FnSlot) {
										for (auto &k : method->overloadings) {
											SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));
										}
									} else {
										AstNodePtr<FnNode> correspondingMethod = correspondingMember.template castTo<FnNode>();

										for (auto &k : method->overloadings) {
											bool b = false;

											for (auto &l : correspondingMethod->overloadings) {
												if (k->params.size() != l->params.size()) {
													continue;
												}

												SLKC_RETURN_IF_COMP_ERROR(isFnSignatureSame(k->params.data(), l->params.data(), k->params.size(), {}, {}, b));

												if (b) {
													goto classMatched;
												}
											}

											SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));

										classMatched:;
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
				}

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileEnv, clsNode.template castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> clsNode = m.template castTo<InterfaceNode>();

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
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.template castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->getAstNodeType() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
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

				bool isCyclicInherited = false;
				SLKC_RETURN_IF_COMP_ERROR(clsNode->isCyclicInherited(isCyclicInherited));

				if (isCyclicInherited) {
					if (clsNode->cyclicInheritanceError.hasValue()) {
						SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(std::move(*clsNode->cyclicInheritanceError)));
						clsNode->cyclicInheritanceError.reset();
						continue;
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileEnv, clsNode.template castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Struct: {
				AstNodePtr<StructNode> clsNode = m.template castTo<StructNode>();

				slake::HostObjectRef<slake::StructObject> cls;

				if (!(cls = slake::StructObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				peff::Set<AstNodePtr<InterfaceNode>> involvedInterfaces(compileEnv->allocator.get());

				for (auto &i : clsNode->implTypes) {
					AstNodePtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document->sharedFromThis(), i.template castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->getAstNodeType() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collectInvolvedInterfaces(compileEnv->document, implementedTypeNode.template castTo<InterfaceNode>(), involvedInterfaces, true); e) {
										if (e->errorKind != CompilationErrorKind::CyclicInheritedInterface)
											return e;
									}

									bool isCyclicInherited = false;
									SLKC_RETURN_IF_COMP_ERROR(clsNode->isRecursedType(isCyclicInherited));

									if (isCyclicInherited) {
										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::RecursedStruct)));
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
						if (j->getAstNodeType() == AstNodeType::FnSlot) {
							AstNodePtr<FnNode> method = j.template castTo<FnNode>();

							if (auto it = clsNode->memberIndices.find(j->name); it != clsNode->memberIndices.end()) {
								AstNodePtr<MemberNode> correspondingMember = clsNode->members.at(it.value());

								if (correspondingMember->getAstNodeType() != AstNodeType::FnSlot) {
									for (auto &k : method->overloadings) {
										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));
									}
								} else {
									AstNodePtr<FnNode> correspondingMethod = correspondingMember.template castTo<FnNode>();

									for (auto &k : method->overloadings) {
										bool b = false;

										for (auto &l : correspondingMethod->overloadings) {
											if (k->params.size() != l->params.size()) {
												continue;
											}

											SLKC_RETURN_IF_COMP_ERROR(isFnSignatureSame(k->params.data(), l->params.data(), k->params.size(), {}, {}, b));

											if (b) {
												goto structMatched;
											}
										}

										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, AbstractMethodNotImplementedErrorExData{ k })));

									structMatched:;
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

				SLKC_RETURN_IF_COMP_ERROR(compileModule(compileEnv, clsNode.template castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::FnSlot: {
				AstNodePtr<FnNode> slotNode = m.template castTo<FnNode>();
				slake::HostObjectRef<slake::FnObject> slotObject;

				if (!(slotObject = slake::FnObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (!slotObject->name.build(slotNode->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				for (auto &i : slotNode->overloadings) {
					peff::Option<CompilationError> e;

					for (size_t j = &i - slotNode->overloadings.data() + 1; j < slotNode->overloadings.size(); ++j) {
						bool whether;
						SLKC_RETURN_IF_COMP_ERROR(isFnSignatureDuplicated(i, slotNode->overloadings.at(j), whether));
						if (whether) {
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::FunctionOverloadingDuplicated)));
							break;
						}
					}

					compileEnv->reset();

					switch (mod->getAstNodeType()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
							if (!(i->accessModifier & slake::ACCESS_STATIC)) {
								if (!(compileEnv->thisNode = makeAstNode<ThisNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
									return genOutOfMemoryCompError();
								compileEnv->thisNode->thisType = i->parent->parent->sharedFromThis().template castTo<MemberNode>();
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

					if (!slotObject->overloadings.insert(slake::FnSignature{ fnObject->paramTypes, fnObject->isWithVarArgs(), fnObject->genericParams.size(), slake::TypeId::Void }, fnObject.get())) {
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
