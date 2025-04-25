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
			if (compileContext->fnCompileContext.breakStmtJumpDestLabel == UINT32_MAX) {
				return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidBreakUsage);
			}
			if (compileContext->fnCompileContext.breakStmtBlockLevel > compileContext->fnCompileContext.blockCompileContexts.size()) {
				SLKC_RETURN_IF_COMP_ERROR(
					compileContext->emitIns(
						slake::Opcode::LEAVE,
						UINT32_MAX,
						{ slake::Value(compileContext->fnCompileContext.breakStmtBlockLevel - compileContext->fnCompileContext.blockCompileContexts.size()) }));
			}
			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, compileContext->fnCompileContext.breakStmtJumpDestLabel) }));
			break;
		case StmtKind::Continue:
			if (compileContext->fnCompileContext.continueStmtJumpDestLabel == UINT32_MAX) {
				return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidContinueUsage);
			}
			if (compileContext->fnCompileContext.continueStmtBlockLevel > compileContext->fnCompileContext.blockCompileContexts.size()) {
				SLKC_RETURN_IF_COMP_ERROR(
					compileContext->emitIns(
						slake::Opcode::LEAVE,
						UINT32_MAX,
						{ slake::Value(compileContext->fnCompileContext.continueStmtBlockLevel - compileContext->fnCompileContext.blockCompileContexts.size()) }));
			}
			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, compileContext->fnCompileContext.continueStmtJumpDestLabel) }));
			break;
		case StmtKind::For: {
			peff::SharedPtr<ForStmtNode> s = stmt.castTo<ForStmtNode>();

			PrevBreakPointHolder breakPointHolder(compileContext);
			PrevContinuePointHolder continuePointHolder(compileContext);

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::ENTER,
					UINT32_MAX,
					{}));
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushBlockContext());
			peff::ScopeGuard popBlockContextGuard([compileContext]() noexcept {
				compileContext->popBlockContext();
			});

			uint32_t bodyLabel;
			SLKC_RETURN_IF_COMP_ERROR(compileContext->allocLabel(bodyLabel));

			compileContext->fnCompileContext.breakStmtBlockLevel = compileContext->fnCompileContext.blockCompileContexts.size();
			SLKC_RETURN_IF_COMP_ERROR(compileContext->allocLabel(compileContext->fnCompileContext.breakStmtJumpDestLabel));
			compileContext->fnCompileContext.continueStmtBlockLevel = compileContext->fnCompileContext.blockCompileContexts.size();
			SLKC_RETURN_IF_COMP_ERROR(compileContext->allocLabel(compileContext->fnCompileContext.continueStmtJumpDestLabel));

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

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, compileContext->fnCompileContext.continueStmtJumpDestLabel) }));

			compileContext->getLabel(bodyLabel)->offset = compileContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, s->body));

			if (s->cond) {
				compileContext->getLabel(compileContext->fnCompileContext.continueStmtJumpDestLabel)->offset = compileContext->getCurInsOff();

				{
					CompileExprResult result(compileContext->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->cond, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
				}

				uint32_t conditionReg = compileContext->allocReg();

				CompileExprResult result(compileContext->allocator.get());

				peff::SharedPtr<TypeNameNode> tn;

				if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document).castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->cond, ExprEvalPurpose::RValue, tn, conditionReg, result));

				SLKC_RETURN_IF_COMP_ERROR(
					compileContext->emitIns(
						slake::Opcode::JT,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::Label, bodyLabel) }));
			} else {
				compileContext->getLabel(compileContext->fnCompileContext.continueStmtJumpDestLabel)->offset = compileContext->getCurInsOff();

				{
					CompileExprResult result(compileContext->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->cond, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
				}
			}

			compileContext->getLabel(compileContext->fnCompileContext.breakStmtJumpDestLabel)->offset = compileContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::LEAVE,
					UINT32_MAX,
					{ slake::Value((uint32_t)1) }));
			break;
		}
		case StmtKind::While: {
			peff::SharedPtr<WhileStmtNode> s = stmt.castTo<WhileStmtNode>();

			PrevBreakPointHolder breakPointHolder(compileContext);
			PrevContinuePointHolder continuePointHolder(compileContext);

			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushBlockContext());
			peff::ScopeGuard popBlockContextGuard([compileContext]() noexcept {
				compileContext->popBlockContext();
			});

			uint32_t bodyLabel;
			SLKC_RETURN_IF_COMP_ERROR(compileContext->allocLabel(bodyLabel));

			if (!s->isDoWhile) {
				SLKC_RETURN_IF_COMP_ERROR(
					compileContext->emitIns(
						slake::Opcode::JMP,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::Label, compileContext->fnCompileContext.continueStmtJumpDestLabel) }));
			}

			compileContext->getLabel(bodyLabel)->offset = compileContext->getCurInsOff();
			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, s->body));

			compileContext->getLabel(compileContext->fnCompileContext.continueStmtJumpDestLabel)->offset = compileContext->getCurInsOff();

			{
				CompileExprResult result(compileContext->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->cond, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
			}

			uint32_t conditionReg = compileContext->allocReg();

			CompileExprResult result(compileContext->allocator.get());

			peff::SharedPtr<TypeNameNode> tn;

			if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document).castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->cond, ExprEvalPurpose::RValue, tn, conditionReg, result));

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::JT,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, bodyLabel) }));

			compileContext->getLabel(compileContext->fnCompileContext.breakStmtJumpDestLabel)->offset = compileContext->getCurInsOff();
			break;
		}
		case StmtKind::Return: {
			peff::SharedPtr<ReturnStmtNode> s = stmt.castTo<ReturnStmtNode>();

			uint32_t reg = compileContext->allocReg();

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->value, ExprEvalPurpose::RValue, compileContext->fnCompileContext.currentFn->returnType, reg, result));

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::RET,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::RegRef, reg) }));
			break;
		}
		case StmtKind::Yield: {
			peff::SharedPtr<ReturnStmtNode> s = stmt.castTo<ReturnStmtNode>();

			uint32_t reg = compileContext->allocReg();

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->value, ExprEvalPurpose::RValue, compileContext->fnCompileContext.currentFn->returnType, reg, result));

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::YIELD,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::RegRef, reg) }));
			break;
		}
		case StmtKind::If: {
			peff::SharedPtr<IfStmtNode> s = stmt.castTo<IfStmtNode>();

			uint32_t reg = compileContext->allocReg();

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, s->cond, ExprEvalPurpose::RValue, compileContext->fnCompileContext.currentFn->returnType, reg, result));

			uint32_t endLabel, falseLabel;
			SLKC_RETURN_IF_COMP_ERROR(compileContext->allocLabel(endLabel));
			SLKC_RETURN_IF_COMP_ERROR(compileContext->allocLabel(falseLabel));

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::JF,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, falseLabel) }));

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, s->trueBody));

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, endLabel) }));

			compileContext->getLabel(falseLabel)->offset = compileContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, s->falseBody));

			compileContext->getLabel(endLabel)->offset = compileContext->getCurInsOff();
			break;
		}
		case StmtKind::With:
			break;
		case StmtKind::Switch:
			break;
		case StmtKind::CodeBlock: {
			peff::SharedPtr<CodeBlockStmtNode> s = stmt.castTo<CodeBlockStmtNode>();

			PrevBreakPointHolder breakPointHolder(compileContext);
			PrevContinuePointHolder continuePointHolder(compileContext);

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::ENTER,
					UINT32_MAX,
					{}));
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushBlockContext());
			peff::ScopeGuard popBlockContextGuard([compileContext]() noexcept {
				compileContext->popBlockContext();
			});

			for (size_t i = 0; i < s->body.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, s->body.at(i)));
			}

			SLKC_RETURN_IF_COMP_ERROR(
				compileContext->emitIns(
					slake::Opcode::LEAVE,
					UINT32_MAX,
					{ slake::Value((uint32_t)1) }));
			break;
		}
		case StmtKind::Goto:
			break;
		case StmtKind::Bad:
			break;
		default:
			std::terminate();
	}

	return {};
}
