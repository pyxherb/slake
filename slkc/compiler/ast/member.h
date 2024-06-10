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
			std::deque<size_t> idxAccessModifierTokens;

			MemberNode *parent = nullptr;  // Don't use std::shared_ptr - or it will cause problems about bad_std::weak_ptr exception.

			Compiler *compiler = nullptr;

			std::deque<std::shared_ptr<TypeNameNode>> genericArgs;
			MemberNode *originalValue = nullptr;

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			std::shared_ptr<Scope> scope;

			bool isImported = false;
			bool isCompiling = false;

			MemberNode() = default;
			inline MemberNode(const MemberNode &other) : AstNode(other) {
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

			/// @brief Get placeholder generic parameters during the member is being compiled.
			/// @return Placeholder arguments if the member is being compiled, an empty deque otherwise.
			inline std::deque<std::shared_ptr<TypeNameNode>> getPlaceholderGenericArgs() const {
				if (!isCompiling)
					return {};

				std::deque<std::shared_ptr<TypeNameNode>> placeholderGenericArgs;

				for (auto &i : genericParams) {
					placeholderGenericArgs.push_back(
						std::make_shared<CustomTypeNameNode>(
							IdRef{ IdRefEntry{ SourceLocation(), SIZE_MAX, i->name, {} } },
							compiler,
							scope.get()));
				}

				return placeholderGenericArgs;
			}
		};

		struct MemberNodeCompilingStatusGuard {
			std::shared_ptr<MemberNode> memberNode;
			bool prevStatus;

			inline MemberNodeCompilingStatusGuard(std::shared_ptr<MemberNode> memberNode)
				: memberNode(memberNode),
				  prevStatus(memberNode->isCompiling) {
				memberNode->isCompiling = true;
			}
			inline ~MemberNodeCompilingStatusGuard() {
				memberNode->isCompiling = prevStatus;
			}
		};
	}
}

#endif
