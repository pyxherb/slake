#include "typename.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCCustomTypeNameNode::BCCustomTypeNameNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::BCCustom, selfAllocator, document) {
}

SLKC_API BCCustomTypeNameNode::~BCCustomTypeNameNode() {
}
