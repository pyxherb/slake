#ifndef _SLKC_COMPILER_EXPR_HH
#define _SLKC_COMPILER_EXPR_HH

#include <slake/debug.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base.hh"
#include "modifier.hh"

namespace Slake {
	namespace Compiler {
		enum class UnaryOp : int {
			NEG = 0,
			NOT,
			REV,
			INC_F,
			INC_B,
			DEC_F,
			DEC_B
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
	}
}

namespace std {
	inline std::string to_string(Slake::Compiler::UnaryOp op) {
		switch (op) {
			case Slake::Compiler::UnaryOp::INC_F:
			case Slake::Compiler::UnaryOp::INC_B:
				return "++";
			case Slake::Compiler::UnaryOp::DEC_F:
			case Slake::Compiler::UnaryOp::DEC_B:
				return "--";
			case Slake::Compiler::UnaryOp::NEG:
				return "-";
			case Slake::Compiler::UnaryOp::NOT:
				return "!";
			case Slake::Compiler::UnaryOp::REV:
				return "~";
		}
		throw std::invalid_argument("Invalid unary operator");
	}

	inline std::string to_string(Slake::Compiler::BinaryOp op) {
		switch (op) {
			case Slake::Compiler::BinaryOp::ADD:
				return "+";
			case Slake::Compiler::BinaryOp::SUB:
				return "-";
			case Slake::Compiler::BinaryOp::MUL:
				return "*";
			case Slake::Compiler::BinaryOp::DIV:
				return "/";
			case Slake::Compiler::BinaryOp::MOD:
				return "%";
			case Slake::Compiler::BinaryOp::AND:
				return "&";
			case Slake::Compiler::BinaryOp::OR:
				return "|";
			case Slake::Compiler::BinaryOp::XOR:
				return "^";
			case Slake::Compiler::BinaryOp::LAND:
				return "&&";
			case Slake::Compiler::BinaryOp::LOR:
				return "||";
			case Slake::Compiler::BinaryOp::EQ:
				return "==";
			case Slake::Compiler::BinaryOp::NEQ:
				return "!=";
			case Slake::Compiler::BinaryOp::GTEQ:
				return ">=";
			case Slake::Compiler::BinaryOp::LTEQ:
				return "<=";
			case Slake::Compiler::BinaryOp::GT:
				return ">";
			case Slake::Compiler::BinaryOp::LT:
				return "<";
			case Slake::Compiler::BinaryOp::ASSIGN:
				return "=";
			case Slake::Compiler::BinaryOp::ADD_ASSIGN:
				return "+=";
			case Slake::Compiler::BinaryOp::SUB_ASSIGN:
				return "-=";
			case Slake::Compiler::BinaryOp::MUL_ASSIGN:
				return "*=";
			case Slake::Compiler::BinaryOp::DIV_ASSIGN:
				return "/=";
			case Slake::Compiler::BinaryOp::MOD_ASSIGN:
				return "%=";
			case Slake::Compiler::BinaryOp::AND_ASSIGN:
				return "&=";
			case Slake::Compiler::BinaryOp::OR_ASSIGN:
				return "|=";
			case Slake::Compiler::BinaryOp::XOR_ASSIGN:
				return "^=";
		}
		throw std::invalid_argument("Invalid binary operator");
	}
}

namespace Slake {
	namespace Compiler {
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

		class Expr : public BasicLocated, public IStringifiable {
		public:
			inline Expr(location loc) : BasicLocated(loc) {}
			virtual inline ~Expr() {}
			virtual inline ExprType getType() { return ExprType::NONE; }
		};

		class InlineSwitchCase final : public BasicLocated, public IStringifiable {
		public:
			std::shared_ptr<Expr> condition, x;

			inline InlineSwitchCase(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<Expr> x) : BasicLocated(loc) {
				this->condition = condition;
				this->x = x;
			}
			virtual inline ~InlineSwitchCase() {}
			virtual inline std::string toString() const override {
				return std::to_string(*condition) + " : " + std::to_string(*x);
			}
		};

		using InlineSwitchCaseList = std::vector<std::shared_ptr<InlineSwitchCase>>;

		class InlineSwitchExpr : public Expr {
		public:
			std::shared_ptr<Expr> condition;
			std::shared_ptr<InlineSwitchCaseList> caseList;

			inline InlineSwitchExpr(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<InlineSwitchCaseList> caseList) : Expr(loc) {
				this->condition = condition;
				this->caseList = caseList;
			}
			virtual inline ~InlineSwitchExpr() {}

			virtual inline std::string toString() const override {
				std::string s = std::to_string(*condition) + "=> {";

				auto i = caseList->begin();
				s = std::to_string((**i));
				while (i != caseList->end())
					s += ", " + std::to_string((**i));

				s += " }";
				return s;
			}
		};

