#include "fn.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCFnNode::BCFnNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::BCFn, self_allocator, document),
	  overloadings(self_allocator) {
}

SLKC_API BCFnNode::~BCFnNode() {
}

SLKC_API BCFnOverloadingNode::BCFnOverloadingNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::BCFnOverloading, self_allocator, document),
	  params(self_allocator),
	  param_indices(self_allocator),
	  generic_params(self_allocator),
	  generic_param_indices(self_allocator),
	  idx_param_comma_tokens(self_allocator),
	  idx_generic_param_comma_tokens(self_allocator),
	  body(self_allocator) {
}

SLKC_API BCFnOverloadingNode::~BCFnOverloadingNode() {
}
