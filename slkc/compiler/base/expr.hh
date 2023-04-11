#ifndef _SLKC_COMPILER_EXPR_HH
#define _SLKC_COMPILER_EXPR_HH

#include <functional>
#include <memory>
#include <slake/util/byteord.hh>
#include <vector>

#include "typename.hh"

namespace Slake {
	namespace Compiler {
		class Expr;

		enum class UnaryOp : int {
			NEG = 0,  //-x
			NOT,	  //! x
			REV,	  //~x
			INC_F,	  //++x
			INC_B,	  // x++
			DEC_F,	  //--x
			DEC_B	  // x--
		};

		enum class BinaryOp : int {
			ADD = 0,  // x+y
			SUB,	  // x-y
			MUL,	  // x*y
			DIV,	  // x/y
			MOD,	  // x%y
			AND,	  // x&y
			OR,		  // x|y
			XOR,	  // x^y
			LAND,	  // x&&y
			LOR,	  // x||y
			LSH,	  // x<<y
			RSH,	  // x>>y
			EQ,		  // x==y
			NEQ,	  // x!=y
			GTEQ,	  // x>=y
			LTEQ,	  // x<=y
			GT,		  // x>y
			LT,		  // x<y

			ASSIGN_MIN,

			ASSIGN = ASSIGN_MIN,  // x=y
			ADD_ASSIGN,			  // x+=y
			SUB_ASSIGN,			  // x-=y
			MUL_ASSIGN,			  // x*=y
			DIV_ASSIGN,			  // x/=y
			MOD_ASSIGN,			  // x%=y
			AND_ASSIGN,			  // x&=y
			OR_ASSIGN,			  // x|=y
			XOR_ASSIGN,			  // x^=y
			LSH_ASSIGN,			  // x<<=y
			RSH_ASSIGN,			  // x>>=y

			ASSIGN_MAX
		};

		inline bool isAssignment(BinaryOp op) noexcept {
			return (op >= BinaryOp::ASSIGN) && (op <= BinaryOp::ASSIGN_MAX);
		}

		using MatchCase = std::pair<std::shared_ptr<Expr>, std::shared_ptr<Expr>>;
	}
}

namespace std {
	string to_string(Slake::Compiler::UnaryOp op);
	string to_string(Slake::Compiler::BinaryOp op);

	string to_string(Slake::Compiler::MatchCase c);
}

namespace Slake {
	namespace Compiler {
		enum class ExprKind : int {
			NONE = 0,
			UNARY,
			BINARY,
			TERNARY,
			SUBSCRIPT,
			CAST,
			REF,
			NEW,
			LITERAL,
			MATCH,
			CALL,
			AWAIT,
			TYPENAME,
			MAP,
			ARRAY,

			// Interal types
			LABEL
		};

		class Expr : public BasicLocated, public IStringifiable {
		public:
			inline Expr(location loc) : BasicLocated(loc) {}
			virtual inline ~Expr() {}
			virtual inline ExprKind getExprKind() { return ExprKind::NONE; }
		};

		template <ExprKind t>
		class TypedExpr : public Expr {
		public:
			inline TypedExpr(location loc) : Expr(loc) {}
			virtual inline ~TypedExpr() {}
			virtual inline ExprKind getExprKind() { return t; }
		};

		using MatchCaseList = std::vector<MatchCase>;

		class MatchExpr : public TypedExpr<ExprKind::MATCH> {
		public:
			std::shared_ptr<Expr> condition;
			MatchCaseList caseList;

			inline MatchExpr(location loc, std::shared_ptr<Expr> condition, MatchCaseList caseList) : TypedExpr(loc) {
				this->condition = condition;
				this->caseList = caseList;
			}
			virtual inline ~MatchExpr() {}

			virtual inline std::string toString() const override {
				std::string s = std::to_string(*condition) + "=> {";

				auto i = caseList.begin();
				s = std::to_string(*(i->first));
				while (i != caseList.end())
					s += ", " + std::to_string(*(*i).first);

				s += " }";
				return s;
			}
		};

		inline bool isSuffixUnaryOp(UnaryOp op) {
			switch (op) {
				case UnaryOp::INC_B:
				case UnaryOp::DEC_B:
					return true;
				default:
					return false;
			}
		}