		inline bool isSuffixUnaryOp(UnaryOp op) {
			switch (op) {
				case UnaryOp::INC_B:
				case UnaryOp::DEC_B:
					return true;
			}
			return false;
		}

		class UnaryOpExpr : public Expr {
		public:
			std::shared_ptr<Expr> x;
			UnaryOp op;

			inline UnaryOpExpr(location loc, UnaryOp op, std::shared_ptr<Expr> x) : Expr(loc) {
				this->x = x;
				this->op = op;
			}
			virtual inline ~UnaryOpExpr() {}
			virtual inline ExprType getType() { return ExprType::UNARY; }

			virtual inline std::string toString() const override {
				return isSuffixUnaryOp(op)
						   ? "(" + std::to_string(*x) + ')' + std::to_string(op)
						   : std::to_string(op) + "(" + x->toString() + ')';
			}
		};

		class BinaryOpExpr : public Expr {
		public:
			std::shared_ptr<Expr> x, y;
			BinaryOp op;

			inline BinaryOpExpr(location loc, BinaryOp op, std::shared_ptr<Expr> x, std::shared_ptr<Expr> y) : Expr(loc) {
				this->x = x;
				this->y = y;
				this->op = op;
			}
			virtual inline ~BinaryOpExpr() {}
			virtual inline ExprType getType() { return ExprType::BINARY; }

			virtual inline std::string toString() const override {
				return "(" + std::to_string(*x) + " " + std::to_string(op) + " " + std::to_string(*y) + ")";
			}
		};

		class TernaryOpExpr : public Expr {
		public:
			std::shared_ptr<Expr> condition, x, y;

			inline TernaryOpExpr(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<Expr> x, std::shared_ptr<Expr> y) : Expr(loc) {
			}
			virtual inline ~TernaryOpExpr() {}
			virtual inline ExprType getType() { return ExprType::TERNARY; }

			virtual inline std::string toString() const override {
				return "(" + std::to_string(*condition) + " ? " + std::to_string(*x) + " : " + std::to_string(*y) + ")";
			}
		};

		class RefExpr : public Expr {
		public:
			std::string name;  // Empty if referenced to self
			std::shared_ptr<RefExpr> next;

			inline RefExpr(location loc, std::string name, std::shared_ptr<RefExpr> next = std::shared_ptr<RefExpr>()) : Expr(loc) {
				this->name = name;
				this->next = next;
			}
			virtual inline ~RefExpr() {}
			virtual inline ExprType getType() { return ExprType::REF; }

			virtual inline std::string toString() const override {
				std::string s = name;
				if (next)
					s += "." + std::to_string(*next);
				return s;
			}
		};

		class ArgList : public std::vector<std::shared_ptr<Expr>>, public IStringifiable {
		public:
			std::string toString() const {
				if (this->empty())
					return "";
				std::string s;
				auto i = this->begin();
				s = std::to_string(**i);
				while (i != this->end())
					s += ", " + std::to_string(**i);
				return s;
			}
		};

		class CallExpr : public Expr {
		public:
			std::shared_ptr<Expr> target;
			std::shared_ptr<ArgList> args;
			bool isAsync;

			inline CallExpr(location loc, std::shared_ptr<Expr> target, std::shared_ptr<ArgList> args, bool isAsync = false) : Expr(loc) {
				this->target = target;
				this->args = args;
				this->isAsync = isAsync;
			}
			virtual inline ~CallExpr() {}

			virtual inline ExprType getType() override { return ExprType::CALL; }

			virtual inline std::string toString() const override {
				return std::to_string(*target) + "(" + std::to_string(*args) + ")" + (isAsync ? " async" : "");
			}
		};

		class AwaitExpr : public Expr {
		public:
			std::shared_ptr<Expr> target;

			inline AwaitExpr(location loc, std::shared_ptr<Expr> target) : Expr(loc) {
				this->target = target;
			}
			virtual inline ~AwaitExpr() {}

			virtual inline ExprType getType() override { return ExprType::AWAIT; }

			virtual inline std::string toString() const override {
				return "await " + std::to_string(*target);
			}
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
			inline LiteralExpr(location loc) : Expr(loc) {}
			virtual inline ~LiteralExpr() {}

			virtual inline ExprType getType() override { return ExprType::LITERAL; }
			virtual LiteralType getLiteralType() = 0;

			virtual inline std::shared_ptr<LiteralExpr> execUnaryOp(UnaryOp op) { return std::shared_ptr<LiteralExpr>(); };
		};

