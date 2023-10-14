#include "../compiler.h"
#include <type_traits>
#include <cmath>

using namespace slake::slkc;

struct UnaryOpRegistry {
	slake::Opcode opcode;
	bool lvalueOperand;

	inline UnaryOpRegistry(
		slake::Opcode opcode,
		bool lvalueOperand) : opcode(opcode), lvalueOperand(lvalueOperand) {}
};

static map<UnaryOp, UnaryOpRegistry> _unaryOpRegs = {
	{ OP_NOT, { slake::Opcode::NOT, false } },
	{ OP_REV, { slake::Opcode::REV, false } },
	{ OP_INCF, { slake::Opcode::INCF, true } },
	{ OP_DECF, { slake::Opcode::DECF, true } },
	{ OP_INCB, { slake::Opcode::INCB, true } },
	{ OP_DECB, { slake::Opcode::DECB, true } }
};

struct BinaryOpRegistry {
	slake::Opcode opcode;
	bool isLvalueRhs;

	inline BinaryOpRegistry(
		slake::Opcode opcode,
		bool isLvalueRhs)
		: opcode(opcode),
		  isLvalueRhs(isLvalueRhs) {}
};

static map<BinaryOp, BinaryOpRegistry> _binaryOpRegs = {
	{ OP_ADD, { slake::Opcode::ADD, false } },
	{ OP_SUB, { slake::Opcode::SUB, false } },
	{ OP_MUL, { slake::Opcode::MUL, false } },
	{ OP_DIV, { slake::Opcode::DIV, false } },
	{ OP_MOD, { slake::Opcode::MOD, false } },
	{ OP_AND, { slake::Opcode::AND, false } },
	{ OP_OR, { slake::Opcode::OR, false } },
	{ OP_XOR, { slake::Opcode::XOR, false } },
	{ OP_LAND, { slake::Opcode::LAND, false } },
	{ OP_LOR, { slake::Opcode::LOR, false } },
	{ OP_LSH, { slake::Opcode::LSH, false } },
	{ OP_RSH, { slake::Opcode::RSH, false } },
	{ OP_SWAP, { slake::Opcode::SWAP, true } },

	{ OP_ASSIGN, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_ADD, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_SUB, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_MUL, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_DIV, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_MOD, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_AND, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_OR, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_XOR, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_LAND, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_LOR, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_LSH, { slake::Opcode::NOP, false } },
	{ OP_ASSIGN_RSH, { slake::Opcode::NOP, false } },

	{ OP_EQ, { slake::Opcode::EQ, false } },
	{ OP_NEQ, { slake::Opcode::NEQ, false } },
	{ OP_STRICTEQ, { slake::Opcode::SEQ, false } },
	{ OP_STRICTNEQ, { slake::Opcode::SNEQ, false } },
	{ OP_LT, { slake::Opcode::LT, false } },
	{ OP_GT, { slake::Opcode::GT, false } },
	{ OP_LTEQ, { slake::Opcode::LTEQ, false } },
	{ OP_GTEQ, { slake::Opcode::GTEQ, false } },
	{ OP_SUBSCRIPT, { slake::Opcode::AT, false } }
};

static map<BinaryOp, BinaryOp> _assignBinaryOpToOrdinaryBinaryOpMap = {
	{ OP_ASSIGN_ADD, OP_ADD },
	{ OP_ASSIGN_SUB, OP_SUB },
	{ OP_ASSIGN_MUL, OP_MUL },
	{ OP_ASSIGN_DIV, OP_DIV },
	{ OP_ASSIGN_MOD, OP_MOD },
	{ OP_ASSIGN_AND, OP_AND },
	{ OP_ASSIGN_OR, OP_OR },
	{ OP_ASSIGN_XOR, OP_XOR },
	{ OP_ASSIGN_LAND, OP_LAND },
	{ OP_ASSIGN_LOR, OP_LOR },
	{ OP_ASSIGN_LSH, OP_LSH },
	{ OP_ASSIGN_RSH, OP_RSH }
};

