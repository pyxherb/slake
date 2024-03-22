#ifndef _SLKC_COMPILER_AST_MEMBER_H_
#define _SLKC_COMPILER_AST_MEMBER_H_

#include "astnode.h"
#include "ref.h"

namespace slake {
	namespace slkc {
		class MemberNode : public AstNode {
		public:
			AccessModifier access = 0;
			MemberNode* parent = nullptr;

			Compiler *compiler = nullptr;

			deque<shared_ptr<TypeNameNode>> genericArgs;
			MemberNode* originalValue = nullptr;

			MemberNode() = default;
			inline MemberNode(Compiler *compiler, AccessModifier access = 0)
				: compiler(compiler), access(access) {}
			virtual ~MemberNode();

			void bind(MemberNode* parent) {
				assert(!this->parent);
				this->parent = parent;
			}

			void unbind() {
				parent = nullptr;
			}

			virtual RefEntry getName() const = 0;

			MemberNode &operator=(const MemberNode &) = default;
		};
	}
}

#endif
