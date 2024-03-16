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
			shared_ptr<Scope> scope = make_shared<Scope>();
			deque<shared_ptr<CustomTypeNameNode>> parentInterfaces;  // Parent interfaces
			deque<GenericParam> genericParams;

			inline InterfaceNode(
				Location loc,
				string name,
				deque<shared_ptr<CustomTypeNameNode>> parentInterfaces,
				deque<GenericParam> genericParams)
				: _loc(loc), name(name), parentInterfaces(parentInterfaces), genericParams(genericParams) {
				scope->owner = this;
			}
			virtual ~InterfaceNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Interface; }

			virtual RefEntry getName() const override { return RefEntry({}, name, {}); }
		};
	}
}

#endif