template <typename T>
static shared_ptr<ExprNode> _evalConstUnaryOpExpr(
	UnaryOp op,
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
		case OP_NOT:
			if constexpr (is_convertible_v<bool, T>) {
				return make_shared<BoolLiteralExprNode>(x->getLocation(), !(x->data));
			} else {
				return {};
			}
		case OP_REV: {
			if constexpr (is_same_v<T, bool>) {
				return make_shared<LT>(x->getLocation(), !x->data);
			} else if constexpr (is_integral_v<T>) {
				return make_shared<LT>(x->getLocation(), ~x->data);
			} else if constexpr (is_same_v<T, float>) {
				return make_shared<LT>(x->getLocation(), ~(*(uint32_t *)&x->data));
			} else if constexpr (is_same_v<T, double>) {
				return make_shared<LT>(x->getLocation(), ~(*(uint64_t *)&x->data));
			} else
				return {};
		}
		default:
			return {};
	}
}

template <typename T>
static shared_ptr<ExprNode> _evalConstBinaryOpExpr(
	BinaryOp op,
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x,
	shared_ptr<ExprNode> y) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
		case OP_ADD:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data + static_pointer_cast<LT>(y)->data);
			else
				return make_shared<LT>(
					x->getLocation(),
					x->data + static_pointer_cast<LT>(y)->data);
		case OP_SUB:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data - static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data - static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_MUL:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data * static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data * static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_DIV:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(), ((int)x->data) / static_pointer_cast<LT>(y)->data);
			else if constexpr (is_arithmetic_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data / static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_MOD:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					((int)x->data) % static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data * static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>)
				return make_shared<LT>(
					x->getLocation(),
					fmodf(x->data, static_pointer_cast<LT>(y)->data));
			else if constexpr (is_same_v<T, double>)
				return make_shared<LT>(
					x->getLocation(),
					fmod(x->data, static_pointer_cast<LT>(y)->data));
			else
				return {};
		case OP_AND:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data && static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data & static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) & (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) & (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_OR:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data || static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data | static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) | (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) | (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_XOR:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					x->data ^ static_pointer_cast<LT>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data ^ static_pointer_cast<LT>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) ^ (*(uint32_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) ^ (*(uint64_t *)&static_pointer_cast<LT>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_LAND:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data && static_pointer_cast<LT>(y)->data);
			} else
				return {};
		case OP_LOR:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_same_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data || static_pointer_cast<LT>(y)->data);
			} else
				return {};
		case OP_LSH:
			if (y->getExprType() != EXPR_I32)
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data << static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data << static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) << (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) << (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_RSH:
			if (y->getExprType() != EXPR_I32)
				return {};
			if constexpr (is_same_v<T, bool>)
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					(int)x->data >> static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_integral_v<T>)
				return make_shared<LT>(
					x->getLocation(),
					x->data >> static_pointer_cast<I32LiteralExprNode>(y)->data);
			else if constexpr (is_same_v<T, float>) {
				uint32_t tmp = (*(uint32_t *)&x->data) >> (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((float *)&tmp));
			} else if constexpr (is_same_v<T, double>) {
				uint64_t tmp = (*(uint64_t *)&x->data) >> (static_pointer_cast<I32LiteralExprNode>(y)->data);
				return make_shared<LT>(x->getLocation(), *((double *)&tmp));
			} else
				return {};
		case OP_EQ:
			if (y->getExprType() != x->getExprType())
				return {};
			return make_shared<BoolLiteralExprNode>(
				x->getLocation(),
				x->data == static_pointer_cast<LT>(y)->data);
		case OP_NEQ:
			if (y->getExprType() != x->getExprType())
				return {};
			return make_shared<BoolLiteralExprNode>(
				x->getLocation(),
				x->data != static_pointer_cast<LT>(y)->data);
		case OP_LT:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data < static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_GT:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data > static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_LTEQ:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data <= static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_GTEQ:
			if (y->getExprType() != x->getExprType())
				return {};
			if constexpr (is_arithmetic_v<T>)
				return make_shared<BoolLiteralExprNode>(
					x->getLocation(),
					x->data >= static_pointer_cast<LT>(y)->data);
			else
				return {};
		case OP_SUBSCRIPT:
			if (y->getExprType() != EXPR_I32)
				return {};
			if (static_pointer_cast<I32LiteralExprNode>(y)->data < 0)
				return {};

			if constexpr (is_same_v<T, string>) {
				return make_shared<I32LiteralExprNode>(
					x->getLocation(),
					x->data[static_pointer_cast<I32LiteralExprNode>(y)->data]);
			} else
				return {};
		default:
			return {};
	}
}

