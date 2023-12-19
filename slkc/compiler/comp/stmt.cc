#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileStmt(shared_ptr<StmtNode> stmt) {
	switch (stmt->getStmtType()) {
		case STMT_EXPR: {
			pushContext();

			context.evalPurpose = EvalPurpose::STMT;
			compileExpr(static_pointer_cast<ExprStmtNode>(stmt)->expr);

			popContext();
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
				if (context.localVars.count(i.first))
					throw FatalCompilationError(
						{ i.second.loc,
							MSG_ERROR,
							"Redefinition of local variable `" + i.first + "'" });

				uint32_t index = (uint32_t)context.localVars.size();

				auto initValueType = evalExprType(i.second.initValue);

				context.curFn->insertIns(Opcode::LVAR, s->type);
				if (i.second.initValue) {
					if (!isSameType(s->type, initValueType)) {
						if (!areTypesConvertible(initValueType, s->type))
							throw FatalCompilationError(
								{ i.second.initValue->getLocation(),
									MSG_ERROR,
									"Incompatible initial value type" });

						compileExpr(make_shared<CastExprNode>(i.second.initValue->getLocation(), s->type, i.second.initValue), EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP1));
					} else
						compileExpr(i.second.initValue, EvalPurpose::RVALUE, make_shared<LocalVarRefNode>(index));
				}

				context.localVars[i.first] = make_shared<LocalVarNode>(index, s->type);
			}

			break;
		}
		case STMT_BREAK:
			if (!context.breakLabel.size())
				throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Unexpected break statement" });
			if (context.breakScopeLevel < context.curScopeLevel)
				context.curFn->insertIns(
					Opcode::LEAVE,
					make_shared<U32LiteralExprNode>(stmt->getLocation(), context.curScopeLevel - context.breakScopeLevel));
			context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(context.breakLabel));
			break;
		case STMT_CONTINUE:
			if (!context.continueLabel.size())
				throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Unexpected continue statement" });
			context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(context.continueLabel));
			break;
		case STMT_FOR: {
			auto s = static_pointer_cast<ForStmtNode>(stmt);

			auto loc = s->getLocation();
			string beginLabel = "$for_" + to_string(loc.line) + "_" + to_string(loc.column) + "_begin",
				   endLabel = "$for_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			pushContext();

			context.breakScopeLevel = context.curScopeLevel;
			context.continueScopeLevel = context.curScopeLevel;
			context.breakLabel = endLabel;
			context.continueLabel = beginLabel;

			context.curFn->insertIns(Opcode::ENTER);
			compileStmt(s->varDefs);

			context.curFn->insertLabel(beginLabel);

			if (s->condition) {
				compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
				if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
					context.curFn->insertIns(
						Opcode::CAST,
						make_shared<RegRefNode>(RegId::TMP0),
						make_shared<BoolTypeNameNode>(s->condition->getLocation(), true), make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(endLabel), make_shared<RegRefNode>(RegId::TMP0));
			} else {
				context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			compileStmt(s->body);

			compileExpr(s->endExpr, EvalPurpose::STMT, {});

			context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(beginLabel));

			context.curFn->insertLabel(endLabel);
			context.curFn->insertIns(Opcode::LEAVE);

			popContext();
			break;
		}
		case STMT_WHILE: {
			auto s = static_pointer_cast<WhileStmtNode>(stmt);

			auto loc = s->getLocation();
			string beginLabel = "$while_" + to_string(loc.line) + "_" + to_string(loc.column) + "_begin",
				   endLabel = "$while_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			pushContext();

			context.breakScopeLevel = context.curScopeLevel;
			context.continueScopeLevel = context.curScopeLevel;
			context.breakLabel = endLabel;
			context.continueLabel = beginLabel;

			context.curFn->insertLabel(beginLabel);

			compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
			if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
				context.curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(RegId::TMP0),
					make_shared<BoolTypeNameNode>(s->condition->getLocation(), true), make_shared<RegRefNode>(RegId::TMP0));

			compileStmt(s->body);

			context.curFn->insertLabel(endLabel);

			compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
			if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
				context.curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(RegId::TMP0),
					make_shared<BoolTypeNameNode>(s->condition->getLocation(), true), make_shared<RegRefNode>(RegId::TMP0));
			context.curFn->insertIns(Opcode::JT, make_shared<LabelRefNode>(beginLabel), make_shared<RegRefNode>(RegId::TMP0));

			popContext();

			break;
		}
		case STMT_RETURN: {
			auto s = static_pointer_cast<ReturnStmtNode>(stmt);

			auto returnType = context.curFn->returnType;

			if (!s->returnValue) {
				if (returnType->getTypeId() != TYPE_VOID)
					throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Must return a value" });
				else
					context.curFn->insertIns(Opcode::RET, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					if (isSameType(evalExprType(e), returnType)) {
						context.curFn->insertIns(Opcode::RET, e);
					} else {
						if (auto ce = castLiteralExpr(e, returnType->getTypeId()); ce) {
							context.curFn->insertIns(Opcode::RET, ce);
						} else {
							compileExpr(make_shared<CastExprNode>(e->getLocation(), returnType, e), EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
							context.curFn->insertIns(Opcode::RET, make_shared<RegRefNode>(RegId::TMP0));
						}
					}
				} else {
					if (isSameType(evalExprType(s->returnValue), returnType))
						compileExpr(s->returnValue, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
					else
						compileExpr(make_shared<CastExprNode>(s->returnValue->getLocation(), returnType, s->returnValue), EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));

					context.curFn->insertIns(Opcode::RET, make_shared<RegRefNode>(RegId::TMP0));
				}
			}

			break;
		}
		case STMT_YIELD: {
			auto s = static_pointer_cast<YieldStmtNode>(stmt);

			if (!s->returnValue) {
				if (context.curFn->returnType->getTypeId() != TYPE_VOID)
					throw FatalCompilationError({ stmt->getLocation(), MSG_ERROR, "Must yield a value" });
				else
					context.curFn->insertIns(Opcode::YIELD, {});
			} else {
				if (auto e = evalConstExpr(s->returnValue); e) {
					context.curFn->insertIns(Opcode::YIELD, e);
				} else {
					compileExpr(s->returnValue, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
					context.curFn->insertIns(Opcode::YIELD, make_shared<RegRefNode>(RegId::TMP0));
				}
			}

			break;
		}
		case STMT_IF: {
			auto s = static_pointer_cast<IfStmtNode>(stmt);
			auto loc = s->getLocation();

			string endLabel = endLabel = "$if_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			compileExpr(s->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
			if (evalExprType(s->condition)->getTypeId() != TYPE_BOOL)
				context.curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(RegId::TMP0),
					make_shared<BoolTypeNameNode>(s->getLocation(), true), make_shared<RegRefNode>(RegId::TMP0));

			context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(endLabel), make_shared<RegRefNode>(RegId::TMP0));

			compileStmt(s->body);

			if (s->elseBranch)
				compileStmt(s->elseBranch);

			context.curFn->insertLabel(endLabel);

			break;
		}
		case STMT_TRY: {
			auto s = static_pointer_cast<TryStmtNode>(stmt);
			auto loc = s->getLocation();

			string labelPrefix = "$try_" + to_string(loc.line) + "_" + to_string(loc.column),
				   endLabel = labelPrefix + "_final";

			context.curFn->insertIns(Opcode::ENTER);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i)
				context.curFn->insertIns(
					Opcode::PUSHXH,
					s->catchBlocks[i].targetType,
					make_shared<LabelRefNode>(labelPrefix + "_xh_" + to_string(i)));

			compileStmt(s->body);

			for (size_t i = 0; i < s->catchBlocks.size(); ++i) {
				pushContext();

				auto &curBlock = s->catchBlocks[i];
				context.curFn->insertLabel(labelPrefix + "_xh_" + to_string(i));

				if (curBlock.exceptionVarName.size()) {
					context.curFn->insertIns(Opcode::ENTER);

					auto exceptionVar = (context.localVars[curBlock.exceptionVarName] = make_shared<LocalVarNode>(
											 context.localVars.size(),
											 curBlock.targetType));
					context.curFn->insertIns(Opcode::LVAR, curBlock.targetType);
					context.curFn->insertIns(Opcode::LEXCEPT, make_shared<LocalVarRefNode>(exceptionVar->index));
				}

				compileStmt(curBlock.body);

				if (curBlock.exceptionVarName.size())
					context.curFn->insertIns(Opcode::LEAVE);

				context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

				popContext();
			}

			context.curFn->insertIns(Opcode::LEAVE);

			context.curFn->insertLabel(endLabel);

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

			pushContext();

			context.breakScopeLevel = context.curScopeLevel;
			context.breakLabel = endLabel;

			bool preserveR2 = preserveRegister(RegId::R2);

			compileExpr(s->expr, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R2));

			SwitchCase *defaultCase = nullptr;

			for (size_t i = 0; i < s->cases.size(); ++i) {
				auto &curCase = s->cases[i];
				string caseBeginLabel = labelPrefix + "_case" + to_string(i) + "_end",
					   caseEndLabel = labelPrefix + "_case" + to_string(i) + "_end";

				context.curFn->insertLabel(caseBeginLabel);

				if (!curCase.condition) {
					if (defaultCase)
						// The default case is already exist.
						throw FatalCompilationError(
							{ curCase.loc,
								MSG_ERROR,
								"Duplicated default case" });
					defaultCase = &curCase;
				}

				bool preserveR1 = preserveRegister(RegId::R1);

				compileExpr(curCase.condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R1));
				context.curFn->insertIns(Opcode::EQ, make_shared<RegRefNode>(RegId::TMP0), make_shared<RegRefNode>(RegId::R2), make_shared<RegRefNode>(RegId::R1));
				context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(caseEndLabel), make_shared<RegRefNode>(RegId::TMP0));

				compileStmt(make_shared<BlockStmtNode>(curCase.loc, curCase.body));

				context.curFn->insertLabel(caseEndLabel);

				if (i + 1 < s->cases.size())
					context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(labelPrefix + "_case" + to_string(i + 1) + "_end"));
			}

			if (defaultCase)
				compileStmt(make_shared<BlockStmtNode>(defaultCase->loc, defaultCase->body));

			context.curFn->insertLabel(endLabel);

			if (preserveR2)
				restoreRegister(RegId::R2);

			popContext();
			break;
		}
		case STMT_CODEBLOCK: {
			auto s = static_pointer_cast<CodeBlockStmtNode>(stmt);

			pushContext();

			context.curFn->insertIns(Opcode::ENTER);
			++context.curScopeLevel;

			for (auto i : s->body.stmts)
				compileStmt(i);

			--context.curScopeLevel;
			context.curFn->insertIns(Opcode::LEAVE);

			popContext();
			break;
		}
		default:
			throw logic_error("Invalid statement type");
	}
}
