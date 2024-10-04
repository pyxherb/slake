#include "../compiler.h"
#include <algorithm>

using namespace slake::slkc;

bool Compiler::resolveIdRef(std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut, bool ignoreDynamicPrecedings) {
	assert(ref->entries.size());

	if (!ignoreDynamicPrecedings) {
		// Try to resolve the first entry as a local variable.
		if (curMajorContext.curMinorContext.localVars.count(ref->entries[0].name) && (!ref->entries[0].genericArgs.size())) {
			auto localVar = curMajorContext.curMinorContext.localVars.at(ref->entries[0].name);
			if (ref->entries.size() < 2)
				goto lvarSucceeded;

			if (auto scope = scopeOf(localVar->type.get()); scope) {
				IdRefResolveContext newResolveContext;
				newResolveContext.curIndex = 1;
				newResolveContext.isTopLevel = false;
				newResolveContext.isStatic = false;

				if (_resolveIdRef(scope.get(), ref, partsOut, newResolveContext))
					goto lvarSucceeded;
			}

			return false;

		lvarSucceeded:
#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(ref->entries[0].idxToken, [this, &localVar](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.isTopLevelRef = true;
				if (!tokenInfo.semanticInfo.correspondingMember)
					tokenInfo.semanticInfo.correspondingMember = localVar;
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.semanticType = SemanticType::Var;
			});
#endif

			partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ ref->entries[0] }), localVar });
			return true;
		}

		if (curFn) {
			// Try to resolve the first entry as a parameter.
			if (curFn->paramIndices.count(ref->entries[0].name)) {
				auto idxParam = curFn->paramIndices.at(ref->entries[0].name);

				if (ref->entries.size() < 2)
					goto paramSucceeded;

				if (auto scope = scopeOf(curFn->params[idxParam]->type.get()); scope) {
					IdRefResolveContext newResolveContext;
					newResolveContext.curIndex = 1;
					newResolveContext.isTopLevel = false;
					newResolveContext.isStatic = false;

					if (_resolveIdRef(scope.get(), ref, partsOut, newResolveContext))
						goto paramSucceeded;
				}

				return false;

			paramSucceeded:
#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(ref->entries[0].idxToken, [this, idxParam](TokenInfo &tokenInfo) {
					tokenInfo.semanticInfo.isTopLevelRef = true;
					if (!tokenInfo.semanticInfo.correspondingMember)
						tokenInfo.semanticInfo.correspondingMember = curFn->params[idxParam];
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.semanticType = SemanticType::Param;
				});
#endif

				partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ ref->entries[0] }), curFn->params[idxParam] });
				return true;
			}
		}

		if (ref->entries[0].name == "this") {
			auto thisRefNode = std::make_shared<ThisRefNode>();

#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(ref->entries[0].idxToken, [this, &thisRefNode](TokenInfo &tokenInfo) {
				tokenInfo.semanticInfo.isTopLevelRef = true;
				if (!tokenInfo.semanticInfo.correspondingMember)
					tokenInfo.semanticInfo.correspondingMember = thisRefNode;
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.semanticType = SemanticType::Keyword;
			});
#endif

			if (ref->entries.size() > 1) {
				IdRefResolveContext newResolveContext;
				newResolveContext.curIndex = 1;
				newResolveContext.isTopLevel = false;
				newResolveContext.isStatic = false;

				auto result = _resolveIdRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut, newResolveContext);
				partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ ref->entries[0] }), thisRefNode });
				return result;
			} else {
				partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ ref->entries[0] }), thisRefNode });
				return true;
			}
		}
	}

	IdRefResolveContext newResolveContext;
	return _resolveIdRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut, newResolveContext);
}

bool Compiler::resolveIdRefWithScope(Scope *scope, std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut) {
	IdRefResolveContext resolveContext;

	return _resolveIdRef(scope, ref, partsOut, resolveContext);
}

