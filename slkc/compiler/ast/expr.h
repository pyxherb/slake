#ifndef _SLKC_COMPILER_AST_EXPR_H_
#define _SLKC_COMPILER_AST_EXPR_H_

#include "idref.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		enum class ExprType : uint8_t {
			Unary,		// Unary operation
			Binary,		// Binary operation
			Ternary,	// Ternary operation
			Match,		// Match
			HeadedRef,	// Headed reference
			IdRef,		// Reference

			I8,		 // i8 Literal
			I16,	 // i16 Literal
			I32,	 // i32 Literal
			I64,	 // i64 Literal
			U8,		 // u8 Literal
			U16,	 // u16 Literal
			U32,	 // u32 Literal
			U64,	 // u64 Literal
			F32,	 // f32 Literal
			F64,	 // f64 Literal
			String,	 // std::string Literal
			Bool,	 // bool Literal
			Null,	 // null

			Array,	// Array

			Call,	// Call
			Await,	// Await

			New,	 // New
			Typeof,	 // Typeof
			Cast,	 // Cast

			VarArg,	 // Varidic arguments

			Wrapper,  // Expression wrapper

			Bad,  // Bad expression
		};

		class ExprNode : public AstNode {
		public:
			virtual ~ExprNode() = default;

			virtual ExprType getExprType() const = 0;

			virtual inline NodeType getNodeType() const override { return NodeType::Expr; }
		};

		enum class UnaryOp : uint8_t {
			LNot,
			Not,
			Neg
		};

		enum class BinaryOp : uint8_t {
			Add = 0,
			Sub,
			Mul,
			Div,
			Mod,
			And,
			Or,
			Xor,
			LAnd,
			LOr,
			Lsh,
			Rsh,

			Assign,
			AssignAdd,
			AssignSub,
			AssignMul,
			AssignDiv,
			AssignMod,
			AssignAnd,
			AssignOr,
			AssignXor,
			AssignLsh,
			AssignRsh,

			Eq,
			Neq,
			StrictEq,
			StrictNeq,
			Lt,
			Gt,
			LtEq,
			GtEq,
			Cmp,
			Subscript
		};

		inline bool isAssignBinaryOp(BinaryOp op) {
			return (op >= BinaryOp::Assign) && (op < BinaryOp::Eq);
		}

		class HeadedIdRefExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> head;
			std::shared_ptr<IdRefNode> ref;

			size_t idxOpToken = SIZE_MAX;

			inline HeadedIdRefExprNode(std::shared_ptr<ExprNode> head, std::shared_ptr<IdRefNode> ref) : head(head), ref(ref) {}
			virtual ~HeadedIdRefExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::HeadedRef; }
		};

		class UnaryOpExprNode : public ExprNode {
		public:
			UnaryOp op;
			std::shared_ptr<ExprNode> x;

			size_t idxOpToken = SIZE_MAX;

			inline UnaryOpExprNode(UnaryOp op, std::shared_ptr<ExprNode> x)
				: op(op), x(x) {}
			virtual ~UnaryOpExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Unary; }
		};

		class BinaryOpExprNode : public ExprNode {
		public:
			BinaryOp op;
			std::shared_ptr<ExprNode> lhs, rhs;

			size_t idxOpToken = SIZE_MAX, idxClosingToken = SIZE_MAX;

			inline BinaryOpExprNode(BinaryOp op, std::shared_ptr<ExprNode> lhs)
				: op(op), lhs(lhs) {}
			virtual ~BinaryOpExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Binary; }
		};

		class TernaryOpExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> condition, x, y;

			size_t idxQuestionToken = SIZE_MAX, idxColonToken = SIZE_MAX;

			inline TernaryOpExprNode(std::shared_ptr<ExprNode> condition) : condition(condition) {}
			virtual ~TernaryOpExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Ternary; }
		};

		class MatchExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> condition;
			std::deque<std::pair<std::shared_ptr<ExprNode>, std::shared_ptr<ExprNode>>> cases;

			inline MatchExprNode(
				std::shared_ptr<ExprNode> condition,
				std::deque<std::pair<std::shared_ptr<ExprNode>, std::shared_ptr<ExprNode>>> cases)
				: condition(condition), cases(cases) {}
			virtual ~MatchExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Match; }
		};

		class BasicLiteralExprNode : public ExprNode {
		public:
			size_t idxToken = SIZE_MAX;

			inline BasicLiteralExprNode(size_t idxToken) : idxToken(idxToken) {}
			virtual ~BasicLiteralExprNode() = default;
		};

		template <typename T, ExprType xt>
		class LiteralExprNode : public BasicLiteralExprNode {
		public:
			T data;

			inline LiteralExprNode(T data, size_t idxToken = SIZE_MAX) : BasicLiteralExprNode(idxToken), data(data) {}
			virtual ~LiteralExprNode() = default;

			virtual ExprType getExprType() const override { return xt; }
		};

		template <typename T>
		constexpr ExprType getLiteralExprType() {
			if constexpr (std::is_same_v<T, int8_t>)
				return ExprType::I8;
			else if constexpr (std::is_same_v<T, int16_t>)
				return ExprType::I16;
			else if constexpr (std::is_same_v<T, int32_t>)
				return ExprType::I32;
			else if constexpr (std::is_same_v<T, int64_t>)
				return ExprType::I64;
			else if constexpr (std::is_same_v<T, uint8_t>)
				return ExprType::U8;
			else if constexpr (std::is_same_v<T, uint16_t>)
				return ExprType::U16;
			else if constexpr (std::is_same_v<T, uint32_t>)
				return ExprType::U32;
			else if constexpr (std::is_same_v<T, uint64_t>)
				return ExprType::U64;
			else if constexpr (std::is_same_v<T, float>)
				return ExprType::F32;
			else if constexpr (std::is_same_v<T, double>)
				return ExprType::F64;
			else if constexpr (std::is_same_v<T, std::string>)
				return ExprType::String;
			else if constexpr (std::is_same_v<T, bool>)
				return ExprType::Bool;
			else
				static_assert(!std::is_same_v<T, T>);
		}

		using I8LiteralExprNode = LiteralExprNode<int8_t, ExprType::I8>;
		using I16LiteralExprNode = LiteralExprNode<int16_t, ExprType::I16>;
		using I32LiteralExprNode = LiteralExprNode<int32_t, ExprType::I32>;
		using I64LiteralExprNode = LiteralExprNode<int64_t, ExprType::I64>;
		using U8LiteralExprNode = LiteralExprNode<uint32_t, ExprType::U8>;
		using U16LiteralExprNode = LiteralExprNode<uint32_t, ExprType::U16>;
		using U32LiteralExprNode = LiteralExprNode<uint32_t, ExprType::U32>;
		using U64LiteralExprNode = LiteralExprNode<uint64_t, ExprType::U64>;
		using F32LiteralExprNode = LiteralExprNode<float, ExprType::F32>;
		using F64LiteralExprNode = LiteralExprNode<double, ExprType::F64>;
		using StringLiteralExprNode = LiteralExprNode<std::string, ExprType::String>;
		using BoolLiteralExprNode = LiteralExprNode<bool, ExprType::Bool>;

		class NullLiteralExprNode : public BasicLiteralExprNode {
		public:
			inline NullLiteralExprNode(size_t idxToken = SIZE_MAX) : BasicLiteralExprNode(idxToken) {}
			virtual ~NullLiteralExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Null; }
		};

		class IdRefExprNode : public ExprNode {
		public:
			std::shared_ptr<IdRefNode> ref;

			inline IdRefExprNode(std::shared_ptr<IdRefNode> ref)
				: ref(ref) {
				assert(!ref->entries.empty());
			}
			virtual ~IdRefExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::IdRef; }
		};

		class ArrayExprNode : public ExprNode {
		public:
			size_t idxLBraceToken = SIZE_MAX, idxRBraceToken = SIZE_MAX;
			std::deque<std::shared_ptr<ExprNode>> elements;
			std::deque<size_t> idxCommaTokens;

			// Evaluated element type, for value compilation.
			std::shared_ptr<TypeNameNode> evaluatedElementType;

			inline ArrayExprNode() {}
			virtual ~ArrayExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Array; }
		};

		class StmtNode;

		class CallExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> target;
			std::deque<std::shared_ptr<ExprNode>> args;
			bool isAsync = false;

			size_t idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;
			std::deque<size_t> idxCommaTokens;

			CallExprNode() = default;
			virtual ~CallExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Call; }
		};

		class AwaitExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> target;

			inline AwaitExprNode(std::shared_ptr<ExprNode> target) : target(target) {}
			virtual ~AwaitExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Await; }
		};

		class NewExprNode : public ExprNode {
		public:
			std::shared_ptr<TypeNameNode> type;
			std::deque<std::shared_ptr<ExprNode>> args;

			size_t idxNewToken = SIZE_MAX,
				   idxLParentheseToken = SIZE_MAX,
				   idxRParentheseToken = SIZE_MAX;
			std::deque<size_t> idxCommaTokens;

			inline NewExprNode() {}
			virtual ~NewExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::New; }
		};

		class TypeofExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> target;

			inline TypeofExprNode(std::shared_ptr<ExprNode> target)
				: target(target) {}
			virtual ~TypeofExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Typeof; }
		};

		class CastExprNode : public ExprNode {
		public:
			std::shared_ptr<TypeNameNode> targetType;
			std::shared_ptr<ExprNode> target;

			size_t idxAsKeyword = SIZE_MAX;

			inline CastExprNode(
				std::shared_ptr<ExprNode> target)
				: target(target) {}
			inline CastExprNode(
				std::shared_ptr<TypeNameNode> targetType,
				std::shared_ptr<ExprNode> target)
				: targetType(targetType), target(target) {}
			virtual ~CastExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Cast; }
		};

		class VarArgExprNode : public ExprNode {
		public:
			inline VarArgExprNode() {}
			virtual ~VarArgExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::VarArg; }
		};

		class WrapperExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> expr;

			inline WrapperExprNode(std::shared_ptr<ExprNode> expr)
				: expr(expr) {}
			virtual ~WrapperExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Wrapper; }
		};

		class BadExprNode : public ExprNode {
		public:
			std::shared_ptr<ExprNode> expr;

			inline BadExprNode(std::shared_ptr<ExprNode> expr)
				: expr(expr) {}
			virtual ~BadExprNode() = default;

			virtual ExprType getExprType() const override { return ExprType::Bad; }
		};
	}
}

namespace std {
	std::string to_string(slake::slkc::UnaryOp op);
	std::string to_string(slake::slkc::BinaryOp op);
}

#endif
