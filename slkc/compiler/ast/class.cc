#include "class.h"

using namespace slake::slkc;

shared_ptr<AstNode> ClassNode::doDuplicate() {
	return make_shared<ClassNode>(*this);
}

shared_ptr<AstNode> InterfaceNode::doDuplicate() {
	return make_shared<InterfaceNode>(*this);
}

shared_ptr<AstNode> TraitNode::doDuplicate() {
	return make_shared<TraitNode>(*this);
}
