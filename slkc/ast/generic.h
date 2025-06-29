#ifndef _SLKC_AST_GENERIC_H_
#define _SLKC_AST_GENERIC_H_

#include "module.h"

namespace slkc {
	class NamespaceNode;

	class GenericConstraint {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::SharedPtr<TypeNameNode> baseType;
		peff::DynArray<peff::SharedPtr<TypeNameNode>> implTypes;

		SLKC_API GenericConstraint(peff::Alloc *selfAllocator);
		SLKC_API virtual ~GenericConstraint();

		SLKC_API void dealloc() noexcept;
	};
	using GenericConstraintPtr = std::unique_ptr<GenericConstraint, peff::DeallocableDeleter<GenericConstraint>>;

	GenericConstraintPtr duplicateGenericConstraint(peff::Alloc *allocator, const GenericConstraint *constraint);

	class ParamTypeListGenericConstraint {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::DynArray<peff::SharedPtr<TypeNameNode>> argTypes;
		bool hasVarArg = false;

		SLKC_API ParamTypeListGenericConstraint(peff::Alloc *selfAllocator);
		SLKC_API virtual ~ParamTypeListGenericConstraint();

		SLKC_API void dealloc() noexcept;
	};
	using ParamTypeListGenericConstraintPtr = std::unique_ptr<ParamTypeListGenericConstraint, peff::DeallocableDeleter<ParamTypeListGenericConstraint>>;

	ParamTypeListGenericConstraintPtr duplicateParamTypeListGenericConstraint(peff::Alloc *allocator, const ParamTypeListGenericConstraint *constraint);

	class GenericParamNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		GenericConstraintPtr genericConstraint;

		ParamTypeListGenericConstraintPtr paramTypeListGenericConstraint;

		bool isParamTypeList = false;

		SLKC_API GenericParamNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~GenericParamNode();
	};
}

#endif
