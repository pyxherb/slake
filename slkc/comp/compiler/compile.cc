#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileTypeName(
	slake::Runtime *runtime,
	slake::HostRefHolder &hostRefHolder,
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
			peff::SharedPtr<Document> doc = t->document.lock();
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

					if (!(hostRefHolder.addObject(obj.get()))) {
						return genOutOfRuntimeMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(runtime, hostRefHolder, fullName->entries.data(), fullName->entries.size(), nullptr, 0, false, obj));

					typeOut = slake::Type(slake::TypeId::Instance, obj.get());
					break;
				}
				case AstNodeType::GenericParam: {
					slake::HostObjectRef<slake::StringObject> obj;

					peff::String s(&runtime->globalHeapPoolAlloc);

					if (!s.build(m->name)) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!(obj = slake::StringObject::alloc(runtime, std::move(s)))) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!(hostRefHolder.addObject(obj.get()))) {
						return genOutOfRuntimeMemoryCompError();
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
			peff::SharedPtr<Document> doc = t->document.lock();

			slake::Type st;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, t->elementType, st));

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(runtime, st))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!(hostRefHolder.addObject(obj.get()))) {
				return genOutOfRuntimeMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Array, obj.get());
			break;
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode> t = typeName.castTo<RefTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document.lock();

			slake::Type st;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, t->referencedType, st));

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(runtime, st))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!(hostRefHolder.addObject(obj.get()))) {
				return genOutOfRuntimeMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Ref, obj.get());
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileIdRef(
	slake::Runtime *runtime,
	slake::HostRefHolder &hostRefHolder,
	const IdRefEntry *entries,
	size_t nEntries,
	peff::SharedPtr<TypeNameNode> *paramTypes,
	size_t nParams,
	bool hasVarArgs,
	slake::HostObjectRef<slake::IdRefObject> &idRefOut) {
	slake::HostObjectRef<slake::IdRefObject> id;
	if (!(id = slake::IdRefObject::alloc(runtime))) {
		return genOutOfRuntimeMemoryCompError();
	}
	if (!id->entries.resizeUninitialized(nEntries)) {
		return genOutOfRuntimeMemoryCompError();
	}

	for (size_t i = 0; i < id->entries.size(); ++i) {
		peff::constructAt<slake::IdRefEntry>(&id->entries.at(i), slake::IdRefEntry(&runtime->globalHeapPoolAlloc));
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
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, ce.genericArgs.at(i), e.genericArgs.at(i)));
		}
	}

	if (!id->paramTypes.resize(nParams)) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < nParams; ++i) {
		SLKC_RETURN_IF_COMP_ERROR(compileTypeName(runtime, hostRefHolder, paramTypes[i], id->paramTypes.at(i)));
	}

	id->hasVarArgs = hasVarArgs;

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileValueExpr(
	slake::Runtime *runtime,
	slake::HostRefHolder &hostRefHolder,
	peff::SharedPtr<ExprNode> expr,
	slake::Value &valueOut) {
	switch (expr->exprKind) {
		case ExprKind::IdRef: {
			peff::SharedPtr<IdRefExprNode> e = expr.castTo<IdRefExprNode>();
			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(
				compileIdRef(
					runtime,
					hostRefHolder,
					e->idRefPtr->entries.data(),
					e->idRefPtr->entries.size(),
					nullptr,
					0,
					false,
					id));

			if (!hostRefHolder.addObject(id.get())) {
				return genOutOfRuntimeMemoryCompError();
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
			peff::String data(&runtime->globalHeapPoolAlloc);

			if (!data.build(e->data)) {
				return genOutOfRuntimeMemoryCompError();
			}

			slake::HostObjectRef<slake::StringObject> s;

			if (!(s = slake::StringObject::alloc(runtime, std::move(data)))) {
				return genOutOfMemoryCompError();
			}

			if (!hostRefHolder.addObject(s.get())) {
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
