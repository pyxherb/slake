#include "../compiler.h"

#undef max

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
		case TypeNameKind::BCCustom: {
			AstNodePtr<bc::BCCustomTypeNameNode> t = typeName.castTo<bc::BCCustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();

			slake::HostObjectRef<slake::CustomTypeDefObject> typeDef;

			if (!(typeDef = slake::CustomTypeDefObject::alloc(compileEnv->runtime))) {
				return genOutOfRuntimeMemoryCompError();
			}

			slake::HostObjectRef<slake::IdRefObject> obj;

			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, t->idRefPtr->entries.data(), t->idRefPtr->entries.size(), nullptr, 0, false, {}, obj));

			if (!(compileEnv->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeDef->typeObject = obj.get();

			typeOut = slake::TypeRef(slake::TypeId::Instance, typeDef.get());
			break;
		}
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode> t = typeName.castTo<CustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->sharedFromThis();
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileEnv, doc, t, m));

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
				case AstNodeType::ScopedEnum: {
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

					typeOut = slake::TypeRef(slake::TypeId::ScopedEnum, typeDef.get());
					break;
				}
				case AstNodeType::UnionEnum: {
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

					typeOut = slake::TypeRef(slake::TypeId::UnionEnum, typeDef.get());
					break;
				}
				case AstNodeType::UnionEnumItem: {
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

					typeOut = slake::TypeRef(slake::TypeId::UnionEnumItem, typeDef.get());
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
			slake::Value value;
			if (ce.genericArgs.at(i)->getAstNodeType() == AstNodeType::Expr) {
				SLKC_RETURN_IF_COMP_ERROR(compileValueExpr(compileEnv, compilationContext, ce.genericArgs.at(i).castTo<ExprNode>(), value));
			} else {
				slake::TypeRef typeRef;
				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, ce.genericArgs.at(i).castTo<TypeNameNode>(), typeRef));
				assert(typeRef.typeId != slake::TypeId::Invalid);
				value = typeRef;
			}
			assert(value != slake::ValueType::Invalid);
			e.genericArgs.at(i) = value;
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

			slake::Reference entityRef = slake::Reference::makeObjectRef(s.get());

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

			slake::Reference entityRef = slake::Reference::makeObjectRef(nullptr);

			valueOut = slake::Value(entityRef);
			break;
		}
		case ExprKind::RegIndex: {
			AstNodePtr<RegIndexExprNode> e = expr.castTo<RegIndexExprNode>();

			valueOut = slake::Value(slake::ValueType::RegIndex, e->reg);
			break;
		}
		case ExprKind::BCLabel: {
			AstNodePtr<bc::BCLabelExprNode> e = expr.castTo<bc::BCLabelExprNode>();

			valueOut = slake::Value(compilationContext->getLabelOffset(*(compilationContext->getLabelIndexByName(e->name))));
			break;
		}
		case ExprKind::TypeName: {
			AstNodePtr<TypeNameExprNode> e = expr.castTo<TypeNameExprNode>();

			slake::TypeRef t;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, e->type, t));

			valueOut = slake::Value(t);
			break;
		}
		case ExprKind::InitializerList:
			return CompilationError(expr->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);
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

		if (gpNode->inputType) {
			// TODO: Detect if the input type is compatible with the compile-time expressions.
			if ((e = compileTypeName(compileEnv, compilationContext, gpNode->inputType, gp.inputType))) {
				if (e->errorKind == CompilationErrorKind::OutOfMemory)
					return e;
				if (!compileEnv->errors.pushBack(std::move(*e))) {
					return genOutOfMemoryCompError();
				}
				e.reset();
			}
		} else {
			if (gpNode->isParamTypeList) {
				// TODO: Implement it.
			} else if (gpNode->genericConstraint) {
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
		}

		if (!genericParamListOut.pushBack(std::move(gp))) {
			return genOutOfRuntimeMemoryCompError();
		}
	}

	return {};
}

#define MNEMONIC_NAME_CASE(name)    \
	if (sv == #name) {              \
		return slake::Opcode::name; \
	} else {                        \
	}

