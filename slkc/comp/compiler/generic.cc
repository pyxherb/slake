#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> TopLevelCompileContext::lookupGenericCacheTable(
	peff::SharedPtr<MemberNode> originalObject,
	GenericCacheTable *&tableOut) {
	if (auto it = genericCacheDir.find(originalObject); it != genericCacheDir.end()) {
		tableOut = &it.value();
		return {};
	}
	tableOut = nullptr;
	return {};
}

SLKC_API std::optional<CompilationError> TopLevelCompileContext::lookupGenericCache(
	peff::SharedPtr<MemberNode> originalObject,
	const peff::DynArray<peff::SharedPtr<TypeNameNode>> &genericArgs,
	peff::SharedPtr<MemberNode> &memberOut) const {
	const GenericCacheTable *tab;

	SLKC_RETURN_IF_COMP_ERROR(lookupGenericCacheTable(originalObject, tab));

	if (tab) {
		if (auto it = tab->find(genericArgs); it != tab->endConst()) {
			memberOut = it.value();
			return {};
		}
	}

	memberOut = {};
	return {};
}

static std::optional<CompilationError> _walkTypeNameForGenericInstantiation(
	peff::SharedPtr<TypeNameNode> &typeName,
	const GenericInstantiationContext &context) {
	if (!typeName) {
		return {};
	}

	if (typeName->typeNameKind == TypeNameKind::Custom) {
		peff::SharedPtr<CustomTypeNameNode> tn = typeName.castTo<CustomTypeNameNode>();

		if (tn->idRefPtr->entries.size() == 1) {
			IdRefEntry &entry = tn->idRefPtr->entries.at(0);

			if (!entry.genericArgs.size()) {
				if (auto it = context.mappedGenericArgs.find(entry.name);
					it != context.mappedGenericArgs.end()) {
					typeName = it.value();
				}
			}
		}
	}

	return {};
}

static std::optional<CompilationError> _walkNodeForGenericInstantiation(
	peff::SharedPtr<MemberNode> astNode,
	const GenericInstantiationContext &context) {
	if (!astNode) {
		return {};
	}

	switch (astNode->astNodeType) {
		case AstNodeType::FnSlot: {
			peff::SharedPtr<FnSlotNode> fnSlot = astNode.castTo<FnSlotNode>();

			for (auto i : fnSlot->overloadings) {
				for (auto j : i->genericParams) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.castTo<MemberNode>(), context));
				}

				if ((context.mappedNode != astNode) && (i->genericParams.size())) {
					GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

					for (auto &[k, v] : context.mappedGenericArgs) {
						if (auto it = i->genericParamIndices.find(k);
							it == i->genericParamIndices.end()) {
							if (!innerContext.mappedGenericArgs.insert(std::string_view(k), peff::SharedPtr<TypeNameNode>(v))) {
								return genOutOfMemoryCompError();
							}
						}
					}

					for (auto &j : i->params) {
						SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(j->type, innerContext));
					}

					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(i->returnType, innerContext));

					// No need to substitute the function body, we just care about the declaration.
				} else {
					for (auto &j : i->params) {
						SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(j->type, context));
					}

					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(i->returnType, context));
				}
			}
			break;
		}
		case AstNodeType::Var: {
			peff::SharedPtr<VarNode> varNode = astNode.castTo<VarNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(varNode->type, context));
			break;
		}
		case AstNodeType::Class: {
			peff::SharedPtr<ClassNode> cls = astNode.castTo<ClassNode>();

			for (auto j : cls->genericParams) {
				SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.castTo<MemberNode>(), context));
			}

			if ((context.mappedNode != astNode) && (cls->genericParams.size())) {
				GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

				for (auto &[k, v] : context.mappedGenericArgs) {
					if (auto it = cls->genericParamIndices.find(k);
						it == cls->genericParamIndices.end()) {
						if (!innerContext.mappedGenericArgs.insert(std::string_view(k), peff::SharedPtr<TypeNameNode>(v))) {
							return genOutOfMemoryCompError();
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->baseType, innerContext));

				for (auto &k : cls->implementedTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, innerContext));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.second, innerContext));
				}
			} else {
				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->baseType, context));

				for (auto &k : cls->implementedTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, context));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.second, context));
				}
			}
			break;
		}
		case AstNodeType::Interface: {
			peff::SharedPtr<InterfaceNode> cls = astNode.castTo<InterfaceNode>();

			for (auto j : cls->genericParams) {
				SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.castTo<MemberNode>(), context));
			}

			if ((context.mappedNode != astNode) && (cls->genericParams.size())) {
				GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

				for (auto &[k, v] : context.mappedGenericArgs) {
					if (auto it = cls->genericParamIndices.find(k);
						it == cls->genericParamIndices.end()) {
						if (!innerContext.mappedGenericArgs.insert(std::string_view(k), peff::SharedPtr<TypeNameNode>(v))) {
							return genOutOfMemoryCompError();
						}
					}
				}

				for (auto &k : cls->implementedTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, innerContext));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.second, innerContext));
				}
			} else {
				for (auto &k : cls->implementedTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, context));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.second, context));
				}
			}
			break;
		}
		case AstNodeType::GenericParam: {
			peff::SharedPtr<GenericParamNode> cls = astNode.castTo<GenericParamNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->baseType, context));

			for (auto &k : cls->implementedTypes) {
				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, context));
			}
			break;
		}
		default:;
	}

	return {};
}