bool Compiler::_resolveIdRef(Scope *scope, std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut, IdRefResolveContext resolveContext) {
	// Break looping resolutions - For example, when a the scope's owner is a class and it has a
	// parent class, where:
	//
	// classNode->scope == classNode->parentClass->scope
	//
	// And we have to resolve the parent class for verification, so the resolution call stack
	// will be:
	//
	// _resolveCustomTypeName (resolves classNode->parentClass)
	// -> _resolveIdRef (with classNode->parentClass->scope)
	// -> _resolveIdRefWithOwner (resolves classNode->parentClass->scope->owner->parentClass, which equals to classNode->parentClass)
	// -> _resolveCustomTypeName (resolves classNode->parentClass->scope->owner->parentClass)
	// -> _resolveIdRef (with classNode->parentClass->scope->owner->parentClass->scope, which equals to classNode->scope)
	// -> ...
	//
	// which causes a infinite recursion and we have to break that by not resolving scopes that is
	// already in a resolution.
	if (resolveContext.resolvingScopes.count(scope))
		return false;
	resolveContext.resolvingScopes.insert(scope);

	auto &curEntry = ref->entries[resolveContext.curIndex];

#if SLKC_WITH_LANGUAGE_SERVER
	// Update corresponding semantic information.
	{
		TokenContext tokenContext = TokenContext(curFn, curMajorContext);
		if (!resolveContext.keepTokenScope)
			tokenContext.curScope = scope->shared_from_this();

		updateTokenInfo(curEntry.idxAccessOpToken, [&tokenContext, &resolveContext](TokenInfo &precedingAccessOpTokenInfo) {
			precedingAccessOpTokenInfo.tokenContext = tokenContext;
			precedingAccessOpTokenInfo.semanticInfo.isTopLevelRef = resolveContext.isTopLevel;
			precedingAccessOpTokenInfo.semanticInfo.isStatic = resolveContext.isStatic;

			/* if (!resolveContext.isTopLevel)
				precedingAccessOpTokenInfo.completionContext = CompletionContext::MemberAccess;*/
		});

		updateTokenInfo(curEntry.idxToken, [&tokenContext, &resolveContext](TokenInfo &tokenInfo) {
			tokenInfo.tokenContext = tokenContext;
			tokenInfo.semanticInfo.isTopLevelRef = resolveContext.isTopLevel;
			tokenInfo.semanticInfo.isStatic = resolveContext.isStatic;

			/* if (!resolveContext.isTopLevel)
				tokenInfo.completionContext = CompletionContext::MemberAccess;*/
		});

		for (auto &i : curEntry.genericArgs) {
			updateCompletionContext(i, CompletionContext::Type);
		}
	}
#endif

	// Return false if the reference is incomplete.
	if (curEntry.idxToken == SIZE_MAX)
		return false;

	if (curEntry.name == "base") {
		if (!resolveContext.isStatic)
			return false;

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(curEntry.idxToken, [this, scope](TokenInfo &tokenInfo) {
			if (!tokenInfo.semanticInfo.correspondingMember)
				tokenInfo.semanticInfo.correspondingMember = scope->owner->shared_from_this();
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Keyword;
		});
#endif

		bool result = _resolveIdRefWithOwner(scope, ref, partsOut, resolveContext);
		partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ curEntry }), std::make_shared<BaseRefNode>() });
		return result;
	}

	GenericNodeInstantiationContext genericInstantiationContext = { nullptr, {} };

	if (std::shared_ptr<MemberNode> m; scope->members.count(curEntry.name)) {
		m = scope->members.at(curEntry.name);

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(curEntry.idxToken, [this, &resolveContext, &m](TokenInfo &tokenInfo) {
			if (!tokenInfo.semanticInfo.correspondingMember)
				tokenInfo.semanticInfo.correspondingMember = m;

			switch (m->getNodeType()) {
				case NodeType::Class:
					tokenInfo.semanticType = SemanticType::Class;
					break;
				case NodeType::Interface:
					tokenInfo.semanticType = SemanticType::Interface;
					break;
				case NodeType::Var:
					tokenInfo.semanticType = resolveContext.isTopLevel ? SemanticType::Var : SemanticType::Property;
					break;
				case NodeType::Fn:
					tokenInfo.semanticType = resolveContext.isTopLevel ? SemanticType::Fn : SemanticType::Method;
					break;
			}
		});
#endif

		switch (m->getNodeType()) {
			case NodeType::Class:
				if (!resolveContext.isStatic)
					return false;
				break;
			case NodeType::Interface:
				if (!resolveContext.isStatic)
					return false;
				break;
			case NodeType::Var: {
				auto member = std::static_pointer_cast<VarNode>(m);

				if (!resolveContext.isStatic) {
					if (member->access & ACCESS_STATIC) {
						return false;
					}
				} else if (resolveContext.isStatic) {
					if (!(member->access & ACCESS_STATIC)) {
						return false;
					}
				}

				resolveContext.isStatic = false;
				break;
			}
			case NodeType::Fn: {
				auto member = std::static_pointer_cast<FnNode>(m);

				for (auto i : member->overloadingRegistries) {
					if (resolveContext.isStatic) {
						if (i->access & ACCESS_STATIC) {
							goto foundReachableOverloading;
						}
					} else if (!resolveContext.isStatic) {
						if (!(i->access & ACCESS_STATIC)) {
							goto foundReachableOverloading;
						}
					}
				}

				return false;

			foundReachableOverloading:
				break;
			}
			case NodeType::GenericParam: {
				break;
			}
		}

		if (curEntry.genericArgs.size()) {
			genericInstantiationContext.genericArgs = &curEntry.genericArgs;
			m = instantiateGenericNode(m, genericInstantiationContext);
		}

		if (++resolveContext.curIndex >= ref->entries.size()) {
			// All entries have been resolved, return true.
			partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ curEntry }), m });
			curMajorContext.curMinorContext.isLastResolvedTargetStatic = resolveContext.isStatic;
			return true;
		}

		if (auto scope = scopeOf(m.get()); scope) {
			IdRefResolveContext newResolveContext;
			newResolveContext.curIndex = resolveContext.curIndex;
			newResolveContext.isTopLevel = false;
			newResolveContext.isStatic = resolveContext.isStatic;

			if (_resolveIdRef(scope.get(), ref, partsOut, newResolveContext)) {
				switch (m->getNodeType()) {
					case NodeType::Var:
						partsOut.push_front({ std::make_shared<IdRefNode>(IdRefEntries{ curEntry }), m });
						break;
					default:
						partsOut.front().first->entries.push_front(curEntry);
				}
				return true;
			}
		}
	}

	if (_resolveIdRefWithOwner(scope, ref, partsOut, resolveContext))
		return true;

	// Resolve with the parent scope - we should only do this on the top level.
	//
	// Consider a following example:
	//
	// ```C++
	// let g_var : i32;
	//
	// class Test {
	//     pub fn new(i32 initValue) {
	//         g_var = initValue;
	//     }
	// }
	// ```
	//
	// As the case above, the g_var is defined outside the class and cannot be
	// resolved with the class's scope, that is why we need to resolve the
	// scope with the parent scope.
	//
	// Note that resolution of subscopes is always be restricted in the
	// resolved scope from the parent, so we don't need to care about them.
	//
	if (resolveContext.isTopLevel && scope->parent) {
		IdRefResolveContext newResolveContext;
		newResolveContext.keepTokenScope = true;

		return _resolveIdRef(scope->parent, ref, partsOut, newResolveContext);
	}

	return false;
}

