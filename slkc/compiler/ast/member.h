#ifndef _SLKC_COMPILER_AST_MEMBER_H_
#define _SLKC_COMPILER_AST_MEMBER_H_

#include "astnode.h"

namespace slake {
	namespace slkc {
		class MemberNode : public AstNode {
		public:
			AccessModifier access;
			weak_ptr<AstNode> parent;

			inline MemberNode(AccessModifier access = 0)
				: access(access) {}
			virtual ~MemberNode() = default;

			void bind(shared_ptr<AstNode> parent) {
				assert(this->parent.expired());
				this->parent = parent;
			}

			void unbind() {
				parent.reset();
			}
		};
	}
}

#endif
