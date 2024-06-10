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
			IdRef target;

			inline AliasNode(Compiler *compiler, std::string name, IdRef target)
				: MemberNode(compiler, ACCESS_PUB), name(name), target(target) {}
			virtual ~AliasNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Alias; }

			virtual inline IdRefEntry getName() const { return { sourceLocation, SIZE_MAX, name, genericArgs }; }
		};
	}
}

#endif
