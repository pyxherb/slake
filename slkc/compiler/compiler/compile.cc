#include "compile.hh"

#include <slkparse.hh>

#include "utils.hh"

using namespace Slake;
using namespace Compiler;

void Slake::Compiler::compileLeftExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing) {
	auto &fn = s->fnDefs[s->currentFn];
	assert(fn);
	switch (expr->getType()) {
		default:
			throw parser::syntax_error(expr->getLocation(), "Expected left value");
	}
}

void Compiler::compileRightExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing) {
	auto &fn = s->fnDefs[s->currentFn];
	assert(fn);
	switch (expr->getType()) {
		case ExprType::LITERAL: {
			fn->insertIns(Ins(Opcode::PUSH, { expr }));
			break;
		}
		case ExprType::TERNARY: {
			auto falseLabel = "$TOP_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$TOP_END_" + std::to_string(fn->body.size());

			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			compileRightExpr(e->condition, s);

			fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });
			compileRightExpr(e->x, s);	// True branch.
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });
			fn->insertLabel(falseLabel);
			compileRightExpr(e->y, s);	// False branch.
			fn->insertLabel(endLabel);
		}
		case ExprType::UNARY: {
			auto e = std::static_pointer_cast<UnaryOpExpr>(expr);
			Opcode opcode;
			if (!_unaryOp2opcodeMap.count(e->op))
				throw parser::syntax_error(expr->getLocation(), "Invalid operator detected");
			opcode = _unaryOp2opcodeMap.at(e->op);
			compileRightExpr(e->x, s);
			if (isSuffixUnaryOp(e->op)) {
				compileRightExpr(e->x, s);
			}
			fn->insertIns({ opcode, {} });
			break;
		}
		case ExprType::BINARY: {
			auto e = std::static_pointer_cast<BinaryOpExpr>(expr);
			compileRightExpr(e->y, s);
			compileRightExpr(e->x, s);

			Opcode opcode;
			if (_binaryOp2opcodeMap.count(e->op)) {
				opcode = _binaryOp2opcodeMap.at(e->op);
				fn->insertIns({ opcode, {} });
			} else if (e->op != BinaryOp::ASSIGN)
				throw parser::syntax_error(e->getLocation(), "Invalid operator detected");

			if (isAssignment(e->op)) {
				auto x = evalConstExpr(e->x, s);
				// Convert to direct operation if it is evaluatable.
				fn->insertIns({ Opcode::STORE, { x ? x : e->x } });
			}

			break;
		}
		case ExprType::CALL: {
			auto calle = std::static_pointer_cast<CallExpr>(expr);
			auto t = std::static_pointer_cast<FnTypeName>(evalExprType(s, calle->target));
			if (t->kind != TypeNameKind::FN)
				throw parser::syntax_error(calle->target->getLocation(), "Expression is not callable");

			Opcode opcode = Opcode::CALL;
			opcode = calle->isAsync ? Opcode::ACALL : Opcode::CALL;

			auto ce = evalConstExpr(calle->target, s);
			if (ce)
				fn->insertIns({ opcode, { ce } });
			else {
				for (auto &i : *(calle->args))
					compileRightExpr(i, s);
				compileRightExpr(calle->target, s);
				fn->insertIns({ opcode, {} });
			}

			break;
		}
		case ExprType::REF: {
			auto ref = std::static_pointer_cast<RefExpr>(expr);

			//
			// Local variables are unavailable if the flag was set.
			//
			if (!(isRecursing) && s->context.lvars.count(ref->name)) {
				fn->insertIns({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(ref->getLocation(), s->context.lvars[ref->name].stackPos) } });

				if (ref->next) {
					auto savedScope = s->scope;

					auto &lvarType = s->context.lvars[ref->name].type;
					if (lvarType->kind != TypeNameKind::CUSTOM)
						throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
					{
						auto t = Scope::getCustomType(std::static_pointer_cast<CustomTypeName>(lvarType));
						if (!t)
							throw parser::syntax_error(expr->getLocation(), "Type was not defined");
						if (!t->getScope())
							throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						s->scope = t->getScope();
					}
					compileRightExpr(ref->next, s, true);

					s->scope = savedScope;
				}
				break;
			}
			if (evalExprType(s, ref)) {
				bool isStatic = ref->isStatic;
				if (!isRecursing) {
					if (ref->name == "_lock")
						printf("");

					if ((!isStatic) && (!s->context.lvars.count(ref->name))) {
						auto v = s->scope->getVar(ref->name);
						if (v && v->accessModifier & ACCESS_STATIC)
							isStatic = true;
					}

					if (isStatic) {
						auto fullRef = s->scope->resolve();
						if (fullRef) {
							fullRef->next = ref;
							ref = fullRef;
						}
					} else if (!s->scope->parent.expired())
						ref = std::make_shared<RefExpr>(ref->getLocation(), "this", false, ref);
				}
				fn->insertIns({ isRecursing ? Opcode::RLOAD : Opcode::LOAD, { ref } });
				break;
			}
			throw parser::syntax_error(expr->getLocation(), "`" + ref->name + "' was undefined");
		}
		default:
			throw std::logic_error("Invalid expression type detected");
	}
}

