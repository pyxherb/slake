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

		enum class NodeType : uint8_t {
			Class = 0,
			Interface,
			Trait,
			Fn,
			Stmt,
			Expr,
			TypeName,
			Module,
			Alias,
			Var,
			GenericParam,

			CompiledFn,
			LabelRef,
			LocalVar,
			ArgRef,
			GenericArgRef,
			Comment,

			LocalVarRef,
			RegRef,
			ThisRef,
			BaseRef
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

			inline bool operator<(Location loc) const {
				if (line < loc.line)
					return true;
				if (line > loc.line)
					return false;
				return column < loc.column;
			}
		};

		class Compiler;

		class AstNode : public std::enable_shared_from_this<AstNode> {
		public:
			virtual ~AstNode() = default;

			virtual Location getLocation() const = 0;
			virtual NodeType getNodeType() const = 0;
			
			virtual shared_ptr<AstNode> duplicate();

			inline shared_ptr<AstNode> getSharedPtr() {
				return shared_from_this();
			}
		};
	}
}

namespace std {
	inline string to_string(slake::slkc::Location loc) {
		return std::to_string(loc.line) + ", " + std::to_string(loc.column);
	}
}

#endif
