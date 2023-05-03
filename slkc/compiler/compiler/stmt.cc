#include <slkparse.hh>

#include "misc.hh"

using namespace Slake;
using namespace Slake::Compiler;

void State::compileStmt(std::shared_ptr<Stmt> src) {
	assert(fnDefs.count(currentFn));
	auto &fn = fnDefs[currentFn];

	switch (src->getType()) {
		case StmtType::CODEBLOCK: {
			auto ctxt = context;

			auto stmt = std::static_pointer_cast<CodeBlock>(src);
			auto state = std::make_shared<State>();
			state->scope = scope;

			std::string endLabel = "$BLK_" + std::to_string(fn->body.size());
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->ins)
				compileStmt(i);

			fn->insertLabel(endLabel);

			fn->insertIns({ Opcode::LEAVE, {} });

			ctxt.returned = context.returned;
			context = ctxt;
			break;
		}
		case StmtType::IF: {
			auto stmt = std::static_pointer_cast<IfStmt>(src);
			compileRightExpr(stmt->condition);

			auto falseLabel = "$IF_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$IF_END_" + std::to_string(fn->body.size());

			compileRightExpr(stmt->condition);

			fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });

			// True branch.
			compileStmt(stmt->thenBlock);
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });

			// False branch.
			fn->insertLabel(falseLabel);
			compileStmt(stmt->elseBlock);

			fn->insertLabel(endLabel);
			break;
		}
		case StmtType::FOR: {
			auto ctxt = context;

			auto stmt = std::static_pointer_cast<ForStmt>(src);
			compileRightExpr(stmt->condition);

			auto conditionLabel = "$FOR_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$FOR_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$FOR_END_" + std::to_string(fn->body.size());

			enterLoop();
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->varDecl->declList) {
				context.lvars[i->name] = LocalVar(context.stackCur, stmt->varDecl->typeName);
			}

			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock);

			fn->insertLabel(conditionLabel);
			compileRightExpr(stmt->condition);
			fn->insertIns({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });

			fn->insertLabel(endLabel);
			fn->insertIns({ Opcode::LEAVE, {} });
			leaveLoop();

			context = ctxt;
			break;
		}
		case StmtType::WHILE: {
			auto stmt = std::static_pointer_cast<WhileStmt>(src);
			auto conditionLabel = "$WHILE_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$WHILE_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$WHILE_END_" + std::to_string(fn->body.size());

			enterLoop();
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock);

			fn->insertLabel(conditionLabel);
			compileRightExpr(stmt->condition);
			fn->insertIns({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });
			leaveLoop();
			break;
		}
		case StmtType::TIMES: {
			auto stmt = std::static_pointer_cast<TimesStmt>(src);
			auto conditionLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$TIMES_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 counterName = "$TIMES_CNT_" + std::to_string(fn->body.size());

			enterLoop();
			fn->insertIns({ Opcode::ENTER, {} });

			context.lvars[counterName] = LocalVar(context.stackCur, evalExprType(stmt->timesExpr));
			compileRightExpr(stmt->timesExpr);

			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock);

			fn->insertLabel(conditionLabel);
			compileRightExpr(std::make_shared<RefExpr>(stmt->getLocation(), counterName, false));
			fn->insertIns({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });

			fn->insertLabel(endLabel);
			fn->insertIns({ Opcode::LEAVE, {} });
			leaveLoop();
			break;
		}
		case StmtType::EXPR: {
			auto stmt = std::static_pointer_cast<ExprStmt>(src);
			for (auto &i = stmt; i; i = i->next) {
				compileRightExpr(stmt->expr);
				fn->insertIns({ Opcode::POP, {} });
			}
			break;
		}
		case StmtType::CONTINUE: {
			auto stmt = std::static_pointer_cast<ContinueStmt>(src);
			if (!context.nContinueLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected continue statement");
			fn->insertIns({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::BREAK: {
			auto stmt = std::static_pointer_cast<ContinueStmt>(src);
			if (!context.nBreakLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected break statement");
			fn->insertIns({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::RETURN: {
			auto stmt = std::static_pointer_cast<ReturnStmt>(src);

			if (!stmt->expr) {
				if (scope->fnDefs[currentFn]->returnTypeName->kind != TypeNameKind::NONE)
					throw parser::syntax_error(stmt->expr->getLocation(), "Missing return value");
			} else if (scope->fnDefs[currentFn]->returnTypeName->kind == TypeNameKind::NONE)
				throw parser::syntax_error(stmt->expr->getLocation(), "Unexpected return value");

			auto type = evalExprType(stmt->expr);
			if ((!isConvertible(scope->fnDefs[currentFn]->returnTypeName, type)))
				throw parser::syntax_error(stmt->expr->getLocation(), "Incompatible expression type");

			auto e = evalConstExpr(stmt->expr);
			if (e) {
				fn->insertIns(Ins{ Opcode::RET, { e } });
			} else {
				compileRightExpr(stmt->expr);
				fn->insertIns(Ins{ Opcode::RET, {} });
			}

			context.returned = true;
			break;
		}
		case StmtType::SWITCH: {
			auto stmt = std::static_pointer_cast<SwitchStmt>(src);
			auto conditionName = "$SW_COND_" + std::to_string(fn->body.size()),
				 endLabel = "$SW_END_" + std::to_string(fn->body.size());

			compileRightExpr(stmt->condition);
			context.lvars[conditionName] = LocalVar(context.stackCur, evalExprType(stmt->condition));

			enterSwitch();
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			{
				std::size_t j = 0;
				for (auto &i : *stmt->caseList) {
					auto caseEndLabel =
						"$SW_" +
						std::to_string(stmt->getLocation().begin.line) + "_" +
						std::to_string(stmt->getLocation().begin.column) + "_" +
						"END_ " + std::to_string(j);

					compileRightExpr(i->condition);
					fn->insertIns({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(i->getLocation(), context.lvars[conditionName].stackPos) } });
					fn->insertIns({ Opcode::EQ, {} });
					fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(caseEndLabel) } });

					compileStmt(i->x);
					fn->insertIns(
						{ Opcode::JMP,
							{ std::make_shared<LabelExpr>("$SW_" +
														  std::to_string(stmt->getLocation().begin.line) + "_" +
														  std::to_string(stmt->getLocation().begin.column) + "_" +
														  "END_ " + std::to_string(++j)) } });

					fn->insertLabel(caseEndLabel);
				}
			}

			fn->insertIns({ Opcode::LEAVE, {} });
			leaveSwitch();
			break;
		}
		case StmtType::VAR_DEF: {
			auto stmt = std::static_pointer_cast<VarDefStmt>(src);
			if (stmt->accessModifier & (~(ACCESS_CONST | ACCESS_VOLATILE | ACCESS_NATIVE)))
				throw parser::syntax_error(stmt->getLocation(), "Invalid modifier combination");

			if (stmt->typeName->kind == TypeNameKind::CUSTOM) {
				auto t = std::static_pointer_cast<CustomTypeName>(stmt->typeName);
				auto type = t->resolveType();
			}
			for (auto &i : stmt->declList) {
				context.lvars[stmt->declList[i->name]->name] = LocalVar((std::uint32_t)(context.stackCur++), stmt->typeName);
				if (i->initValue) {
					auto expr = evalConstExpr(i->initValue);
					if (expr)
						fn->insertIns(Ins(Opcode::PUSH, { expr }));
					else
						compileRightExpr(i->initValue);
				} else if (stmt->isConst())
					throw parser::syntax_error(i->getLocation(), "Constants must be initialized");
				else
					fn->insertIns(Ins(Opcode::PUSH, { std::make_shared<NullLiteralExpr>(stmt->getLocation()) }));
			}
			break;
		}
		default:
			throw std::logic_error("Invalid statement type detected");
	}
}
