#include "typename_base.h"

using namespace slkc;

SLKC_API TypeNameNode::TypeNameNode(TypeNameKind type_name_kind, peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : AstNode(AstNodeType::TypeName, self_allocator, document), type_name_kind(type_name_kind) {
}

SLKC_API TypeNameNode::TypeNameNode(const TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context)
	: AstNode(rhs, self_allocator, context),
	  type_name_kind(rhs.type_name_kind),
	  is_final(rhs.is_final),
	  is_local(rhs.is_local),
	  is_nullable(rhs.is_nullable),
	  idx_final_token(rhs.idx_final_token),
	  idx_local_token(rhs.idx_local_token) {
}

SLKC_API TypeNameNode::~TypeNameNode() {
}
