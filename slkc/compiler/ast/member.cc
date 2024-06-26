#include "../compiler.h"

using namespace slake::slkc;

MemberNode::~MemberNode() {
}

bool Compiler::isDynamicMember(std::shared_ptr<AstNode> member) {
	switch (member->getNodeType()) {
		case NodeType::Module:
		case NodeType::Class:
		case NodeType::Interface:
		case NodeType::Alias:
			return false;
	}

	auto m = std::static_pointer_cast<MemberNode>(member);

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
