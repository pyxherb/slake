#ifndef _SLKC_AST_BC_EXPR_H_
#define _SLKC_AST_BC_EXPR_H_

#include "../expr.h"

namespace slkc {
	namespace bc {
		class BCLabelExprNode : public ExprNode {
		public:
			peff::String name;

			SLKC_API BCLabelExprNode(peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document, peff::String &&name);
			SLKC_API virtual ~BCLabelExprNode();
		};
	}
}

#endif
