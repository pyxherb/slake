#ifndef _SLKC_COMPILER_AST_ASTNODE_H_
#define _SLKC_COMPILER_AST_ASTNODE_H_

#include <deque>
#include <memory>
#include <unordered_map>
#include <slake/runtime.h>

namespace slake {
	namespace slkc {
		enum class NodeType : uint8_t {
			Class = 0,
			Interface,
			Fn,
			FnOverloadingValue,
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
			Comment,

			RegRef,
			IdRef,
			ThisRef,
			BaseRef,

			Root,

			Bad
		};

		struct SourcePosition {
			size_t line, column;

			inline SourcePosition() : line(SIZE_MAX), column(SIZE_MAX) {}
			inline SourcePosition(size_t line, size_t column) : line(line), column(column) {}

			inline bool operator<(const SourcePosition &loc) const {
				if (line < loc.line)
					return true;
				if (line > loc.line)
					return false;
				return column < loc.column;
			}

			inline bool operator>(const SourcePosition &loc) const {
				if (line > loc.line)
					return true;
				if (line < loc.line)
					return false;
				return column > loc.column;
			}

			inline bool operator==(const SourcePosition &loc) const {
				return (line == loc.line) && (column == loc.column);
			}

			inline bool operator>=(const SourcePosition &loc) const {
				return ((*this) == loc) || ((*this) > loc);
			}

			inline bool operator<=(const SourcePosition &loc) const {
				return ((*this) == loc) || ((*this) < loc);
			}
		};

		struct SourceLocation {
			SourcePosition beginPosition, endPosition;
		};

		struct SourceDocument;

		struct TokenRange {
			SourceDocument *document = nullptr;
			size_t beginIndex = SIZE_MAX, endIndex = SIZE_MAX;

			inline TokenRange() = default;
			inline TokenRange(SourceDocument *document, size_t index) : document(document), beginIndex(index), endIndex(index) {}
			inline TokenRange(SourceDocument *document, size_t beginIndex, size_t endIndex) : document(document), beginIndex(beginIndex), endIndex(endIndex) {}

			operator bool() {
				return beginIndex != SIZE_MAX;
			}
		};

		class Compiler;

		class AstNode : public std::enable_shared_from_this<AstNode> {
		private:
			virtual std::shared_ptr<AstNode> doDuplicate();

		public:
			TokenRange tokenRange;

			AstNode() = default;
			inline AstNode(const AstNode& other) {
				tokenRange = other.tokenRange;
			}
			virtual ~AstNode() = default;

			virtual NodeType getNodeType() const = 0;

			/// @brief Duplicate the member.
			/// @note Some members may not be duplicated, because duplication is only used by generic mechanism and it does not overwrite anything.
			/// @tparam T
			/// @return
			template<typename T>
			inline std::shared_ptr<T> duplicate() {
				return std::static_pointer_cast<T>(doDuplicate());
			}
		};
	}
}

namespace std {
	inline std::string to_string(slake::slkc::SourcePosition loc) {
		return std::to_string(loc.line + 1) + ":" + std::to_string(loc.column + 1);
	}
	inline std::string to_string(slake::slkc::SourceLocation loc) {
		return std::to_string(loc.beginPosition) + ", " + std::to_string(loc.endPosition);
	}
}

#endif
