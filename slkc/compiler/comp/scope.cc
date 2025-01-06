#include "../compiler.h"

using namespace slake::slkc;

std::shared_ptr<Scope> Compiler::scopeOf(CompileContext *compileContext, AstNode *node) {
	switch (node->getNodeType()) {
		case NodeType::Class:
			return ((ClassNode *)node)->scope;
		case NodeType::Interface:
			return ((InterfaceNode *)node)->scope;
		case NodeType::Module:
			return ((ModuleNode *)node)->scope;
		case NodeType::TypeName: {
			auto t = ((TypeNameNode *)node);
			if (t->getTypeId() == TypeId::Custom)
				return scopeOf(compileContext, resolveCustomTypeName(compileContext, (CustomTypeNameNode *)t).get());
			return {};
		}
		case NodeType::Alias: {
			IdRefResolvedParts resolvedParts;
			bool isStatic;

			if (!resolveIdRefWithScope(compileContext, ((AliasNode *)node)->scope, ((AliasNode *)node)->target, isStatic, resolvedParts))
				return {};
			return scopeOf(compileContext, resolvedParts.back().second.get());
		}
		case NodeType::Var: {
			auto n = (VarNode *)node;
			if (n->type) {
				if (n->type->getTypeId() == TypeId::Custom)
					return scopeOf(compileContext, resolveCustomTypeName(compileContext, (CustomTypeNameNode *)n->type.get()).get());
			}
			return {};
		}
		case NodeType::GenericParam: {
			auto n = (GenericParamNode *)node;

			if (!n->cachedMergedScope.expired())
				return n->cachedMergedScope.lock();

			std::shared_ptr<Scope> newScope = std::make_shared<Scope>();

			if (n->baseType) {
				auto baseTypeScope = scopeOf(compileContext, n->baseType.get());

				if (baseTypeScope)
					newScope = mergeScope(compileContext, newScope.get(), baseTypeScope.get());
			}

			for (auto i : n->interfaceTypes) {
				auto interfaceTypeScope = scopeOf(compileContext, i.get());

				if (interfaceTypeScope)
					newScope = mergeScope(compileContext, newScope.get(), interfaceTypeScope.get());
			}

			n->cachedMergedScope = newScope;

			return newScope;
		}
		default:
			return {};
	}
}

std::shared_ptr<Scope> slake::slkc::Compiler::mergeScope(CompileContext *compileContext, Scope *a, Scope *b, bool keepStaticMembers) {
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
							if (isFnOverloadingDuplicated(compileContext, j, k))
								goto fnOverloadingDuplicated;
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
