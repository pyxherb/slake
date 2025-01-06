#ifndef _SLKC_COMPILER_AST_ALIAS_H_
#define _SLKC_COMPILER_AST_ALIAS_H_

#include "typename.h"
#include "member.h"
#include <variant>

namespace slake {
	namespace slkc {
		class AliasNode : public MemberNode {
		public:
			std::string name;
			Scope *scope;
			std::shared_ptr<IdRefNode> target;

			inline AliasNode(Compiler *compiler, std::string name, Scope *scope, std::shared_ptr<IdRefNode> target)
				: MemberNode(compiler, ACCESS_PUB), name(name), scope(scope), target(target) {}
			virtual ~AliasNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Alias; }

			virtual inline IdRefEntry getName() const { return { tokenRange, SIZE_MAX, name, genericArgs }; }
		};
	}
}

#endif
