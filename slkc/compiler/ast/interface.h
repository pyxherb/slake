#ifndef _SLKC_COMPILER_AST_INTERFACE_H_
#define _SLKC_COMPILER_AST_INTERFACE_H_

#include "class.h"

namespace slake {
	namespace slkc {
		class InterfaceNode : public MemberNode {
		private:
			Location _loc;

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;

			deque<shared_ptr<TypeNameNode>> parentInterfaces;  // Parent interfaces

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			shared_ptr<Scope> scope = make_shared<Scope>();

			InterfaceNode() = default;
			inline InterfaceNode(const InterfaceNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				parentInterfaces.resize(other.parentInterfaces.size());
				for (size_t i = 0; i < other.parentInterfaces.size(); ++i)
					parentInterfaces[i] = other.parentInterfaces[i]->duplicate<TypeNameNode>();
			}
			inline InterfaceNode(
				Location loc,
				string name,
				deque<shared_ptr<TypeNameNode>> parentInterfaces,
				GenericParamNodeList genericParams)
				: _loc(loc),
				  name(name),
				  parentInterfaces(parentInterfaces) {
				scope = make_shared<Scope>();
				setScope(make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~InterfaceNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Interface; }

			virtual RefEntry getName() const override { return RefEntry(_loc, name, genericArgs); }
		};
	}
}

#endif
