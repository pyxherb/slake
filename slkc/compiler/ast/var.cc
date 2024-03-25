#include "var.h"

using namespace slake::slkc;

shared_ptr<AstNode> VarNode::doDuplicate() {
	return make_shared<VarNode>(*this);
}
