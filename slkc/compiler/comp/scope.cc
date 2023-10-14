#include "../compiler.h"

using namespace slake::slkc;

shared_ptr<Scope> Compiler::scopeOf(shared_ptr<AstNode> node) {
	switch (node->getNodeType()) {
		case AST_CLASS:
			return static_pointer_cast<ClassNode>(node)->scope;
		case AST_INTERFACE:
			return static_pointer_cast<InterfaceNode>(node)->scope;
		case AST_TRAIT:
			return static_pointer_cast<TraitNode>(node)->scope;
		case AST_MODULE:
			return static_pointer_cast<ModuleNode>(node)->scope;
		case AST_TYPENAME: {
			auto t = static_pointer_cast<TypeNameNode>(node);
			if (t->getTypeId()==TYPE_CUSTOM)
				return scopeOf(resolveCustomType(static_pointer_cast<CustomTypeNameNode>(t)));
			return {};
		}
		default:
			return {};
	}
}
