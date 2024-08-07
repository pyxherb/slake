#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileStmt(std::shared_ptr<StmtNode> stmt) {
	switch (stmt->getStmtType()) {
		case StmtType::Expr: {
			auto s = std::static_pointer_cast<ExprStmtNode>(stmt);

			if (s->expr) {
				slxfmt::SourceLocDesc sld;
				sld.offIns = curFn->body.size();
				sld.line = stmt->sourceLocation.beginPosition.line;
				sld.column = stmt->sourceLocation.beginPosition.column;

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

				std::shared_ptr<TypeNameNode> varType = i.second.type ? i.second.type : std::make_shared<AnyTypeNameNode>(SIZE_MAX);

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
							{ i.second.initValue->sourceLocation,
								MessageType::Error,
								"Error deducing type of the initial value" });

					if (!isSameType(varType, initValueType)) {
						throw FatalCompilationError(
							{ i.second.initValue->sourceLocation,
								MessageType::Error,
								"Incompatible initial value type" });
					} else {
						uint32_t
							resultRegIndex = allocReg(),
							tmpRegIndex = allocReg();

						compileExpr(i.second.initValue, EvalPurpose::LValue, std::make_shared<RegRefNode>(resultRegIndex));
						_insertIns(
							Opcode::LLOAD,
							std::make_shared<RegRefNode>(tmpRegIndex),
							{ std::make_shared<U32LiteralExprNode>(index) });
						_insertIns(
							Opcode::STORE,
							{},
							{ std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(resultRegIndex) });
					}
				} else {
					if (i.second.initValue) {
						std::shared_ptr<TypeNameNode> initValueType = evalExprType(i.second.initValue);

						if (!initValueType)
							throw FatalCompilationError(
								{ i.second.initValue->sourceLocation,
									MessageType::Error,
									"Error deducing type of the initial value" });

						if (!isSameType(varType, initValueType)) {
							if (!isTypeNamesConvertible(initValueType, varType))
								throw FatalCompilationError(
									{ i.second.initValue->sourceLocation,
										MessageType::Error,
										"Incompatible initial value type" });

							uint32_t
								resultRegIndex = allocReg(),
								tmpRegIndex = allocReg();

							compileExpr(std::make_shared<CastExprNode>(varType, i.second.initValue), EvalPurpose::RValue, std::make_shared<RegRefNode>(resultRegIndex));
							_insertIns(
								Opcode::LLOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								{ std::make_shared<U32LiteralExprNode>(index) });
							_insertIns(
								Opcode::STORE,
								{},
								{ std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(resultRegIndex) });
						} else {
							uint32_t
								resultRegIndex = allocReg(),
								tmpRegIndex = allocReg();

							compileExpr(i.second.initValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(resultRegIndex));
							_insertIns(
								Opcode::LLOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								{ std::make_shared<U32LiteralExprNode>(index) });
							_insertIns(
								Opcode::STORE,
								{},
								{ std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(resultRegIndex) });
						}
					}
				}
			}

			break;
		}
		case StmtType::Break:
			if (curMajorContext.curMinorContext.breakLabel.empty())
				throw FatalCompilationError({ stmt->sourceLocation, MessageType::Error, "Unexpected break statement" });

			if (curMajorContext.curMinorContext.breakScopeLevel < curMajorContext.curScopeLevel)
				_insertIns(
					Opcode::LEAVE,
					{},
					{ std::make_shared<U32LiteralExprNode>(curMajorContext.curScopeLevel - curMajorContext.curMinorContext.breakScopeLevel) });

			_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(curMajorContext.curMinorContext.breakLabel) });
			break;
		case StmtType::Continue:
			if (curMajorContext.curMinorContext.continueLabel.size())
				throw FatalCompilationError({ stmt->sourceLocation, MessageType::Error, "Unexpected continue statement" });

			_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(curMajorContext.curMinorContext.continueLabel) });
			break;
		case StmtType::For: {
			auto s = std::static_pointer_cast<ForStmtNode>(stmt);

			auto loc = s->sourceLocation;
			std::string beginLabel = "$for_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_begin",
						endLabel = "$for_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.continueScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;
			curMajorContext.curMinorContext.continueLabel = beginLabel;

			_insertIns(Opcode::ENTER, {}, {});
			compileStmt(s->varDefs);

			_insertLabel(beginLabel);

			uint32_t tmpRegIndex;

			if (s->condition) {
				tmpRegIndex = allocReg();

				auto conditionType = evalExprType(s->condition);
				auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

				if (!isSameType(conditionType, boolType)) {
					if (!isTypeNamesConvertible(conditionType, boolType))
						throw FatalCompilationError(
							{ s->condition->sourceLocation,
								MessageType::Error,
								"Expecting a boolean expression" });

					compileExpr(
						std::make_shared<CastExprNode>(
							boolType,
							s->condition),
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(tmpRegIndex));
				} else
					compileExpr(s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				_insertIns(Opcode::JF, {}, { std::make_shared<LabelRefNode>(endLabel), std::make_shared<RegRefNode>(tmpRegIndex) });
			} else {
				_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });
			}

			compileStmt(s->body);

			compileExpr(s->endExpr, EvalPurpose::Stmt, {});

			_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(beginLabel) });

			_insertLabel(endLabel);
			_insertIns(Opcode::LEAVE, {}, {});

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

			auto loc = s->sourceLocation;
			std::string beginLabel = "$while_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_begin",
						endLabel = "$while_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.continueScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;
			curMajorContext.curMinorContext.continueLabel = beginLabel;

			_insertLabel(beginLabel);

			uint32_t tmpRegIndex = allocReg();

			auto conditionType = evalExprType(s->condition);
			auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

			if (!isSameType(conditionType, boolType)) {
				if (!isTypeNamesConvertible(conditionType, boolType))
					throw FatalCompilationError(
						{ s->condition->sourceLocation,
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(
					std::make_shared<CastExprNode>(
						boolType,
						s->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(tmpRegIndex));
			} else
				compileExpr(s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

			if (s->body)
				compileStmt(s->body);

			_insertLabel(endLabel);

			_insertIns(Opcode::JT, {}, { std::make_shared<LabelRefNode>(beginLabel), std::make_shared<RegRefNode>(tmpRegIndex) });

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
					throw FatalCompilationError({ stmt->sourceLocation, MessageType::Error, "Must return a value" });
				else
					_insertIns(Opcode::RET, {}, { {} });
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					if (isSameType(evalExprType(e), returnType)) {
						_insertIns(Opcode::RET, {}, { e });
					} else {
						if (auto ce = castLiteralExpr(e, returnType->getTypeId()); ce) {
							_insertIns(Opcode::RET, {}, { ce });
						} else {
							uint32_t tmpRegIndex = allocReg();

							compileExpr(std::make_shared<CastExprNode>(returnType, e), EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
							_insertIns(Opcode::RET, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
						}
					}
				} else {
					uint32_t tmpRegIndex = allocReg();

					auto exprType = evalExprType(s->returnValue);

					if (!exprType)
						throw FatalCompilationError({ s->returnValue->sourceLocation, MessageType::Error, "Cannot deduce type of the return value" });

					if (isSameType(exprType, returnType))
						compileExpr(s->returnValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
					else
						compileExpr(std::make_shared<CastExprNode>(returnType, s->returnValue), EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

					_insertIns(Opcode::RET, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
				}
			}

			break;
		}
		case StmtType::Yield: {
			auto s = std::static_pointer_cast<YieldStmtNode>(stmt);

			if (!(curFn->isAsync))
				throw FatalCompilationError({ stmt->sourceLocation, MessageType::Error, "Cannot yield in a non-asynchronous function" });

			if (!s->returnValue) {
				if (curFn->returnType->getTypeId() != TypeId::Void)
					throw FatalCompilationError({ stmt->sourceLocation, MessageType::Error, "Must yield a value" });
				else
					_insertIns(Opcode::YIELD, {}, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					_insertIns(Opcode::YIELD, {}, { e });
				} else {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(s->returnValue, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
					_insertIns(Opcode::YIELD, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
				}
			}

			break;
		}
		case StmtType::If: {
			auto s = std::static_pointer_cast<IfStmtNode>(stmt);
			auto loc = s->sourceLocation;

			std::string falseBranchLabel = "$if_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_false",
						endLabel = "$if_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

			uint32_t tmpRegIndex = allocReg();

			auto conditionType = evalExprType(s->condition);
			auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

			if (!isSameType(conditionType, boolType)) {
				if (!isTypeNamesConvertible(conditionType, boolType))
					throw FatalCompilationError(
						{ s->condition->sourceLocation,
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(
					std::make_shared<CastExprNode>(
						boolType,
						s->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(tmpRegIndex));
			} else
				compileExpr(s->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));

			_insertIns(Opcode::JF, {}, { std::make_shared<LabelRefNode>(falseBranchLabel), std::make_shared<RegRefNode>(tmpRegIndex) });

			compileStmt(s->body);
			if (s->elseBranch) {
				_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });
			}

			_insertLabel(falseBranchLabel);

			if (s->elseBranch) {
				compileStmt(s->elseBranch);
			}

			_insertLabel(endLabel);

			break;
		}
		case StmtType::Try: {
			auto s = std::static_pointer_cast<TryStmtNode>(stmt);
			auto loc = s->sourceLocation;

			std::string labelPrefix = "$try_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column),
						endLabel = labelPrefix + "_final";

			_insertIns(Opcode::ENTER, {}, {});

			for (size_t i = 0; i < s->catchBlocks.size(); ++i)
				_insertIns(
					Opcode::PUSHXH,
					{},
					{ s->catchBlocks[i].targetType,
						std::make_shared<LabelRefNode>(labelPrefix + "_xh_" + std::to_string(i)) });

			compileStmt(s->body);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i) {
				pushMajorContext();

				auto &curBlock = s->catchBlocks[i];
				_insertLabel(labelPrefix + "_xh_" + std::to_string(i));

				if (curBlock.exceptionVarName.size()) {
					_insertIns(Opcode::ENTER, {}, {});

					uint32_t localVarIndex = allocLocalVar(curBlock.exceptionVarName, curBlock.targetType);
					uint32_t tmpRegIndex = allocReg(),
							 exceptRegIndex = allocReg();

					_insertIns(Opcode::LLOAD,
						std::make_shared<RegRefNode>(tmpRegIndex),
						{ std::make_shared<U32LiteralExprNode>(localVarIndex) });

					_insertIns(Opcode::LEXCEPT, std::make_shared<RegRefNode>(exceptRegIndex), {});
					_insertIns(Opcode::STORE,
						{},
						{ std::make_shared<RegRefNode>(tmpRegIndex),
							std::make_shared<RegRefNode>(exceptRegIndex) });
				}

				compileStmt(curBlock.body);

				if (curBlock.exceptionVarName.size())
					_insertIns(Opcode::LEAVE, {}, {});

				_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });

				popMajorContext();
			}

			_insertIns(Opcode::LEAVE, {}, {});

			_insertLabel(endLabel);

			if (s->finalBlock.body)
				compileStmt(s->finalBlock.body);

			break;
		}
		case StmtType::Switch: {
			auto s = std::static_pointer_cast<SwitchStmtNode>(stmt);
			auto loc = s->sourceLocation;

			std::string labelPrefix = "$switch_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column),
						condLocalVarName = labelPrefix + "_cond",
						defaultLabel = labelPrefix + "_label",
						endLabel = labelPrefix + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;

			uint32_t matcheeRegIndex = allocReg();
			uint32_t conditionRegIndex = allocReg();

			compileExpr(s->expr, EvalPurpose::RValue, std::make_shared<RegRefNode>(matcheeRegIndex));

			SwitchCase *defaultCase = nullptr;

			for (size_t i = 0; i < s->cases.size(); ++i) {
				auto &curCase = s->cases[i];
				std::string caseBeginLabel = labelPrefix + "_case" + std::to_string(i) + "_end",
							caseEndLabel = labelPrefix + "_case" + std::to_string(i) + "_end";

				_insertLabel(caseBeginLabel);

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

				uint32_t cmpResultRegIndex = allocReg();
				_insertIns(
					Opcode::EQ,
					std::make_shared<RegRefNode>(cmpResultRegIndex),
					{ std::make_shared<RegRefNode>(matcheeRegIndex),
						std::make_shared<RegRefNode>(conditionRegIndex) });
				_insertIns(Opcode::JF, {}, { std::make_shared<LabelRefNode>(caseEndLabel), std::make_shared<RegRefNode>(cmpResultRegIndex) });

				compileStmt(std::make_shared<CodeBlockStmtNode>(CodeBlock{ SourceLocation(), curCase.body }));

				_insertLabel(caseEndLabel);

				if (i + 1 < s->cases.size())
					_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(labelPrefix + "_case" + std::to_string(i + 1) + "_end") });
			}

			if (defaultCase)
				compileStmt(std::make_shared<CodeBlockStmtNode>(CodeBlock{ SourceLocation(), defaultCase->body }));

			_insertLabel(endLabel);

			popMinorContext();
			break;
		}
		case StmtType::CodeBlock: {
			auto s = std::static_pointer_cast<CodeBlockStmtNode>(stmt);

			pushMajorContext();

			_insertIns(Opcode::ENTER, {}, {});
			++curMajorContext.curScopeLevel;

			for (auto i : s->body.stmts) {
				try {
					compileStmt(i);
				} catch (FatalCompilationError e) {
					messages.push_back(e.message);
				}
			}

			--curMajorContext.curScopeLevel;
			_insertIns(Opcode::LEAVE, {}, {});

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
