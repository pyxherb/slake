#include "fn.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCFnNode::BCFnNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::BCFn, selfAllocator, document),
	  overloadings(selfAllocator) {
}

SLKC_API BCFnNode::~BCFnNode() {
}

SLKC_API BCFnOverloadingNode::BCFnOverloadingNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::BCFnOverloading, selfAllocator, document),
	  params(selfAllocator),
	  paramIndices(selfAllocator),
	  genericParams(selfAllocator),
	  genericParamIndices(selfAllocator),
	  idxParamCommaTokens(selfAllocator),
	  idxGenericParamCommaTokens(selfAllocator),
	  body(selfAllocator) {
}

SLKC_API BCFnOverloadingNode::~BCFnOverloadingNode() {
}
