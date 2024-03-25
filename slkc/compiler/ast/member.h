#ifndef _SLKC_COMPILER_AST_MEMBER_H_
#define _SLKC_COMPILER_AST_MEMBER_H_

#include "astnode.h"
#include "ref.h"
#include "generic.h"

namespace slake {
	namespace slkc {
		class MemberNode : public AstNode {
		public:
			AccessModifier access = 0;
			MemberNode *parent = nullptr;

			Compiler *compiler = nullptr;

			deque<shared_ptr<TypeNameNode>> genericArgs;
			MemberNode *originalValue = nullptr;

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			shared_ptr<Scope> scope;

			MemberNode() = default;
			inline MemberNode(const MemberNode &other) {
				access = other.access;
				parent = other.parent;

				compiler = other.compiler;

				genericArgs = other.genericArgs;
				originalValue = other.originalValue;

				{
					const size_t genericParamSize = other.genericParams.size();
					genericParams.resize(genericParamSize);
					for (size_t i = 0; i < genericParamSize; ++i) {
						genericParams[i] = other.genericParams[i]->duplicate<GenericParamNode>();
					}
				}
				genericParamIndices = other.genericParamIndices;

				if (other.scope)
					(scope = shared_ptr<Scope>(other.scope->duplicate()))->owner = this;
			}
			inline MemberNode(Compiler *compiler, AccessModifier access = 0)
				: compiler(compiler), access(access) {}
			virtual ~MemberNode();

			void bind(MemberNode *parent) {
				assert(!this->parent);
				this->parent = parent;
			}

			void unbind() {
				parent = nullptr;
			}

			virtual RefEntry getName() const = 0;

			inline void setGenericParams(const GenericParamNodeList &genericParams) {
				this->genericParams = genericParams;
				genericParamIndices = genGenericParamIndicies(genericParams);
			}

			inline void setScope(shared_ptr<Scope> scope) {
				this->scope = scope;
				scope->owner = this;
			}
		};
	}
}

#endif
