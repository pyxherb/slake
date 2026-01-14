#include "expr.h"

using namespace slkc;
using namespace slkc::bc;

SLKC_API BCLabelExprNode::BCLabelExprNode(
	peff::Alloc *selfAllocator,
	const peff::SharedPtr<Document> &document,
	peff::String &&name)
	: ExprNode(ExprKind::BCLabel, selfAllocator, document),
	  name(std::move(name)) {
}
SLKC_API BCLabelExprNode::~BCLabelExprNode() {
}