template <typename T>
static shared_ptr<ExprNode> _castLiteralExpr(
	shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> expr,
	Type targetType) {
	switch (targetType) {
		case TYPE_I8:
			if constexpr (std::is_convertible_v<T, int8_t>) {
				return make_shared<I32LiteralExprNode>(expr->getLocation(), (int8_t)expr->data);
			} else
				return {};
		case TYPE_I16:
			if constexpr (std::is_convertible_v<T, int16_t>) {
				return make_shared<I32LiteralExprNode>(expr->getLocation(), (int16_t)expr->data);
			} else
				return {};
		case TYPE_I32:
			if constexpr (std::is_convertible_v<T, int32_t>) {
				return make_shared<I32LiteralExprNode>(expr->getLocation(), (int32_t)expr->data);
			} else
				return {};
		case TYPE_I64:
			if constexpr (std::is_convertible_v<T, int64_t>) {
				return make_shared<I64LiteralExprNode>(expr->getLocation(), (int64_t)expr->data);
			} else
				return {};
		case TYPE_U8:
			if constexpr (std::is_convertible_v<T, uint8_t>) {
				return make_shared<U8LiteralExprNode>(expr->getLocation(), (int8_t)expr->data);
			} else
				return {};
		case TYPE_U16:
			if constexpr (std::is_convertible_v<T, uint16_t>) {
				return make_shared<U16LiteralExprNode>(expr->getLocation(), (int16_t)expr->data);
			} else
				return {};
		case TYPE_U32:
			if constexpr (std::is_convertible_v<T, uint32_t>) {
				return make_shared<U32LiteralExprNode>(expr->getLocation(), (int32_t)expr->data);
			} else
				return {};
		case TYPE_U64:
			if constexpr (std::is_convertible_v<T, uint64_t>) {
				return make_shared<U64LiteralExprNode>(expr->getLocation(), (int64_t)expr->data);
			} else
				return {};
		case TYPE_F32:
			if constexpr (std::is_convertible_v<T, float>) {
				return make_shared<F32LiteralExprNode>(expr->getLocation(), (float)expr->data);
			} else
				return {};
		case TYPE_F64:
			if constexpr (std::is_convertible_v<T, double>) {
				return make_shared<F64LiteralExprNode>(expr->getLocation(), (double)expr->data);
			} else
				return {};
		case TYPE_CHAR:
			// stub
			return {};
		case TYPE_WCHAR:
			// stub
			return {};
		case TYPE_BOOL:
			if constexpr (std::is_convertible_v<T, bool>) {
				return make_shared<BoolLiteralExprNode>(expr->getLocation(), (bool)expr->data);
			} else
				return {};
	}
}

static shared_ptr<ExprNode> _castLiteralExpr(shared_ptr<ExprNode> expr, Type targetType) {
	switch (expr->getExprType()) {
		case EXPR_I32:
			return _castLiteralExpr<int32_t>(static_pointer_cast<I32LiteralExprNode>(expr), targetType);
		case EXPR_I64:
			return _castLiteralExpr<int64_t>(static_pointer_cast<I64LiteralExprNode>(expr), targetType);
		case EXPR_U32:
			return _castLiteralExpr<uint32_t>(static_pointer_cast<U32LiteralExprNode>(expr), targetType);
		case EXPR_U64:
			return _castLiteralExpr<uint64_t>(static_pointer_cast<U64LiteralExprNode>(expr), targetType);
		case EXPR_F32:
			return _castLiteralExpr<float>(static_pointer_cast<F32LiteralExprNode>(expr), targetType);
		case EXPR_F64:
			return _castLiteralExpr<double>(static_pointer_cast<F64LiteralExprNode>(expr), targetType);
		case EXPR_BOOL:
			return _castLiteralExpr<bool>(static_pointer_cast<BoolLiteralExprNode>(expr), targetType);
	}
}

