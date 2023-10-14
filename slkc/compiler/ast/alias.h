#ifndef _SLKC_COMPILER_AST_ALIAS_H_
#define _SLKC_COMPILER_AST_ALIAS_H_

#include "typename.h"
#include <variant>

namespace slake {
	namespace slkc {
		class AliasNode : public AstNode {
		private:
			Location _loc;

		public:
			string name;
			Ref target;

			inline AliasNode(Location loc, string name, Ref target)
				: _loc(loc), name(name), target(target) {}
			virtual ~AliasNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_ALIAS; }
		};
	}
}

#endif
