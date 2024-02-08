#include "../compiler.h"

using namespace slake::slkc;

shared_ptr<Scope> Compiler::scopeOf(AstNode* node) {
	switch (node->getNodeType()) {
		case AST_CLASS:
			return ((ClassNode*)node)->scope;
		case AST_INTERFACE:
			return ((InterfaceNode*)node)->scope;
		case AST_TRAIT:
			return ((TraitNode*)node)->scope;
		case AST_MODULE:
			return ((ModuleNode*)node)->scope;
		case AST_TYPENAME: {
			auto t = ((TypeNameNode*)node);
			if (t->getTypeId()==TYPE_CUSTOM)
				return scopeOf(resolveCustomType((CustomTypeNameNode*)t).get());
			return {};
		}
		default:
			return {};
	}
}
