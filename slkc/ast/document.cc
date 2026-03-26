#include "../comp/compiler.h"

using namespace slkc;

SLKC_API Document::Document(peff::Alloc *allocator) : allocator(allocator), external_module_providers(allocator), generic_cache_dir(allocator) {
}

SLKC_API Document::~Document() {
	_do_clear_deferred_destructible_ast_nodes();
}

SLKC_API void Document::_do_clear_deferred_destructible_ast_nodes() {
	AstNode *i, *next;

	while ((i = destructible_ast_node_list)) {
		destructible_ast_node_list = nullptr;

		while (i) {
			next = i->_next_destructible;
			i->_destructor(i);
			i = next;
		};
	}
}

SLAKE_API GenericArgListCmp::GenericArgListCmp(Document *document, CompileEnv *compile_env) : document(document), compile_env(compile_env) {
}

SLAKE_API GenericArgListCmp::GenericArgListCmp(const GenericArgListCmp &r) : document(r.document), compile_env(r.compile_env) {
}

SLAKE_API GenericArgListCmp::GenericArgListCmp::~GenericArgListCmp() {
}

SLAKE_API peff::Option<int> GenericArgListCmp::operator()(const peff::DynArray<AstNodePtr<AstNode>> &lhs, const peff::DynArray<AstNodePtr<AstNode>> &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;
	for (size_t i = 0; i < lhs.size(); ++i) {
		AstNodePtr<AstNode> l = lhs.at(i), r = rhs.at(i);

		if (l->get_ast_node_type() < r->get_ast_node_type())
			return -1;
		if (l->get_ast_node_type() > r->get_ast_node_type())
			return 1;
		switch (l->get_ast_node_type()) {
			case AstNodeType::Expr: {
				NormalCompilationContext compilation_context(compile_env.get(), nullptr);
				AstNodePtr<ExprNode> le, re;
				{
					PathEnv path_env(compile_env->allocator.get());
					if (auto e = eval_const_expr(compile_env.get(), &compilation_context, &path_env, l.cast_to<ExprNode>(), le); e) {
						stored_error = std::move(e);
						return {};
					}
				}
				if (!le) {
					stored_error = CompilationError(l->token_range, CompilationErrorKind::RequiresCompTimeExpr);
					return {};
				}
				{
					PathEnv path_env(compile_env->allocator.get());
					if (auto e = eval_const_expr(compile_env.get(), &compilation_context, &path_env, r.cast_to<ExprNode>(), re); e) {
						stored_error = std::move(e);
						return {};
					}
				}
				if (!re) {
					stored_error = CompilationError(l->token_range, CompilationErrorKind::RequiresCompTimeExpr);
					return {};
				}
				if (le->expr_kind < re->expr_kind)
					return -1;
				if (le->expr_kind > re->expr_kind)
					return 1;
				switch (le->expr_kind) {
					case ExprKind::I8: {
						int8_t ld = le.cast_to<I8LiteralExprNode>()->data, rd = re.cast_to<I8LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::I16: {
						int16_t ld = le.cast_to<I16LiteralExprNode>()->data, rd = re.cast_to<I16LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::I32: {
						int32_t ld = le.cast_to<I32LiteralExprNode>()->data, rd = re.cast_to<I32LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::I64: {
						int64_t ld = le.cast_to<I64LiteralExprNode>()->data, rd = re.cast_to<I64LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U8: {
						uint8_t ld = le.cast_to<U8LiteralExprNode>()->data, rd = re.cast_to<U8LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U16: {
						uint16_t ld = le.cast_to<U16LiteralExprNode>()->data, rd = re.cast_to<U16LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U32: {
						uint32_t ld = le.cast_to<U32LiteralExprNode>()->data, rd = re.cast_to<U32LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U64: {
						uint64_t ld = le.cast_to<U64LiteralExprNode>()->data, rd = re.cast_to<U64LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::F32: {
						float ld = le.cast_to<F32LiteralExprNode>()->data, rd = re.cast_to<F32LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::F64: {
						double ld = le.cast_to<F64LiteralExprNode>()->data, rd = re.cast_to<F64LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::Bool: {
						int ld = le.cast_to<BoolLiteralExprNode>()->data, rd = re.cast_to<BoolLiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::String: {
						std::string_view ld = le.cast_to<StringLiteralExprNode>()->data, rd = re.cast_to<StringLiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::Null:
						return 0;
					default:
						stored_error = CompilationError(l->token_range, CompilationErrorKind::RequiresCompTimeExpr);
						return {};
				}
				break;
			}
			case AstNodeType::TypeName: {
				int result;
				if (auto e = type_name_cmp(l.cast_to<TypeNameNode>(), r.cast_to<TypeNameNode>(), result); e) {
					stored_error = std::move(e);
					return {};
				}
				if (result)
					return result;
				return 0;
			}
			default:
				std::terminate();
		}
	}

	std::terminate();
}
