#include "var.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> VarNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<VarNode> duplicated_node(make_ast_node<VarNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API VarNode::VarNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Var, self_allocator, document) {
}

SLKC_API VarNode::VarNode(const VarNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.type && !(type = rhs.type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.initial_value && !(initial_value = rhs.initial_value->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	is_type_deduced_from_initial_value = rhs.is_type_deduced_from_initial_value;
	idx_reg = rhs.idx_reg;

	succeeded_out = true;
}

SLKC_API VarNode::~VarNode() {
}
