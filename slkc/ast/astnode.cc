#include "astnode.h"

using namespace slkc;

SLKC_API AstNode::AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : astNodeType(astNodeType), document(document) {
	assert(document);
}

SLKC_API AstNode::~AstNode() {
}

SLKC_API peff::SharedPtr<AstNode> AstNode::doDuplicate(peff::Alloc *newAllocator) const {
	std::terminate();
}
