#include "../compiler.h"

using namespace slake::slkc;

shared_ptr<Scope> Compiler::scopeOf(AstNode* node) {
	switch (node->getNodeType()) {
		case NodeType::Class:
			return ((ClassNode*)node)->scope;
		case NodeType::Interface:
			return ((InterfaceNode*)node)->scope;
		case NodeType::Trait:
			return ((TraitNode*)node)->scope;
		case NodeType::Module:
			return ((ModuleNode*)node)->scope;
		case NodeType::TypeName: {
			auto t = ((TypeNameNode*)node);
			if (t->getTypeId()==Type::Custom)
				return scopeOf(resolveCustomType((CustomTypeNameNode*)t).get());
			return {};
		}
		case NodeType::Alias: {
			deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;

			resolveRef(((AliasNode*)node)->target, resolvedParts);
			return scopeOf(resolvedParts.back().second.get());
		}
		default:
			return {};
	}
}
