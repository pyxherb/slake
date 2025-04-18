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

					if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
						return genOutOfMemoryCompError();
					}

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, false, obj));

					typeOut = slake::Type(slake::TypeId::Instance, obj.get());
					break;
				}
				case AstNodeType::GenericParam: {
					slake::HostObjectRef<slake::StringObject> obj;

					peff::String s(&compileContext->runtime->globalHeapPoolAlloc);

					if (!s.build(m->name)) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!(obj = slake::StringObject::alloc(compileContext->runtime, std::move(s)))) {
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
			peff::SharedPtr<Document> doc = t->document.lock();

			slake::Type st;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, t->elementType, st));

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(compileContext->runtime, st))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
			}

			typeOut = slake::Type(slake::TypeId::Array, obj.get());
			break;
		}
		case TypeNameKind::Ref: {
			peff::SharedPtr<RefTypeNameNode> t = typeName.castTo<RefTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document.lock();

			slake::Type st;

			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, t->referencedType, st));

			slake::HostObjectRef<slake::TypeDefObject> obj;

			if (!(obj = slake::TypeDefObject::alloc(compileContext->runtime, st))) {
				return genOutOfRuntimeMemoryCompError();
			}

			if (!(compileContext->hostRefHolder.addObject(obj.get()))) {
				return genOutOfMemoryCompError();
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
	CompileContext *compileContext,
	const IdRefEntry *entries,
	size_t nEntries,
	peff::SharedPtr<TypeNameNode> *paramTypes,
	size_t nParams,
	bool hasVarArgs,
	slake::HostObjectRef<slake::IdRefObject> &idRefOut) {
	slake::HostObjectRef<slake::IdRefObject> id;
	if (!(id = slake::IdRefObject::alloc(compileContext->runtime))) {
		return genOutOfRuntimeMemoryCompError();
	}
	if (!id->entries.resizeUninitialized(nEntries)) {
		return genOutOfRuntimeMemoryCompError();
	}

	for (size_t i = 0; i < id->entries.size(); ++i) {
		peff::constructAt<slake::IdRefEntry>(&id->entries.at(i), slake::IdRefEntry(&compileContext->runtime->globalHeapPoolAlloc));
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

	if (!id->paramTypes.resize(nParams)) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < nParams; ++i) {
		SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, paramTypes[i], id->paramTypes.at(i)));
	}

	id->hasVarArgs = hasVarArgs;

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
			peff::String data(&compileContext->runtime->globalHeapPoolAlloc);

			if (!data.build(e->data)) {
				return genOutOfRuntimeMemoryCompError();
			}

			slake::HostObjectRef<slake::StringObject> s;

			if (!(s = slake::StringObject::alloc(compileContext->runtime, std::move(data)))) {
				return genOutOfMemoryCompError();
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

SLKC_API std::optional<CompilationError> slkc::compileModule(
	CompileContext *compileContext,
	peff::SharedPtr<ModuleNode> mod,
	slake::ModuleObject *modOut) {
	std::optional<CompilationError> compilationError;
	for (auto i : mod->anonymousImports) {
		slake::HostObjectRef<slake::IdRefObject> id;

		SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, i->idRef->entries.data(), i->idRef->entries.size(), nullptr, 0, false, id));

		if (!modOut->unnamedImports.pushBack(id.get())) {
			return genOutOfRuntimeMemoryCompError();
		}
	}

	for (auto [k, v] : mod->memberIndices) {
		peff::SharedPtr<MemberNode> m = mod->members.at(v);

		switch (m->astNodeType) {
			case AstNodeType::Class: {
				peff::SharedPtr<ClassNode> clsNode = m.castTo<ClassNode>();

				slake::HostObjectRef<slake::ClassObject> cls;

				if (!(cls = slake::ClassObject::alloc(compileContext->runtime, slake::ScopeUniquePtr(slake::Scope::alloc(&compileContext->runtime->globalHeapPoolAlloc, nullptr)), mod->accessModifier, {}))) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (clsNode->baseType) {
					peff::SharedPtr<MemberNode> baseTypeNode;

					if (clsNode->baseType->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document.lock(), clsNode->baseType.castTo<CustomTypeNameNode>(), baseTypeNode))) {
							if (baseTypeNode) {
								if (baseTypeNode->astNodeType != AstNodeType::Class) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(clsNode->baseType->tokenRange, CompilationErrorKind::ExpectingClassName)));
								}

								bool isCyclicInherited = false;
								SLKC_RETURN_IF_COMP_ERROR(isBaseOf(clsNode->document.lock(), clsNode, clsNode, isCyclicInherited));

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
				}

				for (auto &i : clsNode->implementedTypes) {
					peff::SharedPtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document.lock(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->astNodeType != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
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

				if (!modOut->scope->putMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Interface: {
				peff::SharedPtr<InterfaceNode> clsNode = m.castTo<InterfaceNode>();

				slake::HostObjectRef<slake::InterfaceObject> cls;

				if (!(cls = slake::InterfaceObject::alloc(compileContext->runtime, slake::ScopeUniquePtr(slake::Scope::alloc(&compileContext->runtime->globalHeapPoolAlloc, nullptr)), mod->accessModifier, { &compileContext->runtime->globalHeapPoolAlloc }))) {
					return genOutOfRuntimeMemoryCompError();
				}

				if (!cls->name.build(m->name)) {
					return genOutOfRuntimeMemoryCompError();
				}

				for (auto &i : clsNode->implementedTypes) {
					peff::SharedPtr<MemberNode> implementedTypeNode;

					if (i->typeNameKind == TypeNameKind::Custom) {
						if (!(compilationError = resolveCustomTypeName(clsNode->document.lock(), i.castTo<CustomTypeNameNode>(), implementedTypeNode))) {
							if (implementedTypeNode) {
								if (implementedTypeNode->astNodeType != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->tokenRange, CompilationErrorKind::ExpectingInterfaceName)));
								}
								bool isCyclicInherited = false;
								SLKC_RETURN_IF_COMP_ERROR(isImplementedByInterface(clsNode->document.lock(), clsNode, clsNode, isCyclicInherited));

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

				if (!modOut->scope->putMember(cls.get())) {
					return genOutOfRuntimeMemoryCompError();
				}

				break;
			}
			case AstNodeType::Import: {
				peff::String s(&compileContext->runtime->globalHeapPoolAlloc);

				if (!s.build(k)) {
					return genOutOfRuntimeMemoryCompError();
				}

				slake::HostObjectRef<slake::IdRefObject> id;
				peff::SharedPtr<ImportNode> importNode = m.castTo<ImportNode>();

				SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, importNode->idRef->entries.data(), importNode->idRef->entries.size(), nullptr, 0, false, id));

				if (!modOut->imports.insert(std::move(s), id.get())) {
					return genOutOfMemoryCompError();
				}
				break;
			}
			case AstNodeType::FnSlot: {
				peff::SharedPtr<FnSlotNode> slotNode = m.castTo<FnSlotNode>();

				for (auto i : slotNode->overloadings) {
					compileContext->fnCompileContext.reset();

					peff::SharedPtr<BlockCompileContext> blockContext;
					if (!(blockContext = peff::makeShared<BlockCompileContext>(compileContext->allocator.get(), compileContext->allocator.get())))
						return genOutOfMemoryCompError();
					if (!compileContext->fnCompileContext.blockCompileContexts.pushBack(std::move(blockContext)))
						return genOutOfMemoryCompError();

					compileContext->fnCompileContext.currentFn = i;

					std::optional<CompilationError> e;
					for (auto j : i->body->body) {
						if ((e = compileStmt(compileContext, j))) {
							if (e->errorKind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compileContext->errors.pushBack(std::move(*e))) {
								return genOutOfMemoryCompError();
							}
							e.reset();
						}
					}
				}
				break;
			}
			default:
				break;
		}
	}
	return {};
}
