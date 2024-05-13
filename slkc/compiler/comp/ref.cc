#include "../compiler.h"
#include <algorithm>

using namespace slake::slkc;

bool Compiler::resolveIdRef(IdRef ref, deque<pair<IdRef, shared_ptr<AstNode>>> &partsOut, bool ignoreDynamicPrecedings) {
	assert(ref.size());

	if (!ignoreDynamicPrecedings) {
		// Try to resolve the first entry as a local variable.
		if (curMajorContext.curMinorContext.localVars.count(ref[0].name) && (!ref[0].genericArgs.size())) {
			auto newRef = ref;
			newRef.pop_front();

			auto localVar = curMajorContext.curMinorContext.localVars.at(ref[0].name);
			if (!newRef.size())
				goto lvarSucceeded;

			if (auto scope = scopeOf(localVar->type.get()); scope) {
				IdRefResolveContext newResolveContext;
				newResolveContext.isTopLevel = false;
				newResolveContext.isStatic = false;

				if (_resolveIdRef(scope.get(), newRef, partsOut, newResolveContext))
					goto lvarSucceeded;
			}

			return false;

		lvarSucceeded:
#if SLKC_WITH_LANGUAGE_SERVER
			// Update corresponding semantic information.
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
			tokenInfo.semanticInfo.isTopLevelRef = true;
			tokenInfo.semanticInfo.correspondingMember = localVar;
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Var;
#endif

			partsOut.push_front({ IdRef{ ref.front() }, localVar });
			return true;
		}

		if (curFn) {
			// Try to resolve the first entry as a parameter.
			if (curFn->paramIndices.count(ref[0].name)) {
				auto newRef = ref;
				newRef.pop_front();

				auto idxParam = curFn->paramIndices.at(ref[0].name);

				if (!newRef.size())
					goto paramSucceeded;

				if (auto scope = scopeOf(curFn->params[idxParam]->type.get()); scope) {
					IdRefResolveContext newResolveContext;
					newResolveContext.isTopLevel = false;
					newResolveContext.isStatic = false;

					if (_resolveIdRef(scope.get(), newRef, partsOut, newResolveContext))
						goto paramSucceeded;
				}

				return false;

			paramSucceeded:
#if SLKC_WITH_LANGUAGE_SERVER
				// Update corresponding semantic information.
				auto &tokenInfo = tokenInfos[ref[0].idxToken];
				tokenInfo.semanticInfo.isTopLevelRef = true;
				tokenInfo.semanticInfo.correspondingMember = curFn->params[idxParam];
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.semanticType = SemanticType::Param;
#endif

				partsOut.push_front({ IdRef{ ref.front() }, make_shared<ArgRefNode>((uint32_t)idxParam) });
				return true;
			}
		}

		if (ref[0].name == "this") {
			auto thisRefNode = make_shared<ThisRefNode>();

#if SLKC_WITH_LANGUAGE_SERVER
			// Update corresponding semantic information.
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
			tokenInfo.semanticInfo.isTopLevelRef = true;
			tokenInfo.semanticInfo.correspondingMember = thisRefNode;
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Keyword;
#endif

			if (ref.size() > 1) {
				IdRefResolveContext newResolveContext;
				newResolveContext.isTopLevel = false;
				newResolveContext.isStatic = false;

				ref.pop_front();
				auto result = _resolveIdRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut, newResolveContext);
				partsOut.push_front({ IdRef{ ref.front() }, thisRefNode });
				return result;
			} else {
				partsOut.push_front({ IdRef{ ref.front() }, thisRefNode });
				return true;
			}
		}
	}

	IdRefResolveContext newResolveContext;
	return _resolveIdRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut, newResolveContext);
}

bool Compiler::resolveIdRefWithScope(Scope *scope, IdRef ref, deque<pair<IdRef, shared_ptr<AstNode>>> &partsOut) {
	IdRefResolveContext resolveContext;

	return _resolveIdRef(scope, ref, partsOut, resolveContext);
}

