#include "import.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> ImportNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ImportNode> duplicated_node(make_ast_node<ImportNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ImportNode::ImportNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::Import, self_allocator, document) {
}

SLKC_API ImportNode::ImportNode(const ImportNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	if (!(id_ref = duplicate_id_ref(allocator, rhs.id_ref.get()))) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ImportNode::~ImportNode() {
}
