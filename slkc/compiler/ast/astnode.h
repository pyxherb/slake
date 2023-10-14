#ifndef _SLKC_COMPILER_AST_ASTNODE_H_
#define _SLKC_COMPILER_AST_ASTNODE_H_

#include <deque>
#include <memory>
#include <unordered_map>
#include <antlr4-runtime.h>
#include <slake/runtime.h>

namespace slake {
	namespace slkc {
		using namespace std;

		enum NodeType : uint8_t {
			AST_CLASS = 0,
			AST_INTERFACE,
			AST_TRAIT,
			AST_FN,
			AST_STMT,
			AST_EXPR,
			AST_TYPENAME,
			AST_MODULE,
			AST_ALIAS,
			AST_VAR,

			AST_COMPILED_FN,
			AST_LABEL_REF,
			AST_LOCAL_VAR,
			AST_ARG_REF,
			AST_COMMENT,

			AST_REG_REF,
			AST_LVAR_REF
		};

		struct Location {
			size_t line;
			size_t column;

			inline Location() : line(0), column(0) {}
			inline Location(size_t line, size_t column) : line(line), column(column) {}
			inline Location(antlr4::tree::TerminalNode *node)
				: line(node->getSymbol()->getLine()), column(node->getSymbol()->getCharPositionInLine()) {
			}
			inline Location(antlr4::Token *node)
				: line(node->getLine()), column(node->getCharPositionInLine()) {
			}
		};

		class AstNode {
		public:
			virtual ~AstNode() = default;

			virtual Location getLocation() const = 0;
			virtual NodeType getNodeType() const = 0;
		};
	}
}

namespace std {
	inline string to_string(slake::slkc::Location loc) {
		return std::to_string(loc.line) + ", " + std::to_string(loc.column);
	}
}

#endif
