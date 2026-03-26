#ifndef _SLKC_AST_BC_TYPENAME_H_
#define _SLKC_AST_BC_TYPENAME_H_

#include "../typename.h"

namespace slkc {
	namespace bc {
		class BCCustomTypeNameNode : public TypeNameNode {
		public:
			IdRefPtr id_ref_ptr;

			SLKC_API BCCustomTypeNameNode(peff::Alloc *self_allocator, const peff::SharedPtr<Document> &document);
			SLKC_API virtual ~BCCustomTypeNameNode();
		};
	}
}

#endif
