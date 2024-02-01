#ifndef _SLKC_COMPILER_AST_EXPR_H_
#define _SLKC_COMPILER_AST_EXPR_H_

#include "ref.h"
#include "typename.h"

namespace slake {
	namespace slkc {
		enum ExprType : uint8_t {
			EXPR_UNARY,		  // Unary operation
			EXPR_BINARY,	  // Binary operation
			EXPR_TERNARY,	  // Ternary operation
			EXPR_MATCH,		  // Match
			EXPR_HEADED_REF,  // Headed reference
			EXPR_REF,		  // Reference

			EXPR_I8,	  // i8 Literal
			EXPR_I16,	  // i16 Literal
			EXPR_I32,	  // i32 Literal
			EXPR_I64,	  // i64 Literal
			EXPR_U8,	  // u8 Literal
			EXPR_U16,	  // u16 Literal
			EXPR_U32,	  // u32 Literal
			EXPR_U64,	  // u64 Literal
			EXPR_F32,	  // f32 Literal
			EXPR_F64,	  // f64 Literal
			EXPR_STRING,  // string Literal
			EXPR_BOOL,	  // bool Literal

			EXPR_ARRAY,	 // Array
			EXPR_MAP,	 // Map

			EXPR_CLOSURE,  // Closure

			EXPR_CALL,	 // Call
			EXPR_AWAIT,	 // Await

			EXPR_NEW,	  // New
			EXPR_TYPEOF,  // Typeof
			EXPR_CAST	  // Cast
		};

		class ExprNode : public AstNode {
		public:
			virtual ~ExprNode() = default;

			virtual ExprType getExprType() const = 0;

			virtual inline NodeType getNodeType() const override { return AST_EXPR; }
		};

		enum UnaryOp : uint8_t {
			OP_NOT,
			OP_REV,
			OP_INCF,
			OP_DECF,
			OP_INCB,
			OP_DECB
		};

		inline bool isForwardUnaryOp(UnaryOp op) {
			switch (op) {
				case OP_INCF:
				case OP_DECF:
					return true;
				default:
					return false;
			}
		}
		inline bool isBackwardUnaryOp(UnaryOp op) { return !isForwardUnaryOp(op); }

		enum BinaryOp : uint8_t {
			OP_ADD = 0,
			OP_SUB,
			OP_MUL,
			OP_DIV,
			OP_MOD,
			OP_AND,
			OP_OR,
			OP_XOR,
			OP_LAND,
			OP_LOR,
			OP_LSH,
			OP_RSH,
			OP_SWAP,

			OP_ASSIGN,
			OP_ASSIGN_ADD,
			OP_ASSIGN_SUB,
			OP_ASSIGN_MUL,
			OP_ASSIGN_DIV,
			OP_ASSIGN_MOD,
			OP_ASSIGN_AND,
			OP_ASSIGN_OR,
			OP_ASSIGN_XOR,
			OP_ASSIGN_LAND,
			OP_ASSIGN_LOR,
			OP_ASSIGN_LSH,
			OP_ASSIGN_RSH,

			OP_EQ,
			OP_NEQ,
			OP_STRICTEQ,
			OP_STRICTNEQ,
			OP_LT,
			OP_GT,
			OP_LTEQ,
			OP_GTEQ,
			OP_SUBSCRIPT
		};

		inline bool isAssignBinaryOp(BinaryOp op) {
			return (op >= OP_ASSIGN) && (op < OP_EQ);
		}

		class HeadedRefExprNode : public ExprNode {
		public:
			shared_ptr<ExprNode> head;
			Ref ref;

			inline HeadedRefExprNode(shared_ptr<ExprNode> head, Ref ref) : head(head), ref(ref) {}
			virtual ~HeadedRefExprNode() = default;

			virtual inline Location getLocation() const override { return head->getLocation(); }

			virtual ExprType getExprType() const override { return EXPR_HEADED_REF; }
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

			virtual ExprType getExprType() const override { return EXPR_UNARY; }
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

			virtual ExprType getExprType() const override { return EXPR_BINARY; }
		};

		class TernaryOpExprNode : public ExprNode {
		public:
			shared_ptr<ExprNode> condition, x, y;

