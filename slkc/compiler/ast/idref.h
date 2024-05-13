#ifndef _SLKC_COMPILER_AST_REF_H_
#define _SLKC_COMPILER_AST_REF_H_

#include "astnode.h"
#include <stdexcept>

namespace slake {
	namespace slkc {
		class TypeNameNode;

		struct IdRefEntry {
			Location loc;
			size_t idxAccessOpToken = SIZE_MAX;	 // Index of preceding access operator token
			size_t idxToken;
			std::string name;
			std::deque<std::shared_ptr<TypeNameNode>> genericArgs;

			inline IdRefEntry(Location loc, size_t idxToken, std::string name, std::deque<std::shared_ptr<TypeNameNode>> genericArgs = {})
				: loc(loc), idxToken(idxToken), name(name), genericArgs(genericArgs) {}
		};

		using IdRef = std::deque<IdRefEntry>;

		inline bool isCompleteIdRef(const IdRef& ref) {
			return ref.back().idxToken != SIZE_MAX;
		}

		IdRef duplicateIdRef(const IdRef &other);

		class ThisRefNode : public AstNode {
		public:
			inline ThisRefNode() = default;
			virtual ~ThisRefNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Should not get location of a this reference"); }

			virtual inline NodeType getNodeType() const override { return NodeType::ThisRef; }
		};

		class BaseRefNode : public AstNode {
		public:
			inline BaseRefNode() = default;
			virtual ~BaseRefNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Should not get location of a this reference"); }

			virtual inline NodeType getNodeType() const override { return NodeType::BaseRef; }
		};

		class Compiler;
	}
}

namespace std {
	std::string to_string(const slake::slkc::IdRef &ref, slake::slkc::Compiler *compiler);
}

#endif
