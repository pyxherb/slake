#include "class.h"

using namespace slake::slkc;

shared_ptr<AstNode> ClassNode::doDuplicate() {
	return make_shared<ClassNode>(*this);
}
