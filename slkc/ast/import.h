#ifndef _SLKC_AST_IMPORT_H_
#define _SLKC_AST_IMPORT_H_

#include "module.h"

namespace slkc {
	class ImportNode : public MemberNode {
	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const override;

	public:
		IdRefPtr idRef;

		SLKC_API ImportNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API ImportNode(const ImportNode &rhs, peff::Alloc *allocator, bool &succeededOut);
		SLKC_API virtual ~ImportNode();
	};
}

#endif