shared_ptr<ExprNode> Compiler::evalConstExpr(shared_ptr<ExprNode> expr) {
	// stub
	return {};

	switch (expr->getExprType()) {
		case EXPR_I8:
		case EXPR_I16:
		case EXPR_I32:
		case EXPR_I64:
		case EXPR_U8:
		case EXPR_U16:
		case EXPR_U32:
		case EXPR_U64:
		case EXPR_F32:
		case EXPR_F64:
		case EXPR_STRING:
		case EXPR_BOOL:
			return expr;
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);
			if (!evalConstExpr(e->x))
				return {};

			switch (e->x->getExprType()) {
				case EXPR_I32:
					return _evalConstUnaryOpExpr<int32_t>(e->op, static_pointer_cast<I32LiteralExprNode>(e->x));
				case EXPR_I64:
					return _evalConstUnaryOpExpr<int64_t>(e->op, static_pointer_cast<I64LiteralExprNode>(e->x));
				case EXPR_U32:
					return _evalConstUnaryOpExpr<uint32_t>(e->op, static_pointer_cast<U32LiteralExprNode>(e->x));
				case EXPR_U64:
					return _evalConstUnaryOpExpr<uint64_t>(e->op, static_pointer_cast<U64LiteralExprNode>(e->x));
				case EXPR_F32:
					return _evalConstUnaryOpExpr<float>(e->op, static_pointer_cast<F32LiteralExprNode>(e->x));
				case EXPR_F64:
					return _evalConstUnaryOpExpr<double>(e->op, static_pointer_cast<F64LiteralExprNode>(e->x));
				case EXPR_STRING:
					return _evalConstUnaryOpExpr<string>(e->op, static_pointer_cast<StringLiteralExprNode>(e->x));
				case EXPR_BOOL:
					return _evalConstUnaryOpExpr<bool>(e->op, static_pointer_cast<BoolLiteralExprNode>(e->x));
				default:
					return {};
			}
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			auto lhs = evalConstExpr(e->lhs), rhs = evalConstExpr(e->rhs);

			if ((!lhs) || (!rhs))
				return {};

			switch (lhs->getExprType()) {
				case EXPR_I32:
					return _evalConstBinaryOpExpr<int32_t>(
						e->op,
						static_pointer_cast<I32LiteralExprNode>(lhs),
						rhs);
				case EXPR_I64:
					return _evalConstBinaryOpExpr<int64_t>(
						e->op,
						static_pointer_cast<I64LiteralExprNode>(lhs),
						rhs);
				case EXPR_U32:
					return _evalConstBinaryOpExpr<uint32_t>(
						e->op,
						static_pointer_cast<U32LiteralExprNode>(lhs),
						rhs);
				case EXPR_U64:
					return _evalConstBinaryOpExpr<uint64_t>(
						e->op,
						static_pointer_cast<U64LiteralExprNode>(lhs),
						rhs);
				case EXPR_F32:
					return _evalConstBinaryOpExpr<float>(
						e->op,
						static_pointer_cast<F32LiteralExprNode>(lhs),
						rhs);
				case EXPR_F64:
					return _evalConstBinaryOpExpr<double>(
						e->op,
						static_pointer_cast<F64LiteralExprNode>(lhs),
						rhs);
				case EXPR_STRING:
					return _evalConstBinaryOpExpr<string>(
						e->op,
						static_pointer_cast<StringLiteralExprNode>(lhs),
						rhs);
				case EXPR_BOOL:
					return _evalConstBinaryOpExpr<bool>(
						e->op,
						static_pointer_cast<BoolLiteralExprNode>(lhs),
						rhs);
				default:
					return {};
			}
		}
		case EXPR_TERNARY: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			auto condition = evalConstExpr(e->condition);
			if (condition) {
				if (static_pointer_cast<BoolLiteralExprNode>(_castLiteralExpr(condition, TYPE_BOOL))->data)
					return e->x;
				return e->y;
			}

			return {};
		}
		case EXPR_ARRAY: {
			auto e = static_pointer_cast<ArrayExprNode>(expr);
			for (auto i : e->elements)
				if (!evalConstExpr(i))
					return false;
			return e;
		}
		case EXPR_MAP: {
			auto e = static_pointer_cast<MapExprNode>(expr);
			for (auto i : e->pairs)
				if ((!evalConstExpr(i.first)) || (!evalConstExpr(i.second)))
					return false;
			return e;
		}
		case EXPR_REF: {
			auto e = static_pointer_cast<RefExprNode>(expr);
		}
		case EXPR_CAST: {
			auto e = static_pointer_cast<CastExprNode>(expr);
			if (!isLiteralType(e->targetType))
				return false;

			switch (e->getExprType()) {
				case EXPR_I32: {
					switch (e->targetType->getTypeId()) {
						case TYPE_I8: {
						}
						case TYPE_I16: {
						}
						case TYPE_I32: {
						}
						case TYPE_I64: {
						}
						case TYPE_U8: {
						}
						case TYPE_U16: {
						}
						case TYPE_U32: {
						}
						case TYPE_U64: {
						}
						case TYPE_F32: {
						}
						case TYPE_F64: {
						}
						case TYPE_CHAR: {
						}
						case TYPE_WCHAR: {
						}
						case TYPE_BOOL: {
						}
					}
				}
				case EXPR_U32: {
				}
				case EXPR_U64: {
				}
				case EXPR_I64: {
				}
				case EXPR_F32: {
				}
				case EXPR_F64: {
				}
				case EXPR_BOOL: {
				}
			}
			return evalConstExpr(e->target);
		}
		case EXPR_MATCH:
			// stub
		default:
			return {};
	}
}

