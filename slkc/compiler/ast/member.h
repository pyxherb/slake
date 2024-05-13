#ifndef _SLKC_COMPILER_AST_MEMBER_H_
#define _SLKC_COMPILER_AST_MEMBER_H_

#include "astnode.h"
#include "idref.h"
#include "generic.h"

namespace slake {
	namespace slkc {
		class MemberNode : public AstNode {
		public:
			AccessModifier access = 0;
			MemberNode *parent = nullptr; // Don't use std::shared_ptr - or it will cause problems about bad_std::weak_ptr exception.

			Compiler *compiler = nullptr;

			std::deque<std::shared_ptr<TypeNameNode>> genericArgs;
			MemberNode *originalValue = nullptr;

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			std::shared_ptr<Scope> scope;

			bool isImported = false;

			MemberNode() = default;
			inline MemberNode(const MemberNode &other) {
				access = other.access;
				parent = other.parent;

				compiler = other.compiler;

				genericArgs.resize(other.genericArgs.size());
				for (size_t i = 0; i < other.genericArgs.size(); ++i)
					genericArgs[i] = other.genericArgs[i]->duplicate<TypeNameNode>();
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
					setScope(std::shared_ptr<Scope>(other.scope->duplicate()));
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

			virtual IdRefEntry getName() const = 0;

			inline void setGenericParams(const GenericParamNodeList &genericParams) {
				this->genericParams = genericParams;
				genericParamIndices = genGenericParamIndicies(genericParams);
			}

			inline void setScope(std::shared_ptr<Scope> scope) {
				this->scope = scope;
				scope->setOwner(this);
			}
		};
	}
}

#endif
