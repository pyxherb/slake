#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileStmt(CompileContext *compileContext, std::shared_ptr<StmtNode> stmt) {
	SourceDocument *curDoc = compileContext->compiler->getCurDoc();

	switch (stmt->getStmtType()) {
	case StmtType::Expr: {
		auto s = std::static_pointer_cast<ExprStmtNode>(stmt);

		if (s->expr) {
			slxfmt::SourceLocDesc sld;
			sld.offIns = compileContext->curFn->body.size();
			sld.line = tokenRangeToSourceLocation(stmt->tokenRange).beginPosition.line;
			sld.column = tokenRangeToSourceLocation(stmt->tokenRange).beginPosition.column;

			compileExpr(compileContext, std::static_pointer_cast<ExprStmtNode>(stmt)->expr, EvalPurpose::Stmt, {});

			sld.nIns = compileContext->curFn->body.size() - sld.offIns;
			compileContext->curFn->srcLocDescs.push_back(sld);
		}

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxSemicolonToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Stmt;
			});
#endif
		break;
	}
	case StmtType::VarDef: {
		auto s = std::static_pointer_cast<VarDefStmtNode>(stmt);

		for (auto &i : s->varDefs) {
#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(i.second.idxNameToken, [this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.semanticType = SemanticType::Var;
				tokenInfo.completionContext = CompletionContext::Name;
			});

			updateTokenInfoForTrailingSpaces(
				compileContext, i.second.idxColonToken + 1,
				curDoc->lexer->tokens.size(),
				[this, &compileContext](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
					tokenInfo.completionContext = CompletionContext::Type;
				});

			if (i.second.type)
				updateCompletionContext(i.second.type, CompletionContext::Type);
#endif

			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.localVars.count(i.first))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(i.second.tokenRange),
						MessageType::Error,
						"Redefinition of local variable `" + i.first + "'" });

			std::shared_ptr<TypeNameNode> varType = i.second.type ? i.second.type : std::make_shared<AnyTypeNameNode>(SIZE_MAX);

			uint32_t index = compileContext->allocLocalVar(i.first, varType);

			compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType = varType;
			if (isLValueType(varType)) {
				if (!i.second.initValue) {
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(i.second.tokenRange),
							MessageType::Error,
							"Reference variables require an initial value" });
				}

				std::shared_ptr<TypeNameNode> initValueType = evalExprType(compileContext, i.second.initValue);

				if (!initValueType)
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(i.second.initValue->tokenRange),
							MessageType::Error,
							"Error deducing type of the initial value" });

				if (!isSameType(compileContext, varType, initValueType)) {
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(i.second.initValue->tokenRange),
							MessageType::Error,
							"Incompatible initial value type" });
				} else {
					uint32_t
						resultRegIndex = compileContext->allocReg(),
						tmpRegIndex = compileContext->allocReg();

					compileExpr(compileContext, i.second.initValue, EvalPurpose::LValue, std::make_shared<RegRefNode>(resultRegIndex));
					compileContext->_insertIns(
						Opcode::MOV,
						std::make_shared<RegRefNode>(tmpRegIndex),
						{ std::make_shared<RegRefNode>(index) });
					compileContext->_insertIns(
						Opcode::STORE,
						{},
						{ std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(resultRegIndex) });
				}
			} else {
				if (i.second.initValue) {
					std::shared_ptr<TypeNameNode> initValueType = evalExprType(compileContext, i.second.initValue);

					if (!initValueType)
						throw FatalCompilationError(
							{ tokenRangeToSourceLocation(i.second.initValue->tokenRange),
								MessageType::Error,
								"Error deducing type of the initial value" });

					if (!isSameType(compileContext, varType, initValueType)) {
						if (!isTypeNamesConvertible(compileContext, initValueType, varType))
							throw FatalCompilationError(
								{ tokenRangeToSourceLocation(i.second.initValue->tokenRange),
									MessageType::Error,
									"Incompatible initial value type" });

						uint32_t
							resultRegIndex = compileContext->allocReg(),
							tmpRegIndex = compileContext->allocReg();

						compileExpr(compileContext, std::make_shared<CastExprNode>(varType, i.second.initValue), EvalPurpose::RValue, std::make_shared<RegRefNode>(resultRegIndex));
						compileContext->_insertIns(
							Opcode::MOV,
							std::make_shared<RegRefNode>(tmpRegIndex),
							{ std::make_shared<RegRefNode>(index) });
						compileContext->_insertIns(
							Opcode::STORE,
							{},
							{ std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(resultRegIndex) });
					} else {
						uint32_t
							resultRegIndex = compileContext->allocReg(),
							tmpRegIndex = compileContext->allocReg();

						compileExpr(compileContext, i.second.initValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(resultRegIndex));
						compileContext->_insertIns(
							Opcode::MOV,
							std::make_shared<RegRefNode>(tmpRegIndex),
							{ std::make_shared<RegRefNode>(index) });
						compileContext->_insertIns(
							Opcode::STORE,
							{},
							{ std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(resultRegIndex) });
					}
				}
			}
		}

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxSemicolonToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Stmt;
			});
