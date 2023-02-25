#include <slkparse.hh>

using namespace Slake;
using namespace Slake::Compiler;

template <typename T>
static void _writeValue(const T &value, std::fstream &fs) {
	fs.write((char *)&value, sizeof(value));
}
template <typename T>
static void _writeValue(const T &&value, std::fstream &fs) {
	T v = value;
	_writeValue(v, fs);
}
template <typename T>
static void _writeValue(const T &value, std::streamsize size, std::fstream &fs) {
	fs.write((char *)&value, size);
}

static std::unordered_map<LiteralType, Slake::SlxFmt::ValueType> _lt2vtMap = {
	{ Compiler::LT_INT, SlxFmt::ValueType::I32 },
	{ Compiler::LT_UINT, SlxFmt::ValueType::U32 },
	{ Compiler::LT_LONG, SlxFmt::ValueType::I64 },
	{ Compiler::LT_ULONG, SlxFmt::ValueType::U64 },
	{ Compiler::LT_FLOAT, SlxFmt::ValueType::FLOAT },
	{ Compiler::LT_DOUBLE, SlxFmt::ValueType::DOUBLE },
	{ Compiler::LT_BOOL, SlxFmt::ValueType::BOOL },
	{ Compiler::LT_STRING, SlxFmt::ValueType::STRING },
	{ Compiler::LT_NULL, SlxFmt::ValueType::NONE },
	{ Compiler::LT_UUID, SlxFmt::ValueType::UUID }
};

static std::unordered_map<LiteralType, TypeNameKind> _lt2tnKindMap = {
	{ Compiler::LT_INT, TypeNameKind::I32 },
	{ Compiler::LT_UINT, TypeNameKind::U32 },
	{ Compiler::LT_LONG, TypeNameKind::I64 },
	{ Compiler::LT_ULONG, TypeNameKind::U64 },
	{ Compiler::LT_FLOAT, TypeNameKind::FLOAT },
	{ Compiler::LT_DOUBLE, TypeNameKind::DOUBLE },
	{ Compiler::LT_BOOL, TypeNameKind::BOOL },
	{ Compiler::LT_STRING, TypeNameKind::STRING },
	{ Compiler::LT_NULL, TypeNameKind::NONE }
};

