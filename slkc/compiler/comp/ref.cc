#include "../compiler.h"
#include <algorithm>

using namespace slake::slkc;

bool Compiler::resolveRef(Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut) {
	assert(ref.size());

	// Try to resolve the first entry as a local variable.
	if (curMajorContext.curMinorContext.localVars.count(ref[0].name) && (!ref[0].genericArgs.size())) {
		auto newRef = ref;
		newRef.pop_front();

		auto localVar = curMajorContext.curMinorContext.localVars.at(ref[0].name);
		if (!newRef.size())
			goto lvarSucceeded;

		if (auto scope = scopeOf(localVar->type.get()); scope) {
			if (_resolveRef(scope.get(), newRef, partsOut))
				goto lvarSucceeded;
		}

		if (false) {
		lvarSucceeded:
			// Update corresponding semantic information.
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
			tokenInfo.semanticInfo.correspondingMember = localVar;
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Var;

			partsOut.push_front({ Ref{ ref.front() }, localVar });
			return true;
		}
	}

	// Try to resolve the first entry as a parameter.
	if (curFn->paramIndices.count(ref[0].name)) {
		auto newRef = ref;
		newRef.pop_front();

		auto idxParam = curFn->paramIndices.at(ref[0].name);

		if (!newRef.size())
			goto paramSucceeded;

		if (auto scope = scopeOf(curFn->params[idxParam]->type.get()); scope) {
			if (_resolveRef(scope.get(), newRef, partsOut))
				goto paramSucceeded;
		}

		if (false) {
		paramSucceeded:
			// Update corresponding semantic information.
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
			tokenInfo.semanticInfo.correspondingMember = curFn->params[idxParam];
			tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
			tokenInfo.semanticType = SemanticType::Param;

			partsOut.push_front({ Ref{ ref.front() }, make_shared<ArgRefNode>(idxParam) });
			return true;
		}
	}

	if (ref[0].name == "this") {
		auto thisRefNode = make_shared<ThisRefNode>();

		// Update corresponding semantic information.
		auto &tokenInfo = tokenInfos[ref[0].idxToken];
		tokenInfo.semanticInfo.correspondingMember = thisRefNode;
		tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
		tokenInfo.semanticType = SemanticType::Keyword;

		if (ref.size() > 1) {
			ref.pop_front();
			auto result = _resolveRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut);
			partsOut.push_front({ Ref{ ref.front() }, thisRefNode });
			return result;
		} else {
			partsOut.push_front({ Ref{ ref.front() }, thisRefNode });
			return true;
		}
	}

	return _resolveRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut, true);
}

bool Compiler::resolveRefWithScope(Scope *scope, Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut) {
	return _resolveRef(scope, ref, partsOut);
}

/// @brief Resolve a reference with a scope.
/// @param scope Scope for resolution.
/// @param ref Reference to be resolved.
/// @return true if succeeded, false otherwise.
bool Compiler::_resolveRef(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut, bool isTopLevel) {
	if (ref[0].name == "lab")
		puts("");
	// Update corresponding semantic information.
	{
		TokenContext tokenContext = TokenContext(curFn, curMajorContext);
		tokenContext.curScope = scope->shared_from_this();

		if (ref[0].idxAccessOpToken != SIZE_MAX) {
			auto &precedingAccessOpTokenInfo = tokenInfos[ref[0].idxAccessOpToken];
			precedingAccessOpTokenInfo.tokenContext = tokenContext;

			if (!isTopLevel)
				precedingAccessOpTokenInfo.completionContext = CompletionContext::MemberAccess;
		}

		if (ref[0].idxToken != SIZE_MAX) {
			auto &tokenInfo = tokenInfos[ref[0].idxToken];
			tokenInfo.tokenContext = tokenContext;

			if (!isTopLevel)
				tokenInfo.completionContext = CompletionContext::MemberAccess;
		}
	}

	// Raise an error if the reference is incomplete.
	if (ref[0].idxToken == SIZE_MAX)
		throw FatalCompilationError(
			Message(
				ref[0].loc,
				MessageType::Error,
				"Expecting an identifier"));

	if (ref[0].name == "base") {
		auto newRef = ref;
		newRef.pop_front();

		// Update corresponding semantic information.
		auto &tokenInfo = tokenInfos[ref[0].idxToken];
		tokenInfo.semanticInfo.correspondingMember = scope->owner->shared_from_this();
		tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
		tokenInfo.semanticType = SemanticType::Keyword;

		bool result = _resolveRefWithOwner(scope, newRef, partsOut);
		partsOut.push_front({ Ref{ ref.front() }, make_shared<BaseRefNode>() });
		return result;
	}

	GenericNodeInstantiationContext genericInstantiationContext = { nullptr, {} };

	if (shared_ptr<MemberNode> m; scope->members.count(ref[0].name)) {
		auto newRef = ref;
		newRef.pop_front();

		m = scope->members.at(ref[0].name);

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
				case NodeType::GenericParam:
					tokenInfo.semanticType = SemanticType::TypeParam;
					break;
				case NodeType::Param:
					tokenInfo.semanticType = SemanticType::Param;
					break;
				case NodeType::Var:
					tokenInfo.semanticType = isTopLevel ? SemanticType::Var : SemanticType::Property;
					break;
				case NodeType::LocalVar:
					tokenInfo.semanticType = SemanticType::Var;
					break;
				case NodeType::Fn:
					tokenInfo.semanticType = isTopLevel ? SemanticType::Fn : SemanticType::Method;
					break;
			}
		}

		if (ref[0].genericArgs.size()) {
			genericInstantiationContext.genericArgs = &ref[0].genericArgs;
			m = instantiateGenericNode(m, genericInstantiationContext);
		}

		if (!newRef.size()) {
			// All entries have been resolved, return true.
			partsOut.push_front({ Ref{ ref.front() }, m });
			return true;
		}

		auto newScope = scopeOf(m.get());

		if (newScope) {
			if (_resolveRef(newScope.get(), newRef, partsOut)) {
				switch (m->getNodeType()) {
					case NodeType::Var:
						partsOut.push_front({ Ref{ ref.front() }, m });
						break;
					default:
						partsOut.front().first.push_front(ref.front());
				}
				return true;
			}
		}
	}

	if (_resolveRefWithOwner(scope, ref, partsOut))
		return true;

	if (isTopLevel && scope->parent)
		return _resolveRef(scope->parent, ref, partsOut, isTopLevel);

	return false;
}

