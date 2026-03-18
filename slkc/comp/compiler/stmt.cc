#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::compileVarDefStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<VarDefStmtNode> s,
	uint32_t sldIndex) {
	for (auto &i : s->varDefEntries) {
		if (compilationContext->getLocalVarInCurLevel(i->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->initialValue->tokenRange, CompilationErrorKind::LocalVarAlreadyExists)));
		} else {
			AstNodePtr<VarNode> newVar;

			uint32_t localVarReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(localVarReg));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLocalVar(s->tokenRange, i->name, localVarReg, i->type, newVar));

			if (i->initialValue) {
				uint32_t initialValueReg;

				CompileExprResult result(compileEnv->allocator.get());

				if (i->type) {
					newVar->type = i->type;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, newVar->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								sldIndex, slake::Opcode::LVAR,
								localVarReg,
								{ slake::Value(type) }));
					}

					AstNodePtr<TypeNameNode> exprType;

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, i->initialValue, exprType));

					bool same;

					SLKC_RETURN_IF_COMP_ERROR(isSameType(i->type, exprType, same));

					if (!same) {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, pathEnv, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initialValue, exprType, result));
						initialValueReg = result.idxResultRegOut;
					} else {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::LValue, i->type, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::RValue, i->type, result));
						}
						initialValueReg = result.idxResultRegOut;
					}
				} else {
					AstNodePtr<TypeNameNode> deducedType;

					newVar->isTypeDeducedFromInitialValue = true;

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, i->initialValue, deducedType));

					if (!deducedType) {
						return CompilationError(s->tokenRange, CompilationErrorKind::ErrorDeducingVarType);
					}

					newVar->type = deducedType;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, newVar->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								sldIndex, slake::Opcode::LVAR,
								localVarReg,
								{ slake::Value(type) }));
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(deducedType, b));

					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::LValue, {}, result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::RValue, {}, result));
					}
					initialValueReg = result.idxResultRegOut;
				}

				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						sldIndex, slake::Opcode::STORE,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::RegIndex, localVarReg), slake::Value(slake::ValueType::RegIndex, initialValueReg) }));
			} else {
				if (!i->type) {
					return CompilationError(s->tokenRange, CompilationErrorKind::RequiresInitialValue);
				}

				newVar->type = i->type;

				{
					slake::TypeRef type;
					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, newVar->type, type));

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							sldIndex, slake::Opcode::LVAR,
							localVarReg,
							{ slake::Value(type) }));
				}
			}
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileForStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ForStmtNode> s,
	uint32_t sldIndex) {
	AstNodePtr<BoolTypeNameNode> boolType;

	if (!(boolType = makeAstNode<BoolTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	PrevBreakPointHolder breakPointHolder(compilationContext);
	PrevContinuePointHolder continuePointHolder(compilationContext);

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::ENTER,
			UINT32_MAX,
			{}));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
	peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
		compilationContext->leaveBlock();
	});

	uint32_t bodyLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(bodyLabel));

	uint32_t normalExitLabel, breakLabel, condLabel, stepLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(normalExitLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
	compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(condLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(stepLabel));
	compilationContext->setContinueLabel(stepLabel, compilationContext->getBlockLevel());

	for (auto &i : s->varDefEntries) {
		if (compilationContext->getLocalVarInCurLevel(i->name)) {
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushError(CompilationError(i->initialValue->tokenRange, CompilationErrorKind::LocalVarAlreadyExists)));
		} else {
			AstNodePtr<VarNode> newVar;

			uint32_t localVarReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(localVarReg));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLocalVar(s->tokenRange, i->name, localVarReg, i->type, newVar));

			if (i->initialValue) {
				uint32_t initialValueReg;

				CompileExprResult result(compileEnv->allocator.get());

				if (i->type) {
					newVar->type = i->type;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, newVar->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								sldIndex, slake::Opcode::LVAR,
								localVarReg,
								{ slake::Value(type) }));
					}

					AstNodePtr<TypeNameNode> exprType;

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, i->initialValue, exprType));

					bool same;

					SLKC_RETURN_IF_COMP_ERROR(isSameType(i->type, exprType, same));

					if (!same) {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, pathEnv, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initialValue, exprType, result));
						initialValueReg = result.idxResultRegOut;
					} else {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::LValue, i->type, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::RValue, i->type, result));
						}
						initialValueReg = result.idxResultRegOut;
					}
				} else {
					AstNodePtr<TypeNameNode> deducedType;

					newVar->isTypeDeducedFromInitialValue = true;

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, i->initialValue, deducedType));

					if (!deducedType) {
						return CompilationError(s->tokenRange, CompilationErrorKind::ErrorDeducingVarType);
					}

					newVar->type = deducedType;

					{
						slake::TypeRef type;
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, newVar->type, type));

						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								sldIndex, slake::Opcode::LVAR,
								localVarReg,
								{ slake::Value(type) }));
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(deducedType, b));

					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::LValue, {}, result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, i->initialValue, ExprEvalPurpose::RValue, {}, result));
					}
					initialValueReg = result.idxResultRegOut;
				}

				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						sldIndex, slake::Opcode::STORE,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::RegIndex, localVarReg), slake::Value(slake::ValueType::RegIndex, initialValueReg) }));
			} else {
				if (!i->type) {
					return CompilationError(s->tokenRange, CompilationErrorKind::RequiresInitialValue);
				}

				newVar->type = i->type;

				{
					slake::TypeRef type;
					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, newVar->type, type));

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							sldIndex, slake::Opcode::LVAR,
							localVarReg,
							{ slake::Value(type) }));
				}
			}
		}
	}

	if (s->cond) {
		CompileExprResult result(compileEnv->allocator.get());

		uint32_t conditionReg;

		bool isSame;

		AstNodePtr<TypeNameNode> exprType;

		SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, s->cond, exprType, boolType.castTo<TypeNameNode>()));

		SLKC_RETURN_IF_COMP_ERROR(isSameType(boolType.castTo<TypeNameNode>(), exprType, isSame));

		AstNodePtr<ExprNode> constCondExpr;

		if (!isSame) {
			AstNodePtr<CastExprNode> castExpr;

			SLKC_RETURN_IF_COMP_ERROR(genImplicitCastExpr(
				compileEnv,
				s->cond,
				boolType.castTo<TypeNameNode>(),
				castExpr));

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, castExpr.castTo<ExprNode>(), constCondExpr));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, s->cond, constCondExpr));
		}

		compilationContext->setLabelOffset(condLabel, compilationContext->getCurInsOff());

		SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, pathEnv, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), s->cond, exprType, result));
		conditionReg = result.idxResultRegOut;

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, conditionReg), slake::Value(slake::ValueType::Label, bodyLabel), slake::Value(slake::ValueType::Label, normalExitLabel) }));

		compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

		{
			PathEnv bodyPathEnv(compileEnv->allocator.get());
			bodyPathEnv.execPossibility =
				constCondExpr
					? (constCondExpr.castTo<BoolLiteralExprNode>()->data
							  ? PathPossibility::Must
							  : PathPossibility::Never)
					: PathPossibility::May;
			bodyPathEnv.noReturnPossibility =
				constCondExpr
					? (constCondExpr.castTo<BoolLiteralExprNode>()->data
							  ? PathPossibility::Must
							  : PathPossibility::Never)
					: PathPossibility::May;
			bodyPathEnv.breakPossibility = PathPossibility::May;

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, pathEnv, s->body));

			compilationContext->setLabelOffset(stepLabel, compilationContext->getCurInsOff());

			if (s->step) {
				PathEnv stepPathEnv(compileEnv->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, s->step, ExprEvalPurpose::Stmt, {}, result));
				SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(bodyPathEnv, stepPathEnv));
			}

			SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(*pathEnv, bodyPathEnv));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, condLabel) }));
	} else {
		CompileExprResult result(compileEnv->allocator.get());

		compilationContext->setLabelOffset(condLabel, compilationContext->getCurInsOff());

		compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

		{
			PathEnv bodyPathEnv(compileEnv->allocator.get());
			bodyPathEnv.execPossibility = PathPossibility::Must;
			bodyPathEnv.noReturnPossibility = PathPossibility::Must;
			bodyPathEnv.breakPossibility = PathPossibility::May;

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, pathEnv, s->body));

			compilationContext->setLabelOffset(stepLabel, compilationContext->getCurInsOff());

			if (s->step) {
				PathEnv stepPathEnv(compileEnv->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, s->step, ExprEvalPurpose::Stmt, {}, result));
				SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(bodyPathEnv, stepPathEnv));
			}

			SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(*pathEnv, bodyPathEnv));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, condLabel) }));
	}

	compilationContext->setLabelOffset(normalExitLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));

	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileIfStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<IfStmtNode> s,
	uint32_t sldIndex) {
	AstNodePtr<BoolTypeNameNode> boolType;

	if (!(boolType = makeAstNode<BoolTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	uint32_t conditionReg;

	CompileExprResult result(compileEnv->allocator.get());

	AstNodePtr<TypeNameNode> exprType;

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, s->cond, exprType, boolType.castTo<TypeNameNode>()));

	PathEnv condEnv(compileEnv->allocator.get());
	condEnv.setParentEnv(pathEnv);

	PathEnv innerPathEnv[2] = {
		PathEnv(compileEnv->allocator.get()),
		PathEnv(compileEnv->allocator.get())
	};

	SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, &condEnv, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), s->cond, exprType, result));

	conditionReg = result.idxResultRegOut;

	uint32_t endLabel, trueLabel, falseLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(endLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(trueLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(falseLabel));

	{
		bool isSame;

		SLKC_RETURN_IF_COMP_ERROR(isSameType(boolType.castTo<TypeNameNode>(), exprType, isSame));

		AstNodePtr<ExprNode> constCondExpr;

		if (!isSame) {
			AstNodePtr<CastExprNode> castExpr;

			SLKC_RETURN_IF_COMP_ERROR(genImplicitCastExpr(
				compileEnv,
				s->cond,
				compileEnv->curOverloading->returnType,
				castExpr));

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, castExpr.castTo<ExprNode>(), constCondExpr));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, s->cond, constCondExpr));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, conditionReg), slake::Value(slake::ValueType::Label, trueLabel), slake::Value(slake::ValueType::Label, falseLabel) }));

		compilationContext->setLabelOffset(trueLabel, compilationContext->getCurInsOff());

		{
			innerPathEnv[0].setParentEnv(&condEnv);
			innerPathEnv[0].execPossibility =
				constCondExpr
					? (constCondExpr.castTo<BoolLiteralExprNode>()->data
							  ? PathPossibility::Must
							  : PathPossibility::Never)
					: PathPossibility::May;
			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, &innerPathEnv[0], s->trueBody));
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, endLabel) }));

		compilationContext->setLabelOffset(falseLabel, compilationContext->getCurInsOff());

		if (s->falseBody) {
			innerPathEnv[1].setParentEnv(pathEnv);
			innerPathEnv[1].execPossibility =
				constCondExpr
					? (constCondExpr.castTo<BoolLiteralExprNode>()->data
							  ? PathPossibility::Never
							  : PathPossibility::Must)
					: PathPossibility::May;
			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, &innerPathEnv[1], s->falseBody));
		}

		SLKC_RETURN_IF_COMP_ERROR(combineParallelPathEnv(compileEnv->allocator.get(), compileEnv, compilationContext, condEnv, innerPathEnv, 2));

		compilationContext->setLabelOffset(endLabel, compilationContext->getCurInsOff());

		SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(*pathEnv, condEnv));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileWhileStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<WhileStmtNode> s,
	uint32_t sldIndex) {
	AstNodePtr<BoolTypeNameNode> boolType;

	if (!(boolType = makeAstNode<BoolTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	CompileExprResult result(compileEnv->allocator.get());

	AstNodePtr<TypeNameNode> exprType;

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, s->cond, exprType, boolType.castTo<TypeNameNode>()));

	PrevBreakPointHolder breakPointHolder(compilationContext);
	PrevContinuePointHolder continuePointHolder(compilationContext);

	uint32_t conditionReg;

	uint32_t bodyLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(bodyLabel));

	uint32_t normalExitLabel, breakLabel, continueLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(normalExitLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
	compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(continueLabel));
	compilationContext->setContinueLabel(continueLabel, compilationContext->getBlockLevel());

	bool isSame;

	SLKC_RETURN_IF_COMP_ERROR(isSameType(boolType.castTo<TypeNameNode>(), exprType, isSame));

	AstNodePtr<ExprNode> constCondExpr;

	if (!isSame) {
		AstNodePtr<CastExprNode> castExpr;

		SLKC_RETURN_IF_COMP_ERROR(genImplicitCastExpr(
			compileEnv,
			s->cond,
			compileEnv->curOverloading->returnType,
			castExpr));

		SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, castExpr.castTo<ExprNode>(), constCondExpr));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, s->cond, constCondExpr));
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::ENTER,
			UINT32_MAX,
			{}));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
	peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
		compilationContext->leaveBlock();
	});

	{
		compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

		SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, pathEnv, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), s->cond, exprType, result));

		conditionReg = result.idxResultRegOut;

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, conditionReg),
					slake::Value(slake::ValueType::Label, bodyLabel),
					slake::Value(slake::ValueType::Label, normalExitLabel) }));
	}

	compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

	{
		PathEnv bodyPathEnv(compileEnv->allocator.get());
		bodyPathEnv.setParentEnv(pathEnv);
		bodyPathEnv.execPossibility =
			constCondExpr
				? (constCondExpr.castTo<BoolLiteralExprNode>()->data
						  ? PathPossibility::Must
						  : PathPossibility::Never)
				: PathPossibility::May;
		bodyPathEnv.noReturnPossibility =
			constCondExpr
				? (constCondExpr.castTo<BoolLiteralExprNode>()->data
						  ? PathPossibility::Must
						  : PathPossibility::Never)
				: PathPossibility::May;
		bodyPathEnv.breakPossibility = PathPossibility::May;

		SLKC_RETURN_IF_COMP_ERROR(tryCompileStmt(compileEnv, compilationContext, &bodyPathEnv, s->body));
		SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, &bodyPathEnv, s->body));

		SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(*pathEnv, bodyPathEnv));
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, continueLabel) }));

	compilationContext->setLabelOffset(normalExitLabel, compilationContext->getCurInsOff());
	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));
	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileDoWhileStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<DoWhileStmtNode> s,
	uint32_t sldIndex) {
	AstNodePtr<BoolTypeNameNode> boolType;

	if (!(boolType = makeAstNode<BoolTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	CompileExprResult result(compileEnv->allocator.get());

	AstNodePtr<TypeNameNode> exprType;

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, s->cond, exprType, boolType.castTo<TypeNameNode>()));

	PrevBreakPointHolder breakPointHolder(compilationContext);
	PrevContinuePointHolder continuePointHolder(compilationContext);

	uint32_t conditionReg;

	uint32_t bodyLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(bodyLabel));

	uint32_t normalExitLabel, breakLabel, continueLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(normalExitLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
	compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(continueLabel));
	compilationContext->setContinueLabel(continueLabel, compilationContext->getBlockLevel());

	bool isSame;

	SLKC_RETURN_IF_COMP_ERROR(isSameType(boolType.castTo<TypeNameNode>(), exprType, isSame));

	AstNodePtr<ExprNode> constCondExpr;

	if (!isSame) {
		AstNodePtr<CastExprNode> castExpr;

		SLKC_RETURN_IF_COMP_ERROR(genImplicitCastExpr(
			compileEnv,
			s->cond,
			compileEnv->curOverloading->returnType,
			castExpr));

		SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, castExpr.castTo<ExprNode>(), constCondExpr));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, s->cond, constCondExpr));
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::ENTER,
			UINT32_MAX,
			{}));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
	peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
		compilationContext->leaveBlock();
	});

	compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

	PathEnv bodyPathEnv(compileEnv->allocator.get());
	// execPossibility is defaultly set to must.
	bodyPathEnv.noReturnPossibility =
		constCondExpr
			? (constCondExpr.castTo<BoolLiteralExprNode>()->data
					  ? PathPossibility::Must
					  : PathPossibility::Never)
			: PathPossibility::May;
	bodyPathEnv.breakPossibility = PathPossibility::May;

	SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, &bodyPathEnv, s->body));

	SLKC_RETURN_IF_COMP_ERROR(combinePathEnv(*pathEnv, bodyPathEnv));

	compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, pathEnv, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), s->cond, exprType, result));
	conditionReg = result.idxResultRegOut;

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::BR,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::RegIndex, conditionReg),
				slake::Value(slake::ValueType::Label, bodyLabel),
				slake::Value(slake::ValueType::Label, breakLabel) }));

	compilationContext->setLabelOffset(normalExitLabel, compilationContext->getCurInsOff());
	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));
	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileWithStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<WithStmtNode> s,
	uint32_t sldIndex) {
	AstNodePtr<CustomTypeNameNode> tn;

	if (!(tn = makeAstNode<CustomTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document)))
		return genOutOfMemoryCompError();

	peff::DynArray<AstNodePtr<GenericParamNode>> involvedGenericParams(compileEnv->allocator.get());

	AstNodePtr<MemberNode> m;

	for (auto &i : s->constraints) {
		tn->idRefPtr.reset();

		IdRefPtr idRefPtr(peff::allocAndConstruct<IdRef>(compileEnv->allocator.get(), ASTNODE_ALIGNMENT, compileEnv->allocator.get()));
		if (!idRefPtr)
			return genOutOfMemoryCompError();

		IdRefEntry e(compileEnv->allocator.get());

		if (!e.name.build(i->genericParamName))
			return genOutOfMemoryCompError();

		if (!idRefPtr->entries.pushBack(std::move(e)))
			return genOutOfMemoryCompError();

		tn->tokenRange = s->tokenRange;

		tn->idRefPtr = std::move(idRefPtr);

		tn->contextNode = toWeakPtr(compileEnv->thisNode->thisType);

		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileEnv, compileEnv->document, tn, m));

		if (!m)
			return CompilationError(tn->tokenRange, CompilationErrorKind::ExpectingTypeName);

		if (AstNodeType::GenericParam != m->getAstNodeType()) {
			return CompilationError(tn->tokenRange, CompilationErrorKind::TypeIsNotSubstitutable);
		}

		if (!involvedGenericParams.pushBack(m.castTo<GenericParamNode>()))
			return genOutOfMemoryCompError();
	}

	peff::DynArray<GenericConstraintPtr> originalConstraints(compileEnv->allocator.get());

	for (auto &i : involvedGenericParams) {
		GenericConstraintPtr constraint;

		if (!(constraint = duplicateGenericConstraint(compileEnv->allocator.get(), i->genericConstraint.get())))
			return genOutOfMemoryCompError();

		if (!(originalConstraints.pushBack(std::move(constraint))))
			return genOutOfMemoryCompError();
	}

	peff::ScopeGuard restoreConstraintsGuard([&involvedGenericParams, &originalConstraints]() noexcept {
		for (size_t i = 0; i < involvedGenericParams.size(); ++i) {
			involvedGenericParams.at(i)->genericConstraint = std::move(originalConstraints.at(i));
		}
	});

	for (size_t i = 0; i < involvedGenericParams.size(); ++i) {
		// TODO: Replace the base type and add the additional implemented types.
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileSwitchStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<SwitchStmtNode> s,
	uint32_t sldIndex) {
	PrevBreakPointHolder breakPointHolder(compilationContext);

	AstNodePtr<TypeNameNode> matchType;

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, s->condition, matchType));

	if (!matchType)
		return CompilationError(s->condition->tokenRange, CompilationErrorKind::ErrorDeducingSwitchConditionType);

	uint32_t conditionReg;

	CompileExprResult result(compileEnv->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, s->condition, ExprEvalPurpose::RValue, {}, result));

	conditionReg = result.idxResultRegOut;

	if (!result.evaluatedType)
		return CompilationError(s->condition->tokenRange, CompilationErrorKind::ErrorDeducingSwitchConditionType);

	AstNodePtr<TypeNameNode> conditionType = result.evaluatedType;

	uint32_t breakLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
	compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());

	uint32_t defaultLabel;

	bool isDefaultSet = false;

	// Key = jump source, value = value register
	peff::Map<uint32_t, uint32_t> matchValueEvalLabels(compileEnv->allocator.get());

	// NOTE: Current switch statement is obsolete, we have to design a new one.
	peff::Set<AstNodePtr<ExprNode>> prevCaseConditions(compileEnv->allocator.get());

	for (size_t i = 0; i < s->caseOffsets.size(); ++i) {
		auto curCase = s->body.at(s->caseOffsets.at(i)).castTo<CaseLabelStmtNode>();

		uint32_t evalValueLabel;
		SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(evalValueLabel));

		if (curCase->isDefaultCase()) {
			if (isDefaultSet)
				return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);

			defaultLabel = evalValueLabel;

			isDefaultSet = true;
		} else {
			AstNodePtr<ExprNode> resultExpr;

			SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, curCase->condition, resultExpr));

			if (!resultExpr) {
				return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::ErrorEvaluatingConstSwitchCaseCondition);
			}

			AstNodePtr<TypeNameNode> resultExprType;

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, resultExpr, resultExprType));

			if (!resultExprType) {
				return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);
			}

			bool b;

			SLKC_RETURN_IF_COMP_ERROR(isSameType(conditionType, resultExprType, b));

			if (!b)
				return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);

			for (auto &j : prevCaseConditions) {
				bool b;

				AstNodePtr<BinaryExprNode> ce;

				if (!(ce = makeAstNode<BinaryExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
					return genOutOfMemoryCompError();

				ce->binaryOp = BinaryOp::Eq;

				ce->lhs = j;

				ce->rhs = resultExpr;

				AstNodePtr<ExprNode> cmpResult;

				SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, pathEnv, ce.castTo<ExprNode>(), cmpResult));

				assert(cmpResult);

				assert(cmpResult->exprKind == ExprKind::Bool);

				if (cmpResult.castTo<BoolLiteralExprNode>()->data) {
					return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);
				}
			}

			AstNodePtr<BinaryExprNode> cmpExpr;

			if (!(cmpExpr = makeAstNode<BinaryExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
				return genOutOfMemoryCompError();
			}

			cmpExpr->binaryOp = BinaryOp::Eq;

			cmpExpr->tokenRange = curCase->condition->tokenRange;

			if (!(cmpExpr->lhs = makeAstNode<RegIndexExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, conditionReg, conditionType).castTo<ExprNode>())) {
				return genOutOfMemoryCompError();
			}
			cmpExpr->lhs->tokenRange = curCase->condition->tokenRange;

			cmpExpr->rhs = curCase->condition;

			AstNodePtr<BoolTypeNameNode> boolTypeName;

			if (!(boolTypeName = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
				return genOutOfMemoryCompError();

			uint32_t cmpResultReg;
			{
				CompileExprResult cmpExprResult(compileEnv->allocator.get());

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, cmpExpr.castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.castTo<TypeNameNode>(), cmpExprResult));

				cmpResultReg = cmpExprResult.idxResultRegOut;
			}

			if (!prevCaseConditions.insert(AstNodePtr<ExprNode>(resultExpr)))
				return genOutOfMemoryCompError();

			uint32_t succeededValueLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(succeededValueLabel));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::BR, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, cmpResultReg), slake::Value(slake::ValueType::Label, evalValueLabel), slake::Value(slake::ValueType::Label, succeededValueLabel) }));

			compilationContext->setLabelOffset(succeededValueLabel, compilationContext->getCurInsOff());
		}

		matchValueEvalLabels.insert(s->caseOffsets.at(i), +evalValueLabel);
	}

	if (isDefaultSet) {
		SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, defaultLabel) }));
	}

	for (size_t i = 0; i < s->body.size(); ++i) {
		AstNodePtr<StmtNode> curStmt = s->body.at(i);

		if (curStmt->stmtKind == StmtKind::CaseLabel) {
			compilationContext->setLabelOffset(matchValueEvalLabels.at(i), compilationContext->getCurInsOff());
			continue;
		}

		SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, pathEnv, curStmt));
	}

	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileExprStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ExprStmtNode> s,
	uint32_t sldIndex) {
	CompileExprResult result(compileEnv->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, s->expr, ExprEvalPurpose::Stmt, {}, result));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileBreakStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<BreakStmtNode> s,
	uint32_t sldIndex) {
	uint32_t breakLabelId = compilationContext->getBreakLabel();
	if (compilationContext->getBreakLabel() == UINT32_MAX) {
		return CompilationError(s->tokenRange, CompilationErrorKind::InvalidBreakUsage);
	}
	uint32_t level = compilationContext->getBreakLabelBlockLevel();
	if (uint32_t curLevel = compilationContext->getBlockLevel();
		curLevel > level) {
		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::LEAVE,
				UINT32_MAX,
				{ slake::Value((uint32_t)(curLevel - level)) }));
	}
	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, breakLabelId) }));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileContinueStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ContinueStmtNode> s,
	uint32_t sldIndex) {
	uint32_t breakLabelId = compilationContext->getContinueLabel();
	if (compilationContext->getContinueLabel() == UINT32_MAX) {
		return CompilationError(s->tokenRange, CompilationErrorKind::InvalidBreakUsage);
	}
	uint32_t level = compilationContext->getContinueLabelBlockLevel();
	if (uint32_t curLevel = compilationContext->getBlockLevel();
		curLevel > level) {
		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::LEAVE,
				UINT32_MAX,
				{ slake::Value(curLevel - level) }));
	}
	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, breakLabelId) }));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileReturnStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<ReturnStmtNode> s,
	uint32_t sldIndex) {
	uint32_t reg;

	if (s->value) {
		if (compileEnv->curOverloading->returnType->typeNameKind == TypeNameKind::Void)
			return CompilationError(s->value->tokenRange, CompilationErrorKind::ReturnValueTypeDoesNotMatch);
		CompileExprResult result(compileEnv->allocator.get());

		AstNodePtr<TypeNameNode> valueType;

		SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, pathEnv, s->value, valueType));

		bool l;
		SLKC_RETURN_IF_COMP_ERROR(isLValueType(compileEnv->curOverloading->returnType, l));

		bool b;
		SLKC_RETURN_IF_COMP_ERROR(isSameType(valueType, compileEnv->curOverloading->returnType, b));
		if (b) {
			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, s->value, l ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, compileEnv->curOverloading->returnType, result));
			reg = result.idxResultRegOut;
		} else {
			SLKC_RETURN_IF_COMP_ERROR(isConvertible(valueType, compileEnv->curOverloading->returnType, true, b));
			if (b) {
				CompileExprResult result(compileEnv->allocator.get());

				AstNodePtr<CastExprNode> castExpr;

				SLKC_RETURN_IF_COMP_ERROR(genImplicitCastExpr(
					compileEnv,
					s->value,
					compileEnv->curOverloading->returnType,
					castExpr));

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, castExpr.castTo<ExprNode>(), ExprEvalPurpose::RValue, compileEnv->curOverloading->returnType, result));
				reg = result.idxResultRegOut;
			} else
				return CompilationError(s->value->tokenRange, CompilationErrorKind::ReturnValueTypeDoesNotMatch);
		}

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::RET,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, reg) }));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::RET,
				UINT32_MAX,
				{}));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileYieldStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	AstNodePtr<YieldStmtNode> s,
	uint32_t sldIndex) {
	uint32_t reg;

	if (s->value) {
		CompileExprResult result(compileEnv->allocator.get());

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, pathEnv, s->value, ExprEvalPurpose::RValue, compileEnv->curOverloading->returnType, result));
		reg = result.idxResultRegOut;

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::YIELD,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, reg) }));
	} else {
		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::YIELD,
				UINT32_MAX,
				{}));
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileStmt(
	CompileEnv *compileEnv,
	CompilationContext *compilationContext,
	PathEnv *pathEnv,
	const AstNodePtr<StmtNode> &stmt) {
	uint32_t sldIndex;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->registerSourceLocDesc(tokenRangeToSld(stmt->tokenRange), sldIndex));

	switch (stmt->stmtKind) {
		case StmtKind::Expr: {
			SLKC_RETURN_IF_COMP_ERROR(compileExprStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<ExprStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::VarDef: {
			SLKC_RETURN_IF_COMP_ERROR(compileVarDefStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<VarDefStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Break: {
			SLKC_RETURN_IF_COMP_ERROR(compileBreakStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<BreakStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Continue: {
			SLKC_RETURN_IF_COMP_ERROR(compileContinueStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<ContinueStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::For: {
			SLKC_RETURN_IF_COMP_ERROR(compileForStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<ForStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::While: {
			SLKC_RETURN_IF_COMP_ERROR(compileWhileStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<WhileStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::DoWhile: {
			SLKC_RETURN_IF_COMP_ERROR(compileDoWhileStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<DoWhileStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Return: {
			SLKC_RETURN_IF_COMP_ERROR(compileReturnStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<ReturnStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Yield: {
			SLKC_RETURN_IF_COMP_ERROR(compileYieldStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<YieldStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::If:
			SLKC_RETURN_IF_COMP_ERROR(compileIfStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<IfStmtNode>(), sldIndex));
			break;
		case StmtKind::With:
			SLKC_RETURN_IF_COMP_ERROR(compileWithStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<WithStmtNode>(), sldIndex));
			break;
		case StmtKind::Switch:
			SLKC_RETURN_IF_COMP_ERROR(compileSwitchStmt(compileEnv, compilationContext, pathEnv, stmt.castTo<SwitchStmtNode>(), sldIndex));
			break;
		case StmtKind::CaseLabel:
			return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidCaseLabelUsage);
		case StmtKind::CodeBlock: {
			AstNodePtr<CodeBlockStmtNode> s = stmt.castTo<CodeBlockStmtNode>();

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					sldIndex, slake::Opcode::ENTER,
					UINT32_MAX,
					{}));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
			peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
				compilationContext->leaveBlock();
			});

			for (size_t i = 0; i < s->body.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, pathEnv, s->body.at(i)));
			}

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					sldIndex, slake::Opcode::LEAVE,
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

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::tryCompileStmt(
	CompileEnv *compileEnv,
	CompilationContext *parentCompilationContext,
	PathEnv *pathEnv,
	const AstNodePtr<StmtNode> &stmt) {
	NormalCompilationContext tmpCtxt(compileEnv, parentCompilationContext);
	return compileStmt(compileEnv, &tmpCtxt, pathEnv, stmt);
}
