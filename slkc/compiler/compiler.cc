#include <slkparse.hh>

using namespace Slake;
using namespace Compiler;

void Slake::Compiler::deinit() {
	currentTrait.reset();
	currentClass.reset();
	currentEnum.reset();
	currentScope.reset();
	currentStruct.reset();
}

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

static std::unordered_map<LiteralType, SlxFmt::ValueType> _lt2vtMap = {
	{ LT_INT, SlxFmt::ValueType::I32 },
	{ LT_UINT, SlxFmt::ValueType::U32 },
	{ LT_LONG, SlxFmt::ValueType::I64 },
	{ LT_ULONG, SlxFmt::ValueType::U64 },
	{ LT_FLOAT, SlxFmt::ValueType::FLOAT },
	{ LT_DOUBLE, SlxFmt::ValueType::DOUBLE },
	{ LT_BOOL, SlxFmt::ValueType::BOOL },
	{ LT_STRING, SlxFmt::ValueType::STRING },
	{ LT_NULL, SlxFmt::ValueType::NONE }
};

static std::unordered_map<LiteralType, TypeNameKind> _lt2tnKindMap = {
	{ LT_INT, TypeNameKind::I32 },
	{ LT_UINT, TypeNameKind::U32 },
	{ LT_LONG, TypeNameKind::I64 },
	{ LT_ULONG, TypeNameKind::U64 },
	{ LT_FLOAT, TypeNameKind::FLOAT },
	{ LT_DOUBLE, TypeNameKind::DOUBLE },
	{ LT_BOOL, TypeNameKind::BOOL },
	{ LT_STRING, TypeNameKind::STRING },
	{ LT_NULL, TypeNameKind::NONE }
};

static std::unordered_map<TypeNameKind, SlxFmt::ValueType> _tnKind2vtMap = {
	{ TypeNameKind::I8, SlxFmt::ValueType::I8 },
	{ TypeNameKind::I16, SlxFmt::ValueType::I16 },
	{ TypeNameKind::I32, SlxFmt::ValueType::I32 },
	{ TypeNameKind::I64, SlxFmt::ValueType::I64 },
	{ TypeNameKind::U8, SlxFmt::ValueType::U8 },
	{ TypeNameKind::U16, SlxFmt::ValueType::U16 },
	{ TypeNameKind::U32, SlxFmt::ValueType::U32 },
	{ TypeNameKind::U64, SlxFmt::ValueType::U64 },
	{ TypeNameKind::FLOAT, SlxFmt::ValueType::FLOAT },
	{ TypeNameKind::DOUBLE, SlxFmt::ValueType::DOUBLE },
	{ TypeNameKind::BOOL, SlxFmt::ValueType::BOOL },
	{ TypeNameKind::STRING, SlxFmt::ValueType::STRING },
	{ TypeNameKind::NONE, SlxFmt::ValueType::NONE },
	{ TypeNameKind::CUSTOM, SlxFmt::ValueType::OBJECT },
	{ TypeNameKind::ARRAY, SlxFmt::ValueType::ARRAY }
};

static std::unordered_map<UnaryOp, Opcode> _unaryOp2opcodeMap = {
	{ UnaryOp::INC_F, Opcode::INC },
	{ UnaryOp::INC_B, Opcode::INC },
	{ UnaryOp::DEC_F, Opcode::DEC },
	{ UnaryOp::DEC_B, Opcode::DEC },
	{ UnaryOp::NEG, Opcode::NEG },
	{ UnaryOp::NOT, Opcode::NOT },
	{ UnaryOp::REV, Opcode::REV }
};