		class UnaryOpExpr : public TypedExpr<ExprKind::UNARY> {
		public:
			std::shared_ptr<Expr> x;
			UnaryOp op;

			inline UnaryOpExpr(location loc, UnaryOp op, std::shared_ptr<Expr> x) : TypedExpr(loc), x(x), op(op) {}
			virtual inline ~UnaryOpExpr() {}

			virtual inline std::string toString() const override {
				return isSuffixUnaryOp(op)
						   ? "(" + std::to_string(*x) + ')' + std::to_string(op)
						   : std::to_string(op) + "(" + x->toString() + ')';
			}
		};

		class BinaryOpExpr : public TypedExpr<ExprKind::BINARY> {
		public:
			std::shared_ptr<Expr> x, y;
			BinaryOp op;

			inline BinaryOpExpr(location loc, BinaryOp op, std::shared_ptr<Expr> x, std::shared_ptr<Expr> y) : TypedExpr(loc), x(x), y(y), op(op) {}
			virtual inline ~BinaryOpExpr() {}

			virtual inline std::string toString() const override {
				return "(" + std::to_string(*x) + ") " + std::to_string(op) + " (" + std::to_string(*y) + ")";
			}
		};

		class TernaryOpExpr : public TypedExpr<ExprKind::TERNARY> {
		public:
			std::shared_ptr<Expr> condition, x, y;

			inline TernaryOpExpr(location loc, std::shared_ptr<Expr> condition, std::shared_ptr<Expr> x, std::shared_ptr<Expr> y)
				: TypedExpr(loc),
				  condition(condition),
				  x(x),
				  y(y) {}
			virtual inline ~TernaryOpExpr() {}

			virtual inline std::string toString() const override {
				return "(" + std::to_string(*condition) + " ? " + std::to_string(*x) + " : " + std::to_string(*y) + ")";
			}
		};

		class RefExpr : public TypedExpr<ExprKind::REF> {
		public:
			std::string name;
			std::shared_ptr<RefExpr> next;
			bool isStatic;

			inline RefExpr(
				location loc,
				std::string name,
				bool isStatic,
				std::shared_ptr<RefExpr> next = std::shared_ptr<RefExpr>()) : TypedExpr(loc), name(name), next(next), isStatic(isStatic) {}
			virtual inline ~RefExpr() {}

			virtual inline std::string toString() const override {
				std::string s = name;
				if (next)
					s += (isStatic ? "::" : ".") + std::to_string(*next);
				return s;
			}

			virtual inline std::size_t levelLeft() {
				return 1 + (next ? next->levelLeft() : 0);
			}
		};

		class ArgList : public std::vector<std::shared_ptr<Expr>>, public IStringifiable {
		public:
			std::string toString() const {
				if (this->empty())
					return "";
				std::string s;
				auto i = this->begin();
				s = std::to_string(**i++);
				while (i != this->end())
					s += ", " + std::to_string(**i++);
				return s;
			}
		};

		class CallExpr : public TypedExpr<ExprKind::CALL> {
		public:
			std::shared_ptr<Expr> target;
			std::shared_ptr<ArgList> args;
			bool isAsync;

			inline CallExpr(location loc, std::shared_ptr<Expr> target, std::shared_ptr<ArgList> args, bool isAsync = false)
				: TypedExpr(loc),
				  target(target),
				  args(args),
				  isAsync(isAsync) {}
			virtual inline ~CallExpr() {}

			virtual inline std::string toString() const override {
				return std::to_string(*target) + "(" + (args ? std::to_string(*args) : "") + ")" + (isAsync ? " async" : "");
			}
		};

		class AwaitExpr : public TypedExpr<ExprKind::AWAIT> {
		public:
			std::shared_ptr<Expr> target;

			inline AwaitExpr(location loc, std::shared_ptr<Expr> target) : TypedExpr(loc), target(target) {}
			virtual inline ~AwaitExpr() {}

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
			LT_NULL,
			LT_BOOL
		};

		class LiteralExpr : public TypedExpr<ExprKind::LITERAL> {
		public:
			inline LiteralExpr(location loc) : TypedExpr(loc) {}
			virtual inline ~LiteralExpr() {}

			virtual LiteralType getLiteralType() = 0;

