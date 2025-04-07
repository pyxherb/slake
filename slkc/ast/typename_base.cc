#include "typename_base.h"

using namespace slkc;

SLKC_API TypeNameNode::TypeNameNode(TypeNameKind typeNameKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::TypeName, selfAllocator, document), typeNameKind(typeNameKind) {
}

SLKC_API TypeNameNode::TypeNameNode(const TypeNameNode &rhs, peff::Alloc *selfAllocator) : AstNode(rhs, selfAllocator), typeNameKind(rhs.typeNameKind) {
}

SLKC_API TypeNameNode::~TypeNameNode() {
}