SLKC_API std::optional<CompilationError> TopLevelCompileContext::instantiateGenericObject(
	peff::SharedPtr<MemberNode> originalObject,
	const peff::DynArray<peff::SharedPtr<TypeNameNode>> &genericArgs,
	peff::SharedPtr<MemberNode> &memberOut) {
	peff::SharedPtr<MemberNode> duplicatedObject;

	SLKC_RETURN_IF_COMP_ERROR(lookupGenericCache(originalObject, genericArgs, duplicatedObject));
	if (duplicatedObject) {
		memberOut = duplicatedObject;
		return {};
	}

	peff::DynArray<peff::SharedPtr<TypeNameNode>> duplicatedGenericArgs(allocator.get());

	if (!duplicatedGenericArgs.resize(genericArgs.size())) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < duplicatedGenericArgs.size(); ++i) {
		if (!(duplicatedGenericArgs.at(i) = genericArgs.at(i)->duplicate<TypeNameNode>(allocator.get()))) {
			return genOutOfMemoryCompError();
		}
	}

	duplicatedObject = originalObject->duplicate<MemberNode>(allocator.get());

	if (!duplicatedObject) {
		return genOutOfMemoryCompError();
	}

	GenericInstantiationContext instantiationContext(allocator.get(), &genericArgs);
	instantiationContext.mappedNode = duplicatedObject;

	GenericCacheTable *cacheTable;

	{
		peff::ScopeGuard removeCacheDirEntryGuard([this, originalObject]() noexcept {
			genericCacheDir.remove(originalObject);
		});

		if (auto it = genericCacheDir.find(originalObject);
			it != genericCacheDir.end()) {
			cacheTable = &it.value();
			removeCacheDirEntryGuard.release();
		} else {
			if (!genericCacheDir.insert(
					peff::SharedPtr<MemberNode>(originalObject),
					GenericCacheTable(allocator.get(),
						TypeNameListCmp(this)))) {
				return genOutOfMemoryCompError();
			}
			cacheTable = &genericCacheDir.at(originalObject);
		}

		if (!cacheTable->insert(std::move(duplicatedGenericArgs), peff::SharedPtr<MemberNode>(duplicatedObject))) {
			return genOutOfMemoryCompError();
		}

		{
			peff::ScopeGuard removeCacheTableEntryGuard([this, &genericArgs, cacheTable]() noexcept {
				cacheTable->remove(genericArgs);
			});

			// Map generic arguments.
			switch (originalObject->astNodeType) {
				case AstNodeType::Fn: {
					peff::SharedPtr<FnNode> obj = duplicatedObject.castTo<FnNode>();

					if (genericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								genericArgs.front()->tokenRange.beginIndex,
								genericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (auto &[k, v] : obj->genericParamIndices) {
						if (!instantiationContext.mappedGenericArgs.insert(
								std::string_view(k),
								peff::SharedPtr<TypeNameNode>(genericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}

					break;
				}
				case AstNodeType::Class: {
					peff::SharedPtr<ClassNode> obj = duplicatedObject.castTo<ClassNode>();

					if (genericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								genericArgs.front()->tokenRange.beginIndex,
								genericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (auto &[k, v] : obj->genericParamIndices) {
						if (!instantiationContext.mappedGenericArgs.insert(
								std::string_view(k),
								peff::SharedPtr<TypeNameNode>(genericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}
					break;
				}
				case AstNodeType::Interface: {
					peff::SharedPtr<InterfaceNode> obj = duplicatedObject.castTo<InterfaceNode>();

					if (genericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								genericArgs.front()->tokenRange.beginIndex,
								genericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (auto &[k, v] : obj->genericParamIndices) {
						if (!instantiationContext.mappedGenericArgs.insert(
								std::string_view(k),
								peff::SharedPtr<TypeNameNode>(genericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}
					break;
				}
				default:
					return CompilationError(
						TokenRange{
							genericArgs.front()->tokenRange.beginIndex,
							genericArgs.back()->tokenRange.endIndex },
						CompilationErrorKind::MismatchedGenericArgNumber);
			}

			SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(duplicatedObject, instantiationContext));

			removeCacheTableEntryGuard.release();
		}
	}

	memberOut = duplicatedObject;
	return {};
}
