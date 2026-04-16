#include "../compiler.h"
#include <slake/flib/bitop.h>
#include <slake/flib/math/fmod.h>

using namespace slkc;

template <typename T, typename E, typename SetData = typename E::SetData>
peff::Option<CompilationError> _do_simple_int_literal_cast(
	CompileEnv *compile_env,
	AstNodePtr<ExprNode> src,
	AstNodePtr<E> &expr_out) {
	static SetData _set_data;

	switch (src->expr_kind) {
		case ExprKind::I8: {
			AstNodePtr<I8LiteralExprNode> l = src.cast_to<I8LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::I16: {
			AstNodePtr<I16LiteralExprNode> l = src.cast_to<I16LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::I32: {
			AstNodePtr<I32LiteralExprNode> l = src.cast_to<I32LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::I64: {
			AstNodePtr<I64LiteralExprNode> l = src.cast_to<I64LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::U8: {
			AstNodePtr<U8LiteralExprNode> l = src.cast_to<U8LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::U16: {
			AstNodePtr<U16LiteralExprNode> l = src.cast_to<U16LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::U32: {
			AstNodePtr<U32LiteralExprNode> l = src.cast_to<U32LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::U64: {
			AstNodePtr<U64LiteralExprNode> l = src.cast_to<U64LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::F32: {
			AstNodePtr<F32LiteralExprNode> l = src.cast_to<F32LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::F64: {
			AstNodePtr<F64LiteralExprNode> l = src.cast_to<F64LiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		case ExprKind::Bool: {
			AstNodePtr<BoolLiteralExprNode> l = src.cast_to<BoolLiteralExprNode>();

			_set_data(expr_out, (T)l->data);
			break;
		}
		default:
			expr_out = {};
			break;
	}

	return {};
}

peff::Option<CompilationError> _cast_const_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<TypeNameNode> type,
	AstNodePtr<ExprNode> &expr_out) {
	AstNodePtr<CastExprNode> cast_expr;

	if (!(cast_expr = make_ast_node<CastExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document()))) {
		return gen_oom_comp_error();
	}

	cast_expr->source = expr;
	cast_expr->target_type = type;

	bool side_effect_applied;

	SLKC_RETURN_IF_COMP_ERROR(_do_eval_const_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), expr_out));

	return {};
}

static peff::Option<CompilationError> eval_binary_op_expr_operand(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ExprNode> operand_in,
	AstNodePtr<ExprNode> &operand_out) {
	SLKC_RETURN_IF_COMP_ERROR(_do_eval_const_expr(compile_env, compilation_context, path_env, operand_in, operand_out));
	if (!operand_out) {
		return {};
	}
	return {};
}

template <typename LT>
static peff::Option<CompilationError> eval_integral_binary_op_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<TypeNameNode> main_operation_type,
	BinaryOp binary_op,
	AstNodePtr<ExprNode> lhs_in,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<ExprNode> rhs_in,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<ExprNode> &expr_out) {
	AstNodePtr<ExprNode> lhs;
	AstNodePtr<ExprNode> rhs;

	switch (binary_op) {
		case BinaryOp::LAnd:
		case BinaryOp::LOr:
			break;
		default:
			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				lhs_in, lhs));
			if (!lhs) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				rhs_in, rhs));
			if (!rhs) {
				expr_out = {};
				return {};
			}
	}

	switch (binary_op) {
		case BinaryOp::Add: {
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data + rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Sub: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data - rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Mul: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data * rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Div: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data / rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Mod: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data % rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::And: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data & rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Or: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data | rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Xor: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data ^ rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::LAnd: {
			AstNodePtr<BoolTypeNameNode> bool_type;
			if (!(bool_type = make_ast_node<BoolTypeNameNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document())))
				return gen_oom_comp_error();

			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				lhs_in, lhs));
			if (!lhs_in) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, bool_type.cast_to<TypeNameNode>(), lhs));
			if (!lhs) {
				expr_out = {};
				return {};
			}
			auto l = lhs.cast_to<BoolLiteralExprNode>();
			if (!l->data) {
				expr_out = lhs;
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				rhs_in, rhs));
			if (!rhs_in) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, bool_type.cast_to<TypeNameNode>(), rhs));
			if (!rhs) {
				expr_out = {};
				return {};
			}
			if (rhs_type->is_nullable) {
				expr_out = {};
				return {};
			}
			expr_out = rhs;
			break;
		}
		case BinaryOp::LOr: {
			AstNodePtr<BoolTypeNameNode> bool_type;
			if (!(bool_type = make_ast_node<BoolTypeNameNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document())))
				return gen_oom_comp_error();

			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				lhs_in, lhs));
			if (!lhs_in) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, bool_type.cast_to<TypeNameNode>(), lhs));
			if (!lhs) {
				expr_out = {};
				return {};
			}
			auto l = lhs.cast_to<BoolLiteralExprNode>();
			if (l->data) {
				expr_out = lhs;
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				rhs_in, rhs));
			if (!rhs_in) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, bool_type.cast_to<TypeNameNode>(), rhs));
			if (!rhs) {
				expr_out = {};
				return {};
			}
			if (rhs_type->is_nullable) {
				expr_out = {};
				return {};
			}
			expr_out = rhs;
			break;
		}
		case BinaryOp::Shl: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			AstNodePtr<U32TypeNameNode> u32_type;

			if (!(u32_type = make_ast_node<U32TypeNameNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document())))
				return gen_oom_comp_error();
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, u32_type.cast_to<TypeNameNode>(), rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			// TODO: Use a portable one.
			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<BoolLiteralExprNode>()->data << rhs.cast_to<U32LiteralExprNode>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Shr: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			AstNodePtr<U32TypeNameNode> u32_type;

			if (!(u32_type = make_ast_node<U32TypeNameNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document())))
				return gen_oom_comp_error();
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, u32_type.cast_to<TypeNameNode>(), rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			// TODO: Use a portable one.
			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<BoolLiteralExprNode>()->data >> rhs.cast_to<U32LiteralExprNode>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Eq:
		case BinaryOp::StrictEq: {
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (lhs_type->is_nullable) {
				if (rhs->expr_kind == ExprKind::Null) {
					if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
							  compile_env->allocator.get(),
							  compile_env->allocator.get(), compile_env->get_document(),
							  lhs->expr_kind == ExprKind::Null)
								.template cast_to<ExprNode>()))
						return gen_oom_comp_error();
				} else {
					if (lhs->expr_kind == ExprKind::Null) {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  rhs->expr_kind == ExprKind::Null)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					} else {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  lhs.cast_to<LT>()->data == rhs.cast_to<LT>()->data)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					}
				}
			} else {
				if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(), compile_env->get_document(),
						  lhs.cast_to<LT>()->data == rhs.cast_to<LT>()->data)
							.template cast_to<ExprNode>()))
					return gen_oom_comp_error();
			}
			break;
		}
		case BinaryOp::Neq:
		case BinaryOp::StrictNeq: {
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (lhs_type->is_nullable) {
				if (rhs->expr_kind == ExprKind::Null) {
					if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
							  compile_env->allocator.get(),
							  compile_env->allocator.get(), compile_env->get_document(),
							  lhs->expr_kind != ExprKind::Null)
								.template cast_to<ExprNode>()))
						return gen_oom_comp_error();
				} else {
					if (lhs->expr_kind == ExprKind::Null) {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  rhs->expr_kind != ExprKind::Null)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					} else {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  lhs.cast_to<LT>()->data != rhs.cast_to<LT>()->data)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					}
				}
			} else {
				if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(), compile_env->get_document(),
						  lhs.cast_to<LT>()->data != rhs.cast_to<LT>()->data)
							.template cast_to<ExprNode>()))
					return gen_oom_comp_error();
			}
			break;
		}
		case BinaryOp::Gt: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data > rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Lt: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data < rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::GtEq: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data >= rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::LtEq: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data <= rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		// TODO: Implement comparison operation.
		default:
			expr_out = {};
			break;
	}

	return {};
}