bool slake::slkc::Compiler::_resolveRefWithOwner(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut) {
	if (scope->owner) {
		switch (scope->owner->getNodeType()) {
			case NodeType::Class: {
				ClassNode *owner = (ClassNode *)scope->owner;

				// Resolve with the parent class.
				if (owner->parentClass) {
					if (auto p = resolveCustomTypeName((CustomTypeNameNode *)owner->parentClass.get()); p) {
						if (p->getNodeType() != NodeType::Class) {
							throw FatalCompilationError(
								Message(
									owner->parentClass->getLocation(),
									MessageType::Error,
									"`" + to_string(owner->parentClass, this) + "' is not a class"));
						}
						if (_resolveRef(scopeOf(p.get()).get(), ref, partsOut))
							return true;
					} else {
						// Error resolving the parent class - the reference is invalid.
						throw FatalCompilationError(
							Message(
								owner->parentClass->getLocation(),
								MessageType::Error,
								"Class `" + to_string(owner->parentClass, this) + "' was not found"));
					}
				}

				// Resolve with the interfaces.
				for (auto i : owner->implInterfaces) {
					if (auto p = resolveCustomTypeName((CustomTypeNameNode *)i.get()); p) {
						if (p->getNodeType() != NodeType::Class) {
							throw FatalCompilationError(
								Message(
									i->getLocation(),
									MessageType::Error,
									"`" + to_string(i, this) + "' is not an interface"));
						}
						if (_resolveRef(scopeOf(p.get()).get(), ref, partsOut))
							return true;
					} else {
						// Error resolving the interface - the reference is invalid.
						throw FatalCompilationError(
							Message(
								i->getLocation(),
								MessageType::Error,
								"Interface `" + to_string(i, this) + "' was not found"));
					}
				}

				// TODO: Check the generic arguments.
				break;
			}
			case NodeType::Interface: {
				auto owner = (InterfaceNode *)scope->owner;

				for (auto i : owner->parentInterfaces) {
					if (auto p = resolveCustomTypeName((CustomTypeNameNode *)i.get()); p) {
						if (p->getNodeType() != NodeType::Class) {
							throw FatalCompilationError(
								Message(
									i->getLocation(),
									MessageType::Error,
									"`" + to_string(i, this) + "' is not an interface"));
						}
						if (_resolveRef(scopeOf(p.get()).get(), ref, partsOut))
							return true;
					} else {
						// Error resolving the interface - the reference is invalid.
						throw FatalCompilationError(
							Message(
								i->getLocation(),
								MessageType::Error,
								"Interface `" + to_string(i, this) + "' was not found"));
					}
				}
				break;
			}
			case NodeType::Trait: {
				auto owner = (TraitNode *)scope->owner;
				break;
			}
			case NodeType::Module: {
				// Resolve with the parent module.
				auto owner = (ModuleNode *)scope->owner;
				if (owner->parentModule.expired())
					return false;

				if (_resolveRef(owner->parentModule.lock()->scope.get(), ref, partsOut))
					return true;

				break;
			}
			case NodeType::Var: {
				auto owner = (VarNode *)scope->owner;
				if (!owner->parent)
					return false;

				if (_resolveRef(scopeOf((AstNode *)owner->parent).get(), ref, partsOut))
					return false;
			}
		}
	}

	return false;
}

void Compiler::_getFullName(MemberNode *member, Ref &ref) {
	RefEntry entry = member->getName();

	ref.push_front(entry);

	if (!member->parent)
		return;

	_getFullName(member->parent, ref);
}

slake::slkc::Ref Compiler::getFullName(MemberNode *member) {
	Ref ref;

	_getFullName(member, ref);

	return ref;
}