bool Compiler::_resolveIdRef(Scope *scope, const IdRef &ref, deque<pair<IdRef, shared_ptr<AstNode>>> &partsOut, IdRefResolveContext resolveContext) {
#if SLKC_WITH_LANGUAGE_SERVER
	// Update corresponding semantic information.
	{
		TokenContext tokenContext = TokenContext(curFn, curMajorContext);
		if (!resolveContext.keepTokenScope)
			tokenContext.curScope = scope->shared_from_this();

		if (ref[0].idxAccessOpToken != SIZE_MAX) {
			auto &precedingAccessOpTokenInfo = tokenInfos[ref[0].idxAccessOpToken];
			precedingAccessOpTokenInfo.tokenContext = tokenContext;
			precedingAccessOpTokenInfo.semanticInfo.isTopLevelRef = resolveContext.isTopLevel;
			precedingAccessOpTokenInfo.semanticInfo.isStatic = resolveContext.isStatic;

			/* if (!resolveContext.isTopLevel)
				precedingAccessOpTokenInfo.completionContext = CompletionContext::MemberAccess;*/
		}

		if (ref[0].idxToken != SIZE_MAX) {
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
			tokenInfo.tokenContext = tokenContext;
			tokenInfo.semanticInfo.isTopLevelRef = resolveContext.isTopLevel;
			tokenInfo.semanticInfo.isStatic = resolveContext.isStatic;

			/* if (!resolveContext.isTopLevel)
				tokenInfo.completionContext = CompletionContext::MemberAccess;*/
		}
	}
#endif

	// Return false if the reference is incomplete.
	if (ref[0].idxToken == SIZE_MAX)
		return false;

	if (ref[0].name == "base") {
		if (!resolveContext.isStatic)
			return false;

		auto newRef = ref;
		newRef.pop_front();

#if SLKC_WITH_LANGUAGE_SERVER
		// Update corresponding semantic information.
		auto &tokenInfo = tokenInfos[ref[0].idxToken];
		tokenInfo.semanticInfo.correspondingMember = scope->owner->shared_from_this();
		tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
		tokenInfo.semanticType = SemanticType::Keyword;
#endif

		bool result = _resolveIdRefWithOwner(scope, newRef, partsOut, resolveContext);
		partsOut.push_front({ IdRef{ ref.front() }, make_shared<BaseRefNode>() });
		return result;
	}

	GenericNodeInstantiationContext genericInstantiationContext = { nullptr, {} };

	if (shared_ptr<MemberNode> m; scope->members.count(ref[0].name)) {
		auto newRef = ref;
		newRef.pop_front();

		m = scope->members.at(ref[0].name);

#if SLKC_WITH_LANGUAGE_SERVER
		// Update corresponding semantic information.
		{
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
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
		}
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
				auto member = static_pointer_cast<VarNode>(m);

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
			case NodeType::Fn:
			case NodeType::GenericParam: {
				break;
			}
		}

		if (ref[0].genericArgs.size()) {
			genericInstantiationContext.genericArgs = &ref[0].genericArgs;
			m = instantiateGenericNode(m, genericInstantiationContext);
		}

		if (!newRef.size()) {
			// All entries have been resolved, return true.
			partsOut.push_front({ IdRef{ ref.front() }, m });
			return true;
		}

		if (auto scope = scopeOf(m.get()); scope) {
			IdRefResolveContext newResolveContext;
			newResolveContext.isTopLevel = false;
			newResolveContext.isStatic = resolveContext.isStatic;

			if (_resolveIdRef(scope.get(), newRef, partsOut, newResolveContext)) {
				switch (m->getNodeType()) {
					case NodeType::Var:
						partsOut.push_front({ IdRef{ ref.front() }, m });
						break;
					default:
						partsOut.front().first.push_front(ref.front());
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

bool slake::slkc::Compiler::_resolveIdRefWithOwner(Scope *scope, const IdRef &ref, deque<pair<IdRef, shared_ptr<AstNode>>> &partsOut, IdRefResolveContext resolveContext) {
	if (scope->owner) {
		switch (scope->owner->getNodeType()) {
			case NodeType::Class: {
				ClassNode *owner = (ClassNode *)scope->owner;

				// Resolve with the parent class.
				if (owner->parentClass) {
					if (_resolveIdRef(
							scopeOf(
								resolveCustomTypeName(
									(CustomTypeNameNode *)owner->parentClass.get())
									.get())
								.get(),
							ref, partsOut, resolveContext))
						return true;
				}

				// Resolve with the implemented interfaces.
				for (auto i : owner->implInterfaces) {
					if (_resolveIdRef(
							scopeOf(
								resolveCustomTypeName(
									(CustomTypeNameNode *)i.get())
									.get())
								.get(),
							ref, partsOut, resolveContext))
						return true;
				}
				break;
			}
			case NodeType::Interface: {
				auto owner = (InterfaceNode *)scope->owner;

				// Resolve with the inherited interfaces.
				for (auto i : owner->parentInterfaces) {
					if (_resolveIdRef(
							scopeOf(
								resolveCustomTypeName(
									(CustomTypeNameNode *)i.get())
									.get())
								.get(),
							ref, partsOut, resolveContext))
						return true;
				}
				break;
			}
			case NodeType::Trait: {
				auto owner = (TraitNode *)scope->owner;

				// Resolve with the inherited traits.
				for (auto i : owner->parentTraits) {
					if (_resolveIdRef(
							scopeOf(
								resolveCustomTypeName(
									(CustomTypeNameNode *)i.get())
									.get())
								.get(),
							ref, partsOut, resolveContext))
						return true;
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

void Compiler::_getFullName(MemberNode *member, IdRef &ref) {
	IdRefEntry entry = member->getName();

	ref.push_front(entry);

	if (!member->parent)
		return;

	_getFullName(member->parent, ref);
}

slake::slkc::IdRef Compiler::getFullName(MemberNode *member) {
	IdRef ref;

	_getFullName(member, ref);

	return ref;
}
