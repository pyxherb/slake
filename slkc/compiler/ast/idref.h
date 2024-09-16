#ifndef _SLKC_COMPILER_AST_REF_H_
#define _SLKC_COMPILER_AST_REF_H_

#include "astnode.h"
#include <stdexcept>

namespace slake {
	namespace slkc {
		class TypeNameNode;

		struct IdRefEntry {
			TokenRange tokenRange;
			size_t idxAccessOpToken = SIZE_MAX;	 // Index of preceding access operator token
			size_t idxToken = SIZE_MAX;
			std::string name;
			std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

			bool hasParamTypes;
			std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
			bool hasVarArg;

			inline IdRefEntry(
				std::string name,
				std::deque<std::shared_ptr<TypeNameNode>> genericArgs = {},
				bool hasParamTypes = false,
				std::deque<std::shared_ptr<TypeNameNode>> paramTypes = {},
				bool hasVarArg = false)
				: name(name),
				  genericArgs(genericArgs),
				  hasParamTypes(hasParamTypes),
				  paramTypes(paramTypes),
				  hasVarArg(hasVarArg) {}
			inline IdRefEntry(
				TokenRange tokenRange,
				size_t idxToken,
				std::string name,
				std::deque<std::shared_ptr<TypeNameNode>> genericArgs = {},
				bool hasParamTypes = false,
				std::deque<std::shared_ptr<TypeNameNode>> paramTypes = {},
				bool hasVarArg = false)
				: tokenRange(tokenRange),
				  idxToken(idxToken),
				  name(name),
				  genericArgs(genericArgs),
				  hasParamTypes(hasParamTypes),
				  paramTypes(paramTypes),
				  hasVarArg(hasVarArg) {}
		};

		using IdRef = std::deque<IdRefEntry>;

		inline bool isCompleteIdRef(const IdRef &ref) {
			return ref.size() && (ref.back().idxToken != SIZE_MAX);
		}

		IdRef duplicateIdRef(const IdRef &other);

		class ThisRefNode : public AstNode {
		public:
			inline ThisRefNode() = default;
			virtual ~ThisRefNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::ThisRef; }
		};

		class BaseRefNode : public AstNode {
		public:
			inline BaseRefNode() = default;
			virtual ~BaseRefNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::BaseRef; }
		};

		class Compiler;
	}
}

namespace std {
	std::string to_string(const slake::slkc::IdRef &ref, slake::slkc::Compiler *compiler, bool forMangling = false);
}

#endif
