#include "../compiler.h"

using namespace slkc;

SLKC_API std::optional<CompilationError> slkc::compileStmt(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	const peff::SharedPtr<StmtNode> &stmt) {
	switch (stmt->stmtKind) {
		case StmtKind::Expr: {
			peff::SharedPtr<ExprStmtNode> s = stmt.castTo<ExprStmtNode>();

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, s->expr, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
			break;
		}
		case StmtKind::VarDef: {
			peff::SharedPtr<VarDefStmtNode> s = stmt.castTo<VarDefStmtNode>();

			for (auto &i : s->varDefEntries) {
				if (compilationContext->getLocalVarInCurLevel(i->name)) {
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->initialValue->tokenRange, CompilationErrorKind::LocalVarAlreadyExists)));
				} else {
					peff::SharedPtr<VarNode> newVar;

					uint32_t localVarReg;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(localVarReg));

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLocalVar(s->tokenRange, i->name, localVarReg, i->type, newVar));

					if (i->initialValue) {
						uint32_t initialValueReg;

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(initialValueReg));

						CompileExprResult result(compileContext->allocator.get());

						if (i->type) {
							newVar->type = i->type;

							{
								slake::Type type;
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, newVar->type, type));

								SLKC_RETURN_IF_COMP_ERROR(
									compilationContext->emitIns(
										slake::Opcode::LVAR,
										localVarReg,
										{ slake::Value(type) }));
							}

							peff::SharedPtr<TypeNameNode> exprType;

							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, i->initialValue, exprType));

							bool same;

							SLKC_RETURN_IF_COMP_ERROR(isSameType(i->type, exprType, same));

							if (!same) {
								bool b = false;

								SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, initialValueReg, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initialValue, exprType));
							} else {
								bool b = false;

								SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

								if (b) {
									SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::LValue, i->type, initialValueReg, result));
								} else {
									SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::RValue, i->type, initialValueReg, result));
								}
							}
						} else {
							peff::SharedPtr<TypeNameNode> deducedType;

							newVar->isTypeDeducedFromInitialValue = true;

							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, i->initialValue, deducedType));

							if (!deducedType) {
								return CompilationError(stmt->tokenRange, CompilationErrorKind::ErrorDeducingVarType);
							}

							newVar->type = deducedType;

							{
								slake::Type type;
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, newVar->type, type));

								SLKC_RETURN_IF_COMP_ERROR(
									compilationContext->emitIns(
										slake::Opcode::LVAR,
										localVarReg,
										{ slake::Value(type) }));
							}

							bool b = false;
							SLKC_RETURN_IF_COMP_ERROR(isLValueType(deducedType, b));

							if (b) {
								SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::LValue, {}, initialValueReg, result));
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::RValue, {}, initialValueReg, result));
							}
						}

						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::STORE,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegRef, localVarReg), slake::Value(slake::ValueType::RegRef, initialValueReg) }));
					} else {
						if (!i->type) {
							return CompilationError(stmt->tokenRange, CompilationErrorKind::RequiresInitialValue);
						}

						newVar->type = i->type;

						{
							slake::Type type;
							SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, newVar->type, type));

							SLKC_RETURN_IF_COMP_ERROR(
								compilationContext->emitIns(
									slake::Opcode::LVAR,
									localVarReg,
									{ slake::Value(type) }));
						}
					}
				}
			}
			break;
		}
		case StmtKind::Break: {
			uint32_t breakLabelId = compilationContext->getBreakLabel();
			if (compilationContext->getBreakLabel() == UINT32_MAX) {
				return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidBreakUsage);
			}
			uint32_t level = compilationContext->getBreakLabelBlockLevel();
			if (uint32_t curLevel = compilationContext->getBlockLevel();
				curLevel > level) {
				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::LEAVE,
						UINT32_MAX,
						{ slake::Value(curLevel - level) }));
			}
			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, breakLabelId) }));
			break;
		}
		case StmtKind::Continue: {
			uint32_t breakLabelId = compilationContext->getContinueLabel();
			if (compilationContext->getContinueLabel() == UINT32_MAX) {
				return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidBreakUsage);
			}
			uint32_t level = compilationContext->getContinueLabelBlockLevel();
			if (uint32_t curLevel = compilationContext->getBlockLevel();
				curLevel > level) {
				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::LEAVE,
						UINT32_MAX,
						{ slake::Value(curLevel - level) }));
			}
			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, breakLabelId) }));
			break;
		}
		case StmtKind::For: {
			peff::SharedPtr<ForStmtNode> s = stmt.castTo<ForStmtNode>();

			PrevBreakPointHolder breakPointHolder(compilationContext);
			PrevContinuePointHolder continuePointHolder(compilationContext);

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::ENTER,
					UINT32_MAX,
					{}));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
			peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
				compilationContext->leaveBlock();
			});

			uint32_t bodyLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(bodyLabel));

			uint32_t breakLabel, continueLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
			compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(continueLabel));
			compilationContext->setContinueLabel(continueLabel, compilationContext->getBlockLevel());

			for (auto &i : s->varDefEntries) {
				if (compilationContext->getLocalVarInCurLevel(i->name)) {
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushError(CompilationError(i->initialValue->tokenRange, CompilationErrorKind::LocalVarAlreadyExists)));
				} else {
					peff::SharedPtr<VarNode> newVar;

					uint32_t localVarReg;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(localVarReg));

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLocalVar(s->tokenRange, i->name, localVarReg, i->type, newVar));

					if (i->initialValue) {
						uint32_t initialValueReg;

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(initialValueReg));

						CompileExprResult result(compileContext->allocator.get());

						if (i->type) {
							newVar->type = i->type;

							{
								slake::Type type;
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, newVar->type, type));

								SLKC_RETURN_IF_COMP_ERROR(
									compilationContext->emitIns(
										slake::Opcode::LVAR,
										localVarReg,
										{ slake::Value(type) }));
							}

							peff::SharedPtr<TypeNameNode> exprType;

							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, i->initialValue, exprType));

							bool same;

							SLKC_RETURN_IF_COMP_ERROR(isSameType(i->type, exprType, same));

							if (!same) {
								bool b = false;

								SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, initialValueReg, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, i->type, i->initialValue, exprType));
							} else {
								bool b = false;

								SLKC_RETURN_IF_COMP_ERROR(isLValueType(i->type, b));

								if (b) {
									SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::LValue, i->type, initialValueReg, result));
								} else {
									SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::RValue, i->type, initialValueReg, result));
								}
							}
						} else {
							peff::SharedPtr<TypeNameNode> deducedType;

							newVar->isTypeDeducedFromInitialValue = true;

							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, i->initialValue, deducedType));

							if (!deducedType) {
								return CompilationError(stmt->tokenRange, CompilationErrorKind::ErrorDeducingVarType);
							}

							newVar->type = deducedType;

							{
								slake::Type type;
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, newVar->type, type));

								SLKC_RETURN_IF_COMP_ERROR(
									compilationContext->emitIns(
										slake::Opcode::LVAR,
										localVarReg,
										{ slake::Value(type) }));
							}

							bool b = false;
							SLKC_RETURN_IF_COMP_ERROR(isLValueType(deducedType, b));

							if (b) {
								SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::LValue, {}, initialValueReg, result));
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, i->initialValue, ExprEvalPurpose::RValue, {}, initialValueReg, result));
							}
						}

						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::STORE,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegRef, localVarReg), slake::Value(slake::ValueType::RegRef, initialValueReg) }));
					} else {
						if (!i->type) {
							return CompilationError(stmt->tokenRange, CompilationErrorKind::RequiresInitialValue);
						}

						newVar->type = i->type;

						{
							slake::Type type;
							SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, newVar->type, type));

							SLKC_RETURN_IF_COMP_ERROR(
								compilationContext->emitIns(
									slake::Opcode::LVAR,
									localVarReg,
									{ slake::Value(type) }));
						}
					}
				}
			}

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, continueLabel) }));

			compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, compilationContext, s->body));

			if (s->cond) {
				compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

				{
					CompileExprResult result(compileContext->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, s->cond, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
				}

				uint32_t conditionReg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

				CompileExprResult result(compileContext->allocator.get());

				peff::SharedPtr<TypeNameNode> tn, type;

				if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document).castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, s->cond, type, tn));

				SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, conditionReg, ExprEvalPurpose::RValue, tn, s->cond, type));

				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::JT,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::Label, bodyLabel), slake::Value(slake::ValueType::RegRef, conditionReg) }));
			} else {
				compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

				{
					CompileExprResult result(compileContext->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, s->cond, ExprEvalPurpose::Stmt, {}, UINT32_MAX, result));
				}
			}

			compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::LEAVE,
					UINT32_MAX,
					{ slake::Value((uint32_t)1) }));
			break;
		}
		case StmtKind::While: {
			peff::SharedPtr<WhileStmtNode> s = stmt.castTo<WhileStmtNode>();

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
						slake::Opcode::JMP,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::Label, continueLabel) }));
			}

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
			peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
				compilationContext->leaveBlock();
			});

			compilationContext->setLabelOffset(bodyLabel, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, compilationContext, s->body));

			compilationContext->setLabelOffset(continueLabel, compilationContext->getCurInsOff());

			uint32_t conditionReg;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

			CompileExprResult result(compileContext->allocator.get());

			peff::SharedPtr<TypeNameNode> tn, type;

			if (!(tn = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document).castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, s->cond, type, tn));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, conditionReg, ExprEvalPurpose::RValue, tn, s->cond, type));

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::JT,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, bodyLabel), slake::Value(slake::ValueType::RegRef, conditionReg) }));

			compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());
			break;
		}
		case StmtKind::Return: {
			peff::SharedPtr<ReturnStmtNode> s = stmt.castTo<ReturnStmtNode>();

			uint32_t reg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

			if (s->value) {
				CompileExprResult result(compileContext->allocator.get());

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, s->value, ExprEvalPurpose::RValue, compileContext->curOverloading->returnType, reg, result));

				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::RET,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::RegRef, reg) }));
			} else {
				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::RET,
						UINT32_MAX,
						{ slake::Value(slake::EntityRef::makeObjectRef(nullptr)) }));
			}
			break;
		}
		case StmtKind::Yield: {
			peff::SharedPtr<YieldStmtNode> s = stmt.castTo<YieldStmtNode>();

			uint32_t reg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

			if (s->value) {
				CompileExprResult result(compileContext->allocator.get());

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, s->value, ExprEvalPurpose::RValue, compileContext->curOverloading->returnType, reg, result));

				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::YIELD,
						UINT32_MAX,
						{ slake::Value(slake::ValueType::RegRef, reg) }));
			} else {
				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
						slake::Opcode::YIELD,
						UINT32_MAX,
						{ slake::Value(slake::EntityRef::makeObjectRef(nullptr)) }));
			}
			break;
		}
		case StmtKind::If: {
			peff::SharedPtr<IfStmtNode> s = stmt.castTo<IfStmtNode>();

			peff::SharedPtr<BoolTypeNameNode> boolType;

			if (!(boolType = peff::makeShared<BoolTypeNameNode>(
					  compileContext->allocator.get(),
					  compileContext->allocator.get(),
					  compileContext->document))) {
				return genOutOfMemoryCompError();
			}

			uint32_t reg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

			CompileExprResult result(compileContext->allocator.get());

			peff::SharedPtr<TypeNameNode> exprType;

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, s->cond, exprType, boolType.castTo<TypeNameNode>()));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, reg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), s->cond, exprType));

			uint32_t endLabel, falseLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(endLabel));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(falseLabel));

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::JF,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, falseLabel) }));

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, compilationContext, s->trueBody));

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::JMP,
					UINT32_MAX,
					{ slake::Value(slake::ValueType::Label, endLabel) }));

			compilationContext->setLabelOffset(falseLabel, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, compilationContext, s->falseBody));

			compilationContext->setLabelOffset(endLabel, compilationContext->getCurInsOff());
			break;
		}
		case StmtKind::With:
			break;
		case StmtKind::Switch: {
			peff::SharedPtr<SwitchStmtNode> s = stmt.castTo<SwitchStmtNode>();

			PrevBreakPointHolder breakPointHolder(compilationContext);

			peff::SharedPtr<TypeNameNode> matchType;

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, s->condition, matchType));

			if (!matchType)
				return CompilationError(s->condition->tokenRange, CompilationErrorKind::ErrorDeducingSwitchConditionType);

			uint32_t conditionReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, s->condition, ExprEvalPurpose::RValue, {}, conditionReg, result));

			if (!result.evaluatedType)
				return CompilationError(s->condition->tokenRange, CompilationErrorKind::ErrorDeducingSwitchConditionType);

			peff::SharedPtr<TypeNameNode> conditionType = result.evaluatedType;

			uint32_t breakLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(breakLabel));
			compilationContext->setBreakLabel(breakLabel, compilationContext->getBlockLevel());

			uint32_t defaultLabel;

			bool isDefaultSet = false;

			// Key = jump source, value = value register
			peff::Map<uint32_t, uint32_t> matchValueEvalLabels(compileContext->allocator.get());

			if (s->isConst) {
				peff::Set<peff::SharedPtr<ExprNode>> prevCaseConditions(compileContext->allocator.get());

				for (size_t i = 0; i < s->caseOffsets.size(); ++i) {
					auto curCase = s->body.at(s->caseOffsets.at(i)).castTo<CaseLabelStmtNode>();

					uint32_t evalValueLabel;
					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(evalValueLabel));

					if (!curCase->condition) {
						if (isDefaultSet)
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);

						defaultLabel = evalValueLabel;

						isDefaultSet = true;
					} else {
						peff::SharedPtr<ExprNode> resultExpr;

						SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, curCase->condition, resultExpr));

						if (!resultExpr) {
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::ErrorEvaluatingConstSwitchCaseCondition);
						}

						peff::SharedPtr<TypeNameNode> resultExprType;

						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, resultExpr, resultExprType));

						if (!resultExprType) {
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);
						}

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(isSameType(conditionType, resultExprType, b));

						if (!b)
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);

						for (auto &j : prevCaseConditions) {
							bool b;

							peff::SharedPtr<BinaryExprNode> ce;

							if (!(ce = peff::makeShared<BinaryExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document)))
								return genOutOfMemoryCompError();

							ce->binaryOp = BinaryOp::Eq;

							ce->lhs = j;

							ce->rhs = resultExpr;

							peff::SharedPtr<ExprNode> cmpResult;

							SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, ce.castTo<ExprNode>(), cmpResult));

							assert(cmpResult);

							assert(cmpResult->exprKind == ExprKind::Bool);

							if (cmpResult.castTo<BoolLiteralExprNode>()->data) {
								return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);
							}
						}

						peff::SharedPtr<BinaryExprNode> cmpExpr;

						if (!(cmpExpr = peff::makeShared<BinaryExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->binaryOp = BinaryOp::Eq;

						cmpExpr->tokenRange = curCase->condition->tokenRange;

						if (!(cmpExpr->lhs = peff::makeShared<RegRefExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, conditionReg, conditionType).castTo<ExprNode>())) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->rhs = curCase->condition;

						peff::SharedPtr<BoolTypeNameNode> boolTypeName;

						if (!(boolTypeName = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document)))
							return genOutOfMemoryCompError();

						uint32_t cmpResultReg;
						{
							CompileExprResult cmpExprResult(compileContext->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(cmpResultReg));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, cmpExpr.castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.castTo<TypeNameNode>(), cmpResultReg, cmpExprResult));
						}

						if (!prevCaseConditions.insert(peff::SharedPtr<ExprNode>(resultExpr)))
							return genOutOfMemoryCompError();

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JT, UINT32_MAX, { slake::Value(slake::ValueType::Label, evalValueLabel), slake::Value(slake::ValueType::RegRef, cmpResultReg) }));
					}

					matchValueEvalLabels.insert(s->caseOffsets.at(i), +evalValueLabel);
				}

				if (isDefaultSet) {
					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, defaultLabel) }));
				}
			} else {
				for (size_t i = 0; i < s->caseOffsets.size(); ++i) {
					auto curCase = s->body.at(s->caseOffsets.at(i)).castTo<CaseLabelStmtNode>();

					uint32_t evalValueLabel;
					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(evalValueLabel));

					if (!curCase->condition) {
						if (isDefaultSet)
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::DuplicatedSwitchCaseBranch);

						defaultLabel = evalValueLabel;

						isDefaultSet = true;
					} else {
						peff::SharedPtr<TypeNameNode> resultExprType;

						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, curCase->condition, resultExprType));

						if (!resultExprType) {
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);
						}

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(isSameType(conditionType, resultExprType, b));

						if (!b)
							return CompilationError(curCase->condition->tokenRange, CompilationErrorKind::MismatchedSwitchCaseConditionType);

						peff::SharedPtr<BinaryExprNode> cmpExpr;

						if (!(cmpExpr = peff::makeShared<BinaryExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->binaryOp = BinaryOp::Eq;

						cmpExpr->tokenRange = curCase->condition->tokenRange;

						if (!(cmpExpr->lhs = peff::makeShared<RegRefExprNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, conditionReg, conditionType).castTo<ExprNode>())) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->rhs = curCase->condition;

						peff::SharedPtr<BoolTypeNameNode> boolTypeName;

						if (!(boolTypeName = peff::makeShared<BoolTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document)))
							return genOutOfMemoryCompError();

						uint32_t cmpResultReg;
						{
							CompileExprResult cmpExprResult(compileContext->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(cmpResultReg));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, cmpExpr.castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.castTo<TypeNameNode>(), cmpResultReg, cmpExprResult));
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JT, UINT32_MAX, { slake::Value(slake::ValueType::Label, evalValueLabel), slake::Value(slake::ValueType::RegRef, cmpResultReg) }));
					}

					matchValueEvalLabels.insert(s->caseOffsets.at(i), +evalValueLabel);
				}

				if (isDefaultSet) {
					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, defaultLabel) }));
				}
			}

			for (size_t i = 0; i < s->body.size(); ++i) {
				peff::SharedPtr<StmtNode> curStmt = s->body.at(i);

				if (curStmt->stmtKind == StmtKind::CaseLabel) {
					compilationContext->setLabelOffset(matchValueEvalLabels.at(i), compilationContext->getCurInsOff());
					continue;
				}

				SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, compilationContext, curStmt));
			}

			compilationContext->setLabelOffset(breakLabel, compilationContext->getCurInsOff());

			break;
		}
		case StmtKind::CaseLabel:
			return CompilationError(stmt->tokenRange, CompilationErrorKind::InvalidCaseLabelUsage);
		case StmtKind::CodeBlock: {
			peff::SharedPtr<CodeBlockStmtNode> s = stmt.castTo<CodeBlockStmtNode>();

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
					slake::Opcode::ENTER,
					UINT32_MAX,
					{}));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->enterBlock());
			peff::ScopeGuard popBlockContextGuard([compilationContext]() noexcept {
				compilationContext->leaveBlock();
			});

			for (size_t i = 0; i < s->body.size(); ++i) {
				SLKC_RETURN_IF_COMP_ERROR(compileStmt(compileContext, compilationContext, s->body.at(i)));
			}

			SLKC_RETURN_IF_COMP_ERROR(
				compilationContext->emitIns(
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