static std::unordered_map<BinaryOp, Opcode> _binaryOp2opcodeMap = {
	{ BinaryOp::ADD, Opcode::ADD },
	{ BinaryOp::SUB, Opcode::SUB },
	{ BinaryOp::MUL, Opcode::MUL },
	{ BinaryOp::DIV, Opcode::DIV },
	{ BinaryOp::MOD, Opcode::MOD },
	{ BinaryOp::AND, Opcode::AND },
	{ BinaryOp::OR, Opcode::OR },
	{ BinaryOp::XOR, Opcode::XOR },
	{ BinaryOp::LAND, Opcode::LAND },
	{ BinaryOp::LOR, Opcode::LOR },
	{ BinaryOp::LSH, Opcode::LSH },
	{ BinaryOp::RSH, Opcode::RSH },
	{ BinaryOp::EQ, Opcode::EQ },
	{ BinaryOp::NEQ, Opcode::NEQ },
	{ BinaryOp::GTEQ, Opcode::GTEQ },
	{ BinaryOp::LTEQ, Opcode::LTEQ },
	{ BinaryOp::GT, Opcode::GT },
	{ BinaryOp::LT, Opcode::LT },
	{ BinaryOp::ADD_ASSIGN, Opcode::ADD },
	{ BinaryOp::SUB_ASSIGN, Opcode::SUB },
	{ BinaryOp::MUL_ASSIGN, Opcode::MUL },
	{ BinaryOp::DIV_ASSIGN, Opcode::DIV },
	{ BinaryOp::MOD_ASSIGN, Opcode::MOD },
	{ BinaryOp::AND_ASSIGN, Opcode::AND },
	{ BinaryOp::OR_ASSIGN, Opcode::OR },
	{ BinaryOp::XOR_ASSIGN, Opcode::XOR },
	{ BinaryOp::LSH_ASSIGN, Opcode::LSH },
	{ BinaryOp::RSH_ASSIGN, Opcode::RSH }
};

