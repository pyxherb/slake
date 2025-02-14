#include "../compiler.h"

using namespace slake::slkc;

template <typename T>
static std::shared_ptr<ExprNode> _castLiteralExpr(
	std::shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> expr,
	TypeId targetType) {
	switch (targetType) {
	case TypeId::I8:
		if constexpr (std::is_convertible_v<T, int8_t>) {
			return std::make_shared<I32LiteralExprNode>((int8_t)expr->data);
		} else
			return {};
	case TypeId::I16:
		if constexpr (std::is_convertible_v<T, int16_t>) {
			return std::make_shared<I32LiteralExprNode>((int16_t)expr->data);
		} else
			return {};
	case TypeId::I32:
		if constexpr (std::is_convertible_v<T, int32_t>) {
			return std::make_shared<I32LiteralExprNode>((int32_t)expr->data);
		} else
			return {};
	case TypeId::I64:
		if constexpr (std::is_convertible_v<T, int64_t>) {
			return std::make_shared<I64LiteralExprNode>((int64_t)expr->data);
		} else
			return {};
	case TypeId::U8:
		if constexpr (std::is_convertible_v<T, uint8_t>) {
			return std::make_shared<U8LiteralExprNode>((int8_t)expr->data);
		} else
			return {};
	case TypeId::U16:
		if constexpr (std::is_convertible_v<T, uint16_t>) {
			return std::make_shared<U16LiteralExprNode>((int16_t)expr->data);
		} else
			return {};
	case TypeId::U32:
		if constexpr (std::is_convertible_v<T, uint32_t>) {
			return std::make_shared<U32LiteralExprNode>((int32_t)expr->data);
		} else
			return {};
	case TypeId::U64:
		if constexpr (std::is_convertible_v<T, uint64_t>) {
			return std::make_shared<U64LiteralExprNode>((int64_t)expr->data);
		} else
			return {};
	case TypeId::F32:
		if constexpr (std::is_convertible_v<T, float>) {
			return std::make_shared<F32LiteralExprNode>((float)expr->data);
		} else
			return {};
	case TypeId::F64:
		if constexpr (std::is_convertible_v<T, double>) {
			return std::make_shared<F64LiteralExprNode>((double)expr->data);
		} else
			return {};
	case TypeId::Bool:
		if constexpr (std::is_convertible_v<T, bool>) {
			return std::make_shared<BoolLiteralExprNode>((bool)expr->data);
		} else
			return {};
	default:;
	}

	return {};
}

std::shared_ptr<ExprNode> Compiler::castLiteralExpr(std::shared_ptr<ExprNode> expr, TypeId targetType) {
	switch (expr->getExprType()) {
	case ExprType::I32:
		return _castLiteralExpr<int32_t>(std::static_pointer_cast<I32LiteralExprNode>(expr), targetType);
	case ExprType::I64:
		return _castLiteralExpr<int64_t>(std::static_pointer_cast<I64LiteralExprNode>(expr), targetType);
	case ExprType::U32:
		return _castLiteralExpr<uint32_t>(std::static_pointer_cast<U32LiteralExprNode>(expr), targetType);
	case ExprType::U64:
		return _castLiteralExpr<uint64_t>(std::static_pointer_cast<U64LiteralExprNode>(expr), targetType);
	case ExprType::F32:
		return _castLiteralExpr<float>(std::static_pointer_cast<F32LiteralExprNode>(expr), targetType);
	case ExprType::F64:
		return _castLiteralExpr<double>(std::static_pointer_cast<F64LiteralExprNode>(expr), targetType);
	case ExprType::Bool:
		return _castLiteralExpr<bool>(std::static_pointer_cast<BoolLiteralExprNode>(expr), targetType);
	default:;
	}

	return {};
}
