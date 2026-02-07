#include "../compiler.h"

#undef min
#undef max

using namespace slkc;

SLKC_API peff::Option<CompilationError> Document::lookupGenericCacheTable(
	AstNodePtr<MemberNode> originalObject,
	GenericCacheTable *&tableOut) {
	if (auto it = genericCacheDir.find(originalObject.get()); it != genericCacheDir.end()) {
		tableOut = &it.value();
		return {};
	}
	tableOut = nullptr;
	return {};
}

SLKC_API peff::Option<CompilationError> Document::lookupGenericCache(
	AstNodePtr<MemberNode> originalObject,
	const peff::DynArray<AstNodePtr<AstNode>> &genericArgs,
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
	const GenericInstantiationContext &context);

static peff::Option<CompilationError> _walkTypeNameForGenericInstantiation(
	AstNodePtr<AstNode> &astNode,
	peff::SharedPtr<GenericInstantiationContext> context) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 8));

	if (!astNode)
		return {};

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->pushAstNodeTask(AstNodeGenericInstantiationTask{ context, astNode }));

	return {};
}

static peff::Option<CompilationError> _walkTypeNameForGenericInstantiation(
	AstNodePtr<TypeNameNode> &typeName,
	peff::SharedPtr<GenericInstantiationContext> context) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 8));

	if (!typeName) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->pushTypeSlotTask(TypeSlotGenericInstantiationTask{ context, typeName }));

	return {};
}

static peff::Option<CompilationError> _walkNodeForGenericInstantiation(
	AstNodePtr<MemberNode> astNode,
	peff::SharedPtr<GenericInstantiationContext> context) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 8));

	if (!astNode) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->pushMemberTask(MemberGenericInstantiationTask{ context, astNode }));

	return {};
}

