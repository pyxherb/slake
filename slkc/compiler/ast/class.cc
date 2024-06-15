#include "class.h"

using namespace slake::slkc;

std::shared_ptr<AstNode> ClassNode::doDuplicate() {
	return std::make_shared<ClassNode>(*this);
}

std::shared_ptr<AstNode> InterfaceNode::doDuplicate() {
	return std::make_shared<InterfaceNode>(*this);
}