SLKC_API peff::Option<slake::Opcode> _getOpcode(const std::string_view &sv) {
	MNEMONIC_NAME_CASE(INVALID);
	MNEMONIC_NAME_CASE(LOAD);
	MNEMONIC_NAME_CASE(RLOAD);
	MNEMONIC_NAME_CASE(STORE);
	MNEMONIC_NAME_CASE(MOV);
	MNEMONIC_NAME_CASE(LARG);
	MNEMONIC_NAME_CASE(LAPARG);
	MNEMONIC_NAME_CASE(LVAR);
	MNEMONIC_NAME_CASE(ALLOCA);
	MNEMONIC_NAME_CASE(LVALUE);
	MNEMONIC_NAME_CASE(ENTER);
	MNEMONIC_NAME_CASE(LEAVE);
	MNEMONIC_NAME_CASE(ADD);
	MNEMONIC_NAME_CASE(SUB);
	MNEMONIC_NAME_CASE(MUL);
	MNEMONIC_NAME_CASE(DIV);
	MNEMONIC_NAME_CASE(MOD);
	MNEMONIC_NAME_CASE(AND);
	MNEMONIC_NAME_CASE(OR);
	MNEMONIC_NAME_CASE(XOR);
	MNEMONIC_NAME_CASE(LAND);
	MNEMONIC_NAME_CASE(LOR);
	MNEMONIC_NAME_CASE(EQ);
	MNEMONIC_NAME_CASE(NEQ);
	MNEMONIC_NAME_CASE(LT);
	MNEMONIC_NAME_CASE(GT);
	MNEMONIC_NAME_CASE(LTEQ);
	MNEMONIC_NAME_CASE(GTEQ;)
	MNEMONIC_NAME_CASE(LSH);
	MNEMONIC_NAME_CASE(RSH);
	MNEMONIC_NAME_CASE(CMP);
	MNEMONIC_NAME_CASE(NOT);
	MNEMONIC_NAME_CASE(LNOT);
	MNEMONIC_NAME_CASE(NEG);
	MNEMONIC_NAME_CASE(AT);
	MNEMONIC_NAME_CASE(JMP);
	MNEMONIC_NAME_CASE(BR);
	MNEMONIC_NAME_CASE(PUSHARG);
	MNEMONIC_NAME_CASE(PUSHAP);
	MNEMONIC_NAME_CASE(CALL);
	MNEMONIC_NAME_CASE(MCALL);
	MNEMONIC_NAME_CASE(CTORCALL);
	MNEMONIC_NAME_CASE(RET);
	MNEMONIC_NAME_CASE(COCALL);
	MNEMONIC_NAME_CASE(COMCALL);
	MNEMONIC_NAME_CASE(YIELD);
	MNEMONIC_NAME_CASE(RESUME);
	MNEMONIC_NAME_CASE(CODONE);
	MNEMONIC_NAME_CASE(LTHIS);
	MNEMONIC_NAME_CASE(NEW);
	MNEMONIC_NAME_CASE(ARRNEW);
	MNEMONIC_NAME_CASE(THROW);
	MNEMONIC_NAME_CASE(PUSHEH);
	MNEMONIC_NAME_CASE(LEXCEPT);
	MNEMONIC_NAME_CASE(CAST);
	MNEMONIC_NAME_CASE(APTOTUPLE);
	MNEMONIC_NAME_CASE(TYPEOF);
	MNEMONIC_NAME_CASE(CONSTSW);
	MNEMONIC_NAME_CASE(PHI);
	return {};
}
#undef MNEMONIC_NAME_CASE

