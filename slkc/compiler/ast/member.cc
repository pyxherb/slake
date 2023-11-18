#include "../compiler.h"

using namespace slake::slkc;

bool Compiler::isDynamicMember(shared_ptr<AstNode> member) {
	switch (member->getNodeType()) {
		case AST_MODULE:
		case AST_CLASS:
		case AST_INTERFACE:
		case AST_TRAIT:
		case AST_ALIAS:
			return false;
	}

	auto m = static_pointer_cast<MemberNode>(member);

	if (m->parent) {
		switch (m->parent->getNodeType()) {
			case AST_MODULE:
				return false;
			default:
				return !(m->access & ACCESS_STATIC);
		}
	}

	return false;
}
