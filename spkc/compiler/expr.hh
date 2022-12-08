#ifndef _SPKC_SYNTAX_EXPR_HH
#define _SPKC_SYNTAX_EXPR_HH

#include <swampeak/debug.h>

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "modifier.hh"

namespace SpkC {
	namespace Syntax {
		enum class ExprType : int {
			NONE = 0,
			UNARY,
			BINARY,
			TERNARY,
			REF,
			LITERAL,
			INLINE_SW,
			CALL,
			AWAIT,
			TYPENAME,
			MAP,
			ARRAY
		};

		class Expr {
		public:
			virtual inline ~Expr() {}
			virtual inline ExprType getType() { return ExprType::NONE; }
		};

		class InlineSwitchCase final {
		public:
			std::shared_ptr<Expr> condition, x;

			inline InlineSwitchCase(std::shared_ptr<Expr> condition, std::shared_ptr<Expr> x) {
				this->condition = condition;
				this->x = x;
			}
			virtual inline ~InlineSwitchCase() {}
		};

		using InlineSwitchCaseList = std::vector<std::shared_ptr<InlineSwitchCase>>;

		class InlineSwitchExpr : public Expr {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<InlineSwitchCaseList> caseList;

			inline InlineSwitchExpr(std::shared_ptr<Expr> condition, std::shared_ptr<InlineSwitchCaseList> caseList) {
				this->condition = condition;
				this->caseList = caseList;
			}
			virtual inline ~InlineSwitchExpr() {}
		};

		enum class UnaryOp : int {
			NEG = 0,
			NOT,
			REV,
			INC_F,
			INC_B,
			DEC_F,
			DEC_B
		};

		class UnaryOpExpr : public Expr {
		public:
			std::shared_ptr<Expr> x;
			UnaryOp op;

			inline UnaryOpExpr(UnaryOp op, std::shared_ptr<Expr> x) {
				this->x = x;
				this->op = op;
			}
			virtual inline ~UnaryOpExpr() {}
			virtual inline ExprType getType() { return ExprType::UNARY; }
		};

		enum class BinaryOp : int {
			ADD = 0,
			SUB,
			MUL,
			DIV,
			MOD,
			AND,
			OR,
			XOR,
			LAND,
			LOR,
			EQ,
			NEQ,
			GTEQ,
			LTEQ,
			GT,
			LT,
			ASSIGN,
			ADD_ASSIGN,
			SUB_ASSIGN,
			MUL_ASSIGN,
			DIV_ASSIGN,
			MOD_ASSIGN,
			AND_ASSIGN,
			OR_ASSIGN,
			XOR_ASSIGN
		};

		class BinaryOpExpr : public Expr {
		public:
			std::shared_ptr<Expr> x, y;
			BinaryOp op;

			inline BinaryOpExpr(BinaryOp op, std::shared_ptr<Expr> x, std::shared_ptr<Expr> y) {
				this->x = x;
				this->y = y;
				this->op = op;
			}
			virtual inline ~BinaryOpExpr() {}
			virtual inline ExprType getType() { return ExprType::BINARY; }
		};

		class TernaryOpExpr : public Expr {
		public:
			std::shared_ptr<Expr> condition, x, y;

			inline TernaryOpExpr(std::shared_ptr<Expr> condition, std::shared_ptr<Expr> x, std::shared_ptr<Expr> y) {
			}
			virtual inline ~TernaryOpExpr() {}
			virtual inline ExprType getType() { return ExprType::TERNARY; }
		};

		class RefExpr : public Expr {
		public:
			std::string name;  // Empty if referenced to self
			std::shared_ptr<RefExpr> next;

			inline RefExpr(std::string name, std::shared_ptr<RefExpr> next = std::shared_ptr<RefExpr>()) {
				this->name = name;
				this->next = next;
			}
			virtual inline ~RefExpr() {}
			virtual inline ExprType getType() { return ExprType::REF; }
		};

		using ArgList = std::vector<std::shared_ptr<Expr>>;

		class CallExpr : public Expr {
		public:
			std::shared_ptr<Expr> target;
			std::shared_ptr<ArgList> args;
			bool isAsync;

			inline CallExpr(std::shared_ptr<Expr> target, std::shared_ptr<ArgList> args, bool isAsync = false) {
				this->target = target;
				this->args = args;
				this->isAsync = isAsync;
			}
			virtual inline ~CallExpr() {}

			virtual inline ExprType getType() override { return ExprType::CALL; }
		};

		class AwaitExpr : public Expr {
		public:
			std::shared_ptr<Expr> target;

			inline AwaitExpr(std::shared_ptr<Expr> target) {
				this->target = target;
			}
			virtual inline ~AwaitExpr() {}

			virtual inline ExprType getType() override { return ExprType::AWAIT; }
		};

		enum LiteralType : int {
			LT_INT = 0,
			LT_UINT,
			LT_LONG,
			LT_ULONG,
			LT_FLOAT,
			LT_DOUBLE,
			LT_STRING,
			LT_NULL
		};

		class LiteralExpr : public Expr {
		public:
			virtual inline ~LiteralExpr() {}

			virtual inline ExprType getType() override { return ExprType::LITERAL; }
			virtual LiteralType getLiteralType() = 0;
		};

		class NullLiteralExpr : public LiteralExpr {
		public:
			virtual inline ~NullLiteralExpr() {}
			virtual LiteralType getLiteralType() override { return LiteralType::LT_NULL; };
		};

		template <typename T, int LT>
		class SimpleLiteralExpr : public LiteralExpr {
		public:
			T data;

			inline SimpleLiteralExpr(T data) { this->data = data; }
			virtual inline ~SimpleLiteralExpr() {}

			virtual LiteralType getLiteralType() override { return (LiteralType)LT; };

			inline SimpleLiteralExpr<T, LT>& operator=(T& data) {
				this->data = data;
			}
			inline SimpleLiteralExpr<T, LT>& operator=(T&& data) {
				this->data = data;
			}
		};

		using IntLiteralExpr = SimpleLiteralExpr<int, LT_INT>;
		using UIntLiteralExpr = SimpleLiteralExpr<unsigned int, LT_UINT>;
		using LongLiteralExpr = SimpleLiteralExpr<long long, LT_LONG>;
		using ULongLiteralExpr = SimpleLiteralExpr<unsigned long long, LT_ULONG>;
		using FloatLiteralExpr = SimpleLiteralExpr<float, LT_FLOAT>;
		using DoubleLiteralExpr = SimpleLiteralExpr<double, LT_DOUBLE>;
		using StringLiteralExpr = SimpleLiteralExpr<std::string, LT_STRING>;

		class ExprPair : public std::pair<std::shared_ptr<Expr>, std::shared_ptr<Expr>> {
		public:
			inline ExprPair() : pair() {}
			inline ExprPair(std::shared_ptr<Expr> first, std::shared_ptr<Expr> second) : pair(first, second) {}
			virtual inline ~ExprPair() {}
		};

		using PairList = std::vector<std::shared_ptr<ExprPair>>;

		class MapExpr : public Expr {
		public:
			std::shared_ptr<PairList> pairs;

			inline MapExpr(std::shared_ptr<PairList> pairs) {
				this->pairs = pairs;
			}
			virtual inline ~MapExpr() {}

			virtual inline ExprType getType() override { return ExprType::MAP; }
		};
	}
}

#endif
