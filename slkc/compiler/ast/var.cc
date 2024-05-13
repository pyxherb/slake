#include "var.h"

using namespace slake::slkc;

std::shared_ptr<AstNode> VarNode::doDuplicate() {
	return std::make_shared<VarNode>(*this);
}
