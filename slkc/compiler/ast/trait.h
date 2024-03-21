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
			shared_ptr<Scope> scope = make_shared<Scope>();
			deque<shared_ptr<CustomTypeNameNode>> parentTraits;	 // Parent traits

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			TraitNode() = default;
			inline TraitNode(
				Location loc,
				deque<shared_ptr<CustomTypeNameNode>> parentInterfaces,
				GenericParamNodeList genericParams)
				: _loc(loc),
				  scope(scope),
				  genericParams(genericParams),
				  genericParamIndices(genGenericParamIndicies(genericParams)) {
				scope->owner = this;
			}
			virtual ~TraitNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Trait; }

			virtual RefEntry getName() const override { return RefEntry(_loc, name, genericArgs); }

			TraitNode &operator=(const TraitNode &rhs) = default;

			virtual inline shared_ptr<AstNode> duplicate() override {
				shared_ptr<TraitNode> newInstance = make_shared<TraitNode>();
				(*newInstance.get()) = *this;
				return static_pointer_cast<AstNode>(newInstance);
			}
		};
	}
}

#endif
