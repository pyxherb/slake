#ifndef _SLKC_AST_GENERIC_H_
#define _SLKC_AST_GENERIC_H_

#include "module.h"

namespace slkc {
	class NamespaceNode;

	class GenericParamNode : public MemberNode {
	protected:
		SLKC_API virtual peff::SharedPtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		peff::SharedPtr<TypeNameNode> baseType;
		peff::DynArray<peff::SharedPtr<TypeNameNode>> implementedTypes;

		SLKC_API GenericParamNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API GenericParamNode(const GenericParamNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~GenericParamNode();
	};
}

#endif
