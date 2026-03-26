#include "fn.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> FnNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<FnNode> duplicated_node(make_ast_node<FnNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API FnNode::FnNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Fn, self_allocator, document),
	  overloadings(self_allocator) {
}

SLKC_API FnNode::FnNode(const FnNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out), overloadings(allocator) {
	if (!succeeded_out) {
		return;
	}

	if (!overloadings.resize(rhs.overloadings.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < overloadings.size(); ++i) {
		if (!(overloadings.at(i) = rhs.overloadings.at(i)->duplicate<FnOverloadingNode>(allocator))) {
			succeeded_out = false;
			return;
		}

		overloadings.at(i)->set_parent(this);
	}

	succeeded_out = true;
}

SLKC_API FnNode::~FnNode() {
}

SLKC_API AstNodePtr<AstNode> FnOverloadingNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<FnOverloadingNode> duplicated_node(make_ast_node<FnOverloadingNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API FnOverloadingNode::FnOverloadingNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::FnOverloading, self_allocator, document),
	  params(self_allocator),
	  param_indices(self_allocator),
	  generic_params(self_allocator),
	  generic_param_indices(self_allocator),
	  idx_param_comma_tokens(self_allocator),
	  idx_generic_param_comma_tokens(self_allocator) {
}

SLKC_API FnOverloadingNode::FnOverloadingNode(const FnOverloadingNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out)
	: MemberNode(rhs, allocator, context, succeeded_out),
	  params(allocator),
	  param_indices(allocator),
	  generic_params(allocator),
	  generic_param_indices(allocator),
	  idx_param_comma_tokens(allocator),
	  idx_generic_param_comma_tokens(allocator),
	  l_angle_bracket_index(rhs.l_angle_bracket_index),
	  r_angle_bracket_index(rhs.r_angle_bracket_index),
	  lvalue_marker_index(rhs.lvalue_marker_index),
	  return_type_token_index(rhs.return_type_token_index),
	  overloading_kind(rhs.overloading_kind),
	  fn_flags(rhs.fn_flags) {
	/* if (rhs.body && !(body = rhs.body->duplicate<CodeBlockStmtNode>(allocator))) {
		succeeded_out = false;
		return;
	}*/

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.return_type && !(return_type = rhs.return_type->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.overriden_type && !(overriden_type = rhs.overriden_type->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!params.resize(rhs.params.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < params.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(params.at(i) = rhs.params.at(i)->duplicate<VarNode>(allocator))) {
					return false;
				}

				if (!(param_indices.insert(params.at(i)->name, +i))) {
					return false;
				}

				params.at(i)->set_parent(this);
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!generic_params.resize(rhs.generic_params.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < generic_params.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(generic_params.at(i) = rhs.generic_params.at(i)->duplicate<GenericParamNode>(allocator)))
					return false;

				if (!generic_param_indices.insert(generic_params.at(i)->name, +i))
					return false;

				generic_params.at(i)->set_parent(this);
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	is_params_indexed = rhs.is_params_indexed;

	if (!idx_generic_param_comma_tokens.resize(rhs.idx_generic_param_comma_tokens.size())) {
		succeeded_out = false;
		return;
	}
	memcpy(idx_generic_param_comma_tokens.data(), rhs.idx_generic_param_comma_tokens.data(), idx_generic_param_comma_tokens.size() * sizeof(size_t));

	succeeded_out = true;
}

SLKC_API FnOverloadingNode::~FnOverloadingNode() {
}
