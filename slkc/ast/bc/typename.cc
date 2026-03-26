#include "typename.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCCustomTypeNameNode::BCCustomTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::BCCustom, self_allocator, document) {
}

SLKC_API BCCustomTypeNameNode::~BCCustomTypeNameNode() {
}