shared_ptr<TypeNameNode> Compiler::evalExprType(shared_ptr<ExprNode> expr) {
	switch (expr->getExprType()) {
		case EXPR_I8:
			return make_shared<I8TypeNameNode>(Location(), true);
		case EXPR_I16:
			return make_shared<I16TypeNameNode>(Location(), true);
		case EXPR_I32:
			return make_shared<I32TypeNameNode>(Location(), true);
		case EXPR_I64:
			return make_shared<I64TypeNameNode>(Location(), true);
		case EXPR_U8:
			return make_shared<U8TypeNameNode>(Location(), true);
		case EXPR_U16:
			return make_shared<U16TypeNameNode>(Location(), true);
		case EXPR_U32:
			return make_shared<U32TypeNameNode>(Location(), true);
		case EXPR_U64:
			return make_shared<U64TypeNameNode>(Location(), true);
		case EXPR_F32:
			return make_shared<F32TypeNameNode>(Location(), true);
		case EXPR_F64:
			return make_shared<F64TypeNameNode>(Location(), true);
		case EXPR_STRING:
			return make_shared<StringTypeNameNode>(Location(), true);
		case EXPR_BOOL:
			return make_shared<BoolTypeNameNode>(Location(), true);
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			if (isCompoundType(evalExprType(e->x))) {
			} else
				return evalExprType(e->x);

			break;
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			if (isCompoundType(evalExprType(e->lhs))) {
			} else {
				switch (e->op) {
					case OP_ADD:
					case OP_SUB:
					case OP_MUL:
					case OP_DIV:
					case OP_MOD:
					case OP_AND:
					case OP_OR:
					case OP_XOR:
					case OP_LSH:
					case OP_RSH:
					case OP_ASSIGN:
					case OP_ASSIGN_ADD:
					case OP_ASSIGN_SUB:
					case OP_ASSIGN_MUL:
					case OP_ASSIGN_DIV:
					case OP_ASSIGN_MOD:
					case OP_ASSIGN_AND:
					case OP_ASSIGN_OR:
					case OP_ASSIGN_XOR:
					case OP_ASSIGN_LAND:
					case OP_ASSIGN_LOR:
					case OP_ASSIGN_LSH:
					case OP_ASSIGN_RSH:
						return evalExprType(e->lhs);
					case OP_LAND:
					case OP_LOR:
					case OP_EQ:
					case OP_NEQ:
					case OP_STRICTEQ:
					case OP_STRICTNEQ:
					case OP_LT:
					case OP_GT:
					case OP_LTEQ:
					case OP_GTEQ:
						return make_shared<BoolTypeNameNode>(Location(), true);
					case OP_SWAP:
					default:
						assert(false);
				}
			}

			break;
		}
		case EXPR_REF: {
			auto e = static_pointer_cast<RefExprNode>(expr);
			Ref staticPart, dynamicPart;

			if (auto m = resolveRef(e->ref, staticPart, dynamicPart); m) {
				switch (m->getNodeType()) {
					case AST_VAR:
						return static_pointer_cast<VarNode>(m)->type;
					case AST_LOCAL_VAR:
						return static_pointer_cast<LocalVarNode>(m)->type;
					case AST_CLASS:
					case AST_INTERFACE:
					case AST_TRAIT:
						throw FatalCompilationError(
							{ e->getLocation(),
								MSG_ERROR,
								"`" + to_string(e->ref) + "' is a type" });
					default:
						assert(false);
				}
			} else {
				throw FatalCompilationError(
					{ e->getLocation(),
						MSG_ERROR,
						"Identifier not found: `" + to_string(e->ref) + "'" });
			}

			break;
		}
	}
	assert(false);
}

