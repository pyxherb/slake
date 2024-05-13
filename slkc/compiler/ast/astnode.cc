#include "astnode.h"

using namespace slake::slkc;

std::shared_ptr<AstNode> AstNode::doDuplicate() {
	// Throw a logic error if the duplicate method is not implemented.
	throw std::logic_error("The node is not duplicatable");
}
