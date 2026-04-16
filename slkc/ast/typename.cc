#include "typename.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> VoidTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<VoidTypeNameNode> duplicated_node(make_ast_node<VoidTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Void, self_allocator, document) {
}

SLKC_API VoidTypeNameNode::VoidTypeNameNode(const VoidTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API VoidTypeNameNode::~VoidTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I8TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I8TypeNameNode> duplicated_node(peff::make_shared_with_control_block<I8TypeNameNode, AstNodeControlBlock<I8TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I8TypeNameNode::I8TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I8, self_allocator, document) {
}

SLKC_API I8TypeNameNode::I8TypeNameNode(const I8TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API I8TypeNameNode::~I8TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I16TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I16TypeNameNode> duplicated_node(peff::make_shared_with_control_block<I16TypeNameNode, AstNodeControlBlock<I16TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I16TypeNameNode::I16TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I16, self_allocator, document) {
}

SLKC_API I16TypeNameNode::I16TypeNameNode(const I16TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API I16TypeNameNode::~I16TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I32TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I32TypeNameNode> duplicated_node(peff::make_shared_with_control_block<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I32TypeNameNode::I32TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I32, self_allocator, document) {
}

SLKC_API I32TypeNameNode::I32TypeNameNode(const I32TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API I32TypeNameNode::~I32TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> I64TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<I64TypeNameNode> duplicated_node(peff::make_shared_with_control_block<I64TypeNameNode, AstNodeControlBlock<I64TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API I64TypeNameNode::I64TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::I64, self_allocator, document) {
}

SLKC_API I64TypeNameNode::I64TypeNameNode(const I64TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API I64TypeNameNode::~I64TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U8TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U8TypeNameNode> duplicated_node(peff::make_shared_with_control_block<U8TypeNameNode, AstNodeControlBlock<U8TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U8TypeNameNode::U8TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U8, self_allocator, document) {
}

SLKC_API U8TypeNameNode::U8TypeNameNode(const U8TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API U8TypeNameNode::~U8TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U16TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U16TypeNameNode> duplicated_node(peff::make_shared_with_control_block<U16TypeNameNode, AstNodeControlBlock<U16TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U16TypeNameNode::U16TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U16, self_allocator, document) {
}

SLKC_API U16TypeNameNode::U16TypeNameNode(const U16TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API U16TypeNameNode::~U16TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U32TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U32TypeNameNode> duplicated_node(peff::make_shared_with_control_block<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U32TypeNameNode::U32TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U32, self_allocator, document) {
}

SLKC_API U32TypeNameNode::U32TypeNameNode(const U32TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API U32TypeNameNode::~U32TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> U64TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<U64TypeNameNode> duplicated_node(peff::make_shared_with_control_block<U64TypeNameNode, AstNodeControlBlock<U64TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API U64TypeNameNode::U64TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::U64, self_allocator, document) {
}

SLKC_API U64TypeNameNode::U64TypeNameNode(const U64TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API U64TypeNameNode::~U64TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ISizeTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<ISizeTypeNameNode> duplicated_node(make_ast_node<ISizeTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::ISize, self_allocator, document) {
}

SLKC_API ISizeTypeNameNode::ISizeTypeNameNode(const ISizeTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API ISizeTypeNameNode::~ISizeTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> USizeTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<USizeTypeNameNode> duplicated_node(make_ast_node<USizeTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::USize, self_allocator, document) {
}

SLKC_API USizeTypeNameNode::USizeTypeNameNode(const USizeTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API USizeTypeNameNode::~USizeTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> F32TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<F32TypeNameNode> duplicated_node(peff::make_shared_with_control_block<F32TypeNameNode, AstNodeControlBlock<F32TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API F32TypeNameNode::F32TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::F32, self_allocator, document) {
}

SLKC_API F32TypeNameNode::F32TypeNameNode(const F32TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API F32TypeNameNode::~F32TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> F64TypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	peff::SharedPtr<F64TypeNameNode> duplicated_node(peff::make_shared_with_control_block<F64TypeNameNode, AstNodeControlBlock<F64TypeNameNode>>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API F64TypeNameNode::F64TypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::F64, self_allocator, document) {
}

SLKC_API F64TypeNameNode::F64TypeNameNode(const F64TypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API F64TypeNameNode::~F64TypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> StringTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<StringTypeNameNode> duplicated_node(make_ast_node<StringTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API StringTypeNameNode::StringTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::String, self_allocator, document) {
}

SLKC_API StringTypeNameNode::StringTypeNameNode(const StringTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API StringTypeNameNode::~StringTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> BoolTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<BoolTypeNameNode> duplicated_node(make_ast_node<BoolTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Bool, self_allocator, document) {
}

SLKC_API BoolTypeNameNode::BoolTypeNameNode(const BoolTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API BoolTypeNameNode::~BoolTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ObjectTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<ObjectTypeNameNode> duplicated_node(make_ast_node<ObjectTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Object, self_allocator, document) {
}

SLKC_API ObjectTypeNameNode::ObjectTypeNameNode(const ObjectTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API ObjectTypeNameNode::~ObjectTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> AnyTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	AstNodePtr<AnyTypeNameNode> duplicated_node(make_ast_node<AnyTypeNameNode>(new_allocator, *this, new_allocator, context));
	if (!duplicated_node) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Any, self_allocator, document) {
}

SLKC_API AnyTypeNameNode::AnyTypeNameNode(const AnyTypeNameNode &rhs, peff::Alloc *self_allocator, DuplicationContext &context) : TypeNameNode(rhs, self_allocator, context) {
}

SLKC_API AnyTypeNameNode::~AnyTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> CustomTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<CustomTypeNameNode> duplicated_node(make_ast_node<CustomTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Custom, self_allocator, document) {
}

SLKC_API CustomTypeNameNode::CustomTypeNameNode(const CustomTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	if (!(id_ref_ptr = duplicate_id_ref(allocator, rhs.id_ref_ptr.get()))) {
		succeeded_out = false;
		return;
	}

	context_node = rhs.context_node;

	succeeded_out = true;
}

SLKC_API CustomTypeNameNode::~CustomTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> UnpackingTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnpackingTypeNameNode> duplicated_node(make_ast_node<UnpackingTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API UnpackingTypeNameNode::UnpackingTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Unpacking, self_allocator, document) {
}

SLKC_API UnpackingTypeNameNode::UnpackingTypeNameNode(const UnpackingTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(inner_type_name = rhs.inner_type_name->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API UnpackingTypeNameNode::~UnpackingTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ArrayTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ArrayTypeNameNode> duplicated_node(make_ast_node<ArrayTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &element_type) : TypeNameNode(TypeNameKind::Array, self_allocator, document), element_type(element_type) {
}

SLKC_API ArrayTypeNameNode::ArrayTypeNameNode(const ArrayTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(element_type = rhs.element_type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ArrayTypeNameNode::~ArrayTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> TupleTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<TupleTypeNameNode> duplicated_node(make_ast_node<TupleTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API TupleTypeNameNode::TupleTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Tuple, self_allocator, document), element_types(self_allocator), idx_comma_tokens(self_allocator) {
}

SLKC_API TupleTypeNameNode::TupleTypeNameNode(const TupleTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context), element_types(allocator), idx_comma_tokens(allocator) {
	if (!element_types.resize(rhs.element_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < element_types.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(element_types.at(i) = rhs.element_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!idx_comma_tokens.resize_uninit(rhs.idx_comma_tokens.size())) {
		succeeded_out = false;
		return;
	}
	idx_lbracket_token = rhs.idx_lbracket_token;
	idx_rbracket_token = rhs.idx_rbracket_token;

	succeeded_out = true;
}

SLKC_API TupleTypeNameNode::~TupleTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> SIMDTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<SIMDTypeNameNode> duplicated_node(make_ast_node<SIMDTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API SIMDTypeNameNode::SIMDTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::SIMD, self_allocator, document) {
}

SLKC_API SIMDTypeNameNode::SIMDTypeNameNode(const SIMDTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(element_type = rhs.element_type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(width = rhs.width->do_duplicate(allocator, context).cast_to<ExprNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	idx_langle_bracket_token = rhs.idx_langle_bracket_token;
	idx_comma_token = rhs.idx_comma_token;
	idx_rangle_bracket_token = rhs.idx_rangle_bracket_token;

	succeeded_out = true;
}

SLKC_API SIMDTypeNameNode::~SIMDTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> FnTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<FnTypeNameNode> duplicated_node(make_ast_node<FnTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API FnTypeNameNode::FnTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Fn, self_allocator, document), param_types(self_allocator) {
}

SLKC_API FnTypeNameNode::FnTypeNameNode(const FnTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context), param_types(allocator) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.return_type && !(return_type = rhs.return_type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.this_type && !(this_type = rhs.this_type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (!param_types.resize(rhs.param_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < param_types.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(param_types.at(i) = rhs.param_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	has_var_args = rhs.has_var_args;
	is_for_adl = rhs.is_for_adl;

	succeeded_out = true;
}

SLKC_API FnTypeNameNode::~FnTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> RefTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<RefTypeNameNode> duplicated_node(make_ast_node<RefTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API RefTypeNameNode::RefTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referenced_type) : TypeNameNode(TypeNameKind::Ref, self_allocator, document), referenced_type(referenced_type) {
}

SLKC_API RefTypeNameNode::RefTypeNameNode(const RefTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(referenced_type = rhs.referenced_type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API RefTypeNameNode::~RefTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> TempRefTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<TempRefTypeNameNode> duplicated_node(make_ast_node<TempRefTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API TempRefTypeNameNode::TempRefTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document, const AstNodePtr<TypeNameNode> &referenced_type) : TypeNameNode(TypeNameKind::Ref, self_allocator, document), referenced_type(referenced_type) {
}

SLKC_API TempRefTypeNameNode::TempRefTypeNameNode(const TempRefTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (!(referenced_type = rhs.referenced_type->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
				return false;
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API TempRefTypeNameNode::~TempRefTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> ParamTypeListTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ParamTypeListTypeNameNode> duplicated_node(make_ast_node<ParamTypeListTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ParamTypeListTypeNameNode::ParamTypeListTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::ParamTypeList, self_allocator, document), param_types(self_allocator) {
}

SLKC_API ParamTypeListTypeNameNode::ParamTypeListTypeNameNode(const ParamTypeListTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context), param_types(allocator) {
	if (!param_types.resize(rhs.param_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < param_types.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(param_types.at(i) = rhs.param_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	has_var_args = rhs.has_var_args;

	succeeded_out = true;
}

SLKC_API ParamTypeListTypeNameNode::~ParamTypeListTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> UnpackedParamsTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnpackedParamsTypeNameNode> duplicated_node(make_ast_node<UnpackedParamsTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API UnpackedParamsTypeNameNode::UnpackedParamsTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::UnpackedParams, self_allocator, document), param_types(self_allocator) {
}

SLKC_API UnpackedParamsTypeNameNode::UnpackedParamsTypeNameNode(const UnpackedParamsTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context), param_types(allocator) {
	if (!param_types.resize(rhs.param_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < param_types.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(param_types.at(i) = rhs.param_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	has_var_args = rhs.has_var_args;

	succeeded_out = true;
}

SLKC_API UnpackedParamsTypeNameNode::~UnpackedParamsTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> UnpackedArgsTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnpackedArgsTypeNameNode> duplicated_node(make_ast_node<UnpackedArgsTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API UnpackedArgsTypeNameNode::UnpackedArgsTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::UnpackedArgs, self_allocator, document), param_types(self_allocator) {
}

SLKC_API UnpackedArgsTypeNameNode::UnpackedArgsTypeNameNode(const UnpackedArgsTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context), param_types(allocator) {
	if (!param_types.resize(rhs.param_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < param_types.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(param_types.at(i) = rhs.param_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	has_var_args = rhs.has_var_args;

	succeeded_out = true;
}

SLKC_API UnpackedArgsTypeNameNode::~UnpackedArgsTypeNameNode() {
}

SLKC_API AstNodePtr<AstNode> NullTypeNameNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<NullTypeNameNode> duplicated_node(make_ast_node<NullTypeNameNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API NullTypeNameNode::NullTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document) : TypeNameNode(TypeNameKind::Null, self_allocator, document) {
	is_nullable = true;
}

SLKC_API NullTypeNameNode::NullTypeNameNode(const NullTypeNameNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : TypeNameNode(rhs, allocator, context) {
	succeeded_out = true;
}

SLKC_API NullTypeNameNode::~NullTypeNameNode() {
}
