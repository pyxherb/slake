#include "astnode.h"
#include "document.h"

using namespace slkc;

SLKC_API AstNode::AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : astNodeType(astNodeType), selfAllocator(selfAllocator), document(document) {
	assert(document);
	document->clearDeferredDestructibleAstNodes();
}

SLAKE_API AstNode::AstNode(const AstNode &other, peff::Alloc *newAllocator) {
	other.document->clearDeferredDestructibleAstNodes();
	document = other.document;
	selfAllocator = newAllocator;
	astNodeType = other.astNodeType;
	tokenRange = other.tokenRange;
}

SLKC_API AstNode::~AstNode() {
}

SLKC_API peff::SharedPtr<AstNode> AstNode::doDuplicate(peff::Alloc *newAllocator) const {
	std::terminate();
}

SLKC_API void slkc::addAstNodeToDestructibleList(AstNode *astNode, AstNodeDestructor destructor) {
	astNode->nextDestructible = astNode->document->destructibleAstNodeList;
	astNode->destructor = destructor;
	astNode->document->destructibleAstNodeList = astNode;
}
