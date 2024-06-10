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
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			std::string name;
			std::shared_ptr<TypeNameNode> baseType;
			std::deque<std::shared_ptr<TypeNameNode>> traitTypes, interfaceTypes;

			std::weak_ptr<Scope> cachedMergedScope;

			size_t idxNameToken = SIZE_MAX;

			size_t idxParentSlotLParentheseToken = SIZE_MAX,
				   idxParentSlotRParentheseToken = SIZE_MAX;

			size_t idxImplInterfacesColonToken = SIZE_MAX;
			std::deque<size_t> idxImplInterfacesSeparatorTokens;

			size_t idxCommaToken = SIZE_MAX;

			inline GenericParamNode(const GenericParamNode &other) : AstNode(other) {
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
				idxImplInterfacesSeparatorTokens = other.idxImplInterfacesSeparatorTokens;
			}
			inline GenericParamNode(std::string name) : name(name) {}

			virtual inline NodeType getNodeType() const override { return NodeType::GenericParam; }
		};

		using GenericParamNodeList = std::deque<std::shared_ptr<GenericParamNode>>;

		inline std::unordered_map<std::string, size_t> genGenericParamIndicies(const GenericParamNodeList &genericParams) {
			std::unordered_map<std::string, size_t> indices;

			for (size_t i = 0; i < genericParams.size(); ++i)
				indices[genericParams[i]->name] = i;

			return indices;
		}

		std::shared_ptr<GenericParamNode> lookupGenericParam(std::shared_ptr<AstNode> node, std::string name);
	}
}

#endif