bool slake::slkc::Compiler::_resolveIdRefWithOwner(Scope *scope, std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut, IdRefResolveContext resolveContext) {
	if (scope->owner) {
		switch (scope->owner->getNodeType()) {
			case NodeType::Class: {
				ClassNode *owner = (ClassNode *)scope->owner;

				// Resolve with the parent class.
				if (owner->parentClass) {
					if (owner->parentClass->getTypeId() == TypeId::Custom) {
						if (_resolveIdRef(
								scopeOf(
									_resolveCustomTypeName(
										(CustomTypeNameNode *)owner->parentClass.get(),
										resolveContext.resolvingScopes)
										.get())
									.get(),
								ref, partsOut, resolveContext))
							return true;
					}
				}

				// Resolve with the implemented interfaces.
				for (auto i : owner->implInterfaces) {
					if (i->getTypeId() == TypeId::Custom) {
						if (_resolveIdRef(
								scopeOf(
									_resolveCustomTypeName(
										(CustomTypeNameNode *)i.get(),
										resolveContext.resolvingScopes)
										.get())
									.get(),
								ref, partsOut, resolveContext))
							return true;
					}
				}
				break;
			}
			case NodeType::Interface: {
				auto owner = (InterfaceNode *)scope->owner;

				// Resolve with the inherited interfaces.
				for (auto i : owner->parentInterfaces) {
					if (i->getTypeId() == TypeId::Custom) {
						if (_resolveIdRef(
								scopeOf(
									_resolveCustomTypeName(
										(CustomTypeNameNode *)i.get(),
										resolveContext.resolvingScopes)
										.get())
									.get(),
								ref, partsOut, resolveContext))
							return true;
					}
				}
				break;
			}
			case NodeType::Module: {
				// Resolve with the parent module.
				auto owner = (ModuleNode *)scope->owner;
				if (owner->parentModule.expired())
					return false;

				if (_resolveIdRef(owner->parentModule.lock()->scope.get(), ref, partsOut, resolveContext))
					return true;

				break;
			}
			case NodeType::Var: {
				auto owner = (VarNode *)scope->owner;
				if (!owner->parent)
					return false;

				if (_resolveIdRef(scopeOf((AstNode *)owner->parent).get(), ref, partsOut, resolveContext))
					return false;
			}
		}
	}

	return false;
}

void Compiler::_getFullName(MemberNode *member, IdRefEntries &ref) {
	IdRefEntry entry = member->getName();

	ref.push_front(entry);

	switch (member->getNodeType()) {
		case NodeType::FnOverloadingValue: {
			auto m = (FnOverloadingNode *)member;
			if (!m->owner->parent)
				return;
			_getFullName(m->owner->parent, ref);
			break;
		}
		default:
			if (!member->parent)
				return;
			_getFullName(member->parent, ref);
	}
}

std::shared_ptr<IdRefNode> Compiler::getFullName(MemberNode *member) {
	std::shared_ptr<IdRefNode> ref = std::make_shared<IdRefNode>();

	_getFullName(member, ref->entries);

	return ref;
}
