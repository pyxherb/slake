#include "generic.h"

using namespace slkc;

SLKC_API GenericConstraint::GenericConstraint(peff::Alloc *self_allocator) : self_allocator(self_allocator), impl_types(self_allocator) {}
SLKC_API GenericConstraint::~GenericConstraint() {}
SLKC_API void GenericConstraint::dealloc() noexcept {
	peff::destroy_and_release<GenericConstraint>(self_allocator.get(), this, alignof(GenericConstraint));
}

GenericConstraintPtr slkc::duplicate_generic_constraint(peff::Alloc *allocator, const GenericConstraint *constraint){
	GenericConstraintPtr ptr(peff::alloc_and_construct<GenericConstraint>(allocator, alignof(GenericConstraint), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (constraint->base_type && !(ptr->base_type = constraint->base_type->duplicate<TypeNameNode>(allocator))) {
		return nullptr;
	}

	if (!ptr->impl_types.resize(constraint->impl_types.size())) {
		return nullptr;
	}

	for (size_t i = 0; i < ptr->impl_types.size(); ++i) {
		if (!(ptr->impl_types.at(i) = constraint->impl_types.at(i)->duplicate<TypeNameNode>(allocator))) {
			return nullptr;
		}
	}

	return ptr;
}

SLKC_API ParamTypeListGenericConstraint::ParamTypeListGenericConstraint(peff::Alloc *self_allocator) : self_allocator(self_allocator), arg_types(self_allocator) {}
SLKC_API ParamTypeListGenericConstraint::~ParamTypeListGenericConstraint() {}
SLKC_API void ParamTypeListGenericConstraint::dealloc() noexcept {
	peff::destroy_and_release<ParamTypeListGenericConstraint>(self_allocator.get(), this, alignof(ParamTypeListGenericConstraint));
}

ParamTypeListGenericConstraintPtr slkc::duplicate_param_type_list_generic_constraint(peff::Alloc *allocator, const ParamTypeListGenericConstraint *constraint) {
	ParamTypeListGenericConstraintPtr ptr(peff::alloc_and_construct<ParamTypeListGenericConstraint>(allocator, alignof(ParamTypeListGenericConstraint), allocator));

	if (!ptr) {
		return nullptr;
	}

	if (!ptr->arg_types.resize(constraint->arg_types.size())) {
		return nullptr;
	}

	for (size_t i = 0; i < ptr->arg_types.size(); ++i) {
		if (!(ptr->arg_types.at(i) = constraint->arg_types.at(i)->duplicate<TypeNameNode>(allocator))) {
			return nullptr;
		}
	}

	ptr->has_var_arg = constraint->has_var_arg;

	return ptr;
}

SLKC_API AstNodePtr<AstNode> GenericParamNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<GenericParamNode> duplicated_node(make_ast_node<GenericParamNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API GenericParamNode::GenericParamNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::GenericParam, self_allocator, document) {
}

SLKC_API GenericParamNode::GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.input_type && !(input_type = rhs.input_type->do_duplicate(allocator, context).cast_to<TypeNameNode>())) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	if (rhs.generic_constraint && !(generic_constraint = duplicate_generic_constraint(allocator, rhs.generic_constraint.get()))) {
		succeeded_out = false;
		return;
	}

	if (rhs.param_type_list_generic_constraint && !(param_type_list_generic_constraint = duplicate_param_type_list_generic_constraint(allocator, rhs.param_type_list_generic_constraint.get()))) {
		succeeded_out = false;
		return;
	}

	is_param_type_list = rhs.is_param_type_list;

	succeeded_out = true;
}

SLKC_API GenericParamNode::~GenericParamNode() {
}