#endif
		break;
	}
	case StmtType::Break: {
		std::shared_ptr<BreakStmtNode> s = std::static_pointer_cast<BreakStmtNode>(stmt);
#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxSemicolonToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Stmt;
			});
#endif

		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakLabel.empty())
			throw FatalCompilationError({ tokenRangeToSourceLocation(stmt->tokenRange), MessageType::Error, "Unexpected break statement" });

		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakScopeLevel < compileContext->curCollectiveContext.curMajorContext.curScopeLevel)
			compileContext->_insertIns(
				Opcode::LEAVE,
				{},
				{ std::make_shared<U32LiteralExprNode>(compileContext->curCollectiveContext.curMajorContext.curScopeLevel - compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakScopeLevel) });

		compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakLabel) });
		break;
	}
	case StmtType::Continue: {
		std::shared_ptr<ContinueStmtNode> s = std::static_pointer_cast<ContinueStmtNode>(stmt);
#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxSemicolonToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Stmt;
			});
#endif

		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.continueLabel.size())
			throw FatalCompilationError({ tokenRangeToSourceLocation(stmt->tokenRange), MessageType::Error, "Unexpected continue statement" });

		compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(compileContext->curCollectiveContext.curMajorContext.curMinorContext.continueLabel) });
		break;
	}
	case StmtType::For: {
		auto s = std::static_pointer_cast<ForStmtNode>(stmt);

		auto loc = tokenRangeToSourceLocation(s->tokenRange);
		std::string beginLabel = "$for_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_begin",
					endLabel = "$for_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

		compileContext->pushMinorContext();

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakScopeLevel = compileContext->curCollectiveContext.curMajorContext.curScopeLevel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.continueScopeLevel = compileContext->curCollectiveContext.curMajorContext.curScopeLevel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakLabel = endLabel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.continueLabel = beginLabel;

		compileContext->_insertIns(Opcode::ENTER, {}, {});
		compileStmt(compileContext, s->varDefs);

		compileContext->_insertLabel(beginLabel);

		uint32_t tmpRegIndex;

		if (s->condition) {
			tmpRegIndex = compileContext->allocReg();

			auto conditionType = evalExprType(compileContext, s->condition);
			auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

			if (!isSameType(compileContext, conditionType, boolType)) {
				if (!isTypeNamesConvertible(compileContext, conditionType, boolType))
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(s->condition->tokenRange),
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(compileContext,
					std::make_shared<CastExprNode>(
						boolType,
						s->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(tmpRegIndex));
			} else
				compileExpr(compileContext, s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
			compileContext->_insertIns(Opcode::JF, {}, { std::make_shared<LabelRefNode>(endLabel), std::make_shared<RegRefNode>(tmpRegIndex) });
		} else {
			compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });
		}

		compileStmt(compileContext, s->body);

		compileExpr(compileContext, s->endExpr, EvalPurpose::Stmt, {});

		compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(beginLabel) });

		compileContext->_insertLabel(endLabel);
		compileContext->_insertIns(Opcode::LEAVE, {}, {});

		compileContext->popMinorContext();

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->tokenRange.endIndex + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Stmt;
			});
