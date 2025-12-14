#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::compileVarDefStmt(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
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

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(initialValueReg));

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

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, i->initialValue, exprType));

					bool same;

					SLKC_RETURN_IF_COMP_ERROR(isSameType(i->type, exprType, same));

					if (!same) {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, initialValueReg, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initialValue, exprType));
					} else {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::LValue, i->type, initialValueReg, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::RValue, i->type, initialValueReg, result));
						}
					}
				} else {
					AstNodePtr<TypeNameNode> deducedType;

					newVar->isTypeDeducedFromInitialValue = true;

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, i->initialValue, deducedType));

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
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::LValue, {}, initialValueReg, result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::RValue, {}, initialValueReg, result));
					}
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
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<ForStmtNode> s,
	uint32_t sldIndex) {
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

	uint32_t breakLabel, continueLabel, stepLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
	compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(continueLabel));
	compilationContext->setContinueLabel(continueLabel, compilationContext->getBlockLevel());
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(stepLabel));

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

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(initialValueReg));

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

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, i->initialValue, exprType));

					bool same;

					SLKC_RETURN_IF_COMP_ERROR(isSameType(i->type, exprType, same));

					if (!same) {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, initialValueReg, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initialValue, exprType));
					} else {
						bool b = false;

						SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

						if (b) {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::LValue, i->type, initialValueReg, result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::RValue, i->type, initialValueReg, result));
						}
					}
				} else {
					AstNodePtr<TypeNameNode> deducedType;

					newVar->isTypeDeducedFromInitialValue = true;

					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, i->initialValue, deducedType));

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
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::LValue, {}, initialValueReg, result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, i->initialValue, ExprEvalPurpose::RValue, {}, initialValueReg, result));
					}
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

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, continueLabel) }));

	compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, s->body));

	if (s->cond) {
		compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

		{
			CompileExprResult result(compileEnv->allocator.get());
			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->cond, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
		}

		uint32_t conditionReg;
		SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

		CompileExprResult result(compileEnv->allocator.get());

		AstNodePtr<TypeNameNode> tn, type;

		if (!(tn = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document).template castTo<TypeNameNode>())) {
			return genOutOfMemoryCompError();
		}

		SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, s->cond, type, tn));

		SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, conditionReg, ExprEvalPurpose::RValue, tn, s->cond, type));

		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, conditionReg), slake::Value(slake::ValueType::Label, stepLabel), slake::Value(slake::ValueType::Label, breakLabel) }));

		compilationContext->setLabelOffset(stepLabel, compilationContext->getCurInsOff());
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->step, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
	} else {
		compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());
		compilationContext->setLabelOffset(stepLabel, compilationContext->getCurInsOff());

		{
			CompileExprResult result(compileEnv->allocator.get());
			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->step, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, bodyLabel) }));

	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::LEAVE,
			UINT32_MAX,
			{ slake::Value((uint32_t)1) }));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileIfStmt(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<IfStmtNode> s,
	uint32_t sldIndex) {
	AstNodePtr<BoolTypeNameNode> boolType;

	if (!(boolType = makeAstNode<BoolTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	uint32_t reg;

	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

	CompileExprResult result(compileEnv->allocator.get());

	AstNodePtr<TypeNameNode> exprType;

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, s->cond, exprType, boolType.template castTo<TypeNameNode>()));

	SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, reg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), s->cond, exprType));

	uint32_t endLabel, trueLabel, falseLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(endLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(trueLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(falseLabel));

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::BR,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::RegIndex, reg), slake::Value(slake::ValueType::Label, trueLabel), slake::Value(slake::ValueType::Label, falseLabel) }));

	compilationContext->setLabelOffset(trueLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, s->trueBody));

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, endLabel) }));

	compilationContext->setLabelOffset(falseLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, s->falseBody));

	compilationContext->setLabelOffset(endLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileWhileStmt(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<WhileStmtNode> s,
	uint32_t sldIndex) {
	PrevBreakPointHolder breakPointHolder(compilationContext);
	PrevContinuePointHolder continuePointHolder(compilationContext);

	uint32_t bodyLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(bodyLabel));

	uint32_t breakLabel, continueLabel;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(continueLabel));

	if (!s->isDoWhile) {
		SLKC_RETURN_IF_COMP_ERROR(
			compilationContext->emitIns(
				sldIndex, slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, continueLabel) }));
	}

	SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
	peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
		compilationContext->leaveBlock();
	});

	compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

	SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, s->body));

	compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

	uint32_t conditionReg;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

	CompileExprResult result(compileEnv->allocator.get());

	AstNodePtr<TypeNameNode> tn, type;

	if (!(tn = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document).template castTo<TypeNameNode>())) {
		return genOutOfMemoryCompError();
	}

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, s->cond, type, tn));

	SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, conditionReg, ExprEvalPurpose::RValue, tn, s->cond, type));

	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::BR,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::RegIndex, conditionReg), slake::Value(slake::ValueType::Label, bodyLabel), slake::Value(slake::ValueType::Label, breakLabel) }));

	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileWithStmt(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
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

		tn->contextNode = compileEnv->thisNode->thisType;

		SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileEnv->document, tn, m));

		if (!m)
			return CompilationError(tn->tokenRange, CompilationErrorKind::ExpectingTypeName);

		if (AstNodeType::GenericParam != m->getAstNodeType()) {
			return CompilationError(tn->tokenRange, CompilationErrorKind::TypeIsNotSubstitutable);
		}

		if (!involvedGenericParams.pushBack(m.template castTo<GenericParamNode>()))
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
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<SwitchStmtNode> s,
	uint32_t sldIndex) {
	PrevBreakPointHolder breakPointHolder(compilationContext);

	AstNodePtr<TypeNameNode> matchType;

	SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, s->condition, matchType));

	if (!matchType)
		return CompilationError(s->condition->tokenRange, CompilationErrorKind::ErrorDeducingSwitchConditionType);

	uint32_t conditionReg;

	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

	CompileExprResult result(compileEnv->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->condition, ExprEvalPurpose::RValue, {}, conditionReg, result));

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

	if (s->isConst) {
		peff::Set<AstNodePtr<ExprNode>> prevCaseConditions(compileEnv->allocator.get());

		for (size_t i = 0; i < s->caseOffsets.size(); ++i) {
			auto curCase = s->body.at(s->caseOffsets.at(i)).template castTo<CaseLabelStmtNode>();

			uint32_t evalValueLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(evalValueLabel));

			if (!curCase->condition) {
				if (isDefaultSet)
					return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);

				defaultLabel = evalValueLabel;

				isDefaultSet = true;
			} else {
				AstNodePtr<ExprNode> resultExpr;

				SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, curCase->condition, resultExpr));

				if (!resultExpr) {
					return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::ErrorEvaluatingConstSwitchCaseCondition);
				}

				AstNodePtr<TypeNameNode> resultExprType;

				SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, resultExpr, resultExprType));

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

					SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, ce.template castTo<ExprNode>(), cmpResult));

					assert(cmpResult);

					assert(cmpResult->exprKind == ExprKind::Bool);

					if (cmpResult.template castTo<BoolLiteralExprNode>()->data) {
						return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);
					}
				}

				AstNodePtr<BinaryExprNode> cmpExpr;

				if (!(cmpExpr = makeAstNode<BinaryExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				cmpExpr->binaryOp = BinaryOp::Eq;

				cmpExpr->tokenRange = curCase->condition->tokenRange;

				if (!(cmpExpr->lhs = makeAstNode<RegIndexExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, conditionReg, conditionType).template castTo<ExprNode>())) {
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

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(cmpResultReg));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, cmpExpr.template castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.template castTo<TypeNameNode>(), cmpResultReg, cmpExprResult));
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
	} else {
		for (size_t i = 0; i < s->caseOffsets.size(); ++i) {
			auto curCase = s->body.at(s->caseOffsets.at(i)).template castTo<CaseLabelStmtNode>();

			uint32_t evalValueLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(evalValueLabel));

			if (!curCase->condition) {
				if (isDefaultSet)
					return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);

				defaultLabel = evalValueLabel;

				isDefaultSet = true;
			} else {
				AstNodePtr<TypeNameNode> resultExprType;

				SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, curCase->condition, resultExprType));

				if (!resultExprType) {
					return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);
				}

				bool b;

				SLKC_RETURN_IF_COMP_ERROR(isSameType(conditionType, resultExprType, b));

				if (!b)
					return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);

				AstNodePtr<BinaryExprNode> cmpExpr;

				if (!(cmpExpr = makeAstNode<BinaryExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				cmpExpr->binaryOp = BinaryOp::Eq;

				cmpExpr->tokenRange = curCase->condition->tokenRange;

				if (!(cmpExpr->lhs = makeAstNode<RegIndexExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, conditionReg, conditionType).template castTo<ExprNode>())) {
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

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(cmpResultReg));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, cmpExpr.template castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.template castTo<TypeNameNode>(), cmpResultReg, cmpExprResult));
				}

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
	}

	for (size_t i = 0; i < s->body.size(); ++i) {
		AstNodePtr<StmtNode> curStmt = s->body.at(i);

		if (curStmt->stmtKind == StmtKind::CaseLabel) {
			compilationContext->setLabelOffset(matchValueEvalLabels.at(i), compilationContext->getCurInsOff());
			continue;
		}

		SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, curStmt));
	}

	compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileExprStmt(
	CompileEnvironment* compileEnv,
	CompilationContext* compilationContext,
	AstNodePtr<ExprStmtNode> s,
	uint32_t sldIndex) {
	CompileExprResult result(compileEnv->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->expr, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileBreakStmt(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
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
				{ slake::Value(curLevel - level) }));
	}
	SLKC_RETURN_IF_COMP_ERROR(
		compilationContext->emitIns(
			sldIndex, slake::Opcode::JMP,
			UINT32_MAX,
			{ slake::Value(slake::ValueType::Label, breakLabelId) }));

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileContinueStmt(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
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
	CompileEnvironment* compileEnv,
	CompilationContext* compilationContext,
	AstNodePtr<ReturnStmtNode> s,
	uint32_t sldIndex) {
	uint32_t reg;

	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

	if (s->value) {
		CompileExprResult result(compileEnv->allocator.get());

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->value, ExprEvalPurpose::RValue, compileEnv->curOverloading->returnType, reg, result));

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
	CompileEnvironment* compileEnv,
	CompilationContext* compilationContext,
	AstNodePtr<YieldStmtNode> s,
	uint32_t sldIndex) {
	uint32_t reg;

	SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

	if (s->value) {
		CompileExprResult result(compileEnv->allocator.get());

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, s->value, ExprEvalPurpose::RValue, compileEnv->curOverloading->returnType, reg, result));

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
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	const AstNodePtr<StmtNode> &stmt) {
	uint32_t sldIndex;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->registerSourceLocDesc(tokenRangeToSld(stmt->tokenRange), sldIndex));

	switch (stmt->stmtKind) {
		case StmtKind::Expr: {
			SLKC_RETURN_IF_COMP_ERROR(compileExprStmt(compileEnv, compilationContext, stmt.castTo<ExprStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::VarDef: {
			SLKC_RETURN_IF_COMP_ERROR(compileVarDefStmt(compileEnv, compilationContext, stmt.castTo<VarDefStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Break: {
			SLKC_RETURN_IF_COMP_ERROR(compileBreakStmt(compileEnv, compilationContext, stmt.castTo<BreakStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Continue: {
			SLKC_RETURN_IF_COMP_ERROR(compileContinueStmt(compileEnv, compilationContext, stmt.castTo<ContinueStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::For: {
			SLKC_RETURN_IF_COMP_ERROR(compileForStmt(compileEnv, compilationContext, stmt.castTo<ForStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::While: {
			SLKC_RETURN_IF_COMP_ERROR(compileWhileStmt(compileEnv, compilationContext, stmt.castTo<WhileStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Return: {
			SLKC_RETURN_IF_COMP_ERROR(compileReturnStmt(compileEnv, compilationContext, stmt.castTo<ReturnStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::Yield: {
			SLKC_RETURN_IF_COMP_ERROR(compileYieldStmt(compileEnv, compilationContext, stmt.castTo<YieldStmtNode>(), sldIndex));
			break;
		}
		case StmtKind::If:
			SLKC_RETURN_IF_COMP_ERROR(compileIfStmt(compileEnv, compilationContext, stmt.castTo<IfStmtNode>(), sldIndex));
			break;
		case StmtKind::With:
			SLKC_RETURN_IF_COMP_ERROR(compileWithStmt(compileEnv, compilationContext, stmt.castTo<WithStmtNode>(), sldIndex));
			break;
		case StmtKind::Switch:
			SLKC_RETURN_IF_COMP_ERROR(compileSwitchStmt(compileEnv, compilationContext, stmt.castTo<SwitchStmtNode>(), sldIndex));
			break;
		case StmtKind::CaseLabel:
			return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidCaseLabelUsage);
		case StmtKind::CodeBlock: {
			AstNodePtr<CodeBlockStmtNode> s = stmt.template castTo<CodeBlockStmtNode>();

			bool hasFrame = false;

			for (auto i : s->body) {
				if (i->stmtKind == StmtKind::VarDef) {
					hasFrame = true;
				}
			}

			if (hasFrame)
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
				SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileEnv, compilationContext, s->body.at(i)));
			}

			if (hasFrame)
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