SLKC_API peff::Option<CompilationError> slkc::compileModuleLikeNode(
	CompileEnvironment *compileEnv,
	AstNodePtr<ModuleNode> mod,
	slake::BasicModuleObject *modOut) {
	peff::OneshotScopeGuard restoreCurParentAccessNodeGuard([compileEnv, oldNode = compileEnv->curParentAccessNode]() noexcept {
		compileEnv->curParentAccessNode = oldNode;
	});
	compileEnv->curParentAccessNode = mod;

	peff::ScopeGuard restoreCurModuleGuard([compileEnv, oldModule = compileEnv->curModule]() noexcept {
		compileEnv->curModule = oldModule;
	});
	if (mod->getAstNodeType() == AstNodeType::Module)
		compileEnv->curModule = mod;
	else
		restoreCurModuleGuard.release();

	peff::Option<CompilationError> compilationError;
	if (modOut->getObjectKind() == slake::ObjectKind::Module) {
		for (auto i : mod->anonymousImports) {
			NormalCompilationContext compilationContext(compileEnv, nullptr);

			slake::HostObjectRef<slake::IdRefObject> id;

			for (auto &j : compileEnv->document->externalModuleProviders) {
				SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileEnv, i->idRef.get()));
			}

			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, &compilationContext, i->idRef->entries.data(), i->idRef->entries.size(), nullptr, 0, false, {}, id));

			if (!((slake::ModuleObject *)modOut)->unnamedImports.pushBack(id.get()))
				return genOutOfRuntimeMemoryCompError();
		}
	}

	for (auto [k, v] : mod->memberIndices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		if (m->getAstNodeType() == AstNodeType::Import) {
			AstNodePtr<ImportNode> importNode = m.castTo<ImportNode>();

			for (auto &j : compileEnv->document->externalModuleProviders) {
				SLKC_RETURN_IF_COMP_ERROR(j->loadModule(compileEnv, importNode->idRef.get()));
			}

			NormalCompilationContext compilationContext(compileEnv, nullptr);

			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, &compilationContext, importNode->idRef->entries.data(), importNode->idRef->entries.size(), nullptr, 0, false, {}, id));

			if (modOut->getObjectKind() == slake::ObjectKind::Module) {
				if (!((slake::ModuleObject *)modOut)->unnamedImports.pushBack(id.get()))
					return genOutOfMemoryCompError();
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(indexModuleVarMembers(compileEnv, compileEnv->document->rootModule));

	for (auto [k, v] : mod->memberIndices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		NormalCompilationContext compilationContext(compileEnv, nullptr);

		switch (m->getAstNodeType()) {
			case AstNodeType::BCFn: {
				AstNodePtr<bc::BCFnNode> slotNode = m.castTo<bc::BCFnNode>();
				slake::HostObjectRef<slake::FnObject> slotObject;

				if (!(slotObject = slake::FnObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (!slotObject->setName(slotNode->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				for (auto &i : slotNode->overloadings) {
					peff::Option<CompilationError> e;

					compileEnv->reset();

					switch (mod->getAstNodeType()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
							if (!(i->accessModifier & slake::ACCESS_STATIC)) {
								if (!(compileEnv->thisNode = makeAstNode<ThisNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
									return genOutOfMemoryCompError();
								compileEnv->thisNode->thisType = mod->sharedFromThis().castTo<MemberNode>();
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

					uint32_t maxReg = UINT32_MAX;
					NormalCompilationContext compContext(compileEnv, nullptr);
					{
						size_t k = 0;
						for (auto j : i->body) {
							switch (j->stmtKind) {
								case bc::BCStmtKind::Instruction:
									++k;
									break;
								case bc::BCStmtKind::Label: {
									AstNodePtr<bc::LabelBCStmtNode> ins = j.castTo<bc::LabelBCStmtNode>();

									if (auto it = compContext.labelNameIndices.find(ins->name); it != compContext.labelNameIndices.end())
										compContext.setLabelOffset(it.value(), k);
									else {
										uint32_t labelId;
										SLKC_RETURN_IF_COMP_ERROR(compContext.allocLabel(labelId));
										SLKC_RETURN_IF_COMP_ERROR(compContext.setLabelName(labelId, ins->name));
										compContext.setLabelOffset(labelId, k);
									}
									break;
								}
								default:
									std::terminate();
							}
						}
						k = 0;
						for (auto j : i->body) {
							switch (j->stmtKind) {
								case bc::BCStmtKind::Instruction: {
									++k;
									AstNodePtr<bc::InstructionBCStmtNode> ins = j.castTo<bc::InstructionBCStmtNode>();

									uint32_t sldIndex;
									SLKC_RETURN_IF_COMP_ERROR(compContext.registerSourceLocDesc(slake::slxfmt::SourceLocDesc{ ins->line, ins->column }, sldIndex));

									peff::Option<slake::Opcode> opcodeResult = _getOpcode(ins->mnemonic);
									if (!opcodeResult.hasValue())
										return CompilationError(ins->tokenRange, CompilationErrorKind::InvalidMnemonic);
									slake::Opcode opcode = *opcodeResult;
									peff::DynArray<slake::Value> operands(compileEnv->allocator.get());

									if (!operands.resize(ins->operands.size()))
										return genOutOfMemoryCompError();

									for (size_t k = 0; k < operands.size(); ++k) {
										SLKC_RETURN_IF_COMP_ERROR(compileValueExpr(compileEnv, &compContext, ins->operands.at(k), operands.at(k)));
									}

									SLKC_RETURN_IF_COMP_ERROR(compContext.emitIns(sldIndex, opcode, ins->regOut, operands.data(), operands.size()));
									break;
								}
								case bc::BCStmtKind::Label:
									break;
								default:
									std::terminate();
							}
						}
					}
					if (!fnObject->instructions.resize(compContext.generatedInstructions.size())) {
						return genOutOfRuntimeMemoryCompError();
					}
					for (size_t i = 0; i < compContext.generatedInstructions.size(); ++i) {
						fnObject->instructions.at(i) = std::move(compContext.generatedInstructions.at(i));
					}
					compContext.generatedInstructions.clear();

					if (!fnObject->sourceLocDescs.resize(compContext.sourceLocDescs.size()))
						return genOutOfMemoryCompError();
					memcpy(fnObject->sourceLocDescs.data(), compContext.sourceLocDescs.data(), compContext.sourceLocDescs.size() * sizeof(slake::slxfmt::SourceLocDesc));
					compContext.sourceLocDescs.clear();
					compContext.sourceLocDescsMap.clear();

					for (auto &j : fnObject->instructions) {
						for (size_t k = 0; k < j.nOperands; ++k) {
							if (j.operands[k].valueType == slake::ValueType::RegIndex) {
								if (maxReg == UINT32_MAX)
									maxReg = j.operands[k].getRegIndex();
								else if (j.operands[k].getRegIndex() != UINT32_MAX) {
									maxReg = std::max(maxReg, j.operands[k].getRegIndex());
								}
							}
						}
					}

					if (maxReg != UINT32_MAX)
						fnObject->nRegisters = maxReg + 1;
					else
						fnObject->nRegisters = 0;

					if (!slotObject->overloadings.insert(slake::FnSignature{ fnObject->paramTypes, fnObject->isWithVarArgs(), fnObject->genericParams.size(), slake::TypeId::Void }, fnObject.get())) {
						return genOutOfRuntimeMemoryCompError();
					}
				}

				if (!modOut->addMember(slotObject.get())) {
					return genOutOfRuntimeMemoryCompError();
				}
				break;
			}
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

				switch (type.typeId) {
					case slake::TypeId::Any:
					case slake::TypeId::I8:
					case slake::TypeId::I16:
					case slake::TypeId::I32:
					case slake::TypeId::I64:
					case slake::TypeId::ISize:
					case slake::TypeId::U8:
					case slake::TypeId::U16:
					case slake::TypeId::U32:
					case slake::TypeId::U64:
					case slake::TypeId::USize:
					case slake::TypeId::F32:
					case slake::TypeId::F64:
					case slake::TypeId::Bool:
					case slake::TypeId::String:
					case slake::TypeId::Instance:
					case slake::TypeId::Array:
					case slake::TypeId::Tuple:
					case slake::TypeId::SIMD:
					case slake::TypeId::Fn: {
						slake::Value defaultValue;
						if (varNode->initialValue)
							SLKC_RETURN_IF_COMP_ERROR(compileValueExpr(compileEnv, &compilationContext, varNode->initialValue, defaultValue));
						else
							defaultValue = modOut->associatedRuntime->defaultValueOf(type);
						if (!modOut->appendFieldRecord(std::move(fr))) {
							return genOutOfRuntimeMemoryCompError();
						}
						modOut->associatedRuntime->writeVar(slake::Reference::makeStaticFieldRef(modOut, modOut->fieldRecords.size() - 1), defaultValue).unwrap();
						break;
					}
					case slake::TypeId::StructInstance:
					case slake::TypeId::ScopedEnum:
					case slake::TypeId::UnionEnum:
					case slake::TypeId::UnionEnumItem:
						if (!modOut->appendFieldRecordWithoutAlloc(std::move(fr))) {
							return genOutOfRuntimeMemoryCompError();
						}
						// Note that we don't allocate space for types which
						// may have not been compiled, which means we cannot
						// or hard to evaluate their size.
						break;
					case slake::TypeId::GenericArg:
					case slake::TypeId::Ref:
					case slake::TypeId::TempRef:
						if (varNode->initialValue)
							return CompilationError(varNode->initialValue->tokenRange, CompilationErrorKind::TypeIsNotInitializable);
						if (!modOut->appendFieldRecord(std::move(fr))) {
							return genOutOfRuntimeMemoryCompError();
						}
						break;
					case slake::TypeId::ParamTypeList:
					case slake::TypeId::Unpacking:
					case slake::TypeId::Unknown:
					default:
						std::terminate();
				}

				break;
			}
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> clsNode = m.castTo<ClassNode>();

				slake::HostObjectRef<slake::ClassObject> cls;

				if (!(cls = slake::ClassObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->setName(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				if (clsNode->baseType) {
					AstNodePtr<MemberNode> baseTypeNode;

					if (clsNode->baseType->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(compileEnv, clsNode->document->sharedFromThis(), clsNode->baseType.castTo<CustomTypeNameNode>(), baseTypeNode))) {
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

					bool isCyclicInherited = false;
					SLKC_RETURN_IF_COMP_ERROR(clsNode->isCyclicInherited(isCyclicInherited));

					if (isCyclicInherited) {
						SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::CyclicInheritedClass)));
						continue;
					}

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, &compilationContext, clsNode->baseType, cls->baseType));
				}

				peff::Set<AstNodePtr<InterfaceNode>> involvedInterfaces(compileEnv->allocator.get());

				for (auto &i : clsNode->implTypes) {
					AstNodePtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(compileEnv, clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->getAstNodeType() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collectInvolvedInterfaces(compileEnv->document, implementedTypeNode.castTo<InterfaceNode>(), involvedInterfaces, true); e) {
										if (e->errorKind != CompilationErrorKind::CyclicInheritedInterface)
											return e;
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
							if (lhsMember->getAstNodeType() == AstNodeType::Fn) {
								AstNodePtr<FnNode> lhs = lhsMember.castTo<FnNode>();

								if (auto rhsMember = rhsParent->memberIndices.find(lhsMember->name); rhsMember != rhsParent->memberIndices.end()) {
									AstNodePtr<MemberNode> correspondingMember = rhsParent->members.at(rhsMember.value());

									if (correspondingMember->getAstNodeType() != AstNodeType::Fn) {
										// Corresponding member should not be not a function.
										std::terminate();
									}
									AstNodePtr<FnNode> rhs = correspondingMember.castTo<FnNode>();

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

												if (correspondingOverridenMember->getAstNodeType() == AstNodeType::Fn) {
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

														SLKC_RETURN_IF_COMP_ERROR(
															isFnSignatureSame(
																curLhsOverloading->params.data(),
																curOverridenOverloading->params.data(),
																curLhsOverloading->params.size(),
																*rhsIt,
																curOverridenOverloading->overridenType,
																overridenWhether));
														// Specialized overriden, continue.
														if (overridenWhether) {
															b = false;
															goto checkInterfaceMethodsConflicted;
														}
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

				if (!conflictedInterfacesDetected) {
					for (auto &i : involvedInterfaces) {
						for (auto &j : i->members) {
							if (j->getAstNodeType() == AstNodeType::Fn) {
								AstNodePtr<FnNode> method = j.castTo<FnNode>();

								if (auto it = clsNode->memberIndices.find(j->name); it != clsNode->memberIndices.end()) {
									AstNodePtr<MemberNode> correspondingMember = clsNode->members.at(it.value());

									if (correspondingMember->getAstNodeType() != AstNodeType::Fn) {
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

				SLKC_RETURN_IF_COMP_ERROR(compileModuleLikeNode(compileEnv, clsNode.castTo<ModuleNode>(), cls.get()));

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

				if (!cls->setName(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				for (auto &i : clsNode->implTypes) {
					AstNodePtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(compileEnv, clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
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

				SLKC_RETURN_IF_COMP_ERROR(compileModuleLikeNode(compileEnv, clsNode.castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::ScopedEnum: {
				AstNodePtr<ScopedEnumNode> clsNode = m.castTo<ScopedEnumNode>();

				slake::TypeRef baseType = slake::TypeId::Invalid;

				if (clsNode->baseType) {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isScopedEnumBaseType(clsNode->baseType, b));

					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(
							CompilationError(
								clsNode->baseType->tokenRange,
								CompilationErrorKind::InvalidEnumBaseType)));
					}

					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, &compilationContext, clsNode->baseType, baseType));
				}

				SLKC_RETURN_IF_COMP_ERROR(fillScopedEnum(compileEnv, &compilationContext, clsNode));

				slake::HostObjectRef<slake::ScopedEnumObject> cls;

				if (!(cls = slake::ScopedEnumObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->setName(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->baseType = baseType;

				for (auto i : clsNode->members) {
					switch (i->getAstNodeType()) {
						case AstNodeType::EnumItem: {
							AstNodePtr<EnumItemNode> itemNode = i.castTo<EnumItemNode>();

							slake::FieldRecord fr(compileEnv->runtime->getCurGenAlloc());

							if (!fr.name.build(k)) {
								return genOutOfRuntimeMemoryCompError();
							}

							fr.accessModifier = m->accessModifier;
							fr.offset = modOut->localFieldStorage.size();

							fr.type = baseType;

							if (baseType != slake::TypeId::Invalid) {
								slake::Value itemValue;

								AstNodePtr<ExprNode> enumValue;
								AstNodePtr<TypeNameNode> enumValueType;

								assert(itemNode->filledValue);

								SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, &compilationContext, itemNode->filledValue, enumValue));

								if (!enumValue)
									return CompilationError(itemNode->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);
								SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, &compilationContext, itemNode->filledValue, enumValue));

								SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, &compilationContext, enumValue, enumValueType, clsNode->baseType));

								bool isSame;
								SLKC_RETURN_IF_COMP_ERROR(isSameType(enumValueType, clsNode->baseType, isSame));

								if (!isSame) {
									AstNodePtr<CastExprNode> castExpr;

									if (!(castExpr = makeAstNode<CastExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
										return genOutOfMemoryCompError();

									castExpr->targetType = clsNode->baseType;
									castExpr->source = enumValue;
									castExpr->tokenRange = itemNode->filledValue->tokenRange;

									SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, &compilationContext, castExpr.castTo<ExprNode>(), enumValue));
									if (!enumValue)
										SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(
											CompilationError(
												itemNode->filledValue->tokenRange,
												CompilationErrorKind::IncompatibleInitialValueType)));
								}

								SLKC_RETURN_IF_COMP_ERROR(compileValueExpr(compileEnv, &compilationContext, enumValue, itemValue));

								if (!modOut->appendFieldRecord(std::move(fr))) {
									return genOutOfRuntimeMemoryCompError();
								}
								modOut->associatedRuntime->writeVar(slake::Reference::makeStaticFieldRef(modOut, modOut->fieldRecords.size() - 1), itemValue).unwrap();
							} else {
								if (itemNode->filledValue)
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(
										CompilationError(
											itemNode->tokenRange,
											CompilationErrorKind::EnumItemIsNotAssignable)));
								if (!modOut->appendFieldRecordWithoutAlloc(std::move(fr))) {
									return genOutOfRuntimeMemoryCompError();
								}
							}

							break;
						}
						default:
							std::terminate();
					}
				}

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::UnionEnum: {
				AstNodePtr<UnionEnumNode> clsNode = m.castTo<UnionEnumNode>();

				bool isCyclicInherited = false;
				SLKC_RETURN_IF_COMP_ERROR(clsNode->isRecursedType(isCyclicInherited));

				if (isCyclicInherited) {
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(
						CompilationError(
							clsNode->tokenRange,
							CompilationErrorKind::RecursedValueType)));
				}

				slake::HostObjectRef<slake::UnionEnumObject> cls;

				if (!(cls = slake::UnionEnumObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->setName(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				for (auto i : clsNode->members) {
					switch (i->getAstNodeType()) {
						case AstNodeType::UnionEnumItem: {
							AstNodePtr<UnionEnumItemNode> itemNode = i.castTo<UnionEnumItemNode>();

							slake::HostObjectRef<slake::UnionEnumItemObject> item;

							if (!(item = slake::UnionEnumItemObject::alloc(compileEnv->runtime))) {
								return genOutOfRuntimeMemoryCompError();
							}

							item->setAccess(mod->accessModifier);

							if (!item->setName(itemNode->name)) {
								return genOutOfRuntimeMemoryCompError();
							}

							SLKC_RETURN_IF_COMP_ERROR(compileModuleLikeNode(compileEnv, itemNode.castTo<ModuleNode>(), item.get()));

							if (!cls->addMember(item.get())) {
								return genOutOfRuntimeMemoryCompError();
							}

							break;
						}
						default:
							std::terminate();
					}
				}

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Struct: {
				AstNodePtr<StructNode> clsNode = m.castTo<StructNode>();

				slake::HostObjectRef<slake::StructObject> cls;

				if (!(cls = slake::StructObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				cls->setAccess(mod->accessModifier);

				if (!cls->setName(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileGenericParams(compileEnv, &compilationContext, mod, clsNode->genericParams.data(), clsNode->genericParams.size(), cls->genericParams));

				bool isCyclicInherited = false;
				SLKC_RETURN_IF_COMP_ERROR(clsNode->isRecursedType(isCyclicInherited));

				if (isCyclicInherited) {
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(clsNode->tokenRange, CompilationErrorKind::RecursedValueType)));
					continue;
				}

				peff::Set<AstNodePtr<InterfaceNode>> involvedInterfaces(compileEnv->allocator.get());

				for (auto &i : clsNode->implTypes) {
					AstNodePtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(compileEnv, clsNode->document->sharedFromThis(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->getAstNodeType() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collectInvolvedInterfaces(compileEnv->document, implementedTypeNode.castTo<InterfaceNode>(), involvedInterfaces, true); e) {
										if (e->errorKind != CompilationErrorKind::CyclicInheritedInterface)
											return e;
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
						if (j->getAstNodeType() == AstNodeType::Fn) {
							AstNodePtr<FnNode> method = j.castTo<FnNode>();

							if (auto it = clsNode->memberIndices.find(j->name); it != clsNode->memberIndices.end()) {
								AstNodePtr<MemberNode> correspondingMember = clsNode->members.at(it.value());

								if (correspondingMember->getAstNodeType() != AstNodeType::Fn) {
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

				SLKC_RETURN_IF_COMP_ERROR(compileModuleLikeNode(compileEnv, clsNode.castTo<ModuleNode>(), cls.get()));

				if (!modOut->addMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Fn: {
				AstNodePtr<FnNode> slotNode = m.castTo<FnNode>();
				slake::HostObjectRef<slake::FnObject> slotObject;

				if (!(slotObject = slake::FnObject::alloc(compileEnv->runtime))) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (!slotObject->setName(slotNode->name)) {
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
								compileEnv->thisNode->thisType = mod->sharedFromThis().castTo<MemberNode>();
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

						if (!fnObject->sourceLocDescs.resize(compContext.sourceLocDescs.size()))
							return genOutOfMemoryCompError();
						memcpy(fnObject->sourceLocDescs.data(), compContext.sourceLocDescs.data(), compContext.sourceLocDescs.size() * sizeof(slake::slxfmt::SourceLocDesc));
						compContext.sourceLocDescs.clear();
						compContext.sourceLocDescsMap.clear();

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
