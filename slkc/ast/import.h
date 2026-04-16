#ifndef _SLKC_AST_IMPORT_H_
#define _SLKC_AST_IMPORT_H_

#include "module.h"

namespace slkc {
	class ImportNode : public MemberNode {
	public:
		IdRefPtr id_ref;

		SLKC_API ImportNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
		SLKC_API ImportNode(const ImportNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out);
		SLKC_API virtual ~ImportNode();

		SLKC_API virtual AstNodePtr<AstNode> do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const override;
	};
}

#endif
