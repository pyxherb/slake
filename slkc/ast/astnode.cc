#include "astnode.h"
#include "document.h"

using namespace slkc;

SLAKE_API BaseAstNodeDuplicationTask::~BaseAstNodeDuplicationTask() {

}

SLKC_API AstNode::AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : _astNodeType(astNodeType), selfAllocator(selfAllocator), document(document) {
	assert(document);
	document->clearDeferredDestructibleAstNodes();
}

SLAKE_API AstNode::AstNode(const AstNode &other, peff::Alloc *newAllocator, DuplicationContext &context) {
	other.document->clearDeferredDestructibleAstNodes();
	document = other.document;
	selfAllocator = newAllocator;
	_astNodeType = other._astNodeType;
	tokenRange = other.tokenRange;
}

SLKC_API AstNode::~AstNode() {
}

SLKC_API AstNodePtr<AstNode> AstNode::doDuplicate(peff::Alloc *newAllocator, DuplicationContext &context) const {
	std::terminate();
}

SLKC_API void slkc::addAstNodeToDestructibleList(AstNode *astNode, AstNodeDestructor _destructor) {
	astNode->_nextDestructible = astNode->document->destructibleAstNodeList;
	astNode->_destructor = _destructor;
	astNode->document->destructibleAstNodeList = astNode;
}
