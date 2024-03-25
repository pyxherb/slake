#include "interface.h"

using namespace slake::slkc;

shared_ptr<AstNode> InterfaceNode::doDuplicate() {
	return make_shared<InterfaceNode>(*this);
}
