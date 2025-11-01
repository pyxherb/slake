#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> Document::lookupGenericCacheTable(
	AstNodePtr<MemberNode> originalObject,
	GenericCacheTable *&tableOut) {
	if (auto it = genericCacheDir.find(originalObject); it != genericCacheDir.end()) {
		tableOut = &it.value();
		return {};
	}
	tableOut = nullptr;
	return {};
}

SLKC_API peff::Option<CompilationError> Document::lookupGenericCache(
	AstNodePtr<MemberNode> originalObject,
	const peff::DynArray<AstNodePtr<TypeNameNode>> &genericArgs,
	AstNodePtr<MemberNode> &memberOut) const {
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

static peff::Option<CompilationError> _walkTypeNameForGenericInstantiation(
	AstNodePtr<TypeNameNode> &typeName,
	const GenericInstantiationContext &context) {
	if (!typeName) {
		return {};
	}

	switch (typeName->typeNameKind) {
		case TypeNameKind::Array: {
			AstNodePtr<ArrayTypeNameNode> tn = typeName.template castTo<ArrayTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->elementType, context));
			break;
		}
		case TypeNameKind::Ref: {
			AstNodePtr<RefTypeNameNode> tn = typeName.template castTo<RefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->referencedType, context));
			break;
		}
		case TypeNameKind::TempRef: {
			AstNodePtr<TempRefTypeNameNode> tn = typeName.template castTo<TempRefTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->referencedType, context));
			break;
		}
		case TypeNameKind::Fn: {
			AstNodePtr<FnTypeNameNode> tn = typeName.template castTo<FnTypeNameNode>();

			for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->paramTypes.at(i), context));
			}
			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->returnType, context));
			break;
		}
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode> tn = typeName.template castTo<CustomTypeNameNode>();

			if (tn->idRefPtr->entries.size() == 1) {
				IdRefEntry &entry = tn->idRefPtr->entries.at(0);

				if (!entry.genericArgs.size()) {
					if (auto it = context.mappedGenericArgs.find(entry.name);
						it != context.mappedGenericArgs.end()) {
						typeName = it.value();
					}
				}
			}

			for (size_t i = 0; i < tn->idRefPtr->entries.size(); ++i) {
				auto &genericArgs = tn->idRefPtr->entries.at(i).genericArgs;
				for (size_t j = 0; j < genericArgs.size(); ++j) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(genericArgs.at(j), context));
				}
			}
			break;
		}
		case TypeNameKind::Unpacking: {
			AstNodePtr<UnpackingTypeNameNode> tn = typeName.template castTo<UnpackingTypeNameNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->innerTypeName, context));
			break;
		}
		case TypeNameKind::ParamTypeList: {
			AstNodePtr<ParamTypeListTypeNameNode> tn = typeName.template castTo<ParamTypeListTypeNameNode>();

			for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->paramTypes.at(i), context));
			}
			break;
		}
	}

	return {};
}

