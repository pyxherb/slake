#ifndef _SLKC_COMPILER_AST_TRAIT_H_
#define _SLKC_COMPILER_AST_TRAIT_H_

#include "class.h"

namespace slake {
	namespace slkc {
		class TraitNode : public MemberNode {
		private:
			Location _loc;

		public:
			string name;
			shared_ptr<Scope> scope;
			deque<Ref> parentTraits;  // Parent traits

			inline TraitNode(
				Location loc,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: _loc(loc), scope(scope) {}
			virtual ~TraitNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_TRAIT; }
		};
	}
}

#endif