template <typename LT>
static peff::Option<CompilationError> eval_floating_point_binary_op_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<TypeNameNode> main_operation_type,
	BinaryOp binary_op,
	AstNodePtr<ExprNode> lhs_in,
	AstNodePtr<TypeNameNode> lhs_type,
	AstNodePtr<ExprNode> rhs_in,
	AstNodePtr<TypeNameNode> rhs_type,
	AstNodePtr<ExprNode> &expr_out) {
	AstNodePtr<ExprNode> lhs;
	AstNodePtr<ExprNode> rhs;

	switch (binary_op) {
		case BinaryOp::LAnd:
		case BinaryOp::LOr:
			break;
		default:
			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				lhs_in, lhs));
			if (!lhs) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(eval_binary_op_expr_operand(
				compile_env,
				compilation_context,
				path_env,
				rhs_in, rhs));
			if (!rhs) {
				expr_out = {};
				return {};
			}
	}

	switch (binary_op) {
		case BinaryOp::Add: {
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data + rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Sub: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data - rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Mul: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data * rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Div: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<LT>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data / rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Mod: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if constexpr (std::is_same_v<decltype(LT::data), float>) {
				if (!(expr_out = make_ast_node<LT>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(), compile_env->get_document(),
						  slake::flib::fmodf(lhs.cast_to<LT>()->data, rhs.cast_to<LT>()->data))
							.template cast_to<ExprNode>()))
					return gen_oom_comp_error();
			} else {
				if (!(expr_out = make_ast_node<LT>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(), compile_env->get_document(),
						  slake::flib::fmod(lhs.cast_to<LT>()->data, rhs.cast_to<LT>()->data))
							.template cast_to<ExprNode>()))
					return gen_oom_comp_error();
			}
			break;
		}
		case BinaryOp::Eq:
		case BinaryOp::StrictEq: {
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (lhs_type->is_nullable) {
				if (rhs->expr_kind == ExprKind::Null) {
					if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
							  compile_env->allocator.get(),
							  compile_env->allocator.get(), compile_env->get_document(),
							  lhs->expr_kind == ExprKind::Null)
								.template cast_to<ExprNode>()))
						return gen_oom_comp_error();
				} else {
					if (lhs->expr_kind == ExprKind::Null) {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  rhs->expr_kind == ExprKind::Null)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					} else {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  lhs.cast_to<LT>()->data == rhs.cast_to<LT>()->data)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					}
				}
			} else {
				if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(), compile_env->get_document(),
						  lhs.cast_to<LT>()->data == rhs.cast_to<LT>()->data)
							.template cast_to<ExprNode>()))
					return gen_oom_comp_error();
			}
			break;
		}
		case BinaryOp::Neq:
		case BinaryOp::StrictNeq: {
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (lhs_type->is_nullable) {
				if (rhs->expr_kind == ExprKind::Null) {
					if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
							  compile_env->allocator.get(),
							  compile_env->allocator.get(), compile_env->get_document(),
							  lhs->expr_kind != ExprKind::Null)
								.template cast_to<ExprNode>()))
						return gen_oom_comp_error();
				} else {
					if (lhs->expr_kind == ExprKind::Null) {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  rhs->expr_kind != ExprKind::Null)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					} else {
						if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
								  compile_env->allocator.get(),
								  compile_env->allocator.get(), compile_env->get_document(),
								  lhs.cast_to<LT>()->data != rhs.cast_to<LT>()->data)
									.template cast_to<ExprNode>()))
							return gen_oom_comp_error();
					}
				}
			} else {
				if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
						  compile_env->allocator.get(),
						  compile_env->allocator.get(), compile_env->get_document(),
						  lhs.cast_to<LT>()->data != rhs.cast_to<LT>()->data)
							.template cast_to<ExprNode>()))
					return gen_oom_comp_error();
			}
			break;
		}
		case BinaryOp::Gt: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data > rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::Lt: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data < rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::GtEq: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data >= rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		case BinaryOp::LtEq: {
			if ((lhs_type->is_nullable) || (rhs_type->is_nullable)) {
				expr_out = {};
				return {};
			}
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, lhs, main_operation_type, lhs));
			SLKC_RETURN_IF_COMP_ERROR(_cast_const_expr(compile_env, compilation_context, path_env, rhs, main_operation_type, rhs));

			if ((!lhs) || (!rhs)) {
				expr_out = {};
				return {};
			}

			if (!(expr_out = make_ast_node<BoolLiteralExprNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(), compile_env->get_document(),
					  lhs.cast_to<LT>()->data <= rhs.cast_to<LT>()->data)
						.template cast_to<ExprNode>()))
				return gen_oom_comp_error();
			break;
		}
		// TODO: Implement comparison operation.
		default:
			expr_out = {};
			break;
	}

	return {};
}

