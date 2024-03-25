#include "astnode.h"

using namespace slake::slkc;

shared_ptr<AstNode> AstNode::doDuplicate() {
	// Throw a logic error if the duplicate method is not implemented.
	throw logic_error("The node is not duplicatable");
}