		class NullLiteralExpr : public LiteralExpr {
		public:
			inline NullLiteralExpr(location loc) : LiteralExpr(loc) {}
			virtual inline ~NullLiteralExpr() {}
			virtual inline LiteralType getLiteralType() override { return LiteralType::LT_NULL; };

			virtual inline std::string toString() const override {
				return "null";
			}
		};

		template <typename T, int LT>
		class SimpleLiteralExpr : public LiteralExpr {
		public:
			T data;

			inline SimpleLiteralExpr(location loc, T& data) : LiteralExpr(loc) { this->data = data; }
			inline SimpleLiteralExpr(location loc, T&& data) : LiteralExpr(loc) { this->data = data; }
			virtual inline ~SimpleLiteralExpr() {}

			virtual inline LiteralType getLiteralType() override { return (LiteralType)LT; };

			inline SimpleLiteralExpr<T, LT>& operator=(T& data) {
				this->data = data;
			}
			inline SimpleLiteralExpr<T, LT>& operator=(T&& data) {
				this->data = data;
			}

			inline std::shared_ptr<LiteralExpr> _execNegateUnaryOp() {
				if constexpr (std::is_signed<T>::value)
					return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), -data);
				else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execArithmeticUnaryOp(UnaryOp op) {
				if constexpr (std::is_arithmetic<T>::value) {
					switch (op) {
						case UnaryOp::INC_F:
							return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), ++data));
						case UnaryOp::INC_B:
							return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data++));
						case UnaryOp::DEC_F:
							return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), --data));
						case UnaryOp::DEC_B:
							return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data--));
					}
					return std::shared_ptr<LiteralExpr>();
				} else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execNotUnaryOp() {
				if constexpr (std::is_integral<T>::value)
					return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), !data);
				else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execRevUnaryOp() {
				if constexpr (std::is_integral<T>::value)
					return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), !data);
				else
					return std::shared_ptr<LiteralExpr>();
			}

			virtual inline std::shared_ptr<LiteralExpr> execUnaryOp(UnaryOp op) {
				switch (op) {
					case UnaryOp::INC_F:
					case UnaryOp::INC_B:
					case UnaryOp::DEC_F:
					case UnaryOp::DEC_B:
						return _execArithmeticUnaryOp(op);
					case UnaryOp::NEG:
						return _execNegateUnaryOp();
					case UnaryOp::NOT:
						return _execNotUnaryOp();
					case UnaryOp::REV:
						return _execRevUnaryOp();
				}
				return std::shared_ptr<LiteralExpr>();
			};

			virtual inline std::string toString() const override {
				if constexpr (std::is_convertible<T, std::string>::value)
					return data;
				else
					return std::to_string(data);
			}
		};

		using IntLiteralExpr = SimpleLiteralExpr<int, LT_INT>;
		using UIntLiteralExpr = SimpleLiteralExpr<unsigned int, LT_UINT>;
		using LongLiteralExpr = SimpleLiteralExpr<long long, LT_LONG>;
		using ULongLiteralExpr = SimpleLiteralExpr<unsigned long long, LT_ULONG>;
		using FloatLiteralExpr = SimpleLiteralExpr<float, LT_FLOAT>;
		using DoubleLiteralExpr = SimpleLiteralExpr<double, LT_DOUBLE>;
		using StringLiteralExpr = SimpleLiteralExpr<std::string, LT_STRING>;

		class ExprPair : public std::pair<std::shared_ptr<Expr>, std::shared_ptr<Expr>>,
						 public Expr {
		public:
			inline ExprPair(location loc) : Expr(loc), pair() {}
			inline ExprPair(location loc, std::shared_ptr<Expr> first, std::shared_ptr<Expr> second) : Expr(loc), pair(first, second) {}
			virtual inline ~ExprPair() {}

			virtual inline std::string toString() const override {
				return std::to_string(*first) + ":" + std::to_string(*second);
			}
		};

		using PairList = std::vector<std::shared_ptr<ExprPair>>;

		class MapExpr : public Expr {
		public:
			std::shared_ptr<PairList> pairs;

			inline MapExpr(location loc, std::shared_ptr<PairList> pairs) : Expr(loc) {
				this->pairs = pairs;
			}
			virtual inline ~MapExpr() {}

			virtual inline ExprType getType() override { return ExprType::MAP; }

			virtual inline std::string toString() const override {
				std::string s = "[";

				auto i = pairs->begin();
				s = std::to_string(*(**i).first) + " : " + std::to_string(*(**i).second);
				while (i != pairs->end())
					s += ", " + std::to_string(*(**i).first) + " : " + std::to_string(*(**i).second);

				s += " ]";
				return s;
			}
		};
	}
}

#endif
