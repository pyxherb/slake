#include "../comp/compiler.h"

using namespace slkc;

SLKC_API Document::Document(peff::Alloc *allocator): allocator(allocator), externalModuleProviders(allocator), genericCacheDir(allocator) {
}

SLKC_API Document::~Document() {
	_doClearDeferredDestructibleAstNodes();
}

SLKC_API void Document::_doClearDeferredDestructibleAstNodes() {
	AstNode *i, *next;

	while ((i = destructibleAstNodeList)) {
		destructibleAstNodeList = nullptr;

		while (i) {
			next = i->_nextDestructible;
			i->_destructor(i);
			i = next;
		};
	}
}

SLAKE_API GenericArgListCmp::GenericArgListCmp(Document *document, CompileEnvironment *compileEnv) : document(document), compileEnv(compileEnv) {
}

SLAKE_API GenericArgListCmp::GenericArgListCmp(const GenericArgListCmp &r): document(r.document), compileEnv(r.compileEnv) {
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

		if (l->getAstNodeType() < r->getAstNodeType())
			return -1;
		if (l->getAstNodeType() > r->getAstNodeType())
			return 1;
		switch (l->getAstNodeType()) {
			case AstNodeType::Expr: {
				NormalCompilationContext compilationContext(compileEnv.get(), nullptr);
				AstNodePtr<ExprNode> le, re;
				if (auto e = evalConstExpr(compileEnv.get(), &compilationContext, l.castTo<ExprNode>(), le); e) {
					storedError = std::move(e);
					return {};
				}
				if (!le) {
					storedError = CompilationError(l->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);
					return {};
				}
				if (auto e = evalConstExpr(compileEnv.get(), &compilationContext, r.castTo<ExprNode>(), re); e) {
					storedError = std::move(e);
					return {};
				}
				if (!re) {
					storedError = CompilationError(l->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);
					return {};
				}
				if (le->exprKind < re->exprKind)
					return -1;
				if (le->exprKind > re->exprKind)
					return 1;
				switch (le->exprKind) {
					case ExprKind::I8: {
						int8_t ld = le.castTo<I8LiteralExprNode>()->data, rd = re.castTo<I8LiteralExprNode>()->data;
						if(ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::I16: {
						int16_t ld = le.castTo<I16LiteralExprNode>()->data, rd = re.castTo<I16LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::I32: {
						int32_t ld = le.castTo<I32LiteralExprNode>()->data, rd = re.castTo<I32LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::I64: {
						int64_t ld = le.castTo<I64LiteralExprNode>()->data, rd = re.castTo<I64LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U8: {
						uint8_t ld = le.castTo<U8LiteralExprNode>()->data, rd = re.castTo<U8LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U16: {
						uint16_t ld = le.castTo<U16LiteralExprNode>()->data, rd = re.castTo<U16LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U32: {
						uint32_t ld = le.castTo<U32LiteralExprNode>()->data, rd = re.castTo<U32LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::U64: {
						uint64_t ld = le.castTo<U64LiteralExprNode>()->data, rd = re.castTo<U64LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::F32: {
						float ld = le.castTo<F32LiteralExprNode>()->data, rd = re.castTo<F32LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::F64: {
						double ld = le.castTo<F64LiteralExprNode>()->data, rd = re.castTo<F64LiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::Bool: {
						int ld = le.castTo<BoolLiteralExprNode>()->data, rd = re.castTo<BoolLiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::String: {
						std::string_view ld = le.castTo<StringLiteralExprNode>()->data, rd = re.castTo<StringLiteralExprNode>()->data;
						if (ld < rd)
							return -1;
						if (ld > rd)
							return 1;
						return 0;
					}
					case ExprKind::Null:
						return 0;
					default:
						storedError = CompilationError(l->tokenRange, CompilationErrorKind::RequiresCompTimeExpr);
						return {};
				}
				break;
			}
			case AstNodeType::TypeName: {
				int result;
				if (auto e = typeNameCmp(l.castTo<TypeNameNode>(), r.castTo<TypeNameNode>(), result); e) {
					storedError = std::move(e);
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