			virtual inline std::shared_ptr<LiteralExpr> execUnaryOp(UnaryOp op) { return std::shared_ptr<LiteralExpr>(); }
			virtual inline std::shared_ptr<LiteralExpr> execBinaryOp(BinaryOp op, std::shared_ptr<LiteralExpr> y) { return std::shared_ptr<LiteralExpr>(); }
		};

		class NullLiteralExpr : public LiteralExpr {
		public:
			inline NullLiteralExpr(location loc) : LiteralExpr(loc) {}
			virtual inline ~NullLiteralExpr() {}
			virtual inline LiteralType getLiteralType() override { return LiteralType::LT_NULL; };
			virtual inline std::string toString() const override { return "null"; }
		};

		template <typename T, int LT>
		class SimpleLiteralExpr : public LiteralExpr {
		public:
			T data;

			inline SimpleLiteralExpr(location loc, T& data) : LiteralExpr(loc), data(data) {}
			inline SimpleLiteralExpr(location loc, T&& data) : LiteralExpr(loc), data(data) {}
			virtual inline ~SimpleLiteralExpr() {}
			virtual inline LiteralType getLiteralType() override { return (LiteralType)LT; };

			inline SimpleLiteralExpr<T, LT>& operator=(T& data) { this->data = data; }
			inline SimpleLiteralExpr<T, LT>& operator=(T&& data) { this->data = data; }

