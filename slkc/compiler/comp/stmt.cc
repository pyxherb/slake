#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileStmt(std::shared_ptr<StmtNode> stmt) {
	switch (stmt->getStmtType()) {
		case StmtType::Expr: {
			auto s = std::static_pointer_cast<ExprStmtNode>(stmt);

			if (s->expr) {
				slxfmt::SourceLocDesc sld;
				sld.offIns = curFn->body.size();
				sld.line = stmt->getLocation().line;
				sld.column = stmt->getLocation().column;

				compileExpr(std::static_pointer_cast<ExprStmtNode>(stmt)->expr, EvalPurpose::Stmt, {});

				sld.nIns = curFn->body.size() - sld.offIns;
				curFn->srcLocDescs.push_back(sld);
			}

#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(s->idxSemicolonToken, [this](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.completionContext = CompletionContext::Stmt;
			});
#endif
			break;
		}
		case StmtType::VarDef: {
			auto s = std::static_pointer_cast<VarDefStmtNode>(stmt);

			for (auto &i : s->varDefs) {
#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(i.second.idxNameToken, [this](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.semanticType = SemanticType::Var;
					tokenInfo.completionContext = CompletionContext::Name;
				});

				updateTokenInfo(i.second.idxColonToken, [this](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.completionContext = CompletionContext::Type;
				});

				if (i.second.type)
					updateCompletionContext(i.second.type, CompletionContext::Type);
#endif

				if (curMajorContext.curMinorContext.localVars.count(i.first))
					throw FatalCompilationError(
						{ i.second.loc,
							MessageType::Error,
							"Redefinition of local variable `" + i.first + "'" });

				std::shared_ptr<TypeNameNode> varType = i.second.type ? i.second.type : std::make_shared<AnyTypeNameNode>(Location(), SIZE_MAX);

				uint32_t index = allocLocalVar(i.first, varType);

				curMajorContext.curMinorContext.expectedType = varType;
				if (isLValueType(varType)) {
					if (!i.second.initValue) {
						throw FatalCompilationError(
							{ i.second.loc,
								MessageType::Error,
								"Reference variables require an initial value" });
					}

					std::shared_ptr<TypeNameNode> initValueType = evalExprType(i.second.initValue);

					if (!initValueType)
						throw FatalCompilationError(
							{ i.second.initValue->getLocation(),
								MessageType::Error,
								"Error deducing type of the initial value" });

					if (!isSameType(varType, initValueType)) {
						throw FatalCompilationError(
								{ i.second.initValue->getLocation(),
									MessageType::Error,
									"Incompatible initial value type" });
					} else
						compileExpr(i.second.initValue, EvalPurpose::LValue, std::make_shared<LocalVarRefNode>(index));
				} else {
					if (i.second.initValue) {
						std::shared_ptr<TypeNameNode> initValueType = evalExprType(i.second.initValue);

						if (!initValueType)
							throw FatalCompilationError(
								{ i.second.initValue->getLocation(),
									MessageType::Error,
									"Error deducing type of the initial value" });

						if (!isSameType(varType, initValueType)) {
							if (!isTypeNamesConvertible(initValueType, varType))
								throw FatalCompilationError(
									{ i.second.initValue->getLocation(),
										MessageType::Error,
										"Incompatible initial value type" });

							compileExpr(std::make_shared<CastExprNode>(i.second.initValue->getLocation(), varType, i.second.initValue), EvalPurpose::RValue, std::make_shared<LocalVarRefNode>(index));
						} else
							compileExpr(i.second.initValue, EvalPurpose::RValue, std::make_shared<LocalVarRefNode>(index));
					}
				}
			}

			break;
		}
		case StmtType::Break:
			if (!curMajorContext.curMinorContext.breakLabel.empty())
				throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Unexpected break statement" });
			if (curMajorContext.curMinorContext.breakScopeLevel < curMajorContext.curScopeLevel)
				curFn->insertIns(
					Opcode::LEAVE,
					std::make_shared<U32LiteralExprNode>(stmt->getLocation(), curMajorContext.curScopeLevel - curMajorContext.curMinorContext.breakScopeLevel));
			curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(curMajorContext.curMinorContext.breakLabel));
			break;
		case StmtType::Continue:
			if (!curMajorContext.curMinorContext.continueLabel.size())
				throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Unexpected continue statement" });
			curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(curMajorContext.curMinorContext.continueLabel));
			break;
		case StmtType::For: {
			auto s = std::static_pointer_cast<ForStmtNode>(stmt);

			auto loc = s->getLocation();
			std::string beginLabel = "$for_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_begin",
				   endLabel = "$for_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.continueScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;
			curMajorContext.curMinorContext.continueLabel = beginLabel;

			curFn->insertIns(Opcode::ENTER);
			compileStmt(s->varDefs);

			curFn->insertLabel(beginLabel);

			uint32_t tmpRegIndex;

			if (s->condition) {
				tmpRegIndex = allocReg();

				auto conditionType = evalExprType(s->condition);
				auto boolType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);

				if (!isSameType(conditionType, boolType)) {
					if (!isTypeNamesConvertible(conditionType, boolType))
						throw FatalCompilationError(
							{ s->condition->getLocation(),
								MessageType::Error,
								"Expecting a boolean expression" });

					compileExpr(
						std::make_shared<CastExprNode>(
							s->condition->getLocation(),
							boolType,
							s->condition),
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(tmpRegIndex));
				} else
					compileExpr(s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				curFn->insertIns(Opcode::JF, std::make_shared<LabelRefNode>(endLabel), std::make_shared<RegRefNode>(tmpRegIndex, true));
			} else {
				curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(endLabel));
			}

			compileStmt(s->body);

			compileExpr(s->endExpr, EvalPurpose::Stmt, {});

			curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(beginLabel));

			curFn->insertLabel(endLabel);
			curFn->insertIns(Opcode::LEAVE);

			popMinorContext();
			break;
		}
		case StmtType::While: {
			auto s = std::static_pointer_cast<WhileStmtNode>(stmt);

#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(s->idxLParentheseToken, [this](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.semanticType = SemanticType::Keyword;
				tokenInfo.completionContext = CompletionContext::Expr;
			});