void Compiler::compileStmt(std::shared_ptr<Stmt> src, std::shared_ptr<State> s) {
	assert(s->fnDefs.count(s->currentFn));
	auto &fn = s->fnDefs[s->currentFn];

	switch (src->getType()) {
		case StmtType::CODEBLOCK: {
			auto ctxt = s->context;

			auto stmt = std::static_pointer_cast<CodeBlock>(src);
			auto state = std::make_shared<State>();
			state->scope = s->scope;

			std::string endLabel = "$BLK_" + std::to_string(fn->body.size());
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->ins)
				compileStmt(i, s);

			fn->insertLabel(endLabel);

			fn->insertIns({ Opcode::LEAVE, {} });

			ctxt.returned = s->context.returned;
			s->context = ctxt;
			break;
		}
		case StmtType::IF: {
			auto stmt = std::static_pointer_cast<IfStmt>(src);
			compileRightExpr(stmt->condition, s);

			auto falseLabel = "$IF_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$IF_END_" + std::to_string(fn->body.size());

			compileRightExpr(stmt->condition, s);

			fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });

			// True branch.
			compileStmt(stmt->thenBlock, s);
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });

			// False branch.
			fn->insertLabel(falseLabel);
			compileStmt(stmt->elseBlock, s);

			fn->insertLabel(endLabel);
			break;
		}
		case StmtType::FOR: {
			auto ctxt = s->context;

			auto stmt = std::static_pointer_cast<ForStmt>(src);
			compileRightExpr(stmt->condition, s);

			auto conditionLabel = "$FOR_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$FOR_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$FOR_END_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->varDecl->declList) {
				s->context.lvars[i->name] = LocalVar(s->context.stackCur, stmt->varDecl->typeName);
			}

			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileRightExpr(stmt->condition, s);
			fn->insertIns({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });

			fn->insertLabel(endLabel);
			fn->insertIns({ Opcode::LEAVE, {} });
			s->leaveLoop();

			s->context = ctxt;
			break;
		}
		case StmtType::WHILE: {
			auto stmt = std::static_pointer_cast<WhileStmt>(src);
			auto conditionLabel = "$WHILE_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$WHILE_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$WHILE_END_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileRightExpr(stmt->condition, s);
			fn->insertIns({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });
			s->leaveLoop();
			break;
		}
		case StmtType::TIMES: {
			auto stmt = std::static_pointer_cast<TimesStmt>(src);
			auto conditionLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$TIMES_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 counterName = "$TIMES_CNT_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->insertIns({ Opcode::ENTER, {} });

			s->context.lvars[counterName] = LocalVar(s->context.stackCur, evalExprType(s, stmt->timesExpr));
			compileRightExpr(stmt->timesExpr, s);

			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileRightExpr(std::make_shared<RefExpr>(stmt->getLocation(), counterName, false), s);
			fn->insertIns({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });

			fn->insertLabel(endLabel);
			fn->insertIns({ Opcode::LEAVE, {} });
			s->leaveLoop();
			break;
		}
		case StmtType::EXPR: {
			auto stmt = std::static_pointer_cast<ExprStmt>(src);
			for (auto &i = stmt; i; i = i->next) {
				compileRightExpr(stmt->expr, s);
				SlxFmt::InsHeader ih = { Opcode::POP, 0 };
			}
			break;
		}
		case StmtType::CONTINUE: {
			auto stmt = std::static_pointer_cast<ContinueStmt>(src);
			if (!s->context.nContinueLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected continue statement");
			fn->insertIns({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::BREAK: {
			auto stmt = std::static_pointer_cast<ContinueStmt>(src);
			if (!s->context.nBreakLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected break statement");
			fn->insertIns({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::RETURN: {
			auto stmt = std::static_pointer_cast<ReturnStmt>(src);

			if (!stmt->expr) {
				if (s->scope->fnDefs[s->currentFn]->returnTypeName->kind != TypeNameKind::NONE)
					throw parser::syntax_error(stmt->expr->getLocation(), "Missing return value");
			} else if (s->scope->fnDefs[s->currentFn]->returnTypeName->kind == TypeNameKind::NONE)
				throw parser::syntax_error(stmt->expr->getLocation(), "Unexpected return value");

			auto type = evalExprType(s, stmt->expr);
			if ((!isConvertible(s->scope->fnDefs[s->currentFn]->returnTypeName, type)))
				throw parser::syntax_error(stmt->expr->getLocation(), "Incompatible expression type");

			auto e = evalConstExpr(stmt->expr, s);
			if (e) {
				fn->insertIns(Ins{ Opcode::RET, { e } });
			} else {
				compileRightExpr(stmt->expr, s);
				fn->insertIns(Ins{ Opcode::RET, {} });
			}

			s->context.returned = true;
			break;
		}
		case StmtType::SWITCH: {
			auto stmt = std::static_pointer_cast<SwitchStmt>(src);
			auto conditionName = "$SW_COND_" + std::to_string(fn->body.size()),
				 endLabel = "$SW_END_" + std::to_string(fn->body.size());

			compileRightExpr(stmt->condition, s);
			s->context.lvars[conditionName] = LocalVar(s->context.stackCur, evalExprType(s, stmt->condition));

			s->enterSwitch();
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			{
				std::size_t j = 0;
				for (auto &i : *stmt->caseList) {
					auto caseEndLabel =
						"$SW_" +
						std::to_string(stmt->getLocation().begin.line) + "_" +
						std::to_string(stmt->getLocation().begin.column) + "_" +
						"END_ " + std::to_string(j);

					compileRightExpr(i->condition, s);
					fn->insertIns({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(i->getLocation(), s->context.lvars[conditionName].stackPos) } });
					fn->insertIns({ Opcode::EQ, {} });
					fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(caseEndLabel) } });

					compileStmt(i->x, s);
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
			s->leaveSwitch();
			break;
		}
		case StmtType::VAR_DEF: {
			auto stmt = std::static_pointer_cast<VarDefStmt>(src);
			if (stmt->accessModifier & (~(ACCESS_CONST | ACCESS_VOLATILE | ACCESS_NATIVE)))
				throw parser::syntax_error(stmt->getLocation(), "Invalid modifier combination");

			if (stmt->typeName->kind == TypeNameKind::CUSTOM) {
				auto t = std::static_pointer_cast<CustomTypeName>(stmt->typeName);
				auto type = s->scope->getType(t->typeRef);
			}
			for (auto &i : stmt->declList) {
				s->context.lvars[stmt->declList[i->name]->name] = LocalVar((std::uint32_t)(s->context.stackCur++), stmt->typeName);
				if (i->initValue) {
					auto expr = evalConstExpr(i->initValue, s);
					if (!expr)
						fn->insertIns(Ins(Opcode::PUSH, { expr }));
					else
						compileRightExpr(i->initValue, s);
				} else if (stmt->accessModifier & ACCESS_CONST)
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

void writeTypeName(std::shared_ptr<State> s, std::fstream &fs, std::shared_ptr<TypeName> j) {
	_writeValue(_tnKind2vtMap.at(j->kind), fs);
	switch (j->kind) {
		case TypeNameKind::ARRAY: {
			auto tn = std::static_pointer_cast<ArrayTypeName>(j);
			writeTypeName(s, fs, tn->type);
			break;
		}
		case TypeNameKind::MAP: {
			auto tn = std::static_pointer_cast<MapTypeName>(j);
			writeTypeName(s, fs, tn->keyType);
			writeTypeName(s, fs, tn->valueType);
			break;
		}
		case TypeNameKind::CUSTOM: {
			auto tn = std::static_pointer_cast<CustomTypeName>(j);
			auto t = Scope::getCustomType(tn);
			if (!t)
				throw parser::syntax_error(tn->getLocation(), "Type `" + std::to_string(*tn) + "' was not defined");
			// ! FIXME
			writeValue(s, tn->typeRef, fs);
			break;
		}
	}
}

void Compiler::compile(std::shared_ptr<Scope> scope, std::fstream &fs, bool isTopLevel) {
	auto s = std::make_shared<State>();
	s->scope = scope;

	if (isTopLevel) {
		SlxFmt::ImgHeader ih = { 0 };
		std::memcpy(ih.magic, SlxFmt::IMH_MAGIC, sizeof(ih.magic));
		ih.nImports = scope->imports.size();
		ih.fmtVer = 0;
		fs.write((char *)&ih, sizeof(ih));

		for (auto i : scope->imports) {
			_writeValue((std::uint32_t)(i.first.length()), fs);
			_writeValue(*(i.first.c_str()), (std::streamsize)i.first.length(), fs);
			writeValue(s, i.second, fs);
		}
	}

	//
	// Write value descriptors (VAD).
	//
	{
		for (auto &i : scope->vars) {
			SlxFmt::VarDesc vad = { 0 };
			vad.lenName = i.first.length();

			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC | ACCESS_NATIVE))
				throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (i.second->accessModifier & ACCESS_PUB)
				vad.flags |= SlxFmt::VAD_PUB;
			if (i.second->accessModifier & ACCESS_FINAL)
				vad.flags |= SlxFmt::VAD_FINAL;
			if (i.second->accessModifier & ACCESS_STATIC)
				vad.flags |= SlxFmt::VAD_STATIC;
			if (i.second->accessModifier & ACCESS_NATIVE)
				vad.flags |= SlxFmt::VAD_NATIVE;

			if (i.second->initValue)
				vad.flags |= SlxFmt::VAD_INIT;

			fs.write((char *)&vad, sizeof(vad));
			fs.write(i.first.c_str(), i.first.length());

			writeTypeName(s, fs, i.second->typeName);

			if (i.second->initValue) {
				auto e = evalConstExpr(i.second->initValue, s);
				if (!e)
					throw parser::syntax_error(i.second->initValue->getLocation(), "Expression cannot be evaluated in compile-time");
				writeValue(s, e, fs);
			}
		}
		{
			SlxFmt::VarDesc vad = { 0 };
			fs.write((char *)&vad, sizeof(vad));
		}
	}

	// Compile and write functions.
	{
		for (auto &i : scope->fnDefs) {
			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE | ACCESS_NATIVE))
				throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (isTopLevel)
				if (i.second->accessModifier & (ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE | ACCESS_NATIVE))
					throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

			s->currentFn = i.first;						  // Set up current function name of the state.
			s->scope = scope;							  // Set up scope of the state.
			s->fnDefs[i.first] = std::make_shared<Fn>();  // Create a new, empty function.

			// Compile the function if is a definition.
			if (i.second->execBlock) {
				auto ctxt = s->context;
				for (auto &j : *(i.second->params))
					s->context.lvars[j->name] = LocalVar((std::uint32_t)(s->context.stackCur++), j->typeName);
				compileStmt(i.second->execBlock, s);
				ctxt.returned = s->context.returned;

				// Check if current branch has any return statement.
				if (!s->context.returned) {
					if (i.second->returnTypeName->kind != TypeNameKind::NONE)
						throw parser::syntax_error(location(i.second->execBlock->getLocation().end), "Must return a value");
					s->fnDefs[i.first]->insertIns(Ins(Opcode::RET, { std::make_shared<NullLiteralExpr>(i.second->getLocation()) }));
				}
				s->context = ctxt;
			}

			// Write the function descriptor (FND).
			{
				SlxFmt::FnDesc fnd = { 0 };
				if (i.second->accessModifier & ACCESS_PUB)
					fnd.flags |= SlxFmt::FND_PUB;
				if (i.second->accessModifier & ACCESS_FINAL)
					fnd.flags |= SlxFmt::FND_FINAL;
				if (i.second->accessModifier & ACCESS_STATIC)
					fnd.flags |= SlxFmt::FND_STATIC;
				if (i.second->accessModifier & ACCESS_OVERRIDE)
					fnd.flags |= SlxFmt::FND_OVERRIDE;
				if (i.second->accessModifier & ACCESS_NATIVE)
					fnd.flags |= SlxFmt::FND_NATIVE;
				fnd.lenName = i.first.length();
				fnd.lenBody = s->fnDefs[s->currentFn]->body.size();
				fnd.nParams = i.second->params->size();
				_writeValue(fnd, fs);
				_writeValue(*(i.first.c_str()), i.first.length(), fs);
			}

			for (auto j : i.second->genericParams) {
				_writeValue((std::uint32_t)j.size(), fs);
				_writeValue(*(j.c_str()), (std::streamsize)j.size(), fs);
			}

			writeTypeName(s, fs, i.second->returnTypeName);

			for (auto j : *(i.second->params))
				writeTypeName(s, fs, j->typeName);

			// Write for each instructions.
			for (auto &k : s->fnDefs[s->currentFn]->body) {
				SlxFmt::InsHeader ih(k.opcode, k.operands.size());
				_writeValue(ih, fs);
				for (auto &l : k.operands) {
					if (l->getType() == ExprType::LABEL) {
						writeValue(
							s,
							std::make_shared<UIntLiteralExpr>(l->getLocation(),
								s->fnDefs[s->currentFn]->labels[std::static_pointer_cast<LabelExpr>(l)->label]),
							fs);
					} else
						writeValue(s, l, fs);
				}
			}
		}
		SlxFmt::FnDesc fnd = { 0 };
		_writeValue(fnd, fs);
	}

	// Write CTD for each class/trait.
	{
		for (auto i : scope->types) {
			switch (i.second->getKind()) {
				case Type::Kind::CLASS: {
					auto t = std::static_pointer_cast<ClassType>(i.second);
					SlxFmt::ClassTypeDesc ctd = { 0 };

					if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL))
						throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

					if (i.second->accessModifier & ACCESS_PUB)
						ctd.flags |= SlxFmt::CTD_PUB;
					if (i.second->accessModifier & ACCESS_FINAL)
						ctd.flags |= SlxFmt::CTD_FINAL;

					ctd.lenName = i.first.length();
					ctd.nImpls = t->impls->impls.size();
					ctd.nGenericParams = t->genericParams.size();
					_writeValue(ctd, fs);
					_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

					// Write reference to the parent class which is derived by it.
					if (t->parent) {
						ctd.flags |= SlxFmt::CTD_DERIVED;
						if (t->parent->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(t->parent);
						writeTypeName(s, fs, tn);
					}

					// Write references to implemented interfaces.
					for (auto &j : t->impls->impls) {
						if (j->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(j);
						writeTypeName(s, fs, tn);
					}

					compile(std::static_pointer_cast<ClassType>(i.second)->scope, fs, false);
					break;
				}
				case Type::Kind::TRAIT: {
					auto t = std::static_pointer_cast<TraitType>(i.second);
					SlxFmt::ClassTypeDesc ctd = { 0 };
					if (t->accessModifier & ~ACCESS_PUB)
						throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
					ctd.flags |= SlxFmt::CTD_TRAIT | t->accessModifier & ACCESS_PUB ? SlxFmt::CTD_PUB : 0;
					ctd.lenName = i.first.length();
					ctd.nGenericParams = t->genericParams.size();
					_writeValue(ctd, fs);
					_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

					if (t->parent) {
						ctd.flags |= SlxFmt::CTD_DERIVED;
						if (t->parent->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(t->parent);
						writeValue(s, tn->typeRef, fs);
					}

					compile(std::static_pointer_cast<TraitType>(i.second)->scope, fs, false);
					break;
				}
			}
		}
		SlxFmt::ClassTypeDesc ctd = { 0 };
		_writeValue(ctd, fs);
	}

	// Write STD for each structure.
	{
		for (auto i : scope->types) {
			if (i.second->getKind() != Type::Kind::STRUCT)
				continue;
			auto t = std::static_pointer_cast<StructType>(i.second);
			SlxFmt::StructTypeDesc std = { 0 };
			std.nMembers = t->vars.size();
			std.lenName = i.first.size();
			fs.write((char *)&std, sizeof(std));
			_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

			for (auto j : t->varIndices) {
				SlxFmt::StructMemberDesc smd = {};
				auto item = t->vars[j.second];
				if (item->typeName->kind == TypeNameKind::CUSTOM)
					throw parser::syntax_error(item->typeName->getLocation(), "Non-literal types are not acceptable");
				smd.type = _tnKind2vtMap.at(item->typeName->kind);
				smd.lenName = j.first.size();
				_writeValue(smd, fs);
				_writeValue(*(j.first.c_str()), (std::streamsize)j.first.size(), fs);
			}
		}
		SlxFmt::StructTypeDesc std = { 0 };
		fs.write((char *)&std, sizeof(std));
	}
}