static void writeValueDesc(std::shared_ptr<Compiler::Expr> src, std::fstream &fs) {
	SlxFmt::ValueDesc vd = {};
	switch (src->getType()) {
		case Compiler::ExprType::LITERAL: {
			auto literalExpr = std::static_pointer_cast<Compiler::LiteralExpr>(src);
			vd.type = _lt2vtMap.at(literalExpr->getLiteralType());
			_writeValue(vd, fs);
			switch (literalExpr->getLiteralType()) {
				case Compiler::LT_INT: {
					auto expr = std::static_pointer_cast<Compiler::IntLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_UINT: {
					auto expr = std::static_pointer_cast<Compiler::UIntLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_LONG: {
					auto expr = std::static_pointer_cast<Compiler::LongLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_ULONG: {
					auto expr = std::static_pointer_cast<Compiler::ULongLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_FLOAT: {
					auto expr = std::static_pointer_cast<Compiler::FloatLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_DOUBLE: {
					auto expr = std::static_pointer_cast<Compiler::DoubleLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_BOOL: {
					auto expr = std::static_pointer_cast<Compiler::BoolLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case Compiler::LT_STRING: {
					auto expr = std::static_pointer_cast<Compiler::StringLiteralExpr>(literalExpr);
					_writeValue(expr->data.size(), fs);
					_writeValue(expr->data.c_str(), (std::streamsize)expr->data.size(), fs);
					break;
				}
				case Compiler::LT_UUID: {
					auto expr = std::static_pointer_cast<Compiler::UUIDLiteralExpr>(literalExpr);
					_writeValue(expr->data.timeLow, fs);
					_writeValue(expr->data.timeMid, fs);
					_writeValue(expr->data.timeHiAndVer, fs);
					_writeValue(expr->data.clockSeqLow, fs);
					_writeValue(expr->data.clockSeqHiAndReserved, fs);
					_writeValue(expr->data.node, fs);
					break;
				}
			}
			break;
		}
		case Compiler::ExprType::REF: {
			auto expr = std::static_pointer_cast<Compiler::RefExpr>(src);
			vd.type = SlxFmt::ValueType::REF;
			_writeValue(vd, fs);

			for (auto &i = expr; i; i = i->next) {
				SlxFmt::ScopeRefDesc srd = { 0 };
				srd.type = SlxFmt::ScopeRefType::MEMBER;
				if (i->next)
					srd.hasNext = true;
				srd.lenName = i->name.length();
				_writeValue(srd, fs);
				_writeValue(*(i->name.c_str()), i->name.length(), fs);
			}
			break;
		}
		case Compiler::ExprType::ARRAY: {
			auto expr = std::static_pointer_cast<Compiler::ArrayExpr>(src);
			vd.type = SlxFmt::ValueType::ARRAY;
			_writeValue(vd, fs);

			fs << (std::uint32_t)expr->elements.size();
			for (auto &i : expr->elements) {
				auto constExpr = Compiler::evalConstExpr(i);
				if (!constExpr)
					throw Compiler::parser::syntax_error(i->getLocation(), "Expression cannot be evaluated in compile time");
				writeValueDesc(constExpr, fs);
			}
			break;
		}
		default:
			throw Compiler::parser::syntax_error(src->getLocation(), "Expression cannot be evaluated in compile time");
	}
}

/// @brief Evaluate type of an expression.
/// @param state State for the expression.
/// @param expr Expression to evaluate.
/// @param isRecusring Set to false by default, set if we are recursing.
/// @return Type of the expression, null if unknown.
std::shared_ptr<Compiler::TypeName> Compiler::evalExprType(std::shared_ptr<State> state, std::shared_ptr<Expr> expr, bool isRecursing) {
	auto &fn = state->fnDefs[state->currentFn];
	// assert(fn);
	switch (expr->getType()) {
		case ExprType::LITERAL: {
			auto literalType = std::static_pointer_cast<LiteralExpr>(expr)->getLiteralType();
			if (!_lt2tnKindMap.count(literalType))
				throw parser::syntax_error(expr->getLocation(), "Unevaluatable literal type");
			return std::make_shared<TypeName>(expr->getLocation(), _lt2tnKindMap.at(literalType));
		}
		case ExprType::REF: {
			auto ref = std::static_pointer_cast<RefExpr>(expr);
			std::shared_ptr<TypeName> refType;	// Type of current referenced object

			// Check if current reference is not terminal.
			if (!ref->next) {
				if (!isRecursing) {
					auto &vars = state->fnDefs[state->currentFn]->localVars;
					if (vars.count(ref->name))
						return vars[ref->name].type;
				}
			} else
			{
				auto &vars = state->fnDefs[state->currentFn]->localVars;
				if (vars.count(ref->name)) {
					refType = vars[ref->name].type;
					goto succeed;
				}
			}
			{
				auto v = state->scope->getVar(ref->name);
				if (v) {
					if (ref->next) {
						refType = v->typeName;
						goto succeed;
					} else
						return v->typeName;
				}
			}
			{
				auto item = state->scope->getEnumItem(ref);
				if (item) {
					if (ref->next) {
						refType = evalExprType(state, item);
						goto succeed;
					} else
						return evalExprType(state, item);
				}
			}
			{
				auto fn = state->scope->getFn(ref->name);
				if (fn) {
					auto fnType = std::make_shared<FnTypeName>(expr->getLocation(), fn->returnTypeName);
					for (auto &i : *(fn->params))
						fnType->argTypes.push_back(i->typeName);
					if (!ref->next)
						return fnType;
					refType = fnType;
					goto succeed;
				}
			}
			if (ref->next) {
				auto t = state->scope->getType(ref->name);
				if (t) {
					switch (t->getKind()) {
						case Type::Kind::CLASS: {
							auto ct = std::static_pointer_cast<ClassType>(t);
							refType = std::make_shared<CustomTypeName>(ref->getLocation(), std::make_shared<RefExpr>(ref->getLocation(), ref->name), state->scope);
							goto succeed;
						}
						case Type::Kind::TRAIT: {
							auto ct = std::static_pointer_cast<TraitType>(t);
							refType = std::make_shared<CustomTypeName>(ref->getLocation(), std::make_shared<RefExpr>(ref->getLocation(), ref->name), state->scope);
							goto succeed;
						}
					}
				}
			}
			throw parser::syntax_error(ref->getLocation(), "`" + ref->name + "' was not defined");
		succeed:
			switch (refType->typeName) {
				case TypeNameKind::CUSTOM: {
					auto t = std::static_pointer_cast<CustomTypeName>(refType);
					auto sc = t->scope.lock();

					auto type = t->scope.lock()->getType(t->typeRef);
					auto s = std::make_shared<State>();
					if (type) {
						switch (type->getKind()) {
							case Type::Kind::CLASS:
								s->scope = std::static_pointer_cast<ClassType>(type)->scope;
								break;
							case Type::Kind::TRAIT:
								s->scope = std::static_pointer_cast<TraitType>(type)->scope;
								break;
						}
					} else {
						throw parser::syntax_error(ref->getLocation(), "Type was not defined");
					}
					s->currentFn = state->currentFn;
					return evalExprType(s, ref->next, true);
				}
			}
		}
		case ExprType::CALL: {
			auto e = std::static_pointer_cast<CallExpr>(expr);
			if (e->isAsync)
				return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::U32);
			auto exprType = evalExprType(state, e->target);
			if (exprType->typeName != TypeNameKind::FN)
				throw parser::syntax_error(e->target->getLocation(), "Expression is not callable");
			return exprType;
		}
		case ExprType::AWAIT:
			return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::ANY);
		case ExprType::NEW:
			return std::static_pointer_cast<NewExpr>(expr)->type;
		case ExprType::TERNARY: {
			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			auto xType = evalExprType(state, e->x), yType = evalExprType(state, e->y);

			// Check if the condition expression is boolean.
			if (!isConvertible(xType, std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::BOOL)))
				throw parser::syntax_error(e->x->getLocation(), "Expecting a boolean expression");

			// Check if the expressions have the same type.
			if (!isSameType(xType, yType))
				throw parser::syntax_error(e->x->getLocation(), "Operands for ternary operation have different types");
			return xType;
		}
		case ExprType::BINARY: {
			auto e = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto xType = evalExprType(state, e->x), yType = evalExprType(state, e->y);
			switch (e->op) {
				case BinaryOp::LSHIFT:
				case BinaryOp::LSHIFT_ASSIGN:
				case BinaryOp::RSHIFT:
				case BinaryOp::RSHIFT_ASSIGN:
					if (!isConvertible(std::make_shared<TypeName>(yType->getLocation(), TypeNameKind::U32), yType))
						throw parser::syntax_error(e->y->getLocation(), "Incompatible expression types");
					break;
				default:
					if (!isConvertible(xType, yType))
						throw parser::syntax_error(e->y->getLocation(), "Incompatible expression types");
			}
			return xType;
		}
	}
	return std::shared_ptr<TypeName>();
}

void Slake::Compiler::writeIns(Opcode opcode, std::fstream &fs, std::initializer_list<std::shared_ptr<Expr>> operands) {
	assert(operands.size() <= 3);
	writeInsHeader({ opcode, (std::uint8_t)operands.size() }, fs);
	for (auto &i : operands)
		writeValueDesc(i, fs);
}

void Slake::Compiler::compileExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s, bool isRecursing) {
	auto &fn = s->fnDefs[s->currentFn];
	assert(fn);
	switch (expr->getType()) {
		case ExprType::LITERAL: {
			fn->body.push_back(Ins(Opcode::PUSH, { expr }));
			break;
		}
		case ExprType::TERNARY: {
			auto falseLabel = "$TOP_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$TOP_END_" + std::to_string(fn->body.size());

			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			compileExpr(e->condition, s);

			fn->body.push_back({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });
			compileExpr(e->x, s);  // True branch.
			fn->body.push_back({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });
			fn->insertLabel(falseLabel);
			compileExpr(e->y, s);  // False branch.
			fn->insertLabel(endLabel);
		}
		case ExprType::UNARY: {
			auto e = std::static_pointer_cast<UnaryOpExpr>(expr);
			Opcode opcode;
			switch (e->op) {
				case Slake::Compiler::UnaryOp::INC_F:
				case Slake::Compiler::UnaryOp::INC_B:
					opcode = Opcode::INC;
				case Slake::Compiler::UnaryOp::DEC_F:
				case Slake::Compiler::UnaryOp::DEC_B:
					opcode = Opcode::DEC;
				case Slake::Compiler::UnaryOp::NEG:
					opcode = Opcode::NEG;
				case Slake::Compiler::UnaryOp::NOT:
					opcode = Opcode::NOT;
				case Slake::Compiler::UnaryOp::REV:
					opcode = Opcode::REV;
				default:
					throw Compiler::parser::syntax_error(expr->getLocation(), "Invalid operator detected");
			}
			compileExpr(e->x, s);
			fn->body.push_back({ opcode, {} });
			break;
		}
		case ExprType::BINARY: {
			auto e = std::static_pointer_cast<BinaryOpExpr>(expr);
			compileExpr(e->y, s);
			compileExpr(e->x, s);

			Opcode opcode;

			switch (e->op) {
				case Slake::Compiler::BinaryOp::ADD_ASSIGN:
				case Slake::Compiler::BinaryOp::ADD:
					opcode = Opcode::ADD;
					break;
				case Slake::Compiler::BinaryOp::SUB_ASSIGN:
				case Slake::Compiler::BinaryOp::SUB:
					opcode = Opcode::SUB;
					break;
				case Slake::Compiler::BinaryOp::MUL_ASSIGN:
				case Slake::Compiler::BinaryOp::MUL:
					opcode = Opcode::MUL;
					break;
				case Slake::Compiler::BinaryOp::DIV_ASSIGN:
				case Slake::Compiler::BinaryOp::DIV:
					opcode = Opcode::DIV;
					break;
				case Slake::Compiler::BinaryOp::MOD_ASSIGN:
				case Slake::Compiler::BinaryOp::MOD:
					opcode = Opcode::MOD;
					break;
				case Slake::Compiler::BinaryOp::AND_ASSIGN:
				case Slake::Compiler::BinaryOp::AND:
					opcode = Opcode::AND;
				case Slake::Compiler::BinaryOp::OR_ASSIGN:
				case Slake::Compiler::BinaryOp::OR:
					opcode = Opcode::OR;
					break;
				case Slake::Compiler::BinaryOp::XOR_ASSIGN:
				case Slake::Compiler::BinaryOp::XOR:
					opcode = Opcode::XOR;
					break;
				case Slake::Compiler::BinaryOp::LAND:
					opcode = Opcode::LAND;
					break;
				case Slake::Compiler::BinaryOp::LOR:
					opcode = Opcode::LOR;
					break;
				case Slake::Compiler::BinaryOp::EQ:
					opcode = Opcode::EQ;
					break;
				case Slake::Compiler::BinaryOp::NEQ:
					opcode = Opcode::NEQ;
					break;
				case Slake::Compiler::BinaryOp::LSHIFT_ASSIGN:
				case Slake::Compiler::BinaryOp::LSHIFT:
					opcode = Opcode::LSH;
					break;
				case Slake::Compiler::BinaryOp::RSHIFT_ASSIGN:
				case Slake::Compiler::BinaryOp::RSHIFT:
					opcode = Opcode::RSH;
					break;
				case Slake::Compiler::BinaryOp::GTEQ:
					opcode = Opcode::GTEQ;
					break;
				case Slake::Compiler::BinaryOp::LTEQ:
					opcode = Opcode::LTEQ;
					break;
				case Slake::Compiler::BinaryOp::GT:
					opcode = Opcode::GT;
					break;
				case Slake::Compiler::BinaryOp::LT:
					opcode = Opcode::LT;
					break;
				case Slake::Compiler::BinaryOp::ASSIGN:
					break;
				default:
					throw Compiler::parser::syntax_error(e->getLocation(), "Invalid operator detected");
			}

			// ordinary assignment has no extra effects.
			if (e->op != Slake::Compiler::BinaryOp::ASSIGN)
				fn->body.push_back({ opcode, { e->x } });

			if (isAssignment(e->op)) {
				auto x = evalConstExpr(e->x);
				// Convert to direct operation if it is evaluatable.
				fn->body.push_back({ Opcode::STORE, { x ? x : e->x } });
			}

			break;
		}
		case ExprType::CALL: {
			auto calle = std::static_pointer_cast<CallExpr>(expr);
			auto t = std::static_pointer_cast<FnTypeName>(evalExprType(s, calle->target));
			if (t->typeName != TypeNameKind::FN)
				throw parser::syntax_error(calle->target->getLocation(), "Expression is not callable");

			Opcode opcode = Opcode::CALL;
			if (calle->isAsync) {
				if (t->isNative())
					opcode = Opcode::ASYSCALL;
				else
					opcode = Opcode::ACALL;
			} else {
				if (t->isNative())
					opcode = Opcode::SYSCALL;
				else
					opcode = Opcode::CALL;
			}
			auto ce = evalConstExpr(calle->target);
			if (ce)
				fn->body.push_back({ opcode, { ce } });
			else {
				for (auto &i : *(calle->args))
					compileExpr(i, s);
				compileExpr(calle->target, s);
				fn->body.push_back({ opcode, {} });
			}

			break;
		}
		case ExprType::REF: {
			auto ref = std::static_pointer_cast<RefExpr>(expr);
			if (!isRecursing) {
				if (fn->localVars.count(ref->name)) {
					fn->body.push_back({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(ref->getLocation(), fn->localVars[ref->name].stackPos) } });
					if (ref->next) {
						std::shared_ptr<Scope> scope = s->scope;
						auto &localVarType = fn->localVars[ref->name].type;
						switch (localVarType->typeName) {
							case TypeNameKind::CUSTOM: {
								auto tn = std::static_pointer_cast<CustomTypeName>(localVarType);
								auto t = Scope::getCustomType(tn);
								if (!t) {
									throw Compiler::parser::syntax_error(expr->getLocation(), "Type was not defined");
								}
								auto scope = t->getScope();
								if (!scope)
									throw Compiler::parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								s->scope = scope;
								break;
							}
							default:
								throw Compiler::parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						compileExpr(ref->next, s, true);
						s->scope = scope;
					}
					break;
				}
			}
			if (s->scope->getVar(ref->name) || s->scope->getEnumItem(ref)) {
				fn->body.push_back({ Opcode::LOAD, { ref } });
				break;
			}
			{
				auto f = s->scope->getFn(ref->name);
				if (f) {
					if (f->isNative())
						fn->body.push_back({ Opcode::LOAD, { std::make_shared<UUIDLiteralExpr>(ref->getLocation(), f->uuid) } });
					else
						fn->body.push_back({ Opcode::LOAD, { ref } });
					break;
				}
			}
			{
				auto t = s->scope->getType(ref->name);
				if (t) {
					fn->body.push_back({ Opcode::LOAD, { ref } });
					break;
				}
			}
			throw Compiler::parser::syntax_error(expr->getLocation(), "`" + ref->name + "' was undefined");
		}
		default:
			throw std::logic_error("Invalid expression type detected");
	}
}

void Compiler::compileStmt(std::shared_ptr<Compiler::Stmt> src, std::shared_ptr<State> s) {
	auto &fn = s->fnDefs[s->currentFn];
	assert(fn);

	switch (src->getType()) {
		case StmtType::CODEBLOCK: {
			auto stmt = std::static_pointer_cast<Compiler::CodeBlock>(src);
			auto state = std::make_shared<State>();
			state->scope = s->scope;

			std::string endLabel = "$BLK_" + std::to_string(fn->body.size());
			fn->body.push_back({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->ins)
				compileStmt(i, s);

			fn->insertLabel(endLabel);

			fn->body.push_back({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::IF: {
			auto stmt = std::static_pointer_cast<Compiler::IfStmt>(src);
			compileExpr(stmt->condition, s);

			auto falseLabel = "$IF_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$IF_END_" + std::to_string(fn->body.size());

			compileExpr(stmt->condition, s);

			fn->body.push_back({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });
			compileStmt(stmt->thenBlock, s);  // True branch.
			fn->body.push_back({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });
			fn->insertLabel(falseLabel);
			compileStmt(stmt->elseBlock, s);  // False branch.
			fn->insertLabel(endLabel);
			break;
		}
		case StmtType::FOR: {
			auto stmt = std::static_pointer_cast<Compiler::ForStmt>(src);
			compileExpr(stmt->condition, s);

			auto conditionLabel = "$FOR_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$FOR_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$FOR_END_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->body.push_back({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->varDecl->declList) {
				fn->localVars[i->name] = LocalVar(s->stackCur, stmt->varDecl->typeName);
			}

			fn->body.push_back({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileExpr(stmt->condition, s);
			fn->body.push_back({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });

			fn->insertLabel(endLabel);
			fn->body.push_back({ Opcode::LEAVE, {} });
			s->leaveLoop();
			break;
		}
		case StmtType::WHILE: {
			auto stmt = std::static_pointer_cast<Compiler::WhileStmt>(src);
			compileExpr(stmt->condition, s);

			auto conditionLabel = "$WHILE_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$WHILE_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$WHILE_END_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->body.push_back({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileExpr(stmt->condition, s);
			fn->body.push_back({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });
			s->leaveLoop();
		}
		case StmtType::TIMES: {
			auto stmt = std::static_pointer_cast<Compiler::TimesStmt>(src);
			auto conditionLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$TIMES_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 counterName = "$TIMES_CNT_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->body.push_back({ Opcode::ENTER, {} });

			fn->localVars[counterName] = LocalVar(s->stackCur, evalExprType(s, stmt->timesExpr));
			compileExpr(stmt->timesExpr, s);

			fn->body.push_back({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileExpr(std::make_shared<RefExpr>(stmt->getLocation(), counterName), s);
			fn->body.push_back({ Opcode::JT, { std::make_shared<LabelExpr>(bodyLabel) } });

			fn->insertLabel(endLabel);
			fn->body.push_back({ Opcode::LEAVE, {} });
			s->leaveLoop();
			break;
		}
		case StmtType::EXPR: {
			auto stmt = std::static_pointer_cast<Compiler::ExprStmt>(src);
			for (auto &i = stmt; i; i = i->next) {
				compileExpr(stmt->expr, s);
				SlxFmt::InsHeader ih = { Opcode::POP, 0 };
			}
			break;
		}
		case StmtType::CONTINUE: {
			auto stmt = std::static_pointer_cast<Compiler::ContinueStmt>(src);
			if (!s->nContinueLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected continue statement");
			fn->body.push_back({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::BREAK: {
			auto stmt = std::static_pointer_cast<Compiler::ContinueStmt>(src);
			if (!s->nBreakLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected break statement");
			fn->body.push_back({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::RETURN: {
			auto stmt = std::static_pointer_cast<Compiler::ReturnStmt>(src);
			auto type = evalExprType(s, stmt->expr);
			/*if () {

			}*/
			break;
		}
		case StmtType::SWITCH: {
			auto stmt = std::static_pointer_cast<Compiler::SwitchStmt>(src);
			auto conditionName = "$SW_COND_" + std::to_string(fn->body.size()),
				 endLabel = "$SW_END_" + std::to_string(fn->body.size());

			compileExpr(stmt->condition, s);
			fn->localVars[conditionName] = LocalVar(s->stackCur, evalExprType(s, stmt->condition));

			s->enterSwitch();
			fn->body.push_back({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			{
				std::size_t j = 0;
				for (auto &i : *stmt->caseList) {
					auto caseEndLabel =
						"$SW_" +
						std::to_string(stmt->getLocation().begin.line) + "_" +
						std::to_string(stmt->getLocation().begin.column) + "_" +
						"END_ " + std::to_string(j);

					compileExpr(i->condition, s);
					fn->body.push_back({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(i->getLocation(), fn->localVars[conditionName].stackPos) } });
					fn->body.push_back({ Opcode::EQ, {} });
					fn->body.push_back({ Opcode::JF, { std::make_shared<LabelExpr>(caseEndLabel) } });

					compileStmt(i->x, s);
					fn->body.push_back(
						{ Opcode::JMP,
							{ std::make_shared<LabelExpr>("$SW_" +
														  std::to_string(stmt->getLocation().begin.line) + "_" +
														  std::to_string(stmt->getLocation().begin.column) + "_" +
														  "END_ " + std::to_string(++j)) } });

					fn->insertLabel(caseEndLabel);
				}
			}

			fn->body.push_back({ Opcode::LEAVE, {} });
			s->leaveSwitch();
			break;
		}
		case StmtType::VAR_DEF: {
			auto stmt = std::static_pointer_cast<Compiler::VarDefStmt>(src);
			if (stmt->accessModifier & ((~ACCESS_CONST) | (~ACCESS_VOLATILE)) || (stmt->isNative))
				throw parser::syntax_error(stmt->getLocation(), "Invalid modifier combination");

			if (stmt->typeName->typeName == TypeNameKind::CUSTOM) {
				auto t = std::static_pointer_cast<CustomTypeName>(stmt->typeName);
				auto type = s->scope->getType(t->typeRef);
			}
			for (auto &i : stmt->declList) {
				fn->localVars[stmt->declList[i->name]->name] = LocalVar((std::uint32_t)(s->stackCur++), stmt->typeName);
				if (i->initValue) {
					auto expr = evalConstExpr(i->initValue);
					if (!expr)
						fn->body.push_back(Ins(Opcode::PUSH, { expr }));
					else
						compileExpr(i->initValue, s);
				} else
					fn->body.push_back(Ins(Opcode::PUSH, { std::make_shared<NullLiteralExpr>(stmt->getLocation()) }));
			}
			break;
		}
		default:
			throw std::logic_error("Invalid statement type detected");
	}
}

void Compiler::compile(std::shared_ptr<Scope> scope, std::fstream &fs, bool isTopLevel) {
	if (isTopLevel) {
		SlxFmt::ImgHeader ih = { 0 };
		std::memcpy(ih.magic, SlxFmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		fs.write((char *)&ih, sizeof(ih));
	}

	//
	// Write value descriptors (VAD).
	//
	{
		for (auto &i : scope->vars) {
			SlxFmt::VarDesc vad = { 0 };
			vad.lenName = i.first.length();

			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC))
				throw Compiler::parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (i.second->accessModifier & ACCESS_PUB)
				vad.flags |= SlxFmt::VAD_PUB;
			if (i.second->accessModifier & ACCESS_FINAL)
				vad.flags |= SlxFmt::VAD_FINAL;
			if (i.second->accessModifier & ACCESS_STATIC)
				vad.flags |= SlxFmt::VAD_STATIC;

			if (i.second->initValue)
				vad.flags |= SlxFmt::VAD_INIT;
			fs.write((char *)&vad, sizeof(vad));
			fs.write(i.first.c_str(), i.first.length());
			if (i.second->initValue) {
				writeValueDesc(i.second->initValue, fs);
			}
		}
		{
			SlxFmt::VarDesc vad = { 0 };
			fs.write((char *)&vad, sizeof(vad));
		}
	}

	//
	// Compile and write compiled functions.
	//
	{
		for (auto &i : scope->fnDefs) {
			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE))
				throw Compiler::parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (isTopLevel)
				if (i.second->accessModifier & (ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE))
					throw Compiler::parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

			auto state = std::make_shared<State>();
			state->currentFn = i.first;						  // Set up current function name of the state.
			state->scope = scope;							  // Set up scope of the state.
			state->fnDefs[i.first] = std::make_shared<Fn>();  // Create a new, empty function.

			//
			// Compile the function if it is not native.
			//
			if (!i.second->isNative()) {
				for (auto &j : *(i.second->params)) {
					auto &fn = state->fnDefs[i.first];
					fn->localVars[j->name] = LocalVar((std::uint32_t)(state->stackCur++), j->typeName);
				}
				compileStmt(i.second->execBlock, state);
			}

			//
			// Write the function descriptor (FND).
			//
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
				fnd.lenName = i.first.length();
				fnd.lenBody = state->fnDefs[state->currentFn]->body.size();
				fs.write((char *)&fnd, sizeof(fnd));
				fs.write(i.first.c_str(), i.first.size());
			}

			//
			// Write for each instructions.
			//
			for (auto &k : state->fnDefs[state->currentFn]->body) {
				SlxFmt::InsHeader ih(k.opcode, k.operands.size());
				writeInsHeader(ih, fs);
				for (auto &l : k.operands) {
					if (l->getType() == ExprType::LABEL) {
						writeValueDesc(std::make_shared<UIntLiteralExpr>(l->getLocation(), state->fnDefs[state->currentFn]->labels[std::static_pointer_cast<LabelExpr>(l)->label]), fs);
					} else
						writeValueDesc(l, fs);
				}
			}
		}
		SlxFmt::FnDesc fnd = { 0 };
		fs.write((char *)&fnd, sizeof(fnd));
	}

	//
	// Write class type descriptors (CTD).
	//
	{
		for (auto i : scope->types) {
			switch (i.second->getKind()) {
				case Type::Kind::CLASS: {
					auto t = std::static_pointer_cast<ClassType>(i.second);
					SlxFmt::ClassTypeDesc ctd = { 0 };

					if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL))
						throw Compiler::parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

					if (i.second->accessModifier & ACCESS_PUB)
						ctd.flags |= SlxFmt::CTD_PUB;
					if (i.second->accessModifier & ACCESS_FINAL)
						ctd.flags |= SlxFmt::CTD_FINAL;

					ctd.lenName = i.first.length();
					ctd.lenImpls = t->impls->impls.size();
					ctd.nGenericParams = t->genericParams.size();
					if (t->parent) {
						if (t->parent->typeName == TypeNameKind::CUSTOM)
							throw Compiler::parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(t->parent);
						writeValueDesc(tn->typeRef, fs);
					}
					for (auto &j : t->impls->impls) {
						if (j->typeName != TypeNameKind::CUSTOM)
							throw Compiler::parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(j);
						writeValueDesc(tn->typeRef, fs);
					}
					fs.write((char *)&ctd, sizeof(ctd));

					fs.write(i.first.c_str(), (std::streamsize)i.first.size());

					compile(std::static_pointer_cast<ClassType>(i.second)->scope, fs, false);
					break;
				}
				case Type::Kind::TRAIT:
					// compile(std::static_pointer_cast<TraitType>(i.second)->scope, fs, false);
					break;
				case Type::Kind::STRUCT:
				case Type::Kind::ENUM:
					break;
			}
		}
		SlxFmt::ClassTypeDesc ctd = { 0 };
		fs.write((char *)&ctd, sizeof(ctd));
	}
}