#endif

			auto loc = s->getLocation();
			std::string beginLabel = "$while_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_begin",
				   endLabel = "$while_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.continueScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;
			curMajorContext.curMinorContext.continueLabel = beginLabel;

			uint32_t tmpRegIndex = allocReg();

			curFn->insertLabel(beginLabel);

			auto conditionType = evalExprType(s->condition);
			auto boolType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);

			if (!isSameType(conditionType, boolType)) {
				if (!isTypeNamesConvertible(conditionType, boolType))
					throw FatalCompilationError(
						{ s->condition->getLocation(),
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(
					std::make_shared<CastExprNode>(
						s->condition->getLocation(),
						boolType,
						s->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(tmpRegIndex));
			} else
				compileExpr(s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

			if (s->body)
				compileStmt(s->body);

			curFn->insertLabel(endLabel);

			curFn->insertIns(Opcode::JT, std::make_shared<LabelRefNode>(beginLabel), std::make_shared<RegRefNode>(tmpRegIndex, true));

			popMinorContext();

			break;
		}
		case StmtType::Return: {
			auto s = std::static_pointer_cast<ReturnStmtNode>(stmt);

#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(s->idxReturnToken, [this](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.semanticType = SemanticType::Keyword;
				tokenInfo.completionContext = CompletionContext::Expr;
			});
#endif

			auto returnType = curFn->returnType;

			if (!s->returnValue) {
				if (returnType->getTypeId() != TypeId::Void)
					throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Must return a value" });
				else
					curFn->insertIns(Opcode::RET, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					if (isSameType(evalExprType(e), returnType)) {
						curFn->insertIns(Opcode::RET, e);
					} else {
						if (auto ce = castLiteralExpr(e, returnType->getTypeId()); ce) {
							curFn->insertIns(Opcode::RET, ce);
						} else {
							uint32_t tmpRegIndex = allocReg();

							compileExpr(std::make_shared<CastExprNode>(e->getLocation(), returnType, e), EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
							curFn->insertIns(Opcode::RET, std::make_shared<RegRefNode>(tmpRegIndex, true));
						}
					}
				} else {
					uint32_t tmpRegIndex = allocReg();

					auto exprType = evalExprType(s->returnValue);

					if (!exprType)
						throw FatalCompilationError({ s->returnValue->getLocation(), MessageType::Error, "Cannot deduce type of the return value" });

					if (isSameType(exprType, returnType))
						compileExpr(s->returnValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
					else
						compileExpr(std::make_shared<CastExprNode>(s->returnValue->getLocation(), returnType, s->returnValue), EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

					curFn->insertIns(Opcode::RET, std::make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			break;
		}
		case StmtType::Yield: {
			auto s = std::static_pointer_cast<YieldStmtNode>(stmt);

			if (!(curFn->isAsync))
				throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Cannot yield in a non-asynchronous function" });

			if (!s->returnValue) {
				if (curFn->returnType->getTypeId() != TypeId::Void)
					throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Must yield a value" });
				else
					curFn->insertIns(Opcode::YIELD, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					curFn->insertIns(Opcode::YIELD, e);
				} else {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(s->returnValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::YIELD, std::make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			break;
		}
		case StmtType::If: {
			auto s = std::static_pointer_cast<IfStmtNode>(stmt);
			auto loc = s->getLocation();

			std::string falseBranchLabel = "$if_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_false",
				   endLabel = "$if_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_end";

			uint32_t tmpRegIndex = allocReg();

			auto conditionType = evalExprType(s->condition);
			auto boolType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);

			if (!isSameType(conditionType, boolType)) {
				if (!isTypeNamesConvertible(conditionType, boolType))
					throw FatalCompilationError(
						{ s->condition->getLocation(),
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(
					std::make_shared<CastExprNode>(
						s->condition->getLocation(),
						boolType,
						s->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(tmpRegIndex));
			} else
				compileExpr(s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

			curFn->insertIns(Opcode::JF, std::make_shared<LabelRefNode>(falseBranchLabel), std::make_shared<RegRefNode>(tmpRegIndex, true));

			compileStmt(s->body);
			if (s->elseBranch) {
				curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(endLabel));
			}

			curFn->insertLabel(falseBranchLabel);

			if (s->elseBranch) {
				compileStmt(s->elseBranch);
			}

			curFn->insertLabel(endLabel);

			break;
		}
		case StmtType::Try: {
			auto s = std::static_pointer_cast<TryStmtNode>(stmt);
			auto loc = s->getLocation();

			std::string labelPrefix = "$try_" + std::to_string(loc.line) + "_" + std::to_string(loc.column),
				   endLabel = labelPrefix + "_final";

			curFn->insertIns(Opcode::ENTER);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i)
				curFn->insertIns(
					Opcode::PUSHXH,
					s->catchBlocks[i].targetType,
					std::make_shared<LabelRefNode>(labelPrefix + "_xh_" + std::to_string(i)));

			compileStmt(s->body);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i) {
				pushMajorContext();

				auto &curBlock = s->catchBlocks[i];
				curFn->insertLabel(labelPrefix + "_xh_" + std::to_string(i));

				if (curBlock.exceptionVarName.size()) {
					curFn->insertIns(Opcode::ENTER);

					curFn->insertIns(Opcode::LEXCEPT, std::make_shared<LocalVarRefNode>(allocLocalVar(curBlock.exceptionVarName, curBlock.targetType)));
				}

				compileStmt(curBlock.body);

				if (curBlock.exceptionVarName.size())
					curFn->insertIns(Opcode::LEAVE);

				curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(endLabel));

				popMajorContext();
			}

			curFn->insertIns(Opcode::LEAVE);

			curFn->insertLabel(endLabel);

			if (s->finalBlock.body)
				compileStmt(s->finalBlock.body);

			break;
		}
		case StmtType::Switch: {
			auto s = std::static_pointer_cast<SwitchStmtNode>(stmt);
			auto loc = s->getLocation();

			std::string labelPrefix = "$switch_" + std::to_string(loc.line) + "_" + std::to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;

			uint32_t matcheeRegIndex = allocReg(2);
			uint32_t conditionRegIndex = matcheeRegIndex + 1;

			compileExpr(s->expr, EvalPurpose::RValue, std::make_shared<RegRefNode>(matcheeRegIndex));

			SwitchCase *defaultCase = nullptr;

			for (size_t i = 0; i < s->cases.size(); ++i) {
				auto &curCase = s->cases[i];
				std::string caseBeginLabel = labelPrefix + "_case" + std::to_string(i) + "_end",
					   caseEndLabel = labelPrefix + "_case" + std::to_string(i) + "_end";

				curFn->insertLabel(caseBeginLabel);

				if (!curCase.condition) {
					if (defaultCase)
						// The default case is already exist.
						throw FatalCompilationError(
							{ curCase.loc,
								MessageType::Error,
								"Duplicated default case" });
					defaultCase = &curCase;
				}

				compileExpr(curCase.condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));
				curFn->insertIns(
					Opcode::EQ,
					std::make_shared<RegRefNode>(conditionRegIndex),
					std::make_shared<RegRefNode>(matcheeRegIndex, true),
					std::make_shared<RegRefNode>(conditionRegIndex, true));
				curFn->insertIns(Opcode::JF, std::make_shared<LabelRefNode>(caseEndLabel), std::make_shared<RegRefNode>(conditionRegIndex));

				compileStmt(std::make_shared<BlockStmtNode>(curCase.loc, curCase.body));

				curFn->insertLabel(caseEndLabel);

				if (i + 1 < s->cases.size())
					curFn->insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(labelPrefix + "_case" + std::to_string(i + 1) + "_end"));
			}

			if (defaultCase)
				compileStmt(std::make_shared<BlockStmtNode>(defaultCase->loc, defaultCase->body));

			curFn->insertLabel(endLabel);

			popMinorContext();
			break;
		}
		case StmtType::CodeBlock: {
			auto s = std::static_pointer_cast<CodeBlockStmtNode>(stmt);

			pushMajorContext();

			curFn->insertIns(Opcode::ENTER);
			++curMajorContext.curScopeLevel;

			for (auto i : s->body.stmts) {
				try {
					compileStmt(i);
				} catch (FatalCompilationError e) {
					messages.push_back(e.message);
				}
			}

			--curMajorContext.curScopeLevel;
			curFn->insertIns(Opcode::LEAVE);

			popMajorContext();
			break;
		}
		case StmtType::Bad: {
			auto s = std::static_pointer_cast<BadStmtNode>(stmt);

			if (s->body)
				compileStmt(s->body);
			break;
		}
		default:
			throw std::logic_error("Invalid statement type");
	}
}
