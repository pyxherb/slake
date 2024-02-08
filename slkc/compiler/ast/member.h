#ifndef _SLKC_COMPILER_AST_MEMBER_H_
#define _SLKC_COMPILER_AST_MEMBER_H_

#include "astnode.h"
#include "ref.h"

namespace slake {
	namespace slkc {
		class MemberNode : public AstNode {
		public:
			AccessModifier access;
			MemberNode* parent = nullptr;

			inline MemberNode(AccessModifier access = 0)
				: access(access) {}
			virtual ~MemberNode() = default;

			void bind(MemberNode* parent) {
				assert(!this->parent);
				this->parent = parent;
			}

			void unbind() {
				parent = nullptr;
			}

			virtual RefEntry getName() const = 0;
		};
	}
}

#endif
