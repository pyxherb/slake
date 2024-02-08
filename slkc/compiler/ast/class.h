#ifndef _SLKC_COMPILER_AST_CLASS_H_
#define _SLKC_COMPILER_AST_CLASS_H_

#include "member.h"
#include "scope.h"
#include "ref.h"
#include "generic.h"

namespace slake {
	namespace slkc {
		class ClassNode : public MemberNode {
		private:
			Location _loc;

		public:
			string name;
			shared_ptr<CustomTypeNameNode> parentClass;			   // Parent class
			deque<shared_ptr<CustomTypeNameNode>> implInterfaces;  // Implemented interfaces
			deque<GenericParam> genericParams;
			shared_ptr<Scope> scope = make_shared<Scope>();

			inline ClassNode(
				Location loc,
				string name,
				shared_ptr<CustomTypeNameNode> parentClass,
				deque<shared_ptr<CustomTypeNameNode>> implInterfaces,
				deque<GenericParam> genericParams)
				: _loc(loc),
				  name(name),
				  parentClass(parentClass),
				  implInterfaces(implInterfaces),
				  genericParams(genericParams) {
				scope->owner = this;
			}
			virtual ~ClassNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return AST_CLASS; }

			virtual RefEntry getName() const override { return RefEntry({}, name, {}); }
		};
	}
}

#endif
