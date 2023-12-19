#include "../compiler.h"

using namespace slake::slkc;

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

	return {};
}

shared_ptr<ExprNode> Compiler::castLiteralExpr(shared_ptr<ExprNode> expr, Type targetType) {
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

	return {};
}
