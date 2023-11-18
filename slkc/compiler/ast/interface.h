#ifndef _SLKC_COMPILER_AST_INTERFACE_H_
#define _SLKC_COMPILER_AST_INTERFACE_H_

#include "class.h"

namespace slake {
	namespace slkc {
		class InterfaceNode : public MemberNode {
		private:
			Location _loc;

		public:
			string name;
			shared_ptr<Scope> scope;
			deque<shared_ptr<CustomTypeNameNode>> parentInterfaces;  // Parent interfaces

			inline InterfaceNode(
				Location loc,
				shared_ptr<Scope> scope = make_shared<Scope>())
				: _loc(loc), scope(scope) {
				scope->owner = this;
			}
			virtual ~InterfaceNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_INTERFACE; }

			virtual Ref getName() const override { return Ref({ RefEntry({}, name, {}) }); }
		};
	}
}

#endif