void Compiler::compileExpr(shared_ptr<ExprNode> expr) {
	if (auto ce = evalConstExpr(expr); ce) {
		context.curFn->insertIns(Opcode::STORE, context.evalDest, ce);
		return;
	}

	switch (expr->getExprType()) {
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			bool restoreR0 = preserveRegister(RegId::R0);

			compileExpr(
				e->x,
				_unaryOpRegs.at(e->op).lvalueOperand ? EvalPurpose::LVALUE : EvalPurpose::RVALUE,
				make_shared<RegRefNode>(RegId::R0));
			if (context.evalPurpose == EvalPurpose::STMT) {
				if (_unaryOpRegs.at(e->op).lvalueOperand)
					context.curFn->insertIns(_unaryOpRegs.at(e->op).opcode, make_shared<RegRefNode>(RegId::R0), make_shared<RegRefNode>(RegId::R0));
			} else
				context.curFn->insertIns(_unaryOpRegs.at(e->op).opcode, context.evalDest, make_shared<RegRefNode>(RegId::R0, _unaryOpRegs.at(e->op).lvalueOperand));

			if (restoreR0)
				restoreRegister(RegId::R0);
			break;
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			bool restoreR0 = preserveRegister(RegId::R0),
				 restoreR1 = preserveRegister(RegId::R1);

			compileExpr(e->lhs, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R0));

			setRegisterPreserved(RegId::R0);
			compileExpr(e->rhs, _binaryOpRegs.at(e->op).isLvalueRhs ? EvalPurpose::LVALUE : EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R1));
			unsetRegisterPreserved(RegId::R0);

			if (isAssignBinaryOp(e->op)) {
				// RHS of the assignment expression.
				if (auto opcode = _binaryOpRegs.at(_assignBinaryOpToOrdinaryBinaryOpMap.at(e->op)).opcode; opcode != Opcode::NOP)
					context.curFn->insertIns(
						opcode,
						make_shared<RegRefNode>(RegId::R1),
						make_shared<RegRefNode>(RegId::R0, true),
						make_shared<RegRefNode>(RegId::R1, true));

				// LHS of the assignment expression.
				compileExpr(e->lhs, EvalPurpose::LVALUE, make_shared<RegRefNode>(RegId::R0));
				context.curFn->insertIns(Opcode::STORE, make_shared<RegRefNode>(RegId::R0, true), make_shared<RegRefNode>(RegId::R1, true));

				if ((context.evalPurpose != EvalPurpose::STMT) && (context.evalDest))
					context.curFn->insertIns(Opcode::STORE, static_pointer_cast<RegRefNode>(context.evalDest), make_shared<RegRefNode>(RegId::R1, true));
			} else if (context.evalPurpose != EvalPurpose::STMT) {
				context.curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					context.evalDest,
					make_shared<RegRefNode>(RegId::R0, true),
					make_shared<RegRefNode>(RegId::R1, !_binaryOpRegs.at(e->op).isLvalueRhs));
			}

			if (restoreR0)
				restoreRegister(RegId::R0);
			if (restoreR1)
				restoreRegister(RegId::R1);

			break;
		}
		case EXPR_TERNARY: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			auto loc = e->getLocation();
			string falseBranchLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_false",
				   endLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			bool restoreR0 = preserveRegister(RegId::R0);

			compileExpr(e->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R0));
			if (evalExprType(e->condition)->getTypeId() != TYPE_BOOL)
				context.curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(RegId::R0),
					make_shared<BoolTypeNameNode>(e->getLocation(), true), make_shared<RegRefNode>(RegId::R0));

			context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(falseBranchLabel), make_shared<RegRefNode>(RegId::R0, true));

			// Compile the true expression.
			compileExpr(e->x, EvalPurpose::RVALUE, context.evalDest);
			context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

			// Compile the false expression.
			context.curFn->insertLabel(falseBranchLabel);
			compileExpr(e->y, EvalPurpose::RVALUE, context.evalDest);

			context.curFn->insertLabel(endLabel);

			if (restoreR0)
				restoreRegister(RegId::R0);

			break;
		}
		case EXPR_MATCH: {
			auto e = static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->getLocation();

			string labelPrefix = "$match_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			bool restoreR2 = preserveRegister(RegId::R2);

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R2));
			auto condVar = (context.localVars[condLocalVarName] = make_shared<LocalVarNode>(context.localVars.size(), make_shared<AnyTypeNameNode>(loc, false)));

			pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>> defaultCase;

			for (auto i : e->cases) {
				string caseEndLabel = "$match_" + to_string(i.second->getLocation().line) + "_" + to_string(i.second->getLocation().column) + "_caseEnd";

				if (!i.first) {
					if (defaultCase.second)
						// The default case is already exist.
						throw FatalCompilationError(
							{ i.second->getLocation(),
								MSG_ERROR,
								"Duplicated default case" });
					defaultCase = i;
				}

				compileExpr(i.first, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::EQ, make_shared<RegRefNode>(RegId::TMP0), make_shared<RegRefNode>(RegId::R2), make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(caseEndLabel), make_shared<RegRefNode>(RegId::TMP0, true));

				// Leave the minor stack that is created for the local variable.
				compileExpr(i.second, context.evalPurpose, context.evalDest);
				;

				context.curFn->insertLabel(caseEndLabel);
				context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			if (defaultCase.second)
				compileExpr(defaultCase.second);

			context.curFn->insertLabel(endLabel);

			if (restoreR2)
				restoreRegister(RegId::R2);

			break;
		}
		case EXPR_CLOSURE: {
			auto e = static_pointer_cast<ClosureExprNode>(expr);
			break;
		}
		case EXPR_CALL: {
			auto e = static_pointer_cast<CallExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				context.curFn->insertIns(Opcode::CALL, ce);
			} else {
				bool restoreR0 = preserveRegister(RegId::R0);

				for (auto i : e->args) {
					if (auto ce = evalConstExpr(i); ce)
						context.curFn->insertIns(Opcode::PUSHARG, ce);
					else {
						compileExpr(i, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
						context.curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(RegId::TMP0, true));
					}
				}

				compileExpr(e->target, EvalPurpose::CALL, make_shared<RegRefNode>(RegId::R0));
				if (!context.isLastCallTargetStatic)
					restoreRegister(RegId::RTHIS);
				context.curFn->insertIns(e->isAsync ? Opcode::ACALL : Opcode::CALL, make_shared<RegRefNode>(RegId::R0, true));

				if (restoreR0)
					restoreRegister(RegId::R0);
			}

			break;
		}
		case EXPR_AWAIT: {
			auto e = static_pointer_cast<AwaitExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				context.curFn->insertIns(Opcode::AWAIT, ce);
			} else {
				bool restoreR0 = preserveRegister(RegId::R0);

				// TODO: Check if the target is a method.
				compileExpr(e->target, context.evalPurpose, make_shared<RegRefNode>(RegId::R0));
				context.curFn->insertIns(Opcode::AWAIT, make_shared<RegRefNode>(RegId::R0));

				if (restoreR0)
					restoreRegister(RegId::R0);
			}

			break;
		}
		case EXPR_NEW: {
			auto e = static_pointer_cast<NewExprNode>(expr);

			for (auto i : e->args) {
				if (auto ce = evalConstExpr(i); ce)
					context.curFn->insertIns(Opcode::PUSHARG, ce);
				else {
					compileExpr(i, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
					context.curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(RegId::TMP0, true));
				}
			}

			context.curFn->insertIns(Opcode::NEW, context.evalDest, e->type);
			break;
		}
		case EXPR_TYPEOF: {
			auto e = static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				context.curFn->insertIns(Opcode::TYPEOF, context.evalDest, ce);
			} else {
				compileExpr(e->target, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::TYPEOF, context.evalDest, make_shared<RegRefNode>(RegId::TMP0));
			}

			break;
		}
		case EXPR_CAST: {
			auto e = static_pointer_cast<CastExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				context.curFn->insertIns(Opcode::CAST, context.evalDest, ce, e->targetType);
			} else {
				compileExpr(e->target, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::CAST, context.evalDest, make_shared<RegRefNode>(RegId::TMP0), e->targetType);
			}

			break;
		}
		case EXPR_REF: {
			auto e = static_pointer_cast<RefExprNode>(expr);

			Ref staticPart, dynamicPart;
			auto x = resolveRef(e->ref, staticPart, dynamicPart);
			if (!x)
				// The default case is already exist.
				throw FatalCompilationError(
					{ e->getLocation(),
						MSG_ERROR,
						"Identifier not found: `" + to_string(e->ref) + "'" });

			switch (x->getNodeType()) {
				case AST_LOCAL_VAR:
					context.curFn->insertIns(
						Opcode::STORE,
						context.evalDest,
						make_shared<LocalVarRefNode>(
							static_pointer_cast<LocalVarNode>(x)->index,
							context.evalPurpose == EvalPurpose::RVALUE));
					break;
				case AST_ARG_REF:
					static_pointer_cast<ArgRefNode>(x)->unwrapData = (context.evalPurpose == EvalPurpose::RVALUE);
					context.curFn->insertIns(
						Opcode::STORE,
						context.evalDest, x);
					break;
				case AST_FN: {
					context.curFn->insertIns(
						Opcode::LOAD,
						context.evalDest, make_shared<RefExprNode>(staticPart));

					// Check if the target is static.
					if (dynamicPart.size()) {
						if (context.evalPurpose == EvalPurpose::CALL) {
							preserveRegister(RegId::RTHIS);
							context.isLastCallTargetStatic = false;
							context.curFn->insertIns(
								Opcode::STORE,
								make_shared<RegRefNode>(RegId::RTHIS), context.evalDest);
						}

						context.curFn->insertIns(
							Opcode::RLOAD,
							context.evalDest, make_shared<RefExprNode>(dynamicPart));
					}
					break;
				}
				default:
					assert(false);
			}

			break;
		}
		case EXPR_I8:
		case EXPR_I16:
		case EXPR_I32:
		case EXPR_I64:
		case EXPR_U32:
		case EXPR_U64:
		case EXPR_F32:
		case EXPR_F64:
		case EXPR_STRING:
		case EXPR_BOOL:
		case EXPR_ARRAY:
		case EXPR_MAP:
			context.curFn->insertIns(Opcode::STORE, context.evalDest, expr);
			break;
		default:
			assert(false);
	}
}
