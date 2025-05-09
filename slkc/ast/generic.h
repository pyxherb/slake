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

	class GenericParamNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		GenericConstraintPtr genericConstraint;

		SLKC_API GenericParamNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~GenericParamNode();
	};
}

#endif
