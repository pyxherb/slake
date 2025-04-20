#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileStmt(
	CompileContext* compileContext,
	const peff::SharedPtr<StmtNode>& stmt) {
	switch (stmt->stmtKind) {
		case StmtKind::Expr: {
			peff::SharedPtr<ExprStmtNode> s = stmt.castTo<ExprStmtNode>();

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->expr, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
			break;
		}
		case StmtKind::VarDef:
			break;
		case StmtKind::Break:
			break;
		case StmtKind::Continue:
			break;
		case StmtKind::For:
			break;
		case StmtKind::While:
			break;
		case StmtKind::Return:
			break;
		case StmtKind::Yield:
			break;
		case StmtKind::If:
			break;
		case StmtKind::Switch:
			break;
		case StmtKind::CodeBlock:
			break;
		case StmtKind::Goto:
			break;
		case StmtKind::Bad:
			break;
		default:
			std::terminate();
	}

	return {};
}
