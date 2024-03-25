#include "trait.h"

using namespace slake::slkc;

shared_ptr<AstNode> TraitNode::doDuplicate() {
	return make_shared<TraitNode>(*this);
}
