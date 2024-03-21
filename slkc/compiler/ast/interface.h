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
			deque<shared_ptr<CustomTypeNameNode>> parentInterfaces;	 // Parent interfaces

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			InterfaceNode() = default;
			inline InterfaceNode(
				Location loc,
				string name,
				deque<shared_ptr<CustomTypeNameNode>> parentInterfaces,
				GenericParamNodeList genericParams)
				: _loc(loc),
				  name(name),
				  parentInterfaces(parentInterfaces),
				  genericParams(genericParams),
				  genericParamIndices(genGenericParamIndicies(genericParams)) {
				scope->owner = this;
			}
			virtual ~InterfaceNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Interface; }

			virtual RefEntry getName() const override { return RefEntry(_loc, name, genericArgs); }

			InterfaceNode &operator=(const InterfaceNode &) = default;

			virtual inline shared_ptr<AstNode> duplicate() override {
				shared_ptr<InterfaceNode> newInstance = make_shared<InterfaceNode>();
				(*newInstance.get()) = *this;
				return static_pointer_cast<AstNode>(newInstance);
			}
		};
	}
}

#endif
