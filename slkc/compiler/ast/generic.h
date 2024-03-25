#ifndef _SLKC_COMPILER_AST_GENERIC_H_
#define _SLKC_COMPILER_AST_GENERIC_H_

#include "typename.h"
#include "scope.h"
#include <slake/access.h>
#include <string>

namespace slake {
	namespace slkc {
		class MemberNode;

		class GenericParamNode : public AstNode {
		private:
			Location _loc;

			virtual shared_ptr<AstNode> doDuplicate() override;

		public:
			string name;
			shared_ptr<TypeNameNode> baseType;
			deque<shared_ptr<TypeNameNode>> traitTypes, interfaceTypes;

			inline GenericParamNode(const GenericParamNode& other) {
				_loc = other._loc;

				name = other.name;
				baseType = other.baseType;
				traitTypes = other.traitTypes;
				interfaceTypes = other.interfaceTypes;
			}
			inline GenericParamNode(
				Location location,
				string name)
				: _loc(location), name(name) {}

			virtual inline NodeType getNodeType() const override { return NodeType::GenericParam; }
			virtual inline Location getLocation() const override { return _loc; }
		};

		using GenericParamNodeList = deque<shared_ptr<GenericParamNode>>;

		inline unordered_map<string, size_t> genGenericParamIndicies(const GenericParamNodeList &genericParams) {
			unordered_map<string, size_t> indices;

			for (size_t i = 0; i < genericParams.size();++i)
				indices[genericParams[i]->name] = i;

			return indices;
		}

		shared_ptr<GenericParamNode> lookupGenericParam(shared_ptr<AstNode> node, string name);
	}
}

#endif