SLKC_API peff::Option<CompilationError> Document::instantiateGenericObject(
	AstNodePtr<MemberNode> originalObject,
	const peff::DynArray<AstNodePtr<AstNode>> &genericArgs,
	AstNodePtr<MemberNode> &memberOut) {
	SLKC_RETURN_IF_COMP_ERROR(checkStackBounds(1024 * 16));

	AstNodePtr<MemberNode> duplicatedObject;
	SLKC_RETURN_IF_COMP_ERROR(lookupGenericCache(originalObject, genericArgs, duplicatedObject));
	if (duplicatedObject) {
		memberOut = duplicatedObject;
		return {};
	}

	peff::DynArray<AstNodePtr<AstNode>> duplicatedGenericArgs(allocator.get());

	if (!duplicatedGenericArgs.resize(genericArgs.size())) {
		return genOutOfMemoryCompError();
	}

	for (size_t i = 0; i < duplicatedGenericArgs.size(); ++i) {
		if (!(duplicatedGenericArgs.at(i) = genericArgs.at(i)->duplicate<AstNode>(allocator.get()))) {
			return genOutOfMemoryCompError();
		}
	}

	duplicatedObject = originalObject->duplicate<MemberNode>(allocator.get());

	if (!duplicatedObject) {
		return genOutOfMemoryCompError();
	}

	duplicatedObject->setParent(originalObject->parent);

	GenericCacheTable *cacheTable;

	{
		peff::ScopeGuard removeCacheDirEntryGuard([this, originalObject]() noexcept {
			genericCacheDir.remove(originalObject.get());
		});

		if (auto it = genericCacheDir.find(originalObject.get());
			it != genericCacheDir.end()) {
			cacheTable = &it.value();
			removeCacheDirEntryGuard.release();
		} else {
			peff::RcObjectPtr<CompileEnvironment> compileEnv;

			if (!(compileEnv = peff::allocAndConstruct<CompileEnvironment>(allocator.get(), alignof(CompileEnvironment), nullptr, sharedFromThis(), allocator.get(), allocator.get())))
				return genOutOfMemoryCompError();
			if (!genericCacheDir.insert(
					originalObject.get(),
					GenericCacheTable(allocator.get(),
						GenericArgListCmp(this, compileEnv.get())))) {
				return genOutOfMemoryCompError();
			}
			cacheTable = &genericCacheDir.at(originalObject.get());
		}

		auto compileEnv = cacheTable->comparator().compileEnv.get();
		NormalCompilationContext compilationContext(compileEnv, nullptr);
		for (size_t i = 0; i < genericArgs.size(); ++i) {
			AstNodePtr<AstNode> curArg = genericArgs.at(i);

			if (curArg->getAstNodeType() == AstNodeType::Expr) {
				AstNodePtr<ExprNode> evaluatedArg;
				SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, &compilationContext, curArg.castTo<ExprNode>(), evaluatedArg));
				if (!evaluatedArg)
					return CompilationError(
						curArg->tokenRange,
						CompilationErrorKind::RequiresCompTimeExpr);

				evaluatedArg->tokenRange = curArg->tokenRange;
				duplicatedGenericArgs.at(i) = evaluatedArg.castTo<AstNode>();
			}
		}
		{
			// Map generic arguments.
			GenericInstantiationDispatcher dispatcher(allocator.get());
			peff::SharedPtr<GenericInstantiationContext> context;

			if (!(context = peff::makeShared<GenericInstantiationContext>(allocator.get(), allocator.get(), &duplicatedGenericArgs, &dispatcher)))
				return genOutOfMemoryCompError();

			switch (originalObject->getAstNodeType()) {
				case AstNodeType::Fn: {
					AstNodePtr<FnNode> obj = duplicatedObject.castTo<FnNode>();

					peff::DynArray<AstNodePtr<FnOverloadingNode>> overloadings(allocator.get());

					for (auto i : obj->overloadings) {
						context->mappedNode = i.castTo<MemberNode>();

						if (duplicatedGenericArgs.size() != i->genericParams.size())
							continue;

						for (size_t j = 0; j < duplicatedGenericArgs.size(); ++j) {
							AstNodePtr<AstNode> curArg = duplicatedGenericArgs.at(j);

							if (i->genericParams.at(j)->inputType) {
								if (curArg->getAstNodeType() != AstNodeType::Expr)
									return CompilationError(
										curArg->tokenRange,
										CompilationErrorKind::RequiresCompTimeExpr);

								AstNodePtr<TypeNameNode> argType;
								SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, &compilationContext, curArg.castTo<ExprNode>(), argType));

								bool same = false;
								SLKC_RETURN_IF_COMP_ERROR(isSameType(argType, i->genericParams.at(j)->inputType, same));

								if (!same)
									goto fnOverloadingMismatched;
							} else {
								if (curArg->getAstNodeType() != AstNodeType::TypeName)
									goto fnOverloadingMismatched;
							}
						}

						for (auto [k, v] : i->genericParamIndices) {
							if (!context->mappedGenericArgs.insert(
									std::string_view(k),
									AstNodePtr<AstNode>(duplicatedGenericArgs.at(v)))) {
								return genOutOfMemoryCompError();
							}
						}

						SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(i.castTo<MemberNode>(), context));

						if (!overloadings.pushBack(AstNodePtr<FnOverloadingNode>(i))) {
							return genOutOfMemoryCompError();
						}
					fnOverloadingMismatched:;
					}

					if (!overloadings.shrinkToFit()) {
						return genOutOfMemoryCompError();
					}

					obj->overloadings = std::move(overloadings);

					break;
				}
				case AstNodeType::Class: {
					AstNodePtr<ClassNode> obj = duplicatedObject.castTo<ClassNode>();

					context->mappedNode = obj.castTo<MemberNode>();

					if (duplicatedGenericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								duplicatedGenericArgs.front()->tokenRange.moduleNode,
								duplicatedGenericArgs.front()->tokenRange.beginIndex,
								duplicatedGenericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (size_t i = 0; i < duplicatedGenericArgs.size(); ++i) {
						AstNodePtr<AstNode> curArg = duplicatedGenericArgs.at(i);

						if (obj->genericParams.at(i)->inputType) {
							if (curArg->getAstNodeType() != AstNodeType::Expr)
								return CompilationError(
									curArg->tokenRange,
									CompilationErrorKind::RequiresCompTimeExpr);

							AstNodePtr<TypeNameNode> argType;
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, &compilationContext, curArg.castTo<ExprNode>(), argType));

							bool same = false;
							SLKC_RETURN_IF_COMP_ERROR(isSameType(argType, obj->genericParams.at(i)->inputType, same));

							if (!same)
								return CompilationError(
									curArg->tokenRange,
									CompilationErrorKind::TypeArgTypeMismatched);
						} else {
							if (curArg->getAstNodeType() != AstNodeType::TypeName)
								return CompilationError(
									curArg->tokenRange,
									CompilationErrorKind::ExpectingTypeName);
						}
					}

					for (auto [k, v] : obj->genericParamIndices) {
						if (!context->mappedGenericArgs.insert(
								std::string_view(k),
								AstNodePtr<AstNode>(duplicatedGenericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(duplicatedObject, context));
					break;
				}
				case AstNodeType::Interface: {
					AstNodePtr<InterfaceNode> obj = duplicatedObject.castTo<InterfaceNode>();

					context->mappedNode = obj.castTo<MemberNode>();

					if (duplicatedGenericArgs.size() != obj->genericParams.size()) {
						return CompilationError(
							TokenRange{
								duplicatedGenericArgs.front()->tokenRange.moduleNode,
								duplicatedGenericArgs.front()->tokenRange.beginIndex,
								duplicatedGenericArgs.back()->tokenRange.endIndex },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (size_t i = 0; i < duplicatedGenericArgs.size(); ++i) {
						AstNodePtr<AstNode> curArg = duplicatedGenericArgs.at(i);

						if (obj->genericParams.at(i)->inputType) {
							if (curArg->getAstNodeType() != AstNodeType::Expr)
								return CompilationError(
									curArg->tokenRange,
									CompilationErrorKind::RequiresCompTimeExpr);

							AstNodePtr<TypeNameNode> argType;
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, &compilationContext, curArg.castTo<ExprNode>(), argType));

							bool same = false;
							SLKC_RETURN_IF_COMP_ERROR(isSameType(argType, obj->genericParams.at(i)->inputType, same));

							if (!same)
								return CompilationError(
									curArg->tokenRange,
									CompilationErrorKind::TypeArgTypeMismatched);
						} else {
							if (curArg->getAstNodeType() != AstNodeType::TypeName)
								return CompilationError(
									curArg->tokenRange,
									CompilationErrorKind::ExpectingTypeName);
						}
					}

					for (auto [k, v] : obj->genericParamIndices) {
						if (!context->mappedGenericArgs.insert(
								std::string_view(k),
								AstNodePtr<AstNode>(duplicatedGenericArgs.at(v)))) {
							return genOutOfMemoryCompError();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(duplicatedObject, context));
					break;
				}
				default:
					return CompilationError(
						TokenRange{
							duplicatedGenericArgs.front()->tokenRange.moduleNode,
							duplicatedGenericArgs.front()->tokenRange.beginIndex,
							duplicatedGenericArgs.back()->tokenRange.endIndex },
						CompilationErrorKind::MismatchedGenericArgNumber);
			}

			while (true) {
				auto typeNameTasks = std::move(dispatcher.typeTasks);
				auto memberTasks = std::move(dispatcher.memberTasks);
				auto astNodeTasks = std::move(dispatcher.astNodeTasks);

				dispatcher.typeTasks = { allocator.get() };
				dispatcher.memberTasks = { allocator.get() };
				dispatcher.astNodeTasks = { allocator.get() };

				if ((!typeNameTasks.size() &&
						(!memberTasks.size())) &&
					(!astNodeTasks.size()))
					break;

				for (auto &task : typeNameTasks) {
					auto &typeName = task.typeName;

					switch (typeName->typeNameKind) {
						case TypeNameKind::Array: {
							AstNodePtr<ArrayTypeNameNode> tn = typeName.castTo<ArrayTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->elementType, task.context));
							break;
						}
						case TypeNameKind::Ref: {
							AstNodePtr<RefTypeNameNode> tn = typeName.castTo<RefTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->referencedType, task.context));
							break;
						}
						case TypeNameKind::TempRef: {
							AstNodePtr<TempRefTypeNameNode> tn = typeName.castTo<TempRefTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->referencedType, task.context));
							break;
						}
						case TypeNameKind::Fn: {
							AstNodePtr<FnTypeNameNode> tn = typeName.castTo<FnTypeNameNode>();

							for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->paramTypes.at(i), task.context));
							}
							SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->returnType, task.context));
							break;
						}
						case TypeNameKind::Custom: {
							AstNodePtr<CustomTypeNameNode> tn = typeName.castTo<CustomTypeNameNode>();

							if (tn->idRefPtr->entries.size() == 1) {
								IdRefEntry &entry = tn->idRefPtr->entries.at(0);

								if (!entry.genericArgs.size()) {
									if (auto it = task.context->mappedGenericArgs.find(entry.name);
										it != task.context->mappedGenericArgs.end()) {
										if (it.value()->getAstNodeType() != AstNodeType::TypeName)
											return CompilationError(it.value()->tokenRange, CompilationErrorKind::ExpectingTypeName);
										typeName = it.value().castTo<TypeNameNode>();
										break;
									}
								}
							}

							for (size_t i = 0; i < tn->idRefPtr->entries.size(); ++i) {
								auto &genericArgs = tn->idRefPtr->entries.at(i).genericArgs;
								for (size_t j = 0; j < genericArgs.size(); ++j) {
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(genericArgs.at(j), task.context));
								}
							}
							break;
						}
						case TypeNameKind::Unpacking: {
							AstNodePtr<UnpackingTypeNameNode> tn = typeName.castTo<UnpackingTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->innerTypeName, task.context));
							break;
						}
						case TypeNameKind::ParamTypeList: {
							AstNodePtr<ParamTypeListTypeNameNode> tn = typeName.castTo<ParamTypeListTypeNameNode>();

							for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->paramTypes.at(i), task.context));
							}
							break;
						}
					}
				}

				for (auto &task : memberTasks) {
					auto &astNode = task.member;

					if (task.context->mappedNode == astNode) {
						if (!astNode->genericArgs.resize(task.context->genericArgs->size())) {
							return genOutOfMemoryCompError();
						}
						for (size_t i = 0; i < task.context->genericArgs->size(); ++i) {
							if (!(astNode->genericArgs.at(i) =
										task.context->genericArgs->at(i)
											->duplicate<TypeNameNode>(allocator.get()))) {
								return genOutOfMemoryCompError();
							}
						}
					}

					switch (astNode->getAstNodeType()) {
						case AstNodeType::FnOverloading: {
							AstNodePtr<FnOverloadingNode> fnSlot = astNode.castTo<FnOverloadingNode>();

							for (auto i : fnSlot->genericParams) {
								SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(i.castTo<MemberNode>(), task.context));
							}

							if ((task.context->mappedNode != astNode) && (fnSlot->genericParams.size())) {
								peff::SharedPtr<GenericInstantiationContext> innerContext;

								if (!(innerContext = peff::makeShared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->genericArgs, &dispatcher))) {
									return genOutOfMemoryCompError();
								}

								for (auto [k, v] : task.context->mappedGenericArgs) {
									if (auto it = fnSlot->genericParamIndices.find(k);
										it == fnSlot->genericParamIndices.end()) {
										if (!innerContext->mappedGenericArgs.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
											return genOutOfMemoryCompError();
										}
									}
								}

								for (auto &i : fnSlot->params) {
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(i->type, innerContext));
								}

								SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(fnSlot->returnType, innerContext));

								// No need to substitute the function body, we just care about the declaration.
							} else {
								for (auto &i : fnSlot->params) {
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(i->type, task.context));
								}

								SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(fnSlot->returnType, task.context));
							}

							if (!dispatcher.collectedFnOverloadings.contains(fnSlot))
								if (!dispatcher.collectedFnOverloadings.insert(std::move(fnSlot)))
									return genOutOfMemoryCompError();
							break;
						}
						case AstNodeType::Fn: {
							AstNodePtr<FnNode> fnSlot = astNode.castTo<FnNode>();

							for (auto i : fnSlot->overloadings) {
								AstNodePtr<MemberNode> a = i.castTo<MemberNode>();
								SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(a, task.context));
							}

							if (!dispatcher.collectedFns.contains(fnSlot))
								if (!dispatcher.collectedFns.insert(std::move(fnSlot)))
									return genOutOfMemoryCompError();
							break;
						}
						case AstNodeType::Var: {
							AstNodePtr<VarNode> varNode = astNode.castTo<VarNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(varNode->type, task.context));
							break;
						}
						case AstNodeType::Class: {
							AstNodePtr<ClassNode> cls = astNode.castTo<ClassNode>();

							for (auto j : cls->genericParams) {
								SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.castTo<MemberNode>(), task.context));
							}

							if ((task.context->mappedNode != astNode) && (cls->genericParams.size())) {
								peff::SharedPtr<GenericInstantiationContext> innerContext;

								if (!(innerContext = peff::makeShared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->genericArgs, &dispatcher))) {
									return genOutOfMemoryCompError();
								}

								for (auto [k, v] : task.context->mappedGenericArgs) {
									if (auto it = cls->genericParamIndices.find(k);
										it == cls->genericParamIndices.end()) {
										if (!innerContext->mappedGenericArgs.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
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
								SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->baseType, task.context));

								for (auto &k : cls->implTypes) {
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, task.context));
								}

								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, task.context));
								}
							}
							break;
						}
						case AstNodeType::Struct: {
							AstNodePtr<StructNode> cls = astNode.castTo<StructNode>();

							for (auto j : cls->genericParams) {
								SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.castTo<MemberNode>(), task.context));
							}

							if ((task.context->mappedNode != astNode) && (cls->genericParams.size())) {
								peff::SharedPtr<GenericInstantiationContext> innerContext;

								if (!(innerContext = peff::makeShared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->genericArgs, &dispatcher))) {
									return genOutOfMemoryCompError();
								}

								for (auto [k, v] : task.context->mappedGenericArgs) {
									if (auto it = cls->genericParamIndices.find(k);
										it == cls->genericParamIndices.end()) {
										if (!innerContext->mappedGenericArgs.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
											return genOutOfMemoryCompError();
										}
									}
								}
								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, innerContext));
								}
							} else {
								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, task.context));
								}
							}
							break;
						}
						case AstNodeType::Interface: {
							AstNodePtr<InterfaceNode> cls = astNode.castTo<InterfaceNode>();

							for (auto j : cls->genericParams) {
								SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j.castTo<MemberNode>(), task.context));
							}

							if ((task.context->mappedNode != astNode) && (cls->genericParams.size())) {
								peff::SharedPtr<GenericInstantiationContext> innerContext;

								if (!(innerContext = peff::makeShared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->genericArgs, &dispatcher))) {
									return genOutOfMemoryCompError();
								}

								for (auto [k, v] : task.context->mappedGenericArgs) {
									if (auto it = cls->genericParamIndices.find(k);
										it == cls->genericParamIndices.end()) {
										if (!innerContext->mappedGenericArgs.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
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
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, task.context));
								}

								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walkNodeForGenericInstantiation(j, task.context));
								}
							}
							break;
						}
						case AstNodeType::GenericParam: {
							AstNodePtr<GenericParamNode> cls = astNode.castTo<GenericParamNode>();

							if (cls->genericConstraint) {
								SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(cls->genericConstraint->baseType, task.context));

								for (auto &k : cls->genericConstraint->implTypes) {
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(k, task.context));
								}
							}
							break;
						}
						default:;
					}
				}

				for (auto &task : astNodeTasks) {
					auto &astNode = task.node;

					switch (astNode->getAstNodeType()) {
						case AstNodeType::TypeName: {
							const AstNodePtr<TypeNameNode> typeName = astNode.castTo<TypeNameNode>();
							switch (typeName->typeNameKind) {
								case TypeNameKind::Array: {
									AstNodePtr<ArrayTypeNameNode> tn = typeName.castTo<ArrayTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->elementType, task.context));
									break;
								}
								case TypeNameKind::Ref: {
									AstNodePtr<RefTypeNameNode> tn = typeName.castTo<RefTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->referencedType, task.context));
									break;
								}
								case TypeNameKind::TempRef: {
									AstNodePtr<TempRefTypeNameNode> tn = typeName.castTo<TempRefTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->referencedType, task.context));
									break;
								}
								case TypeNameKind::Fn: {
									AstNodePtr<FnTypeNameNode> tn = typeName.castTo<FnTypeNameNode>();

									for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
										SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->paramTypes.at(i), task.context));
									}
									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->returnType, task.context));
									break;
								}
								case TypeNameKind::Custom: {
									AstNodePtr<CustomTypeNameNode> tn = typeName.castTo<CustomTypeNameNode>();

									if (tn->idRefPtr->entries.size() == 1) {
										IdRefEntry &entry = tn->idRefPtr->entries.at(0);

										if (!entry.genericArgs.size()) {
											if (auto it = task.context->mappedGenericArgs.find(entry.name);
												it != task.context->mappedGenericArgs.end()) {
												astNode = it.value();
												break;
											}
										}
									}

									for (size_t i = 0; i < tn->idRefPtr->entries.size(); ++i) {
										auto &genericArgs = tn->idRefPtr->entries.at(i).genericArgs;
										for (size_t j = 0; j < genericArgs.size(); ++j) {
											SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(genericArgs.at(j), task.context));
										}
									}
									break;
								}
								case TypeNameKind::Unpacking: {
									AstNodePtr<UnpackingTypeNameNode> tn = typeName.castTo<UnpackingTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->innerTypeName, task.context));
									break;
								}
								case TypeNameKind::ParamTypeList: {
									AstNodePtr<ParamTypeListTypeNameNode> tn = typeName.castTo<ParamTypeListTypeNameNode>();

									for (size_t i = 0; i < tn->paramTypes.size(); ++i) {
										SLKC_RETURN_IF_COMP_ERROR(_walkTypeNameForGenericInstantiation(tn->paramTypes.at(i), task.context));
									}
									break;
								}
							}
						}
					}
				}
			}

			for (auto fnSlot : dispatcher.collectedFnOverloadings) {
			rescanParams:
				for (size_t i = 0; i < fnSlot->params.size(); ++i) {
					auto curParam = fnSlot->params.at(i);
					AstNodePtr<TypeNameNode> curParamType = fnSlot->params.at(i)->type;

					if (curParamType) {
						if (curParamType->typeNameKind == TypeNameKind::Unpacking) {
							AstNodePtr<UnpackingTypeNameNode> unpackingType = curParamType.castTo<UnpackingTypeNameNode>();

							if (unpackingType->innerTypeName->typeNameKind == TypeNameKind::ParamTypeList) {
								AstNodePtr<ParamTypeListTypeNameNode> innerTypeName = unpackingType->innerTypeName.castTo<ParamTypeListTypeNameNode>();

								fnSlot->params.eraseRange(i, i + 1);

								if (!fnSlot->params.insertRangeInitialized(i, innerTypeName->paramTypes.size())) {
									return genOutOfMemoryCompError();
								}

								for (size_t k = 0; k < innerTypeName->paramTypes.size(); ++k) {
									bool succeeded;

									AstNodePtr<VarNode> p = curParam->duplicate<VarNode>(allocator.get());

									if ((!p) || (!succeeded)) {
										return genOutOfMemoryCompError();
									}

									constexpr static size_t lenName = sizeof("arg_") + (sizeof(size_t) << 1) + 1;
									char nameBuf[lenName] = { 0 };

									sprintf(nameBuf, "arg_%.02zx", i + k);

									if (!p->name.build(nameBuf)) {
										return genOutOfMemoryCompError();
									}

									p->type = innerTypeName->paramTypes.at(k);

									fnSlot->params.at(i + k) = p;
								}

								// Note that we use nullptr for we assuming that errors that require a compile task.context will never happen.
								SLKC_RETURN_IF_COMP_ERROR(reindexFnParams(nullptr, fnSlot));

								if (innerTypeName->hasVarArgs) {
									if (i + 1 != fnSlot->params.size()) {
										return CompilationError(innerTypeName->tokenRange, CompilationErrorKind::InvalidVarArgHintDuringInstantiation);
									}

									fnSlot->fnFlags |= FN_VARG;
								}
							}

							goto rescanParams;
						}
					}
				}
			}

			for (auto fnSlot : dispatcher.collectedFns) {
				for (auto it = fnSlot->overloadings.begin(); it != fnSlot->overloadings.end(); ++it) {
					for (auto jt = it + 1; jt != fnSlot->overloadings.end(); ++jt) {
						bool whether;
						SLKC_RETURN_IF_COMP_ERROR(isFnSignatureDuplicated(*it, *jt, whether));

						if (whether) {
							ModuleNode *mod = nullptr;
							size_t idxMinToken = SIZE_MAX, idxMaxToken = 0;

							for (auto i : *context->genericArgs) {
								if (!mod) {
									mod = i->tokenRange.moduleNode;
								} else if (i->tokenRange.moduleNode != mod)
									std::terminate();
								idxMinToken = std::min(i->tokenRange.beginIndex, idxMinToken);
								idxMaxToken = std::max(i->tokenRange.endIndex, idxMaxToken);
							}

							return CompilationError(
								TokenRange(mod, idxMinToken, idxMaxToken),
								CompilationErrorKind::FunctionOverloadingDuplicatedDuringInstantiation);
						}
					}
				}
			}

			if (!cacheTable->insert(std::move(duplicatedGenericArgs), AstNodePtr<MemberNode>(duplicatedObject))) {
				return genOutOfMemoryCompError();
			}
		}

		removeCacheDirEntryGuard.release();
	}

	memberOut = duplicatedObject;
	return {};
}
