#include "typename_base.h"

using namespace slkc;

SLKC_API TypeNameNode::TypeNameNode(TypeNameKind typeNameKind, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::TypeName, selfAllocator, document), typeNameKind(typeNameKind) {
}

SLKC_API TypeNameNode::TypeNameNode(const TypeNameNode &rhs, peff::Alloc *selfAllocator, DuplicationContext &context) : AstNode(rhs, selfAllocator, context), typeNameKind(rhs.typeNameKind), isFinal(rhs.isFinal) {
}

SLKC_API TypeNameNode::~TypeNameNode() {
}