#endif
		break;
	}
	case StmtType::While: {
		auto s = std::static_pointer_cast<WhileStmtNode>(stmt);

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(s->idxLParentheseToken, [this, &compileContext](TokenInfo &tokenInfo) {
			tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
			tokenInfo.semanticType = SemanticType::Keyword;
			tokenInfo.completionContext = CompletionContext::Expr;
		});
#endif

		auto loc = tokenRangeToSourceLocation(s->tokenRange);
		std::string beginLabel = "$while_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_begin",
					endLabel = "$while_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

		compileContext->pushMinorContext();

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakScopeLevel = compileContext->curCollectiveContext.curMajorContext.curScopeLevel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.continueScopeLevel = compileContext->curCollectiveContext.curMajorContext.curScopeLevel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakLabel = endLabel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.continueLabel = beginLabel;

		compileContext->_insertLabel(beginLabel);

		uint32_t tmpRegIndex = compileContext->allocReg();

		auto conditionType = evalExprType(compileContext, s->condition);
		auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

		if (!isSameType(compileContext, conditionType, boolType)) {
			if (!isTypeNamesConvertible(compileContext, conditionType, boolType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(s->condition->tokenRange),
						MessageType::Error,
						"Expecting a boolean expression" });

			compileExpr(compileContext,
				std::make_shared<CastExprNode>(
					boolType,
					s->condition),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(tmpRegIndex));
		} else
			compileExpr(compileContext, s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

		if (s->body)
			compileStmt(compileContext, s->body);

		compileContext->_insertLabel(endLabel);

		compileContext->_insertIns(Opcode::JT, {}, { std::make_shared<LabelRefNode>(beginLabel), std::make_shared<RegRefNode>(tmpRegIndex) });

		compileContext->popMinorContext();

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxLParentheseToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Expr;
			});
