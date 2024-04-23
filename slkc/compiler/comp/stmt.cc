#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileStmt(shared_ptr<StmtNode> stmt) {
	switch (stmt->getStmtType()) {
		case StmtType::Expr: {
			slxfmt::SourceLocDesc sld;
			sld.offIns = curFn->body.size();
			sld.line = stmt->getLocation().line;
			sld.column = stmt->getLocation().column;

			curMajorContext.curMinorContext.evalPurpose = EvalPurpose::Stmt;
			compileExpr(static_pointer_cast<ExprStmtNode>(stmt)->expr);

			sld.nIns = curFn->body.size() - sld.offIns;
			curFn->srcLocDescs.push_back(sld);
			break;
		}
		case StmtType::VarDef: {
			auto s = static_pointer_cast<VarDefStmtNode>(stmt);

			if (s->type->getTypeId() == Type::Auto) {
				shared_ptr<TypeNameNode> deducedType;

				for (auto i : s->varDefs) {
					if (i.second.initValue) {
						deducedType = evalExprType(i.second.initValue);
						goto foundInitValue;
					}
				}

				throw FatalCompilationError(
					{ s->getLocation(),
						MessageType::Error,
						"No initializer was found, unable to deduce the type" });

			foundInitValue:
				updateCorrespondingTokenInfo(s->type, deducedType, CompletionContext::Type);
				s->type = deducedType;
			} else
				updateCorrespondingTokenInfo(s->type, s->type, CompletionContext::Type);

			for (auto i : s->varDefs) {
				if (curMajorContext.curMinorContext.localVars.count(i.first))
					throw FatalCompilationError(
						{ i.second.loc,
							MessageType::Error,
							"Redefinition of local variable `" + i.first + "'" });

				uint32_t index = allocLocalVar(i.first, s->type);

				if (i.second.initValue) {
					shared_ptr<TypeNameNode> initValueType = evalExprType(i.second.initValue);

					if (!isSameType(s->type, initValueType)) {
						if (!isTypeNamesConvertible(initValueType, s->type))
							throw FatalCompilationError(
								{ i.second.initValue->getLocation(),
									MessageType::Error,
									"Incompatible initial value type" });

						compileExpr(make_shared<CastExprNode>(i.second.initValue->getLocation(), s->type, i.second.initValue), EvalPurpose::RValue, make_shared<LocalVarRefNode>(index));
					} else
						compileExpr(i.second.initValue, EvalPurpose::RValue, make_shared<LocalVarRefNode>(index));
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
					make_shared<U32LiteralExprNode>(stmt->getLocation(), curMajorContext.curScopeLevel - curMajorContext.curMinorContext.breakScopeLevel));
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(curMajorContext.curMinorContext.breakLabel));
			break;
		case StmtType::Continue:
			if (!curMajorContext.curMinorContext.continueLabel.size())
				throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Unexpected continue statement" });
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(curMajorContext.curMinorContext.continueLabel));
			break;
		case StmtType::For: {
			auto s = static_pointer_cast<ForStmtNode>(stmt);

			auto loc = s->getLocation();
			string beginLabel = "$for_" + to_string(loc.line) + "_" + to_string(loc.column) + "_begin",
				   endLabel = "$for_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

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

				compileExpr(s->condition, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
				if (evalExprType(s->condition)->getTypeId() != Type::Bool)
					curFn->insertIns(
						Opcode::CAST,
						make_shared<RegRefNode>(tmpRegIndex),
						make_shared<BoolTypeNameNode>(s->condition->getLocation(), true),
						make_shared<RegRefNode>(tmpRegIndex, true));
				curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(endLabel), make_shared<RegRefNode>(tmpRegIndex, true));
			} else {
				curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			compileStmt(s->body);

			compileExpr(s->endExpr, EvalPurpose::Stmt, {});

			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(beginLabel));

			curFn->insertLabel(endLabel);
			curFn->insertIns(Opcode::LEAVE);

			popMinorContext();
			break;
		}
		case StmtType::While: {
			auto s = static_pointer_cast<WhileStmtNode>(stmt);

			auto loc = s->getLocation();
			string beginLabel = "$while_" + to_string(loc.line) + "_" + to_string(loc.column) + "_begin",
				   endLabel = "$while_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.continueScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;
			curMajorContext.curMinorContext.continueLabel = beginLabel;

			uint32_t tmpRegIndex = allocReg();

			curFn->insertLabel(beginLabel);

			compileExpr(s->condition, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
			if (evalExprType(s->condition)->getTypeId() != Type::Bool)
				curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(tmpRegIndex),
					make_shared<BoolTypeNameNode>(s->condition->getLocation(), true),
					make_shared<RegRefNode>(tmpRegIndex, true));

			compileStmt(s->body);

			curFn->insertLabel(endLabel);

			curFn->insertIns(Opcode::JT, make_shared<LabelRefNode>(beginLabel), make_shared<RegRefNode>(tmpRegIndex, true));

			popMinorContext();

			break;
		}
		case StmtType::Return: {
			auto s = static_pointer_cast<ReturnStmtNode>(stmt);

			auto returnType = curFn->returnType;

			if (!s->returnValue) {
				if (returnType->getTypeId() != Type::Void)
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

							compileExpr(make_shared<CastExprNode>(e->getLocation(), returnType, e), EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
							curFn->insertIns(Opcode::RET, make_shared<RegRefNode>(tmpRegIndex, true));
						}
					}
				} else {
					uint32_t tmpRegIndex = allocReg();

					if (isSameType(evalExprType(s->returnValue), returnType))
						compileExpr(s->returnValue, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
					else
						compileExpr(make_shared<CastExprNode>(s->returnValue->getLocation(), returnType, s->returnValue), EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));

					curFn->insertIns(Opcode::RET, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			break;
		}
		case StmtType::Yield: {
			auto s = static_pointer_cast<YieldStmtNode>(stmt);

			if (!s->returnValue) {
				if (curFn->returnType->getTypeId() != Type::Void)
					throw FatalCompilationError({ stmt->getLocation(), MessageType::Error, "Must yield a value" });
				else
					curFn->insertIns(Opcode::YIELD, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					curFn->insertIns(Opcode::YIELD, e);
				} else {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(s->returnValue, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::YIELD, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			break;
		}
		case StmtType::If: {
			auto s = static_pointer_cast<IfStmtNode>(stmt);
			auto loc = s->getLocation();

			string falseBranchLabel = "$if_" + to_string(loc.line) + "_" + to_string(loc.column) + "_false",
				   endLabel = "$if_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			uint32_t tmpRegIndex = allocReg();

			compileExpr(s->condition, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
			if (evalExprType(s->condition)->getTypeId() != Type::Bool)
				curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(tmpRegIndex),
					make_shared<BoolTypeNameNode>(s->getLocation(), true),
					make_shared<RegRefNode>(tmpRegIndex, true));

			curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(falseBranchLabel), make_shared<RegRefNode>(tmpRegIndex, true));

			compileStmt(s->body);
			if (s->elseBranch) {
				curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			curFn->insertLabel(falseBranchLabel);

			if (s->elseBranch) {
				compileStmt(s->elseBranch);
			}

			curFn->insertLabel(endLabel);

			break;
		}
		case StmtType::Try: {
			auto s = static_pointer_cast<TryStmtNode>(stmt);
			auto loc = s->getLocation();

			string labelPrefix = "$try_" + to_string(loc.line) + "_" + to_string(loc.column),
				   endLabel = labelPrefix + "_final";

			curFn->insertIns(Opcode::ENTER);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i)
				curFn->insertIns(
					Opcode::PUSHXH,
					s->catchBlocks[i].targetType,
					make_shared<LabelRefNode>(labelPrefix + "_xh_" + to_string(i)));

			compileStmt(s->body);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i) {
				pushMajorContext();

				auto &curBlock = s->catchBlocks[i];
				curFn->insertLabel(labelPrefix + "_xh_" + to_string(i));

				if (curBlock.exceptionVarName.size()) {
					curFn->insertIns(Opcode::ENTER);

					curFn->insertIns(Opcode::LEXCEPT, make_shared<LocalVarRefNode>(allocLocalVar(curBlock.exceptionVarName, curBlock.targetType)));
				}

				compileStmt(curBlock.body);

				if (curBlock.exceptionVarName.size())
					curFn->insertIns(Opcode::LEAVE);

				curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

				popMajorContext();
			}

			curFn->insertIns(Opcode::LEAVE);

			curFn->insertLabel(endLabel);

			if (s->finalBlock.body)
				compileStmt(s->finalBlock.body);

			break;
		}
		case StmtType::Switch: {
			auto s = static_pointer_cast<SwitchStmtNode>(stmt);
			auto loc = s->getLocation();

			string labelPrefix = "$switch_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;

			uint32_t matcheeRegIndex = allocReg(2);
			uint32_t conditionRegIndex = matcheeRegIndex + 1;

			compileExpr(s->expr, EvalPurpose::RValue, make_shared<RegRefNode>(matcheeRegIndex));

			SwitchCase *defaultCase = nullptr;

			for (size_t i = 0; i < s->cases.size(); ++i) {
				auto &curCase = s->cases[i];
				string caseBeginLabel = labelPrefix + "_case" + to_string(i) + "_end",
					   caseEndLabel = labelPrefix + "_case" + to_string(i) + "_end";

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

				compileExpr(curCase.condition, EvalPurpose::RValue, make_shared<RegRefNode>(conditionRegIndex));
				curFn->insertIns(
					Opcode::EQ,
					make_shared<RegRefNode>(conditionRegIndex),
					make_shared<RegRefNode>(matcheeRegIndex, true),
					make_shared<RegRefNode>(conditionRegIndex, true));
				curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(caseEndLabel), make_shared<RegRefNode>(conditionRegIndex));

				compileStmt(make_shared<BlockStmtNode>(curCase.loc, curCase.body));

				curFn->insertLabel(caseEndLabel);

				if (i + 1 < s->cases.size())
					curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(labelPrefix + "_case" + to_string(i + 1) + "_end"));
			}

			if (defaultCase)
				compileStmt(make_shared<BlockStmtNode>(defaultCase->loc, defaultCase->body));

			curFn->insertLabel(endLabel);

			popMinorContext();
			break;
		}
		case StmtType::CodeBlock: {
			auto s = static_pointer_cast<CodeBlockStmtNode>(stmt);

			pushMajorContext();

			curFn->insertIns(Opcode::ENTER);
			++curMajorContext.curScopeLevel;

			for (auto i : s->body.stmts) {
				try {
					compileStmt(i);
				}
				catch (FatalCompilationError e) {
					messages.push_back(e.message);
				}
			}

			--curMajorContext.curScopeLevel;
			curFn->insertIns(Opcode::LEAVE);

			popMajorContext();
			break;
		}
		case StmtType::Bad: {
			auto s = static_pointer_cast<BadStmtNode>(stmt);

			// Fill token information for completion.
			for (size_t i = s->beginTokenIndex; i <= s->endTokenIndex; ++i) {
				tokenInfos[i].completionContext = CompletionContext::Stmt;
				tokenInfos[i].tokenContext = TokenContext(curMajorContext);
			}
			break;
		}
		default:
			throw logic_error("Invalid statement type");
	}
}
