#ifndef _SLKC_COMPILER_AST_EXPR_H_
#define _SLKC_COMPILER_AST_EXPR_H_

#include "ref.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		enum class ExprType : uint8_t {
			Unary,		// Unary operation
			Binary,		// Binary operation
			Ternary,	// Ternary operation
			Match,		// Match
			HeadedRef,	// Headed reference
			Ref,		// Reference

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
			String,	 // string Literal
			Bool,	 // bool Literal

			Array,	// Array
			Map,	// Map

			Closure,  // Closure

			Call,	// Call
			Await,	// Await

			New,	 // New
			Typeof,	 // Typeof
			Cast	 // Cast
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
			IncF,
			DecF,
			IncB,
			DecB
		};

		inline bool isForwardUnaryOp(UnaryOp op) {
			switch (op) {
				case UnaryOp::IncF:
				case UnaryOp::DecF:
					return true;
				default:
					return false;
			}
		}
		inline bool isBackwardUnaryOp(UnaryOp op) { return !isForwardUnaryOp(op); }

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
			Swap,

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
			Subscript
		};

		inline bool isAssignBinaryOp(BinaryOp op) {
			return (op >= BinaryOp::Assign) && (op < BinaryOp::Eq);
		}

		class HeadedRefExprNode : public ExprNode {
		public:
			shared_ptr<ExprNode> head;
			Ref ref;

			inline HeadedRefExprNode(shared_ptr<ExprNode> head, Ref ref) : head(head), ref(ref) {}
			virtual ~HeadedRefExprNode() = default;

			virtual inline Location getLocation() const override { return head->getLocation(); }

			virtual ExprType getExprType() const override { return ExprType::HeadedRef; }
		};

		class UnaryOpExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			UnaryOp op;
			shared_ptr<ExprNode> x;

			inline UnaryOpExprNode(Location loc, UnaryOp op, shared_ptr<ExprNode> x)
				: _loc(loc), op(op), x(x) {}
			virtual ~UnaryOpExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Unary; }
		};

		class BinaryOpExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			BinaryOp op;
			shared_ptr<ExprNode> lhs, rhs;

			inline BinaryOpExprNode(Location loc, BinaryOp op, shared_ptr<ExprNode> lhs, shared_ptr<ExprNode> rhs)
				: _loc(loc), op(op), lhs(lhs), rhs(rhs) {}
			virtual ~BinaryOpExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Binary; }
		};

		class TernaryOpExprNode : public ExprNode {
		public:
			shared_ptr<ExprNode> condition, x, y;

			inline TernaryOpExprNode(shared_ptr<ExprNode> condition, shared_ptr<ExprNode> x, shared_ptr<ExprNode> y)
				: condition(condition), x(x), y(y) {}
			virtual ~TernaryOpExprNode() = default;

			virtual inline Location getLocation() const override { return condition->getLocation(); }

			virtual ExprType getExprType() const override { return ExprType::Ternary; }
		};

		class MatchExprNode : public ExprNode {
		public:
			shared_ptr<ExprNode> condition;
			deque<pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>>> cases;

			inline MatchExprNode(
				shared_ptr<ExprNode> condition,
				deque<pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>>> cases)
				: condition(condition), cases(cases) {}
			virtual ~MatchExprNode() = default;

			virtual inline Location getLocation() const override { return condition->getLocation(); }

			virtual ExprType getExprType() const override { return ExprType::Match; }
		};

		template <typename T, ExprType xt>
		class LiteralExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			T data;

			inline LiteralExprNode(Location loc, T data) : _loc(loc), data(data) {}
			virtual ~LiteralExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

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
			else if constexpr (std::is_same_v<T, string>)
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
		using StringLiteralExprNode = LiteralExprNode<string, ExprType::String>;
		using BoolLiteralExprNode = LiteralExprNode<bool, ExprType::Bool>;

		class RefExprNode : public ExprNode {
		public:
			Ref ref;

			inline RefExprNode(Ref ref)
				: ref(ref) {}
			virtual ~RefExprNode() = default;

			virtual inline Location getLocation() const override { return ref[0].loc; }

			virtual ExprType getExprType() const override { return ExprType::Ref; }
		};

		class ArrayExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			deque<shared_ptr<ExprNode>> elements;

			inline ArrayExprNode(Location loc, deque<shared_ptr<ExprNode>> elements) : _loc(loc), elements(elements) {}
			virtual ~ArrayExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Array; }
		};

		class MapExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			deque<pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>>> pairs;

			inline MapExprNode(Location loc, deque<pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>>> pairs)
				: _loc(loc), pairs(pairs) {}
			virtual ~MapExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Map; }
		};

		struct Param {
			Location loc;
			shared_ptr<TypeNameNode> type;
			string name;

			// The original type will be saved during generic instantiation.
			shared_ptr<TypeNameNode> originalType;

			inline Param(
				Location loc,
				shared_ptr<TypeNameNode> type,
				string name) : type(type), name(name), loc(loc) {}
		};

		class StmtNode;

		class ClosureExprNode : public ExprNode {
		public:
			shared_ptr<TypeNameNode> returnType;
			deque<Param> params;
			deque<shared_ptr<ExprNode>> captureList;
			shared_ptr<StmtNode> body;

			inline ClosureExprNode(
				shared_ptr<TypeNameNode> returnType,
				deque<Param> params,
				deque<shared_ptr<ExprNode>> captureList,
				shared_ptr<StmtNode> body)
				: returnType(returnType),
				  params(params),
				  captureList(captureList),
				  body(body) {}
			virtual ~ClosureExprNode() = default;
		};

		class CallExprNode : public ExprNode {
		public:
			shared_ptr<ExprNode> target;
			deque<shared_ptr<ExprNode>> args;
			bool isAsync;

			inline CallExprNode(
				shared_ptr<ExprNode> target,
				deque<shared_ptr<ExprNode>> args,
				bool isAsync) : target(target), args(args), isAsync(isAsync) {}
			virtual ~CallExprNode() = default;

			virtual inline Location getLocation() const override { return target->getLocation(); }

			virtual ExprType getExprType() const override { return ExprType::Call; }
		};

		class AwaitExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> target;

			inline AwaitExprNode(Location loc, shared_ptr<ExprNode> target) : _loc(loc), target(target) {}
			virtual ~AwaitExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Await; }
		};

		class NewExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			shared_ptr<TypeNameNode> type;
			deque<shared_ptr<ExprNode>> args;

			inline NewExprNode(Location loc, shared_ptr<TypeNameNode> type, deque<shared_ptr<ExprNode>> args)
				: _loc(loc), type(type), args(args) {}
			virtual ~NewExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::New; }
		};

		class TypeofExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> target;

			inline TypeofExprNode(Location loc, shared_ptr<ExprNode> target)
				: _loc(loc), target(target) {}
			virtual ~TypeofExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Typeof; }
		};

		class CastExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			shared_ptr<TypeNameNode> targetType;
			shared_ptr<ExprNode> target;

			inline CastExprNode(
				Location loc,
				shared_ptr<TypeNameNode> targetType,
				shared_ptr<ExprNode> target)
				: _loc(loc), targetType(targetType), target(target) {}
			virtual ~CastExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return ExprType::Cast; }
		};
	}
}

namespace std {
	string to_string(slake::slkc::UnaryOp op);
	string to_string(slake::slkc::BinaryOp op);
}

#endif