			inline TernaryOpExprNode(shared_ptr<ExprNode> condition, shared_ptr<ExprNode> x, shared_ptr<ExprNode> y)
				: condition(condition), x(x), y(y) {}
			virtual ~TernaryOpExprNode() = default;

			virtual inline Location getLocation() const override { return condition->getLocation(); }

			virtual ExprType getExprType() const override { return EXPR_TERNARY; }
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

			virtual ExprType getExprType() const override { return EXPR_MATCH; }
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
				return EXPR_I8;
			else if constexpr (std::is_same_v<T, int16_t>)
				return EXPR_I16;
			else if constexpr (std::is_same_v<T, int32_t>)
				return EXPR_I32;
			else if constexpr (std::is_same_v<T, int64_t>)
				return EXPR_I64;
			else if constexpr (std::is_same_v<T, uint8_t>)
				return EXPR_U8;
			else if constexpr (std::is_same_v<T, uint16_t>)
				return EXPR_U16;
			else if constexpr (std::is_same_v<T, uint32_t>)
				return EXPR_U32;
			else if constexpr (std::is_same_v<T, uint64_t>)
				return EXPR_U64;
			else if constexpr (std::is_same_v<T, float>)
				return EXPR_F32;
			else if constexpr (std::is_same_v<T, double>)
				return EXPR_F64;
			else if constexpr (std::is_same_v<T, string>)
				return EXPR_STRING;
			else if constexpr (std::is_same_v<T, bool>)
				return EXPR_BOOL;
			else
				static_assert(!std::is_same_v<T, T>);
		}

		using I8LiteralExprNode = LiteralExprNode<int8_t, EXPR_I8>;
		using I16LiteralExprNode = LiteralExprNode<int16_t, EXPR_I16>;
		using I32LiteralExprNode = LiteralExprNode<int32_t, EXPR_I32>;
		using I64LiteralExprNode = LiteralExprNode<int64_t, EXPR_I64>;
		using U8LiteralExprNode = LiteralExprNode<uint32_t, EXPR_U8>;
		using U16LiteralExprNode = LiteralExprNode<uint32_t, EXPR_U16>;
		using U32LiteralExprNode = LiteralExprNode<uint32_t, EXPR_U32>;
		using U64LiteralExprNode = LiteralExprNode<uint64_t, EXPR_U64>;
		using F32LiteralExprNode = LiteralExprNode<float, EXPR_F32>;
		using F64LiteralExprNode = LiteralExprNode<double, EXPR_F64>;
		using StringLiteralExprNode = LiteralExprNode<string, EXPR_STRING>;
		using BoolLiteralExprNode = LiteralExprNode<bool, EXPR_BOOL>;

		class RefExprNode : public ExprNode {
		public:
			Ref ref;

			inline RefExprNode(Ref ref)
				: ref(ref) {}
			virtual ~RefExprNode() = default;

			virtual inline Location getLocation() const override { return ref[0].loc; }

			virtual ExprType getExprType() const override { return EXPR_REF; }
		};

		class ArrayExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			deque<shared_ptr<ExprNode>> elements;

			inline ArrayExprNode(Location loc, deque<shared_ptr<ExprNode>> elements) : _loc(loc), elements(elements) {}
			virtual ~ArrayExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return EXPR_ARRAY; }
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

			virtual ExprType getExprType() const override { return EXPR_MAP; }
		};

		struct Param {
			Location loc;
			shared_ptr<TypeNameNode> type;
			string name;

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

			virtual ExprType getExprType() const override { return EXPR_CALL; }
		};

		class AwaitExprNode : public ExprNode {
		private:
			Location _loc;

		public:
			shared_ptr<ExprNode> target;

			inline AwaitExprNode(Location loc, shared_ptr<ExprNode> target) : _loc(loc), target(target) {}
			virtual ~AwaitExprNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual ExprType getExprType() const override { return EXPR_AWAIT; }
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

			virtual ExprType getExprType() const override { return EXPR_NEW; }
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

			virtual ExprType getExprType() const override { return EXPR_TYPEOF; }
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

			virtual ExprType getExprType() const override { return EXPR_CAST; }
		};
	}
}

namespace std {
	string to_string(slake::slkc::UnaryOp op);
	string to_string(slake::slkc::BinaryOp op);
}

#endif
