#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileStmt(shared_ptr<StmtNode> stmt) {
	switch (stmt->getStmtType()) {
		case STMT_EXPR: {
			slxfmt::SourceLocDesc sld;
			sld.offIns = curFn->body.size();
			sld.line = stmt->getLocation().line;
			sld.column = stmt->getLocation().column;

			curMajorContext.curMinorContext.evalPurpose = EvalPurpose::STMT;
			compileExpr(static_pointer_cast<ExprStmtNode>(stmt)->expr);

			sld.nIns = curFn->body.size() - sld.offIns;
			curFn->srcLocDescs.push_back(sld);
			break;
		}
		case STMT_VARDEF: {
			auto s = static_pointer_cast<VarDefStmtNode>(stmt);

			if (s->type->getTypeId() == TYPE_AUTO) {
				for (auto i : s->varDefs) {
					if (i.second.initValue) {
						s->type = evalExprType(i.second.initValue);
						goto initValueFound;
					}
				}

				throw FatalCompilationError(
					{ s->getLocation(),
						MSG_ERROR,
						"No initializer was found, unable to deduce the type" });

			initValueFound:;
			}

			for (auto i : s->varDefs) {
				if (curMajorContext.localVars.count(i.first))
					throw FatalCompilationError(
						{ i.second.loc,
							MSG_ERROR,
							"Redefinition of local variable `" + i.first + "'" });

				auto initValueType = evalExprType(i.second.initValue);

				uint32_t index = allocLocalVar(i.first, s->type);

				if (i.second.initValue) {
					if (!isSameType(s->type, initValueType)) {
						if (!areTypesConvertible(initValueType, s->type))
							throw FatalCompilationError(
								{ i.second.initValue->getLocation(),
									MSG_ERROR,
									"Incompatible initial value type" });

						compileExpr(make_shared<CastExprNode>(i.second.initValue->getLocation(), s->type, i.second.initValue), EvalPurpose::RVALUE, make_shared<LocalVarRefNode>(index));
					} else
						compileExpr(i.second.initValue, EvalPurpose::RVALUE, make_shared<LocalVarRefNode>(index));
				}
			}

			break;
		}
		case STMT_BREAK:
			if (!curMajorContext.curMinorContext.breakLabel.empty())
				throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Unexpected break statement" });
			if (curMajorContext.curMinorContext.breakScopeLevel < curMajorContext.curScopeLevel)
				curFn->insertIns(
					Opcode::LEAVE,
					make_shared<U32LiteralExprNode>(stmt->getLocation(), curMajorContext.curScopeLevel - curMajorContext.curMinorContext.breakScopeLevel));
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(curMajorContext.curMinorContext.breakLabel));
			break;
		case STMT_CONTINUE:
			if (!curMajorContext.curMinorContext.continueLabel.size())
				throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Unexpected continue statement" });
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(curMajorContext.curMinorContext.continueLabel));
			break;
		case STMT_FOR: {
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

			if (s->condition) {
				uint32_t tmpRegIndex = allocReg();

				compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
				if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
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

			compileExpr(s->endExpr, EvalPurpose::STMT, {});

			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(beginLabel));

			curFn->insertLabel(endLabel);
			curFn->insertIns(Opcode::LEAVE);

			popMinorContext();
			break;
		}
		case STMT_WHILE: {
			auto s = static_pointer_cast<WhileStmtNode>(stmt);

			auto loc = s->getLocation();
			string beginLabel = "$while_" + to_string(loc.line) + "_" + to_string(loc.column) + "_begin",
				   endLabel = "$while_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.continueScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;
			curMajorContext.curMinorContext.continueLabel = beginLabel;

			curFn->insertLabel(beginLabel);

			uint32_t tmpRegIndex = allocReg();

			compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
			if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
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
		case STMT_RETURN: {
			auto s = static_pointer_cast<ReturnStmtNode>(stmt);

			auto returnType = curFn->returnType;

			if (!s->returnValue) {
				if (returnType->getTypeId() != TYPE_VOID)
					throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Must return a value" });
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

							compileExpr(make_shared<CastExprNode>(e->getLocation(), returnType, e), EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
							curFn->insertIns(Opcode::RET, make_shared<RegRefNode>(tmpRegIndex, true));
						}
					}
				} else {
					uint32_t tmpRegIndex = allocReg();

					if (isSameType(evalExprType(s->returnValue), returnType))
						compileExpr(s->returnValue, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
					else
						compileExpr(make_shared<CastExprNode>(s->returnValue->getLocation(), returnType, s->returnValue), EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));

					curFn->insertIns(Opcode::RET, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			break;
		}
		case STMT_YIELD: {
			auto s = static_pointer_cast<YieldStmtNode>(stmt);

			if (!s->returnValue) {
				if (curFn->returnType->getTypeId() != TYPE_VOID)
					throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Must yield a value" });
				else
					curFn->insertIns(Opcode::YIELD, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					curFn->insertIns(Opcode::YIELD, e);
				} else {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(s->returnValue, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::YIELD, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			break;
		}
		case STMT_IF: {
			auto s = static_pointer_cast<IfStmtNode>(stmt);
			auto loc = s->getLocation();

			string endLabel = endLabel = "$if_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			uint32_t tmpRegIndex = allocReg();

			compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
			if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
				curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(tmpRegIndex),
					make_shared<BoolTypeNameNode>(s->getLocation(), true),
					make_shared<RegRefNode>(tmpRegIndex, true));

			curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(endLabel), make_shared<RegRefNode>(tmpRegIndex));

			compileStmt(s->body);

			if (s->elseBranch)
				compileStmt(s->elseBranch);

			curFn->insertLabel(endLabel);

			break;
		}
		case STMT_TRY: {
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
		case STMT_SWITCH: {
			auto s = static_pointer_cast<SwitchStmtNode>(stmt);
			auto loc = s->getLocation();

			string labelPrefix = "$switch_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			pushMinorContext();

			curMajorContext.curMinorContext.breakScopeLevel = curMajorContext.curScopeLevel;
			curMajorContext.curMinorContext.breakLabel = endLabel;

			uint32_t matcheeRegIndex = allocReg();

			compileExpr(s->expr, EvalPurpose::RVALUE, make_shared<RegRefNode>(matcheeRegIndex));

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
								MSG_ERROR,
								"Duplicated default case" });
					defaultCase = &curCase;
				}

				uint32_t conditionRegIndex = allocReg();

				compileExpr(curCase.condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(conditionRegIndex));
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
		case STMT_CODEBLOCK: {
			auto s = static_pointer_cast<CodeBlockStmtNode>(stmt);

			pushMajorContext();

			curFn->insertIns(Opcode::ENTER);
			++curMajorContext.curScopeLevel;

			for (auto i : s->body.stmts)
				compileStmt(i);

			--curMajorContext.curScopeLevel;
			curFn->insertIns(Opcode::LEAVE);

			popMajorContext();
			break;
		}
		default:
			throw logic_error("Invalid statement type");
	}
}
