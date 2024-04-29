#ifndef _SLKC_COMPILER_AST_ASTNODE_H_
#define _SLKC_COMPILER_AST_ASTNODE_H_

#include <deque>
#include <memory>
#include <unordered_map>
#include <slake/runtime.h>

namespace slake {
	namespace slkc {
		using namespace std;

		enum class NodeType : uint8_t {
			Class = 0,
			Interface,
			Trait,
			Fn,
			FnOverloading,
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
			Param,
			ArgRef,
			GenericArgRef,
			Comment,

			LocalVarRef,
			RegRef,
			ThisRef,
			BaseRef,

			Bad
		};

		struct Location {
			size_t line;
			size_t column;

			inline Location() : line(0), column(0) {}
			inline Location(size_t line, size_t column) : line(line), column(column) {}

			inline bool operator<(const Location &loc) const {
				if (line < loc.line)
					return true;
				if (line > loc.line)
					return false;
				return column < loc.column;
			}

			inline bool operator>(const Location &loc) const {
				if(line > loc.line)
					return true;
				if(line < loc.line)
					return false;
				return column > loc.column;
			}

			inline bool operator==(const Location &loc) const {
				return (line == loc.line) && (column == loc.column);
			}

			inline bool operator>=(const Location &loc) const {
				return ((*this) == loc) || ((*this) > loc);
			}

			inline bool operator<=(const Location &loc) const {
				return ((*this) == loc) || ((*this) < loc);
			}
		};

		class Compiler;

		class AstNode : public std::enable_shared_from_this<AstNode> {
		private:
			virtual shared_ptr<AstNode> doDuplicate();

		public:
			virtual ~AstNode() = default;

			virtual Location getLocation() const = 0;
			virtual NodeType getNodeType() const = 0;

			/// @brief Duplicate the member.
			/// @note Some members may not be duplicated, because duplication is only used by generic mechanism and it does not overwrite anything.
			/// @tparam T
			/// @return
			template<typename T>
			inline shared_ptr<T> duplicate() {
				return static_pointer_cast<T>(doDuplicate());
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
