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

			void bind(shared_ptr<MemberNode> parent) {
				assert(!this->parent);
				this->parent = parent.get();
			}

			void unbind() {
				parent = nullptr;
			}

			virtual Ref getName() const = 0;
		};
	}
}

#endif
