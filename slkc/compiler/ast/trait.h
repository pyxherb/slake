#ifndef _SLKC_COMPILER_AST_TRAIT_H_
#define _SLKC_COMPILER_AST_TRAIT_H_

#include "class.h"

namespace slake {
	namespace slkc {
		class TraitNode : public MemberNode {
		private:
			Location _loc;

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;

			deque<shared_ptr<TypeNameNode>> parentTraits;	 // Parent traits

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			shared_ptr<Scope> scope = make_shared<Scope>();

			TraitNode() = default;
			inline TraitNode(const TraitNode &other) : MemberNode(other) {
				_loc = other._loc;

				name = other.name;

				parentTraits.resize(other.parentTraits.size());
				for (size_t i = 0; i < other.parentTraits.size(); ++i)
					parentTraits[i] = other.parentTraits[i]->duplicate<TypeNameNode>();
			}
			inline TraitNode(
				Location loc,
				deque<shared_ptr<TypeNameNode>> parentInterfaces,
				GenericParamNodeList genericParams)
				: _loc(loc) {
				setScope(make_shared<Scope>());
				setGenericParams(genericParams);
			}
			virtual ~TraitNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Trait; }

			virtual RefEntry getName() const override { return RefEntry(_loc, SIZE_MAX, name, genericArgs); }
		};
	}
}

#endif
