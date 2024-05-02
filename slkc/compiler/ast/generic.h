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

			size_t idxNameToken = SIZE_MAX;

			size_t idxParentSlotLParentheseToken = SIZE_MAX,
				   idxParentSlotRParentheseToken = SIZE_MAX;

			size_t idxImplInterfacesColonToken = SIZE_MAX;
			deque<size_t> idxImplInterfacesCommaTokens;

			size_t idxCommaToken = SIZE_MAX;

			inline GenericParamNode(const GenericParamNode &other) {
				_loc = other._loc;

				name = other.name;
				if (baseType)
					baseType = other.baseType->duplicate<TypeNameNode>();

				traitTypes.resize(other.traitTypes.size());
				for (size_t i = 0; i < other.traitTypes.size(); ++i)
					traitTypes[i] = other.traitTypes[i]->duplicate<TypeNameNode>();

				interfaceTypes.resize(other.interfaceTypes.size());
				for (size_t i = 0; i < other.interfaceTypes.size(); ++i)
					interfaceTypes[i] = other.interfaceTypes[i]->duplicate<TypeNameNode>();

				idxNameToken = other.idxNameToken;

				idxParentSlotLParentheseToken = other.idxParentSlotLParentheseToken;
				idxParentSlotRParentheseToken = other.idxParentSlotRParentheseToken;

				idxImplInterfacesColonToken = other.idxImplInterfacesColonToken;
				idxImplInterfacesCommaTokens = other.idxImplInterfacesCommaTokens;
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

			for (size_t i = 0; i < genericParams.size(); ++i)
				indices[genericParams[i]->name] = i;

			return indices;
		}

		shared_ptr<GenericParamNode> lookupGenericParam(shared_ptr<AstNode> node, string name);
	}
}

#endif
