#include "expr.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCLabelExprNode::BCLabelExprNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	peff::String &&name)
	: ExprNode(ExprKind::BCLabel, self_allocator, document),
	  name(std::move(name)) {
}
SLKC_API BCLabelExprNode::~BCLabelExprNode() {
}