static void writeValueDesc(std::shared_ptr<State> s, std::shared_ptr<Expr> src, std::fstream &fs) {
	SlxFmt::ValueDesc vd = {};
	switch (src->getType()) {
		case ExprType::LITERAL: {
			auto literalExpr = std::static_pointer_cast<LiteralExpr>(src);
			vd.type = _lt2vtMap.at(literalExpr->getLiteralType());
			_writeValue(vd, fs);
			switch (literalExpr->getLiteralType()) {
				case LT_INT:
					_writeValue(std::static_pointer_cast<IntLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_UINT:
					_writeValue(std::static_pointer_cast<UIntLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_LONG:
					_writeValue(std::static_pointer_cast<LongLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_ULONG:
					_writeValue(std::static_pointer_cast<ULongLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_FLOAT:
					_writeValue(std::static_pointer_cast<FloatLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_DOUBLE:
					_writeValue(std::static_pointer_cast<DoubleLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_BOOL: {
					auto expr = std::static_pointer_cast<BoolLiteralExpr>(literalExpr);
					_writeValue(expr->data, fs);
					break;
				}
				case LT_STRING: {
					auto expr = std::static_pointer_cast<StringLiteralExpr>(literalExpr);
					_writeValue(expr->data.size(), fs);
					_writeValue(expr->data.c_str(), (std::streamsize)expr->data.size(), fs);
					break;
				}
			}
			break;
		}
		case ExprType::REF: {
			auto expr = std::static_pointer_cast<RefExpr>(src);
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
		case ExprType::ARRAY: {
			auto expr = std::static_pointer_cast<ArrayExpr>(src);
			vd.type = SlxFmt::ValueType::ARRAY;
			_writeValue(vd, fs);

			fs << (std::uint32_t)expr->elements.size();
			for (auto &i : expr->elements) {
				auto constExpr = evalConstExpr(i, s);
				if (!constExpr)
					throw parser::syntax_error(i->getLocation(), "Expression cannot be evaluated in compile time");
				writeValueDesc(s, constExpr, fs);
			}
			break;
		}
		default:
			throw parser::syntax_error(src->getLocation(), "Expression cannot be evaluated in compile time");
	}
}

/// @brief Evaluate type of an expression.
/// @param state State for the expression.
/// @param expr Expression to evaluate.
/// @param isRecusring Set to false by default, set if we are recursing. DO NOT use local variables in the state if set.
/// @return Type of the expression, null if unknown.
std::shared_ptr<TypeName> Compiler::evalExprType(std::shared_ptr<State> s, std::shared_ptr<Expr> expr, bool isRecursing) {
	auto &fn = s->fnDefs[s->currentFn];
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
			if (!isRecursing) {
				if (fn->lvars.count(ref->name)) {
					if (ref->next) {
						std::shared_ptr<Scope> scope = s->scope;
						switch (fn->lvars[ref->name].type->kind) {
							case TypeNameKind::CUSTOM: {
								auto t = Scope::getCustomType(std::static_pointer_cast<CustomTypeName>(fn->lvars[ref->name].type));
								if (!t)
									throw parser::syntax_error(expr->getLocation(), "Type was not defined");
								if (!t->getScope())
									throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								s->scope = t->getScope();
								break;
							}
							default:
								throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						auto refType = evalExprType(s, ref->next, true);
						s->scope = scope;
						return refType;
					}
					return fn->lvars[ref->name].type;
				}
			}
			{
				auto v = s->scope->getVar(ref->name);
				if (v) {
					if (ref->next) {
						std::shared_ptr<Scope> scope = s->scope;
						switch (v->typeName->kind) {
							case TypeNameKind::CUSTOM: {
								auto tn = std::static_pointer_cast<CustomTypeName>(v->typeName);
								auto t = Scope::getCustomType(tn);
								if (!t) {
									throw parser::syntax_error(expr->getLocation(), "Type was not defined");
								}
								auto scope = t->getScope();
								if (!scope)
									throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
								s->scope = scope;
								break;
							}
							default:
								throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						}
						auto refType = evalExprType(s, ref->next, true);
						s->scope = scope;
						return refType;
					}
					return v->typeName;
				}
			}
			{
				auto e = s->scope->getEnumItem(ref);
				if (e)
					return evalExprType(s, e, true);
			}
			{
				auto fn = s->scope->getFn(ref->name);
				if (fn) {
					auto fnType = std::make_shared<FnTypeName>(expr->getLocation(), fn->returnTypeName);
					for (auto &i : *(fn->params))
						fnType->argTypes.push_back(i->typeName);
					if (ref->next)
						throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->next->name + "' with unsupported type");
					return fnType;
				}
			}
			{
				auto t = s->scope->getType(ref->name);
				if (t) {
					if (!ref->next)
						throw parser::syntax_error(expr->getLocation(), "Unexpected type name");
					std::shared_ptr<Scope> scope = s->scope;
					s->scope = t->getScope();
					auto refType = evalExprType(s, ref->next, true);
					s->scope = scope;
					return refType;
				}
			}
			throw parser::syntax_error(expr->getLocation(), "Undefined identifier: `" + ref->name + "'");
		}
		case ExprType::CALL: {
			auto e = std::static_pointer_cast<CallExpr>(expr);
			if (e->isAsync)
				return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::U32);
			auto exprType = evalExprType(s, e->target);
			if (exprType->kind != TypeNameKind::FN)
				throw parser::syntax_error(e->target->getLocation(), "Expression is not callable");
			return std::static_pointer_cast<FnTypeName>(exprType)->resultType;
		}
		case ExprType::AWAIT:
			return std::make_shared<TypeName>(expr->getLocation(), TypeNameKind::ANY);
		case ExprType::NEW:
			return std::static_pointer_cast<NewExpr>(expr)->type;
		case ExprType::TERNARY: {
			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			auto xType = evalExprType(s, e->x), yType = evalExprType(s, e->y);

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
			auto xType = evalExprType(s, e->x), yType = evalExprType(s, e->y);
			switch (e->op) {
				case BinaryOp::LSH:
				case BinaryOp::LSH_ASSIGN:
				case BinaryOp::RSH:
				case BinaryOp::RSH_ASSIGN:
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

std::shared_ptr<Expr> Slake::Compiler::evalConstExpr(std::shared_ptr<Expr> expr, std::shared_ptr<State> s) {
	switch (expr->getType()) {
		case ExprType::LITERAL:
			return expr;
		case ExprType::UNARY: {
			std::shared_ptr<UnaryOpExpr> opExpr = std::static_pointer_cast<UnaryOpExpr>(expr);
			switch (opExpr->x->getType()) {
				case ExprType::LITERAL:
					return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execUnaryOp(opExpr->op);
				case ExprType::REF: {
					auto ref = std::static_pointer_cast<RefExpr>(opExpr->x);
					//
					// Variable and function are both not evaluatable at compile time.
					//
					if ((currentScope->getVar(ref->name)) || (currentScope->getFn(ref->name)))
						return std::shared_ptr<Expr>();
					{
						auto x = currentScope->getEnumItem(ref);
						if (x)
							return evalConstExpr(x, s);
					}
					break;
				}
			}
			break;
		}
		case ExprType::BINARY: {
			std::shared_ptr<BinaryOpExpr> opExpr = std::static_pointer_cast<BinaryOpExpr>(expr);
			auto x = evalConstExpr(opExpr->x, s);
			if ((!x) || (x->getType() != ExprType::LITERAL))
				return std::shared_ptr<Expr>();
			auto y = evalConstExpr(opExpr->y, s);
			if ((!y) || (y->getType() != ExprType::LITERAL))
				return std::shared_ptr<Expr>();
			return std::static_pointer_cast<LiteralExpr>(opExpr->x)->execBinaryOp(opExpr->op, std::static_pointer_cast<LiteralExpr>(opExpr->y));
		}
		default:
			return std::shared_ptr<Expr>();
	}
}

void Compiler::writeIns(std::shared_ptr<State> s, Opcode opcode, std::fstream &fs, std::initializer_list<std::shared_ptr<Expr>> operands) {
	assert(operands.size() <= 3);
	_writeValue(SlxFmt::InsHeader{ opcode, (std::uint8_t)operands.size() }, fs);
	for (auto &i : operands)
		writeValueDesc(s, i, fs);
}

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
			opcode = _unaryOp2opcodeMap[e->op];
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
				opcode = _binaryOp2opcodeMap[e->op];
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
			if (!(isRecursing) && fn->lvars.count(ref->name)) {
				fn->insertIns({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(ref->getLocation(), fn->lvars[ref->name].stackPos) } });
				if (ref->next) {
					auto savedScope = s->scope;

					auto &lvarType = fn->lvars[ref->name].type;
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
	auto &fn = s->fnDefs[s->currentFn];
	assert(fn);

	switch (src->getType()) {
		case StmtType::CODEBLOCK: {
			auto stmt = std::static_pointer_cast<CodeBlock>(src);
			auto state = std::make_shared<State>();
			state->scope = s->scope;

			std::string endLabel = "$BLK_" + std::to_string(fn->body.size());
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->ins)
				compileStmt(i, s);

			fn->insertLabel(endLabel);

			fn->insertIns({ Opcode::LEAVE, {} });
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
			auto stmt = std::static_pointer_cast<ForStmt>(src);
			compileRightExpr(stmt->condition, s);

			auto conditionLabel = "$FOR_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$FOR_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$FOR_END_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->insertIns({ Opcode::ENTER, { std::make_shared<LabelExpr>(endLabel) } });

			for (auto &i : stmt->varDecl->declList) {
				fn->lvars[i->name] = LocalVar(s->stackCur, stmt->varDecl->typeName);
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
		}
		case StmtType::TIMES: {
			auto stmt = std::static_pointer_cast<TimesStmt>(src);
			auto conditionLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 bodyLabel = "$TIMES_BODY_" + std::to_string(fn->body.size()),
				 endLabel = "$TIMES_END_" + std::to_string(fn->body.size()),
				 counterName = "$TIMES_CNT_" + std::to_string(fn->body.size());

			s->enterLoop();
			fn->insertIns({ Opcode::ENTER, {} });

			fn->lvars[counterName] = LocalVar(s->stackCur, evalExprType(s, stmt->timesExpr));
			compileRightExpr(stmt->timesExpr, s);

			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(conditionLabel) } });

			fn->insertLabel(bodyLabel);
			compileStmt(stmt->execBlock, s);

			fn->insertLabel(conditionLabel);
			compileRightExpr(std::make_shared<RefExpr>(stmt->getLocation(), counterName), s);
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
			if (!s->nContinueLevel)
				throw parser::syntax_error(stmt->getLocation(), "Unexpected continue statement");
			fn->insertIns({ Opcode::LEAVE, {} });
			break;
		}
		case StmtType::BREAK: {
			auto stmt = std::static_pointer_cast<ContinueStmt>(src);
			if (!s->nBreakLevel)
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

			s->returned = true;
			break;
		}
		case StmtType::SWITCH: {
			auto stmt = std::static_pointer_cast<SwitchStmt>(src);
			auto conditionName = "$SW_COND_" + std::to_string(fn->body.size()),
				 endLabel = "$SW_END_" + std::to_string(fn->body.size());

			compileRightExpr(stmt->condition, s);
			fn->lvars[conditionName] = LocalVar(s->stackCur, evalExprType(s, stmt->condition));

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
					fn->insertIns({ Opcode::LLOAD, { std::make_shared<UIntLiteralExpr>(i->getLocation(), fn->lvars[conditionName].stackPos) } });
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
			if (stmt->accessModifier & ((~ACCESS_CONST) | (~ACCESS_VOLATILE)) || (stmt->isNative))
				throw parser::syntax_error(stmt->getLocation(), "Invalid modifier combination");

			if (stmt->typeName->kind == TypeNameKind::CUSTOM) {
				auto t = std::static_pointer_cast<CustomTypeName>(stmt->typeName);
				auto type = s->scope->getType(t->typeRef);
			}
			for (auto &i : stmt->declList) {
				fn->lvars[stmt->declList[i->name]->name] = LocalVar((std::uint32_t)(s->stackCur++), stmt->typeName);
				if (i->initValue) {
					auto expr = evalConstExpr(i->initValue, s);
					if (!expr)
						fn->insertIns(Ins(Opcode::PUSH, { expr }));
					else
						compileRightExpr(i->initValue, s);
				} else
					fn->insertIns(Ins(Opcode::PUSH, { std::make_shared<NullLiteralExpr>(stmt->getLocation()) }));
			}
			break;
		}
		default:
			throw std::logic_error("Invalid statement type detected");
	}
}

void Compiler::compile(std::shared_ptr<Scope> scope, std::fstream &fs, bool isTopLevel) {
	auto s = std::make_shared<State>();
	s->scope = scope;

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
				throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
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
				writeValueDesc(s, i.second->initValue, fs);
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
			if (i.second->accessModifier & ~(ACCESS_PUB | ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE))
				throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");
			if (isTopLevel)
				if (i.second->accessModifier & (ACCESS_FINAL | ACCESS_STATIC | ACCESS_OVERRIDE))
					throw parser::syntax_error(i.second->getLocation(), "Invalid modifier combination");

			auto state = std::make_shared<State>();
			state->currentFn = i.first;						  // Set up current function name of the state.
			state->scope = scope;							  // Set up scope of the state.
			state->fnDefs[i.first] = std::make_shared<Fn>();  // Create a new, empty function.

			// Compile the function if it is not native.
			if (i.second->execBlock) {
				for (auto &j : *(i.second->params)) {
					auto &fn = state->fnDefs[i.first];
					fn->lvars[j->name] = LocalVar((std::uint32_t)(state->stackCur++), j->typeName);
				}
				compileStmt(i.second->execBlock, state);

				if (!state->returned) {
					if (i.second->returnTypeName->kind != TypeNameKind::NONE)
						throw parser::syntax_error(i.second->execBlock->getLocation(), "Must return a value");
					state->fnDefs[i.first]->insertIns(Ins(Opcode::RET, {}));
				}
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
				fnd.lenName = i.first.length();
				fnd.lenBody = state->fnDefs[state->currentFn]->body.size();
				fnd.nParams = i.second->params->size();
				_writeValue(fnd, fs);
				_writeValue(*(i.first.c_str()), i.first.length(), fs);
			}

			for (auto j : *(i.second->params)) {
				_writeValue(_tnKind2vtMap.at(j->typeName->kind), fs);
				if (j->typeName->kind == TypeNameKind::CUSTOM) {
					auto tn = std::static_pointer_cast<CustomTypeName>(j->typeName);
					auto t = Scope::getCustomType(tn);
					if (!t)
						throw parser::syntax_error(tn->getLocation(), "Type `" + std::to_string(*tn) + "' was not defined");

					// ! FIXME
					writeValueDesc(s, tn->typeRef, fs);
				}
			}

			//
			// Write for each instructions.
			//
			for (auto &k : state->fnDefs[state->currentFn]->body) {
				SlxFmt::InsHeader ih(k.opcode, k.operands.size());
				_writeValue(ih, fs);
				for (auto &l : k.operands) {
					if (l->getType() == ExprType::LABEL) {
						writeValueDesc(
							s,
							std::make_shared<UIntLiteralExpr>(l->getLocation(),
								state->fnDefs[state->currentFn]->labels[std::static_pointer_cast<LabelExpr>(l)->label]),
							fs);
					} else
						writeValueDesc(s, l, fs);
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
					ctd.lenImpls = t->impls->impls.size();
					ctd.nGenericParams = t->genericParams.size();
					_writeValue(ctd, fs);
					_writeValue(*(i.first.c_str()), (std::streamsize)i.first.size(), fs);

					if (t->parent) {
						ctd.flags |= SlxFmt::CTD_DERIVED;
						if (t->parent->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(t->parent);
						writeValueDesc(s, tn->typeRef, fs);
					}
					for (auto &j : t->impls->impls) {
						if (j->kind != TypeNameKind::CUSTOM)
							throw parser::syntax_error(i.second->getLocation(), "Invalid parent type");
						auto tn = std::static_pointer_cast<CustomTypeName>(j);
						writeValueDesc(s, tn->typeRef, fs);
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
						writeValueDesc(s, tn->typeRef, fs);
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
		}
		SlxFmt::StructTypeDesc std = { 0 };
		fs.write((char *)&std, sizeof(std));
	}
}
