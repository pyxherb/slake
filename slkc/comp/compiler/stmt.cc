#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileStmt(
	CompileContext *compileContext,
	const peff::SharedPtr<StmtNode> &stmt) {
	peff::SharedPtr<BlockCompileContext> blockCompileContext = compileContext->fnCompileContext.blockCompileContexts.back();

	switch (stmt->stmtKind) {
		case StmtKind::Expr: {
			peff::SharedPtr<ExprStmtNode> s = stmt.castTo<ExprStmtNode>();

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->expr, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
			break;
		}
		case StmtKind::VarDef: {
			peff::SharedPtr<VarDefStmtNode> s = stmt.castTo<VarDefStmtNode>();

			for (auto &i : s->varDefEntries) {
				if (blockCompileContext->localVars.contains(i->name)) {
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->initialValue->tokenRange, CompilationErrorKind::LocalVarAlreadyExists)));
				} else {
					peff::SharedPtr<VarNode> newVar;

					if (!(newVar = peff::makeShared<VarNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
						return genOutOfMemoryCompError();
					}

					if (!newVar->name.build(i->name)) {
						return genOutOfMemoryCompError();
					}

					newVar->type = i->type;

					slake::Type type;
					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, i->type, type));

					uint32_t localVarReg = compileContext->allocReg();

					SLKC_RETURN_IF_COMP_ERROR(
						compileContext->emitIns(
							slake::Opcode::LVAR,
							localVarReg,
							{ slake::Value(type) }));

					newVar->idxReg = localVarReg;

					if (!blockCompileContext->localVars.insert(newVar->name, std::move(newVar))) {
						return genOutOfMemoryCompError();
					}

					if (i->initialValue) {
						uint32_t initialValueReg = compileContext->allocReg();

						CompileExprResult result(compileContext->allocator.get());

						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, i->initialValue, ExprEvalPurpose::LValue, {}, initialValueReg, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, i->initialValue, ExprEvalPurpose::RValue, {}, initialValueReg, result));
						}

						SLKC_RETURN_IF_COMP_ERROR(
							compileContext->emitIns(
								slake::Opcode::STORE,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegRef, initialValueReg), slake::Value(slake::ValueType::RegRef, localVarReg) }));
					}
				}
			}
			break;
		}
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
