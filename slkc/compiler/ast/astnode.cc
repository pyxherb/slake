#include "astnode.h"

using namespace slake::slkc;

shared_ptr<AstNode> AstNode::duplicate() {
	// Throw logic error if the duplicate method is not implemented.
	throw logic_error("The node is not duplicatable");
}