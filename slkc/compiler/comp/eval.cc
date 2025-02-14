#include "../compiler.h"
#include <cmath>

using namespace slake::slkc;

template <typename T>
static std::shared_ptr<ExprNode> _evalConstUnaryOpExpr(
	UnaryOp op,
	std::shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
	case UnaryOp::LNot:
		if constexpr (std::is_convertible_v<bool, T>) {
			return std::make_shared<BoolLiteralExprNode>(!(x->data));
		} else {
			return {};
		}
	case UnaryOp::Not: {
		if constexpr (std::is_same_v<T, bool>) {
			return std::make_shared<LT>(!x->data);
		} else if constexpr (std::is_integral_v<T>) {
			return std::make_shared<LT>(~x->data);
		} else if constexpr (std::is_same_v<T, float>) {
			uint32_t result = ~(*(uint32_t *)&x->data);
			return std::make_shared<LT>(*(float *)&result);
		} else if constexpr (std::is_same_v<T, double>) {
			uint64_t result = ~(*(uint64_t *)&x->data);
			return std::make_shared<LT>(*(double *)&result);
		} else
			return {};
	}
	default:
		return {};
	}
}

template <typename T>
static std::shared_ptr<ExprNode> _evalConstBinaryOpExpr(
	BinaryOp op,
	std::shared_ptr<LiteralExprNode<T, getLiteralExprType<T>()>> x,
	std::shared_ptr<ExprNode> y) {
	using LT = LiteralExprNode<T, getLiteralExprType<T>()>;

	switch (op) {
	case BinaryOp::Add:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				(int)x->data + std::static_pointer_cast<LT>(y)->data);
		else
			return std::make_shared<LT>(
				x->data + std::static_pointer_cast<LT>(y)->data);
	case BinaryOp::Sub:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				(int)x->data - std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<LT>(
				x->data - std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::Mul:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				(int)x->data * std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<LT>(
				x->data * std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::Div:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				((int)x->data) / ((int)std::static_pointer_cast<LT>(y)->data));
		else if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<LT>(
				x->data / std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::Mod:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				((int)x->data) % ((int)std::static_pointer_cast<LT>(y)->data));
		else if constexpr (std::is_integral_v<T>)
			return std::make_shared<LT>(
				x->data % std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_same_v<T, float>)
			return std::make_shared<LT>(
				fmodf(x->data, std::static_pointer_cast<LT>(y)->data));
		else if constexpr (std::is_same_v<T, double>)
			return std::make_shared<LT>(
				fmod(x->data, std::static_pointer_cast<LT>(y)->data));
		else
			return {};
	case BinaryOp::And:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<BoolLiteralExprNode>(
				x->data && std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_integral_v<T>)
			return std::make_shared<LT>(
				x->data & std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_same_v<T, float>) {
			uint32_t tmp = (*(uint32_t *)&x->data) & (*(uint32_t *)&std::static_pointer_cast<LT>(y)->data);
			return std::make_shared<LT>(*((float *)&tmp));
		} else if constexpr (std::is_same_v<T, double>) {
			uint64_t tmp = (*(uint64_t *)&x->data) & (*(uint64_t *)&std::static_pointer_cast<LT>(y)->data);
			return std::make_shared<LT>(*((double *)&tmp));
		} else
			return {};
	case BinaryOp::Or:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<BoolLiteralExprNode>(
				x->data || std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_integral_v<T>)
			return std::make_shared<LT>(
				x->data | std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_same_v<T, float>) {
			uint32_t tmp = (*(uint32_t *)&x->data) | (*(uint32_t *)&std::static_pointer_cast<LT>(y)->data);
			return std::make_shared<LT>(*((float *)&tmp));
		} else if constexpr (std::is_same_v<T, double>) {
			uint64_t tmp = (*(uint64_t *)&x->data) | (*(uint64_t *)&std::static_pointer_cast<LT>(y)->data);
			return std::make_shared<LT>(*((double *)&tmp));
		} else
			return {};
	case BinaryOp::Xor:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				x->data ^ std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_integral_v<T>)
			return std::make_shared<LT>(
				x->data ^ std::static_pointer_cast<LT>(y)->data);
		else if constexpr (std::is_same_v<T, float>) {
			uint32_t tmp = (*(uint32_t *)&x->data) ^ (*(uint32_t *)&std::static_pointer_cast<LT>(y)->data);
			return std::make_shared<LT>(*((float *)&tmp));
		} else if constexpr (std::is_same_v<T, double>) {
			uint64_t tmp = (*(uint64_t *)&x->data) ^ (*(uint64_t *)&std::static_pointer_cast<LT>(y)->data);
			return std::make_shared<LT>(*((double *)&tmp));
		} else
			return {};
	case BinaryOp::LAnd:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>) {
			return std::make_shared<BoolLiteralExprNode>(
				x->data && std::static_pointer_cast<LT>(y)->data);
		} else
			return {};
	case BinaryOp::LOr:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_same_v<T, bool>) {
			return std::make_shared<BoolLiteralExprNode>(
				x->data || std::static_pointer_cast<LT>(y)->data);
		} else
			return {};
	case BinaryOp::Lsh:
		if (y->getExprType() != ExprType::I32)
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				(int)x->data << std::static_pointer_cast<I32LiteralExprNode>(y)->data);
		else if constexpr (std::is_integral_v<T>)
			return std::make_shared<LT>(
				x->data << std::static_pointer_cast<I32LiteralExprNode>(y)->data);
		else if constexpr (std::is_same_v<T, float>) {
			uint32_t tmp = (*(uint32_t *)&x->data) << (std::static_pointer_cast<I32LiteralExprNode>(y)->data);
			return std::make_shared<LT>(*((float *)&tmp));
		} else if constexpr (std::is_same_v<T, double>) {
			uint64_t tmp = (*(uint64_t *)&x->data) << (std::static_pointer_cast<I32LiteralExprNode>(y)->data);
			return std::make_shared<LT>(*((double *)&tmp));
		} else
			return {};
	case BinaryOp::Rsh:
		if (y->getExprType() != ExprType::I32)
			return {};
		if constexpr (std::is_same_v<T, bool>)
			return std::make_shared<I32LiteralExprNode>(
				(int)x->data >> std::static_pointer_cast<I32LiteralExprNode>(y)->data);
		else if constexpr (std::is_integral_v<T>)
			return std::make_shared<LT>(
				x->data >> std::static_pointer_cast<I32LiteralExprNode>(y)->data);
		else if constexpr (std::is_same_v<T, float>) {
			uint32_t tmp = (*(uint32_t *)&x->data) >> (std::static_pointer_cast<I32LiteralExprNode>(y)->data);
			return std::make_shared<LT>(*((float *)&tmp));
		} else if constexpr (std::is_same_v<T, double>) {
			uint64_t tmp = (*(uint64_t *)&x->data) >> (std::static_pointer_cast<I32LiteralExprNode>(y)->data);
			return std::make_shared<LT>(*((double *)&tmp));
		} else
			return {};
	case BinaryOp::Eq:
		if (y->getExprType() != x->getExprType())
			return {};
		return std::make_shared<BoolLiteralExprNode>(
			x->data == std::static_pointer_cast<LT>(y)->data);
	case BinaryOp::Neq:
		if (y->getExprType() != x->getExprType())
			return {};
		return std::make_shared<BoolLiteralExprNode>(
			x->data != std::static_pointer_cast<LT>(y)->data);
	case BinaryOp::Lt:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<BoolLiteralExprNode>(
				x->data < std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::Gt:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<BoolLiteralExprNode>(
				x->data > std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::LtEq:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<BoolLiteralExprNode>(
				x->data <= std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::GtEq:
		if (y->getExprType() != x->getExprType())
			return {};
		if constexpr (std::is_arithmetic_v<T>)
			return std::make_shared<BoolLiteralExprNode>(
				x->data >= std::static_pointer_cast<LT>(y)->data);
		else
			return {};
	case BinaryOp::Subscript:
		if (y->getExprType() != ExprType::I32)
			return {};
		if (std::static_pointer_cast<I32LiteralExprNode>(y)->data < 0)
			return {};

		if constexpr (std::is_same_v<T, std::string>) {
			return std::make_shared<I32LiteralExprNode>(
				x->data[std::static_pointer_cast<I32LiteralExprNode>(y)->data]);
		} else
			return {};
	default:
		return {};
	}
}

std::shared_ptr<ExprNode> Compiler::evalConstExpr(CompileContext *compileContext, std::shared_ptr<ExprNode> expr) {
	switch (expr->getExprType()) {
	case ExprType::I8:
	case ExprType::I16:
	case ExprType::I32:
	case ExprType::I64:
	case ExprType::U8:
	case ExprType::U16:
	case ExprType::U32:
	case ExprType::U64:
	case ExprType::F32:
	case ExprType::F64:
	case ExprType::String:
	case ExprType::Bool:
		return expr;
	case ExprType::Unary: {
		auto e = std::static_pointer_cast<UnaryOpExprNode>(expr);

		if (!e->x)
			return {};

		if (!evalConstExpr(compileContext, e->x))
			return {};

		switch (e->x->getExprType()) {
		case ExprType::I32:
			return _evalConstUnaryOpExpr<int32_t>(e->op, std::static_pointer_cast<I32LiteralExprNode>(e->x));
		case ExprType::I64:
			return _evalConstUnaryOpExpr<int64_t>(e->op, std::static_pointer_cast<I64LiteralExprNode>(e->x));
		case ExprType::U32:
			return _evalConstUnaryOpExpr<uint32_t>(e->op, std::static_pointer_cast<U32LiteralExprNode>(e->x));
		case ExprType::U64:
			return _evalConstUnaryOpExpr<uint64_t>(e->op, std::static_pointer_cast<U64LiteralExprNode>(e->x));
		case ExprType::F32:
			return _evalConstUnaryOpExpr<float>(e->op, std::static_pointer_cast<F32LiteralExprNode>(e->x));
		case ExprType::F64:
			return _evalConstUnaryOpExpr<double>(e->op, std::static_pointer_cast<F64LiteralExprNode>(e->x));
		case ExprType::String:
			return _evalConstUnaryOpExpr<std::string>(e->op, std::static_pointer_cast<StringLiteralExprNode>(e->x));
		case ExprType::Bool:
			return _evalConstUnaryOpExpr<bool>(e->op, std::static_pointer_cast<BoolLiteralExprNode>(e->x));
		default:
			return {};
		}
	}
	case ExprType::Binary: {
		auto e = std::static_pointer_cast<BinaryOpExprNode>(expr);

		if ((!e->lhs) || (!e->rhs))
			return {};

		auto lhs = evalConstExpr(compileContext, e->lhs), rhs = evalConstExpr(compileContext, e->rhs);

		if ((!lhs) || (!rhs))
			return {};

		switch (lhs->getExprType()) {
		case ExprType::I32:
			return _evalConstBinaryOpExpr<int32_t>(
				e->op,
				std::static_pointer_cast<I32LiteralExprNode>(lhs),
				rhs);
		case ExprType::I64:
			return _evalConstBinaryOpExpr<int64_t>(
				e->op,
				std::static_pointer_cast<I64LiteralExprNode>(lhs),
				rhs);
		case ExprType::U32:
			return _evalConstBinaryOpExpr<uint32_t>(
				e->op,
				std::static_pointer_cast<U32LiteralExprNode>(lhs),
				rhs);
		case ExprType::U64:
			return _evalConstBinaryOpExpr<uint64_t>(
				e->op,
				std::static_pointer_cast<U64LiteralExprNode>(lhs),
				rhs);
		case ExprType::F32:
			return _evalConstBinaryOpExpr<float>(
				e->op,
				std::static_pointer_cast<F32LiteralExprNode>(lhs),
				rhs);
		case ExprType::F64:
			return _evalConstBinaryOpExpr<double>(
				e->op,
				std::static_pointer_cast<F64LiteralExprNode>(lhs),
				rhs);
		case ExprType::String:
			return _evalConstBinaryOpExpr<std::string>(
				e->op,
				std::static_pointer_cast<StringLiteralExprNode>(lhs),
				rhs);
		case ExprType::Bool:
			return _evalConstBinaryOpExpr<bool>(
				e->op,
				std::static_pointer_cast<BoolLiteralExprNode>(lhs),
				rhs);
		default:
			return {};
		}
	}
	case ExprType::Array: {
		auto e = std::static_pointer_cast<ArrayExprNode>(expr);

		auto resultExpr = std::make_shared<ArrayExprNode>();

		if ((!compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType) ||
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType->getTypeId() != TypeId::Array)
			return {};

		auto et = std::static_pointer_cast<ArrayTypeNameNode>(compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType);

		for (auto &i : e->elements) {
			auto t = evalExprType(compileContext, i);

			if (!isSameType(compileContext, t, et->elementType)) {
				if (isTypeNamesConvertible(compileContext, t, et->elementType)) {
					auto element = evalConstExpr(
						compileContext,
						std::make_shared<CastExprNode>(
							et,
							i));

					if (!element)
						return {};

					resultExpr->elements.push_back(element);
				} else
					return {};
			} else {
				auto element = evalConstExpr(compileContext, i);
				if (!element)
					return {};

				resultExpr->elements.push_back(element);
			}
		}

		resultExpr->evaluatedElementType = et->elementType;

		return resultExpr;
	}
	case ExprType::IdRef: {
		auto e = std::static_pointer_cast<IdRefExprNode>(expr);

		return {};	// stub
	}
	case ExprType::Cast: {
		return {};	// stub
		auto e = std::static_pointer_cast<CastExprNode>(expr);
		if (!isLiteralTypeName(e->targetType))
			return {};

		switch (e->getExprType()) {
		case ExprType::I32: {
		}
		case ExprType::U32: {
		}
		case ExprType::U64: {
		}
		case ExprType::I64: {
		}
		case ExprType::F32: {
		}
		case ExprType::F64: {
		}
		case ExprType::Bool: {
		}
		default:;
		}
		return evalConstExpr(compileContext, e->target);
	}
	case ExprType::Match:
		// stub
	default:
		return {};
	}
}

std::shared_ptr<TypeNameNode> Compiler::evalExprType(CompileContext *compileContext, std::shared_ptr<ExprNode> expr) {
	std::shared_ptr<TypeNameNode> t;

	compileContext->pushMinorContext();

	compileContext->curCollectiveContext.curMajorContext.curMinorContext.dryRun = true;
	compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose = EvalPurpose::Stmt;
	compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = {};
	compileExpr(compileContext, expr);
	t = compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType;

	compileContext->popMinorContext();

	return t;
}