static peff::Option<CompilationError> _walkNodeForGenericInstantiation(
	AstNodePtr<MemberNode> astNode,
	const GenericInstantiationContext &context) {
	if (!astNode) {
		return {};
	}

	if (context.mappedNode == astNode) {
		if (!astNode->genericArgs.resize(context.genericArgs->size())) {
			return genOutOfMemoryCompError();
		}
		for (size_t i = 0; i < context.genericArgs->size(); ++i) {
			if (!(astNode->genericArgs.at(i) =
						context.genericArgs->at(i)
							->duplicate<TypeNameNode>(context.allocator.get()))) {
				return genOutOfMemoryCompError();
			}
		}
	}

	switch (astNode->getAstNodeType()) {
		case AstNodeType::FnSlot: {
			AstNodePtr<FnNode> fnSlot = astNode.template castTo<FnNode>();

			for (auto i : fnSlot->overloadings) {
				for (auto j : i->genericParams) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.template castTo<MemberNode>(), context));
				}

				if ((context.mappedNode != astNode) && (i->genericParams.size())) {
					GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

					for (auto [k, v] : context.mappedGenericArgs) {
						if (auto it = i->genericParamIndices.find(k);
							it == i->genericParamIndices.end()) {
							if (!innerContext.mappedGenericArgs.insert(std::string_view(k), AstNodePtr<TypeNameNode>(v))) {
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

			rescanParams:
				for (size_t j = 0; j < i->params.size(); ++j) {
					auto curParam = i->params.at(j);
					AstNodePtr<TypeNameNode> curParamType = i->params.at(j)->type;

					if (curParamType) {
						if (curParamType->typeNameKind == TypeNameKind::Unpacking) {
							AstNodePtr<UnpackingTypeNameNode> unpackingType = curParamType.template castTo<UnpackingTypeNameNode>();

							if (unpackingType->innerTypeName->typeNameKind == TypeNameKind::ParamTypeList) {
								AstNodePtr<ParamTypeListTypeNameNode> innerTypeName = unpackingType->innerTypeName.template castTo<ParamTypeListTypeNameNode>();

								if (!i->params.eraseRange(j, j + 1)) {
									return genOutOfMemoryCompError();
								}

								if (!i->params.reserveSlots(j, innerTypeName->paramTypes.size())) {
									return genOutOfMemoryCompError();
								}

								for (size_t k = 0; k < innerTypeName->paramTypes.size(); ++k) {
									bool succeeded;

									DuplicationContext dc(context.allocator.get());

									AstNodePtr<VarNode> p = makeAstNode<VarNode>(context.allocator.get(), *curParam.get(), context.allocator.get(), dc, succeeded);

									if ((!p) || (!succeeded)) {
										return genOutOfMemoryCompError();
									}

									constexpr static size_t lenName = sizeof("arg_") + (sizeof(size_t) << 1) + 1;
									char nameBuf[lenName] = { 0 };

									sprintf(nameBuf, "arg_%.02zx", j + k);

									if (!p->name.build(nameBuf)) {
										return genOutOfMemoryCompError();
									}

									p->type = innerTypeName->paramTypes.at(k);

									i->params.at(j + k) = p;
								}

								// Note that we use nullptr for we assuming that errors that require a compile context will never happen.
								SLKC_RETURN_IF_COMP_ERROR(reindexFnParams(nullptr, i));

								if (innerTypeName->hasVarArgs) {
									if (j + 1 != i->params.size()) {
										return CompilationError(innerTypeName->tokenRange, CompilationErrorKind::InvalidVarArgHintDuringInstantiation);
									}

									i->fnFlags |= FN_VARG;
								}
							}

							goto rescanParams;
						}
					}
				}
			}
			break;
		}
		case AstNodeType::Var: {
			AstNodePtr<VarNode> varNode = astNode.template castTo<VarNode>();

			SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(varNode->type, context));
			break;
		}
		case AstNodeType::Class: {
			AstNodePtr<ClassNode> cls = astNode.template castTo<ClassNode>();

			for (auto j : cls->genericParams) {
				SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.template castTo<MemberNode>(), context));
			}

			if ((context.mappedNode != astNode) && (cls->genericParams.size())) {
				GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

				for (auto [k, v] : context.mappedGenericArgs) {
					if (auto it = cls->genericParamIndices.find(k);
						it == cls->genericParamIndices.end()) {
						if (!innerContext.mappedGenericArgs.insert(std::string_view(k), AstNodePtr<TypeNameNode>(v))) {
							return genOutOfMemoryCompError();
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->baseType, innerContext));

				for (auto &k : cls->implTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, innerContext));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, innerContext));
				}
			} else {
				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->baseType, context));

				for (auto &k : cls->implTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, context));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, context));
				}
			}
			break;
		}
		case AstNodeType::Struct: {
			AstNodePtr<StructNode> cls = astNode.template castTo<StructNode>();

			for (auto j : cls->genericParams) {
				SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.template castTo<MemberNode>(), context));
			}

			if ((context.mappedNode != astNode) && (cls->genericParams.size())) {
				GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

				for (auto [k, v] : context.mappedGenericArgs) {
					if (auto it = cls->genericParamIndices.find(k);
						it == cls->genericParamIndices.end()) {
						if (!innerContext.mappedGenericArgs.insert(std::string_view(k), AstNodePtr<TypeNameNode>(v))) {
							return genOutOfMemoryCompError();
						}
					}
				}
				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, innerContext));
				}
			} else {
				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, context));
				}
			}
			break;
		}
		case AstNodeType::Interface: {
			AstNodePtr<InterfaceNode> cls = astNode.template castTo<InterfaceNode>();

			for (auto j : cls->genericParams) {
				SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.template castTo<MemberNode>(), context));
			}

			if ((context.mappedNode != astNode) && (cls->genericParams.size())) {
				GenericInstantiationContext innerContext(context.allocator.get(), context.genericArgs);

				for (auto [k, v] : context.mappedGenericArgs) {
					if (auto it = cls->genericParamIndices.find(k);
						it == cls->genericParamIndices.end()) {
						if (!innerContext.mappedGenericArgs.insert(std::string_view(k), AstNodePtr<TypeNameNode>(v))) {
							return genOutOfMemoryCompError();
						}
					}
				}

				for (auto &k : cls->implTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, innerContext));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, innerContext));
				}
			} else {
				for (auto &k : cls->implTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, context));
				}

				for (auto j : cls->members) {
					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, context));
				}
			}
			break;
		}
		case AstNodeType::GenericParam: {
			AstNodePtr<GenericParamNode> cls = astNode.template castTo<GenericParamNode>();

			if (cls->genericConstraint) {
				SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->genericConstraint->baseType, context));

				for (auto &k : cls->genericConstraint->implTypes) {
					SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, context));
				}
			}
			break;
		}
		default:;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> Document::instantiateGenericObject(
	AstNodePtr<MemberNode> originalObject,
	const peff::DynArray<AstNodePtr<TypeNameNode>> &genericArgs,
	AstNodePtr<MemberNode> &memberOut) {
	AstNodePtr<MemberNode> duplicatedObject;
	SLKC_RETURN_IF_COMP_ERROR(lookupGenericCache(originalObject, genericArgs, duplicatedObject));
	if (duplicatedObject) {
		memberOut = duplicatedObject;
		return {};
	}

	peff::DynArray<AstNodePtr<TypeNameNode>> duplicatedGenericArgs(allocator.get());

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

	duplicatedObject->setParent(originalObject->parent);

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
					AstNodePtr<MemberNode>(originalObject),
					GenericCacheTable(allocator.get(),
						TypeNameListCmp(this)))) {
				return genOutOfMemoryCompError();
			}
			cacheTable = &genericCacheDir.at(originalObject);
		}

		if (!cacheTable->insert(std::move(duplicatedGenericArgs), AstNodePtr<MemberNode>(duplicatedObject))) {
			return genOutOfMemoryCompError();
		}

		{
			peff::ScopeGuard removeCacheTableEntryGuard([this, &genericArgs, cacheTable]() noexcept {
				cacheTable->remove(genericArgs);
			});

			// Map generic arguments.
			switch (originalObject->getAstNodeType()) {
				case AstNodeType::Fn: {
					AstNodePtr<FnOverloadingNode> obj = duplicatedObject.template castTo<FnOverloadingNode>();

					if (genericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								genericArgs.front()->tokenRange.moduleNode,
								genericArgs.front()->tokenRange.beginIndex,
								genericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (auto [k, v] : obj->genericParamIndices) {
						if (!instantiationContext.mappedGenericArgs.insert(
								std::string_view(k),
								AstNodePtr<TypeNameNode>(genericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}

					break;
				}
				case AstNodeType::Class: {
					AstNodePtr<ClassNode> obj = duplicatedObject.template castTo<ClassNode>();

					if (genericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								genericArgs.front()->tokenRange.moduleNode,
								genericArgs.front()->tokenRange.beginIndex,
								genericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (auto [k, v] : obj->genericParamIndices) {
						if (!instantiationContext.mappedGenericArgs.insert(
								std::string_view(k),
								AstNodePtr<TypeNameNode>(genericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}
					break;
				}
				case AstNodeType::Interface: {
					AstNodePtr<InterfaceNode> obj = duplicatedObject.template castTo<InterfaceNode>();

					if (genericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								genericArgs.front()->tokenRange.moduleNode,
								genericArgs.front()->tokenRange.beginIndex,
								genericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (auto [k, v] : obj->genericParamIndices) {
						if (!instantiationContext.mappedGenericArgs.insert(
								std::string_view(k),
								AstNodePtr<TypeNameNode>(genericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}
					break;
				}
				default:
					return CompilationError(
						TokenRange{
							genericArgs.front()->tokenRange.moduleNode,
							genericArgs.front()->tokenRange.beginIndex,
							genericArgs.back()->tokenRange.endIndex },
						CompilationErrorKind::MismatchedGenericArgNumber);
			}

			SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(duplicatedObject, instantiationContext));

			removeCacheTableEntryGuard.release();
		}

		removeCacheDirEntryGuard.release();
	}

	memberOut = duplicatedObject;
	return {};
}
