#ifndef _SLKC_AST_GENERIC_H_
#define _SLKC_AST_GENERIC_H_

#include "module.h"

namespace slkc {
	class NamespaceNode;

	class GenericConstraint {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		AstNodePtr<TypeNameNode> base_type;
		peff::DynArray<AstNodePtr<TypeNameNode>> impl_types;

		SLKC_API GenericConstraint(peff::Alloc *self_allocator);
		SLKC_API virtual ~GenericConstraint();

		SLKC_API void dealloc() noexcept;
	};
	using GenericConstraintPtr = std::unique_ptr<GenericConstraint, peff::DeallocableDeleter<GenericConstraint>>;

	GenericConstraintPtr duplicate_generic_constraint(peff::Alloc *allocator, const GenericConstraint *constraint);

	class ParamTypeListGenericConstraint {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		peff::DynArray<AstNodePtr<TypeNameNode>> arg_types;
		bool has_var_arg = false;

		SLKC_API ParamTypeListGenericConstraint(peff::Alloc *self_allocator);
		SLKC_API virtual ~ParamTypeListGenericConstraint();

		SLKC_API void dealloc() noexcept;
	};
	using ParamTypeListGenericConstraintPtr = std::unique_ptr<ParamTypeListGenericConstraint, peff::DeallocableDeleter<ParamTypeListGenericConstraint>>;

	ParamTypeListGenericConstraintPtr duplicate_param_type_list_generic_constraint(peff::Alloc *allocator, const ParamTypeListGenericConstraint *constraint);

	class GenericParamNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;

	public:
		AstNodePtr<TypeNameNode> input_type;
		GenericConstraintPtr generic_constraint;
		ParamTypeListGenericConstraintPtr param_type_list_generic_constraint;

		bool is_param_type_list = false;

		SLKC_API GenericParamNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~GenericParamNode();
	};
}

#endif
