#include "../compiler.h"

using namespace slake::slkc;

std::shared_ptr<Scope> Compiler::scopeOf(AstNode *node) {
	switch (node->getNodeType()) {
		case NodeType::Class:
			return ((ClassNode *)node)->scope;
		case NodeType::Interface:
			return ((InterfaceNode *)node)->scope;
		case NodeType::Trait:
			return ((TraitNode *)node)->scope;
		case NodeType::Module:
			return ((ModuleNode *)node)->scope;
		case NodeType::TypeName: {
			auto t = ((TypeNameNode *)node);
			if (t->getTypeId() == TypeId::Custom)
				return scopeOf(resolveCustomTypeName((CustomTypeNameNode *)t).get());
			return {};
		}
		case NodeType::Alias: {
			std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> resolvedParts;

			resolveIdRef(((AliasNode *)node)->target, resolvedParts);
			return scopeOf(resolvedParts.back().second.get());
		}
		case NodeType::Var: {
			auto n = (VarNode *)node;
			if (n->type) {
				if (n->type->getTypeId() == TypeId::Custom)
					return scopeOf(resolveCustomTypeName((CustomTypeNameNode *)n->type.get()).get());
			}
			return {};
		}
		case NodeType::GenericParam: {
			auto n = (GenericParamNode *)node;

			if (!n->cachedMergedScope.expired())
				return n->cachedMergedScope.lock();

			std::shared_ptr<Scope> newScope = std::make_shared<Scope>();

			if (n->baseType) {
				auto baseTypeScope = scopeOf(n->baseType.get());

				if (baseTypeScope)
					newScope = mergeScope(newScope.get(), baseTypeScope.get());
			}

			for (auto i : n->interfaceTypes) {
				auto interfaceTypeScope = scopeOf(i.get());

				if (interfaceTypeScope)
					newScope = mergeScope(newScope.get(), interfaceTypeScope.get());
			}

			for (auto i : n->traitTypes) {
				auto traitTypeScope = scopeOf(i.get());

				if (traitTypeScope)
					newScope = mergeScope(newScope.get(), traitTypeScope.get());
			}

			n->cachedMergedScope = newScope;

			return newScope;
		}
		default:
			return {};
	}
}

std::shared_ptr<Scope> slake::slkc::Compiler::mergeScope(Scope *a, Scope *b, bool keepStaticMembers) {
	std::shared_ptr<Scope> newScope(a->duplicate());

	if (!keepStaticMembers) {
		std::set<std::string> staticMemberNames;
		for (auto &i : newScope->members) {
			if (i.second->access & ACCESS_STATIC)
				staticMemberNames.insert(i.first);
		}

		for (auto &i : staticMemberNames)
			newScope->members.erase(i);
	}

	for (auto &i : b->members) {
		if (!keepStaticMembers) {
			if (i.second->access & ACCESS_STATIC) {
				continue;
			}
		}

		if (newScope->members.count(i.first)) {
			if (newScope->members.at(i.first)->getNodeType() != i.second->getNodeType())
				continue;

			switch (i.second->getNodeType()) {
				case NodeType::Fn: {
					auto fnNew = std::static_pointer_cast<FnNode>(newScope->members.at(i.first)),
						 fnB = std::static_pointer_cast<FnNode> (i.second);

					// Check if the overloading registry is duplicated.
					for (auto &j : fnB->overloadingRegistries) {
						for (auto &k : fnNew->overloadingRegistries) {
							if (j->params.size() != k->params.size())
								continue;

							if (j->genericParams.size() != k->genericParams.size())
								continue;

							for (size_t l = 0; l < j->params.size(); ++l) {
								if (isSameType(j->params[l]->type, k->params[l]->type))
									goto fnOverloadingDuplicated;
							}

							// TODO: Do we actually have to check if the generic parameters' constraints?
						}

						fnNew->overloadingRegistries.push_back(j);
					fnOverloadingDuplicated:;
					}
				}
			}
		} else
			newScope->members[i.first] = i.second;
	}

	return newScope;
}