#endif
		break;
	}
	case StmtType::Return: {
		auto s = std::static_pointer_cast<ReturnStmtNode>(stmt);

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(s->idxReturnToken, [this, &compileContext](TokenInfo &tokenInfo) {
			tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
			tokenInfo.semanticType = SemanticType::Keyword;
			tokenInfo.completionContext = CompletionContext::Expr;
		});
#endif

		auto returnType = compileContext->curFn->returnType;

		if (!s->returnValue) {
			if (returnType->getTypeId() != TypeId::Void)
				throw FatalCompilationError({ tokenRangeToSourceLocation(stmt->tokenRange), MessageType::Error, "Must return a value" });
			else
				compileContext->_insertIns(Opcode::RET, {}, { {} });
		} else {
			if (auto e = evalConstExpr(compileContext, s->returnValue); e) {
				if (isSameType(compileContext, evalExprType(compileContext, e), returnType)) {
					compileContext->_insertIns(Opcode::RET, {}, { e });
				} else {
					if (auto ce = castLiteralExpr(e, returnType->getTypeId()); ce) {
						compileContext->_insertIns(Opcode::RET, {}, { ce });
					} else {
						uint32_t tmpRegIndex = compileContext->allocReg();

						compileExpr(compileContext, std::make_shared<CastExprNode>(returnType, e), EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
						compileContext->_insertIns(Opcode::RET, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
					}
				}
			} else {
				uint32_t tmpRegIndex = compileContext->allocReg();

				auto exprType = evalExprType(compileContext, s->returnValue);

				if (!exprType)
					throw FatalCompilationError({ tokenRangeToSourceLocation(s->returnValue->tokenRange), MessageType::Error, "Cannot deduce type of the return value" });

				if (isSameType(compileContext, exprType, returnType))
					compileExpr(compileContext, s->returnValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				else
					compileExpr(compileContext, std::make_shared<CastExprNode>(returnType, s->returnValue), EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

				compileContext->_insertIns(Opcode::RET, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
			}
		}

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxReturnToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Expr;
			});
#endif
		break;
	}
	case StmtType::Yield: {
		auto s = std::static_pointer_cast<YieldStmtNode>(stmt);

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(s->idxYieldToken, [this, &compileContext](TokenInfo &tokenInfo) {
			tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
			tokenInfo.semanticType = SemanticType::Keyword;
			tokenInfo.completionContext = CompletionContext::Expr;
		});
#endif

		if (!(compileContext->curFn->isAsync))
			throw FatalCompilationError({ tokenRangeToSourceLocation(stmt->tokenRange), MessageType::Error, "Cannot yield in a non-asynchronous function" });

		if (!s->returnValue) {
			if (compileContext->curFn->returnType->getTypeId() != TypeId::Void)
				throw FatalCompilationError({ tokenRangeToSourceLocation(stmt->tokenRange), MessageType::Error, "Must yield a value" });
			else
				compileContext->_insertIns(Opcode::YIELD, {}, {});
		} else {
			if (auto e = evalConstExpr(compileContext, s->returnValue); e) {
				compileContext->_insertIns(Opcode::YIELD, {}, { e });
			} else {
				uint32_t tmpRegIndex = compileContext->allocReg();

				compileExpr(compileContext, s->returnValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				compileContext->_insertIns(Opcode::YIELD, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
			}
		}

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfoForTrailingSpaces(
			compileContext,
			s->idxYieldToken + 1, curDoc->lexer->tokens.size(),
			[this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Expr;
			});
#endif
		break;
	}
	case StmtType::If: {
		auto s = std::static_pointer_cast<IfStmtNode>(stmt);
		auto loc = tokenRangeToSourceLocation(s->tokenRange);

		std::string falseBranchLabel = "$if_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_false",
					endLabel = "$if_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

		uint32_t tmpRegIndex = compileContext->allocReg();

		auto conditionType = evalExprType(compileContext, s->condition);
		auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

		if (!isSameType(compileContext, conditionType, boolType)) {
			if (!isTypeNamesConvertible(compileContext, conditionType, boolType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(s->condition->tokenRange),
						MessageType::Error,
						"Expecting a boolean expression" });

			compileExpr(compileContext,
				std::make_shared<CastExprNode>(
					boolType,
					s->condition),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(tmpRegIndex));
		} else
			compileExpr(compileContext, s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

		compileContext->_insertIns(Opcode::JF, {}, { std::make_shared<LabelRefNode>(falseBranchLabel), std::make_shared<RegRefNode>(tmpRegIndex) });

		compileStmt(compileContext, s->body);
		if (s->elseBranch) {
			compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });
		}

		compileContext->_insertLabel(falseBranchLabel);

		if (s->elseBranch) {
			compileStmt(compileContext, s->elseBranch);
		}

		compileContext->_insertLabel(endLabel);

		break;
	}
	case StmtType::Try: {
		auto s = std::static_pointer_cast<TryStmtNode>(stmt);
		auto loc = tokenRangeToSourceLocation(s->tokenRange);

		std::string labelPrefix = "$try_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column),
					endLabel = labelPrefix + "_final";

		compileContext->_insertIns(Opcode::ENTER, {}, {});

		for (size_t i = 0; i < s->catchBlocks.size(); ++i)
			compileContext->_insertIns(
				Opcode::PUSHXH,
				{},
				{ s->catchBlocks[i].targetType,
					std::make_shared<LabelRefNode>(labelPrefix + "_xh_" + std::to_string(i)) });

		compileStmt(compileContext, s->body);

		for (size_t i = 0; i < s->catchBlocks.size(); ++i) {
			compileContext->pushMajorContext();

			auto &curBlock = s->catchBlocks[i];
			compileContext->_insertLabel(labelPrefix + "_xh_" + std::to_string(i));

			if (curBlock.exceptionVarName.size()) {
				compileContext->_insertIns(Opcode::ENTER, {}, {});

				uint32_t localVarIndex = compileContext->allocLocalVar(curBlock.exceptionVarName, curBlock.targetType);
				uint32_t tmpRegIndex = compileContext->allocReg(),
						 exceptRegIndex = compileContext->allocReg();

				compileContext->_insertIns(Opcode::MOV,
					std::make_shared<RegRefNode>(tmpRegIndex),
					{ std::make_shared<RegRefNode>(localVarIndex) });

				compileContext->_insertIns(Opcode::LEXCEPT, std::make_shared<RegRefNode>(exceptRegIndex), {});
				compileContext->_insertIns(Opcode::STORE,
					{},
					{ std::make_shared<RegRefNode>(tmpRegIndex),
						std::make_shared<RegRefNode>(exceptRegIndex) });
			}

			compileStmt(compileContext, curBlock.body);

			if (curBlock.exceptionVarName.size())
				compileContext->_insertIns(Opcode::LEAVE, {}, {});

			compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });

			compileContext->popMajorContext();
		}

		compileContext->_insertIns(Opcode::LEAVE, {}, {});

		compileContext->_insertLabel(endLabel);

		if (s->finalBlock.body)
			compileStmt(compileContext, s->finalBlock.body);

		break;
	}
	case StmtType::Switch: {
		auto s = std::static_pointer_cast<SwitchStmtNode>(stmt);
		auto loc = tokenRangeToSourceLocation(s->tokenRange);

		std::string labelPrefix = "$switch_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column),
					condLocalVarName = labelPrefix + "_cond",
					defaultLabel = labelPrefix + "_label",
					endLabel = labelPrefix + "_end";

		compileContext->pushMinorContext();

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakScopeLevel = compileContext->curCollectiveContext.curMajorContext.curScopeLevel;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.breakLabel = endLabel;

		uint32_t matcheeRegIndex = compileContext->allocReg();
		uint32_t conditionRegIndex = compileContext->allocReg();

		compileExpr(compileContext, s->expr, EvalPurpose::RValue, std::make_shared<RegRefNode>(matcheeRegIndex));

		SwitchCase *defaultCase = nullptr;

		for (size_t i = 0; i < s->cases.size(); ++i) {
			auto &curCase = s->cases[i];
			std::string caseBeginLabel = labelPrefix + "_case" + std::to_string(i) + "_end",
						caseEndLabel = labelPrefix + "_case" + std::to_string(i) + "_end";

			compileContext->_insertLabel(caseBeginLabel);

			if (!curCase.condition) {
				if (defaultCase)
					// The default case is already exist.
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(curCase.tokenRange),
							MessageType::Error,
							"Duplicated default case" });
				defaultCase = &curCase;
			}

			compileExpr(compileContext, curCase.condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));

			uint32_t cmpResultRegIndex = compileContext->allocReg();
			compileContext->_insertIns(
				Opcode::EQ,
				std::make_shared<RegRefNode>(cmpResultRegIndex),
				{ std::make_shared<RegRefNode>(matcheeRegIndex),
					std::make_shared<RegRefNode>(conditionRegIndex) });
			compileContext->_insertIns(Opcode::JF, {}, { std::make_shared<LabelRefNode>(caseEndLabel), std::make_shared<RegRefNode>(cmpResultRegIndex) });

			compileStmt(compileContext, std::make_shared<CodeBlockStmtNode>(CodeBlock{ {}, curCase.body }));

			compileContext->_insertLabel(caseEndLabel);

			if (i + 1 < s->cases.size())
				compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(labelPrefix + "_case" + std::to_string(i + 1) + "_end") });
		}

		if (defaultCase)
			compileStmt(compileContext, std::make_shared<CodeBlockStmtNode>(CodeBlock{ {}, defaultCase->body }));

		compileContext->_insertLabel(endLabel);

		compileContext->popMinorContext();
		break;
	}
	case StmtType::CodeBlock: {
		auto s = std::static_pointer_cast<CodeBlockStmtNode>(stmt);

		compileContext->pushMajorContext();

		if (compileContext->curCollectiveContext.curMajorContext.curScopeLevel)
			compileContext->_insertIns(Opcode::ENTER, {}, {});
		++compileContext->curCollectiveContext.curMajorContext.curScopeLevel;

		for (auto i : s->body.stmts) {
			try {
				compileStmt(compileContext, i);
			} catch (FatalCompilationError e) {
				pushMessage(
					curDocName,
					e.message);
			}
		}

		--compileContext->curCollectiveContext.curMajorContext.curScopeLevel;
		if (compileContext->curCollectiveContext.curMajorContext.curScopeLevel)
			compileContext->_insertIns(Opcode::LEAVE, {}, {});

		compileContext->popMajorContext();
		break;
	}
	case StmtType::Bad: {
		auto s = std::static_pointer_cast<BadStmtNode>(stmt);

		if (s->body)
			compileStmt(compileContext, s->body);
		break;
	}
	default:
		throw std::logic_error("Invalid statement type");
	}
}
