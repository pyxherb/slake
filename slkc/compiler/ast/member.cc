#include "../compiler.h"

using namespace slake::slkc;

MemberNode::~MemberNode() {
	if (originalValue && (!(compiler->flags & COMP_DELETING))) {
		if (auto it = compiler->_genericCacheDir.find(originalValue); it != compiler->_genericCacheDir.end()) {
			it->second.erase(genericArgs);
		}
	}
}

bool Compiler::isDynamicMember(shared_ptr<AstNode> member) {
	switch (member->getNodeType()) {
		case NodeType::Module:
		case NodeType::Class:
		case NodeType::Interface:
		case NodeType::Trait:
		case NodeType::Alias:
			return false;
	}

	auto m = static_pointer_cast<MemberNode>(member);

	if (m->parent) {
		switch (m->parent->getNodeType()) {
			case NodeType::Module:
				return false;
			default:
				return !(m->access & ACCESS_STATIC);
		}
	}

	return false;
}
