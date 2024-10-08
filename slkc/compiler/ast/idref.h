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

		using IdRefEntries = std::deque<IdRefEntry>;

		class IdRefNode : public AstNode {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate() override;

		public:
			IdRefEntries entries;

			IdRefNode() = default;
			IdRefNode(const IdRefEntries &entries);
			IdRefNode(IdRefEntries &&entries);

			virtual inline NodeType getNodeType() const override { return NodeType::IdRef; }
		};

		inline bool isCompleteIdRef(const IdRefEntries &ref) {
			return ref.size() && (ref.back().idxToken != SIZE_MAX);
		}

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
	std::string to_string(const slake::slkc::IdRefEntries &ref, slake::slkc::Compiler *compiler, bool forMangling = false);
}

#endif
