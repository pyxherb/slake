#include "../compiler.h"
#include <algorithm>

using namespace slake::slkc;

bool Compiler::resolveRef(Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut) {
	assert(ref.size());

	ResolvedOwnersSaver resolvedOwnersSaver(curMajorContext.curMinorContext);

	// Try to resolve the first entry as a local variable.
	if (curMajorContext.localVars.count(ref[0].name) && (!ref[0].genericArgs.size())) {
		auto newRef = ref;
		newRef.pop_front();

		auto localVar = curMajorContext.localVars.at(ref[0].name);
		if (!newRef.size())
			goto lvarSucceeded;

		if (auto scope = scopeOf(localVar->type); scope) {
			_resolveRef(scope.get(), newRef, partsOut);
			goto lvarSucceeded;
		}

		if (false) {
		lvarSucceeded:
			partsOut.push_front({ Ref{ ref.front() }, localVar });
			return true;
		}
	}

	// Try to resolve the first entry as a parameter.
	if (curFn->paramIndices.count(ref[0].name) && (!ref[0].genericArgs.size())) {
		auto newRef = ref;
		newRef.pop_front();

		auto idxParam = curFn->paramIndices.at(ref[0].name);

		if (!newRef.size())
			goto paramSucceeded;

		if (auto scope = scopeOf(curFn->params[idxParam].type); scope) {
			_resolveRef(scope.get(), newRef, partsOut);
			goto paramSucceeded;
		}

		if (false) {
		paramSucceeded:
			partsOut.push_front({ Ref{ ref.front() }, make_shared<ArgRefNode>(idxParam) });
			return true;
		}
	}

	if (ref[0].name == "this") {
		ref.pop_front();
		auto result = _resolveRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut);
		partsOut.push_front({ Ref{ ref.front() }, make_shared<ThisRefNode>() });
		return result;
	}

	return _resolveRef(curMajorContext.curMinorContext.curScope.get(), ref, partsOut);
}

/// @brief Resolve a reference with a scope.
/// @param scope Scope for resolution.
/// @param ref Reference to be resolved.
/// @return true if succeeded, false otherwise.
bool Compiler::_resolveRef(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut) {
	if (ref[0].name == "base") {
		auto newRef = ref;
		newRef.pop_front();
		return _resolveRefWithOwner(scope, newRef, partsOut);
	}

	if (shared_ptr<MemberNode> m; scope->members.count(ref[0].name)) {
		auto newRef = ref;
		newRef.pop_front();

		m = scope->members.at(ref[0].name);

		if (!newRef.size()) {
			// All entries have been resolved, return true.
			partsOut.push_front({ Ref{ ref.front() }, m });
			return true;
		}

		if (_resolveRef(scope, newRef, partsOut)) {
			switch (m->getNodeType()) {
				case AST_VAR:
					partsOut.push_front({ Ref{ ref.front() }, m });
					break;
				default:
					partsOut.front().first.push_front(ref.front());
			}
			return true;
		}
	}

	if (_resolveRefWithOwner(scope, ref, partsOut))
		return true;

	if (scope->parent)
		return _resolveRef(scope->parent, ref, partsOut);

	return {};
}

bool slake::slkc::Compiler::_resolveRefWithOwner(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut) {
	if (scope->owner && (!curMajorContext.curMinorContext.resolvedOwners.count(scope->owner))) {
		curMajorContext.curMinorContext.resolvedOwners.insert(scope->owner);
		switch (scope->owner->getNodeType()) {
			case AST_CLASS: {
				ClassNode *owner = (ClassNode *)scope->owner;

				// Resolve with the parent class.
				if (owner->parentClass) {
					if (auto p = resolveCustomType(owner->parentClass); p) {
						if (p->getNodeType() != AST_CLASS) {
							throw FatalCompilationError(
								Message(
									owner->parentClass->getLocation(),
									MSG_ERROR,
									"`" + to_string(owner->parentClass) + "' is not a class"));
						}
						if (_resolveRef(scopeOf(p).get(), ref, partsOut))
							return true;
					} else {
						// Error resolving the parent class - the reference is invalid.
						throw FatalCompilationError(
							Message(
								owner->parentClass->getLocation(),
								MSG_ERROR,
								"Class `" + to_string(owner->parentClass) + "' was not found"));
					}
				}

				// Resolve with the interfaces.
				for (auto i : owner->implInterfaces) {
					if (auto p = resolveCustomType(i); p) {
						if (p->getNodeType() != AST_CLASS) {
							throw FatalCompilationError(
								Message(
									i->getLocation(),
									MSG_ERROR,
									"`" + to_string(i) + "' is not an interface"));
						}
						if (_resolveRef(scopeOf(p).get(), ref, partsOut))
							return true;
					} else {
						// Error resolving the interface - the reference is invalid.
						throw FatalCompilationError(
							Message(
								i->getLocation(),
								MSG_ERROR,
								"Interface `" + to_string(i) + "' was not found"));
					}
				}

				// TODO: Check the generic arguments.
				break;
			}
			case AST_INTERFACE: {
				auto owner = (InterfaceNode *)scope->owner;

				for (auto i : owner->parentInterfaces) {
					if (auto p = resolveCustomType(i); p) {
						if (p->getNodeType() != AST_CLASS) {
							throw FatalCompilationError(
								Message(
									i->getLocation(),
									MSG_ERROR,
									"`" + to_string(i) + "' is not an interface"));
						}
						if (_resolveRef(scopeOf(p).get(), ref, partsOut))
							return true;
					} else {
						// Error resolving the interface - the reference is invalid.
						throw FatalCompilationError(
							Message(
								i->getLocation(),
								MSG_ERROR,
								"Interface `" + to_string(i) + "' was not found"));
					}
				}
				break;
			}
			case AST_TRAIT: {
				auto owner = (TraitNode *)scope->owner;
				break;
			}
			case AST_MODULE: {
				// Resolve with the parent module.
				auto owner = (ModuleNode *)scope->owner;
				if (owner->parentModule.expired())
					return false;

				if (_resolveRef(owner->parentModule.lock()->scope.get(), ref, partsOut))
					return true;

				break;
			}
			case AST_VAR: {
				auto owner = (VarNode *)scope->owner;
				if (!owner->parent)
					return false;

				if (_resolveRef(scopeOf(shared_ptr<AstNode>(owner->parent)).get(), ref, partsOut))
					return false;
			}
		}
	}

	return false;
}

void Compiler::_getFullName(MemberNode *member, Ref &ref) {
	auto name = member->getName();

	name.insert(name.end(), ref.begin(), ref.end());
	ref = name;

	if (!member->parent)
		return;

	_getFullName(member->parent, ref);
}

slake::slkc::Ref Compiler::getFullName(shared_ptr<MemberNode> member) {
	Ref ref;

	_getFullName(member.get(), ref);

	return ref;
}
