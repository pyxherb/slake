#include "../compiler.h"

using namespace slake::slkc;

shared_ptr<AstNode> Compiler::resolveRef(const Ref &ref, Ref &staticPartOut, Ref &dynamicPartOut) {
	assert(ref.size());

	if (context.localVars.count(ref[0].name) && (!ref[0].genericArgs.size())) {
		auto newRef = ref;
		newRef.pop_front();

		auto localVar = context.localVars.at(ref[0].name);
		if (!newRef.size())
			return localVar;
		if (auto scope = scopeOf(localVar->type); scope)
			return _resolveRef(scope, newRef, staticPartOut, dynamicPartOut, false);
	}

	if (context.curFn->paramIndices.count(ref[0].name) && (!ref[0].genericArgs.size())) {
		auto newRef = ref;
		newRef.pop_front();

		auto idxParam = context.curFn->paramIndices.at(ref[0].name);
		if (!newRef.size())
			return make_shared<ArgRefNode>(idxParam);
		if (auto scope = scopeOf(context.curFn->params[idxParam].type); scope)
			return _resolveRef(scope, newRef, staticPartOut, dynamicPartOut, false);
	}

	return _resolveRef(context.curScope, ref, staticPartOut, dynamicPartOut, false);
}

/// @brief Resolve a reference with a scope.
/// @param scope Scope for resolution.
/// @param ref Reference to be resolved.
/// @return Resolved if succeeded, nullptr otherwise.
shared_ptr<AstNode> Compiler::_resolveRef(shared_ptr<Scope> scope, const Ref &ref, Ref &staticPartOut, Ref &dynamicPartOut, bool afterStaticPart) {
	if (shared_ptr<MemberNode> m; scope->members.count(ref[0].name)) {
		m = scope->members.at(ref[0].name);

		auto newRef = ref;
		newRef.pop_front();
		if (!newRef.size()) {
			if (afterStaticPart)
				dynamicPartOut.push_back(ref.front());
			else
				staticPartOut.push_back(ref.front());
			return m;
		}

		bool prevAfterStaticPart = afterStaticPart;

		switch (scope->owner.lock()->getNodeType()) {
			case AST_CLASS:
			case AST_INTERFACE:
			case AST_TRAIT:
				break;
			case AST_VAR:
			case AST_LOCAL_VAR:
				afterStaticPart = true;
				break;
		}

		if (auto result = _resolveRef(scope, newRef, staticPartOut, dynamicPartOut, afterStaticPart); result) {
			(prevAfterStaticPart ? staticPartOut : dynamicPartOut).push_front(ref[0]);
			return result;
		}
	}

	if (!scope->owner.expired()) {
		switch (scope->owner.lock()->getNodeType()) {
			case AST_CLASS: {
				auto owner = static_pointer_cast<ClassNode>(scope->owner.lock());

				// Resolve the parent class.
				if (owner->parentClass) {
					if (auto p = resolveCustomType(owner->parentClass); p) {
						if (p->getNodeType() != AST_CLASS) {
							throw FatalCompilationError(
								Message(
									owner->parentClass->getLocation(),
									MSG_ERROR,
									"`" + to_string(owner->parentClass) + "' is not a class"));
						}
						if (auto m = _resolveRef(scopeOf(p), ref, staticPartOut, dynamicPartOut, afterStaticPart); m)
							return m;
					} else {
						// Error resolving the parent class - the reference is invalid.
						throw FatalCompilationError(
							Message(
								owner->parentClass->getLocation(),
								MSG_ERROR,
								"Class `" + to_string(owner->parentClass) + "' was not found"));
					}
				}

				// Resolve the interfaces.
				for (auto i : owner->implInterfaces) {
					if (auto p = resolveCustomType(i); p) {
						if (p->getNodeType() != AST_CLASS) {
							throw FatalCompilationError(
								Message(
									i->getLocation(),
									MSG_ERROR,
									"`" + to_string(i) + "' is not an interface"));
						}
						if (auto m = _resolveRef(scopeOf(p), ref, staticPartOut, dynamicPartOut, afterStaticPart); m)
							return m;
					} else {
						// Error resolving the interface - the reference is invalid.
						throw FatalCompilationError(
							Message(
								i->getLocation(),
								MSG_ERROR,
								"Interface `" + to_string(i) + "' was not found"));
					}
				}

				// TODO: Check for generic arguments.
				break;
			}
			case AST_INTERFACE: {
				auto owner = static_pointer_cast<InterfaceNode>(scope->owner.lock());

				for (auto i : owner->parentInterfaces) {
					if (auto p = resolveCustomType(i); p) {
						if (p->getNodeType() != AST_CLASS) {
							throw FatalCompilationError(
								Message(
									i->getLocation(),
									MSG_ERROR,
									"`" + to_string(i) + "' is not an interface"));
						}
						if (auto m = _resolveRef(scopeOf(p), ref, staticPartOut, dynamicPartOut, afterStaticPart); m)
							return m;
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
				auto owner = static_pointer_cast<TraitNode>(scope->owner.lock());
				break;
			}
			case AST_MODULE: {
				// Resolve with the parent module.
				auto owner = static_pointer_cast<ModuleNode>(scope->owner.lock());
				if (owner->parentModule.expired())
					return shared_ptr<MemberNode>();

				if (auto m = _resolveRef(owner->parentModule.lock()->scope, ref, staticPartOut, dynamicPartOut, afterStaticPart); m)
					return m;

				break;
			}
			case AST_VAR: {
				auto owner = static_pointer_cast<VarNode>(scope->owner.lock());
				if (owner->parent.expired())
					return shared_ptr<MemberNode>();

				if (auto m = _resolveRef(scopeOf(owner->parent.lock()), ref, staticPartOut, dynamicPartOut, afterStaticPart); m)
					return m;
			}
		}
	}

	return {};
}