SLAKE_API peff::Option<CompilationError> slkc::_do_eval_const_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<ExprNode> &expr_out) {
reeval:

	switch (expr->expr_kind) {
		case ExprKind::Unary: {
			// stub
			expr_out = {};
			break;
		}
		case ExprKind::Binary: {
			AstNodePtr<BinaryExprNode> e = expr.cast_to<BinaryExprNode>();

			AstNodePtr<TypeNameNode> lhs_type, rhs_type, decayed_lhs_type, decayed_rhs_type;
			SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, compilation_context, path_env, e->lhs, lhs_type, {}));
			SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, compilation_context, path_env, e->rhs, rhs_type, {}));

			SLKC_RETURN_IF_COMP_ERROR(
				remove_ref_of_type(lhs_type, decayed_lhs_type));
			SLKC_RETURN_IF_COMP_ERROR(
				remove_ref_of_type(rhs_type, decayed_rhs_type));

			AstNodePtr<TypeNameNode> main_operation_type;

			switch (e->binary_op) {
				case BinaryOp::Assign:
				case BinaryOp::Shl:
				case BinaryOp::Shr:
				case BinaryOp::ShlAssign:
				case BinaryOp::ShrAssign:
				case BinaryOp::Subscript:
					main_operation_type = decayed_lhs_type;
					break;
				default: {
					// If the both conversions of sides are viable, choose their common type.
					// If only one side is viable, choose the viable side.
					// Or else, choose the left side to generate the error message.
					bool lhs_to_rhs_viability, rhs_to_lhs_viability;
					SLKC_RETURN_IF_COMP_ERROR(is_convertible(decayed_lhs_type, decayed_rhs_type, true, lhs_to_rhs_viability));
					SLKC_RETURN_IF_COMP_ERROR(is_convertible(decayed_rhs_type, decayed_lhs_type, true, rhs_to_lhs_viability));
					if (lhs_to_rhs_viability && rhs_to_lhs_viability) {
						SLKC_RETURN_IF_COMP_ERROR(infer_common_type(decayed_lhs_type, decayed_rhs_type, main_operation_type));
					}
					if (!main_operation_type) {
						if (lhs_to_rhs_viability)
							main_operation_type = decayed_rhs_type;
						else
							main_operation_type = decayed_lhs_type;
					}
				}
			}

			switch (main_operation_type->tn_kind) {
				case TypeNameKind::I8:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<I8LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::I16:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<I16LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::I32:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<I32LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::I64:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<I64LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::U8:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<U8LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::U16:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<U16LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::U32:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<U32LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::U64:
					SLKC_RETURN_IF_COMP_ERROR(eval_integral_binary_op_expr<U64LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::F32:
					SLKC_RETURN_IF_COMP_ERROR(eval_floating_point_binary_op_expr<F32LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				case TypeNameKind::F64:
					SLKC_RETURN_IF_COMP_ERROR(eval_floating_point_binary_op_expr<F64LiteralExprNode>(
						compile_env, compilation_context, path_env,
						main_operation_type,
						e->binary_op,
						e->lhs, decayed_lhs_type,
						e->rhs, decayed_rhs_type,
						expr_out));
					break;
				default:
					expr_out = {};
			}

			break;
		}
		case ExprKind::Ternary: {
			expr_out = {};
			break;
		}
		case ExprKind::IdRef:
			break;
		case ExprKind::I8:
		case ExprKind::I16:
		case ExprKind::I32:
		case ExprKind::I64:
		case ExprKind::U8:
		case ExprKind::U16:
		case ExprKind::U32:
		case ExprKind::U64:
		case ExprKind::F32:
		case ExprKind::F64:
		case ExprKind::String:
		case ExprKind::Bool:
		case ExprKind::Null: {
			expr_out = expr;
			break;
		}
		case ExprKind::Cast: {
			AstNodePtr<CastExprNode> e = expr.cast_to<CastExprNode>();
			AstNodePtr<ExprNode> src;
			SLKC_RETURN_IF_COMP_ERROR(_do_eval_const_expr(compile_env, compilation_context, path_env, e->source, src));

			if (!src) {
				expr_out = {};
				break;
			}

			// The type may be nullable, but the value will still be non-nullable.
			switch (e->target_type->tn_kind) {
				case TypeNameKind::I8: {
					AstNodePtr<I8LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<I8LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int8_t, I8LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}

					break;
				}
				case TypeNameKind::I16: {
					AstNodePtr<I16LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<I16LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int16_t, I16LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}

					break;
				}
				case TypeNameKind::I32: {
					AstNodePtr<I32LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<I32LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int32_t, I32LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::I64: {
					AstNodePtr<I64LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<I64LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int64_t, I64LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U8: {
					AstNodePtr<U8LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<U8LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int8_t, U8LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U16: {
					AstNodePtr<U16LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<U16LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int16_t, U16LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U32: {
					AstNodePtr<U32LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<U32LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int32_t, U32LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::U64: {
					AstNodePtr<U64LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<U64LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<int64_t, U64LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::F32: {
					AstNodePtr<F32LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<F32LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<float, F32LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::F64: {
					AstNodePtr<F64LiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<F64LiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<double, F64LiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::Bool: {
					AstNodePtr<BoolLiteralExprNode> l;

					if ((e->target_type->is_nullable) && (e->source->expr_kind == ExprKind::Null)) {
						expr_out = e->source;
					} else {
						if (!(l = make_ast_node<BoolLiteralExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document(), 0))) {
							return gen_oom_comp_error();
						}
						SLKC_RETURN_IF_COMP_ERROR(_do_simple_int_literal_cast<double, BoolLiteralExprNode>(compile_env, src, l));
						expr_out = l.cast_to<ExprNode>();
					}
					break;
				}
				case TypeNameKind::String: {
					switch (expr->expr_kind) {
						case ExprKind::String:
							expr_out = expr;
							break;
						default:
							break;
					}
					break;
				}
				case TypeNameKind::Object: {
					switch (expr->expr_kind) {
						case ExprKind::Null:
							expr_out = expr;
							break;
						default:
							break;
					}
					break;
				}
				case TypeNameKind::Custom: {
					switch (expr->expr_kind) {
						case ExprKind::Null:
							expr_out = expr;
							break;
						default:
							break;
					}
					break;
				}
				case TypeNameKind::Array: {
					switch (expr->expr_kind) {
						case ExprKind::InitializerList:
						case ExprKind::Null:
							expr_out = expr;
							break;
						default:
							break;
					}
					break;
				}
				default:
					expr_out = {};
			}

			break;
		}
		case ExprKind::Wrapper:
			expr = expr.cast_to<WrapperExprNode>()->target;
			goto reeval;
		case ExprKind::Call:
		case ExprKind::Bad: {
			expr_out = {};
			break;
		}
		default:
			std::terminate();
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::eval_const_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	AstNodePtr<ExprNode> expr,
	AstNodePtr<ExprNode> &expr_out) {
	PathEnv pe(compile_env->allocator.get());
	pe.parent = path_env;

	return _do_eval_const_expr(compile_env, compilation_context, &pe, expr, expr_out);
}
