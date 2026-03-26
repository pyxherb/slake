#include "attribute.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> AttributeDefNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<AttributeDefNode> duplicated_node(make_ast_node<AttributeDefNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API AttributeDefNode::AttributeDefNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::Attribute), generic_params(self_allocator), generic_param_indices(self_allocator), idx_generic_param_comma_tokens(self_allocator) {
}

SLKC_API AttributeDefNode::AttributeDefNode(const AttributeDefNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out), generic_params(allocator), generic_param_indices(allocator), idx_generic_param_comma_tokens(allocator) {
	if (!succeeded_out) {
		return;
	}

	succeeded_out = true;
}

SLKC_API AttributeDefNode::~AttributeDefNode() {
}

SLKC_API AstNodePtr<AstNode> AttributeNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<AttributeNode> duplicated_node(make_ast_node<AttributeNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API AttributeNode::AttributeNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(AstNodeType::Attribute, self_allocator, document),
	  field_data(self_allocator),
	  idx_comma_tokens(self_allocator) {
}

SLKC_API AttributeNode::AttributeNode(const AttributeNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : AstNode(rhs, allocator, context), field_data(allocator), idx_comma_tokens(allocator) {
	if (!(attribute_name = duplicate_id_ref(allocator, rhs.attribute_name.get()))) {
		succeeded_out = false;
		return;
	}

	if (!field_data.resize(rhs.field_data.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < rhs.field_data.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				AstNodePtr<ExprNode> dd;
				if (!(dd = rhs.field_data.at(i)->duplicate<ExprNode>(allocator))) {
					return false;
				}

				field_data.at(i) = dd;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(applied_for = applied_for->duplicate<TypeNameNode>(allocator))) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API AttributeNode::~AttributeNode() {
}