			inline std::shared_ptr<LiteralExpr> _execNegateUnaryOp() {
				if constexpr (std::is_signed<T>::value)
					return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), -data);
				else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execArithmeticUnaryOp(UnaryOp op) {
				if constexpr (std::is_arithmetic<T>::value) {
					if constexpr (std::is_same<T, bool>::value)
						return std::shared_ptr<LiteralExpr>();
					else {
						switch (op) {
							case UnaryOp::INC_F:
								return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), ++data));
							case UnaryOp::INC_B:
								return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data++));
							case UnaryOp::DEC_F:
								return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), --data));
							case UnaryOp::DEC_B:
								return std::static_pointer_cast<LiteralExpr>(std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data--));
							default:
								return std::shared_ptr<LiteralExpr>();
						}
					}
				} else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execIntegralUnaryOp(UnaryOp op) {
				if constexpr (std::is_integral<T>::value) {
					switch (op) {
						case UnaryOp::NOT:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), !data);
						case UnaryOp::REV:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), !data);
						default:
							return std::shared_ptr<LiteralExpr>();
					}
				} else
					return std::shared_ptr<LiteralExpr>();
			}

			virtual inline std::shared_ptr<LiteralExpr> execUnaryOp(UnaryOp op) override {
				switch (op) {
					case UnaryOp::INC_F:
					case UnaryOp::INC_B:
					case UnaryOp::DEC_F:
					case UnaryOp::DEC_B:
						return _execArithmeticUnaryOp(op);
					case UnaryOp::NEG:
						return _execNegateUnaryOp();
					case UnaryOp::REV:
					case UnaryOp::NOT:
						return _execIntegralUnaryOp(op);
					default:
						return std::shared_ptr<LiteralExpr>();
				}
			};

			inline std::shared_ptr<LiteralExpr> _execArithmeticBinaryOp(BinaryOp op, std::shared_ptr<LiteralExpr> y) {
				if constexpr (std::is_arithmetic<T>::value) {
					T yVal;
					switch (y->getLiteralType()) {
						case LT_INT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::int32_t, LT_INT>>(y)->data;
							break;
						case LT_UINT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::uint32_t, LT_UINT>>(y)->data;
							break;
						case LT_LONG:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::int64_t, LT_LONG>>(y)->data;
							break;
						case LT_ULONG:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::uint64_t, LT_ULONG>>(y)->data;
							break;
						case LT_FLOAT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<float, LT_FLOAT>>(y)->data;
							break;
						case LT_DOUBLE:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<double, LT_DOUBLE>>(y)->data;
							break;
						default:
							return std::shared_ptr<LiteralExpr>();
					}
					switch (op) {
						case BinaryOp::ADD:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data + yVal);
						case BinaryOp::SUB:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data - yVal);
						case BinaryOp::MUL:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data * yVal);
						case BinaryOp::DIV:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data / yVal);
						case BinaryOp::MOD:
							if constexpr (std::is_same<T, float>::value) {
								return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), std::fmodf(data, yVal));
							} else if constexpr (std::is_same<T, double>::value) {
								return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), std::fmod(data, yVal));
							} else {
								return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data % yVal);
							}
						default:
							return std::shared_ptr<LiteralExpr>();
					}
				} else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execBitwiseBinaryOp(BinaryOp op, std::shared_ptr<LiteralExpr> y) {
				if constexpr (std::is_integral<T>::value) {
					T yVal;
					switch (y->getLiteralType()) {
						case LT_INT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::int32_t, LT_INT>>(y)->data;
							break;
						case LT_UINT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::uint32_t, LT_UINT>>(y)->data;
							break;
						case LT_LONG:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::int64_t, LT_LONG>>(y)->data;
							break;
						case LT_ULONG:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::uint64_t, LT_ULONG>>(y)->data;
							break;
						default:
							return std::shared_ptr<LiteralExpr>();
					}
					switch (op) {
						case BinaryOp::AND:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data & yVal);
						case BinaryOp::OR:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data | yVal);
						case BinaryOp::XOR:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), data ^ yVal);
						case BinaryOp::LSH:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), Util::getByteOrder() ? Util::swapByteOrder(Util::swapByteOrder(data) << yVal) : data << yVal);
						case BinaryOp::RSH:
							return std::make_shared<SimpleLiteralExpr<T, LT>>(getLocation(), Util::getByteOrder() ? Util::swapByteOrder(Util::swapByteOrder(data) >> yVal) : data >> yVal);
						default:
							return std::shared_ptr<LiteralExpr>();
					}
				} else
					return std::shared_ptr<LiteralExpr>();
			}

			inline std::shared_ptr<LiteralExpr> _execComparativeBinaryOp(BinaryOp op, std::shared_ptr<LiteralExpr> y) {
				if constexpr (std::is_same<T, std::string>::value) {
					if (y->getLiteralType() != LT_STRING)
						return std::shared_ptr<LiteralExpr>();
					return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data == (std::static_pointer_cast<SimpleLiteralExpr<std::string, LT_STRING>>(y)->data));
				} else if constexpr (std::is_arithmetic<T>::value) {
					T yVal;
					switch (y->getLiteralType()) {
						case LT_INT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::int32_t, LT_INT>>(y)->data;
							break;
						case LT_UINT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::uint32_t, LT_UINT>>(y)->data;
							break;
						case LT_LONG:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::int64_t, LT_LONG>>(y)->data;
							break;
						case LT_ULONG:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<std::uint64_t, LT_ULONG>>(y)->data;
							break;
						case LT_FLOAT:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<float, LT_FLOAT>>(y)->data;
							break;
						case LT_DOUBLE:
							yVal = (T)std::static_pointer_cast<SimpleLiteralExpr<double, LT_DOUBLE>>(y)->data;
							break;
						default:
							return std::shared_ptr<LiteralExpr>();
					}
					switch (op) {
						case BinaryOp::EQ:
							return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data == yVal);
						case BinaryOp::NEQ:
							return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data != yVal);
						case BinaryOp::GTEQ:
							return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data >= yVal);
						case BinaryOp::LTEQ:
							return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data <= yVal);
						case BinaryOp::GT:
							return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data > yVal);
						case BinaryOp::LT:
							return std::make_shared<SimpleLiteralExpr<bool, LT_BOOL>>(getLocation(), data < yVal);
						default:
							return std::shared_ptr<LiteralExpr>();
					}
				} else
					return std::shared_ptr<LiteralExpr>();
			}

			virtual inline std::shared_ptr<LiteralExpr> execBinaryOp(BinaryOp op, std::shared_ptr<LiteralExpr> y) override {
				switch (op) {
					case BinaryOp::ADD:
					case BinaryOp::SUB:
					case BinaryOp::MUL:
					case BinaryOp::DIV:
					case BinaryOp::MOD:
						return _execArithmeticBinaryOp(op, y);
					case BinaryOp::AND:
					case BinaryOp::OR:
					case BinaryOp::XOR:
					case BinaryOp::LSH:
					case BinaryOp::RSH:
						return _execBitwiseBinaryOp(op, y);
					case BinaryOp::LAND:
					case BinaryOp::LOR:
					case BinaryOp::EQ:
					case BinaryOp::NEQ:
					case BinaryOp::GTEQ:
					case BinaryOp::LTEQ:
					case BinaryOp::GT:
					case BinaryOp::LT:
						return _execComparativeBinaryOp(op, y);
					default:
						return std::shared_ptr<LiteralExpr>();
				}
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
		using BoolLiteralExpr = SimpleLiteralExpr<bool, LT_BOOL>;
		using StringLiteralExpr = SimpleLiteralExpr<std::string, LT_STRING>;

		class ExprPair : public Expr,
						 public std::pair<std::shared_ptr<Expr>, std::shared_ptr<Expr>> {
		public:
			inline ExprPair(location loc) : Expr(loc), pair() {}
			inline ExprPair(location loc, std::shared_ptr<Expr> first, std::shared_ptr<Expr> second) : Expr(loc), pair(first, second) {}
			virtual inline ~ExprPair() {}

			virtual inline std::string toString() const override {
				return std::to_string(*first) + ":" + std::to_string(*second);
			}
		};

		using PairList = std::vector<std::shared_ptr<ExprPair>>;

		class MapExpr : public TypedExpr<ExprKind::MAP> {
		public:
			std::shared_ptr<PairList> pairs;

			inline MapExpr(location loc, std::shared_ptr<PairList> pairs) : TypedExpr(loc), pairs(pairs) {}
			virtual inline ~MapExpr() {}

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

		class NewExpr : public TypedExpr<ExprKind::NEW> {
		public:
			std::shared_ptr<TypeName> type;
			std::shared_ptr<ArgList> args;

			inline NewExpr(location loc, std::shared_ptr<TypeName> type, std::shared_ptr<ArgList> args) : TypedExpr(loc), type(type), args(args) {}
			virtual inline ~NewExpr() {}

			virtual inline std::string toString() const override {
				return "new " + std::to_string(*type) + "(" + std::to_string(*args) + ")";
			}
		};

		class SubscriptOpExpr : public TypedExpr<ExprKind::SUBSCRIPT> {
		public:
			std::shared_ptr<Expr> target;
			std::shared_ptr<Expr> subscription;

			inline SubscriptOpExpr(location loc, std::shared_ptr<Expr> target, std::shared_ptr<Expr> subscription) : TypedExpr(loc), target(target), subscription(subscription) {}
			virtual inline ~SubscriptOpExpr() {}

			virtual inline std::string toString() const override {
				return std::to_string(*target) + "[" + std::to_string(*subscription) + "]";
			}
		};

		class CastExpr : public TypedExpr<ExprKind::CAST> {
		public:
			std::shared_ptr<Expr> target;
			std::shared_ptr<TypeName> type;
			bool rawCast;

			inline CastExpr(location loc, std::shared_ptr<TypeName> type, std::shared_ptr<Expr> target, bool rawCast = false)
				: TypedExpr(loc), target(target), type(type), rawCast(rawCast) {}
			virtual inline ~CastExpr() {}

			virtual inline std::string toString() const override {
				return rawCast ? "(" + std::to_string(*type) + ")" + std::to_string(*target) : "<" + std::to_string(*type) + ">" + "(" + std::to_string(*target) + ")";
			}
		};

		class ArrayExpr : public TypedExpr<ExprKind::ARRAY> {
		public:
			std::vector<std::shared_ptr<Expr>> elements;

			inline ArrayExpr(location loc, std::vector<std::shared_ptr<Expr>> elements) : TypedExpr(loc) {
				this->elements = elements;
			}
			virtual inline ~ArrayExpr() {}

			virtual inline std::string toString() const override {
				std::string s = "{";

				auto i = elements.begin();
				while (i != elements.end())
					s += (i != elements.begin() ? ", " : "") + std::to_string(**i++);

				s += " }";
				return s;
			}
		};

		class LabelExpr : public TypedExpr<ExprKind::LABEL> {
		public:
			std::string label;

			inline LabelExpr(std::string label) : TypedExpr(location(position())) {
				this->label = label;
			}
			virtual inline ~LabelExpr() {}

			virtual inline std::string toString() const override {
				return label + ":";
			}
		};
	}
}

#endif
