#include <slkparse.hh>

#include "misc.hh"

using namespace Slake;
using namespace Slake::Compiler;

void State::compileRightExpr(std::shared_ptr<Expr> expr, bool isRecursing) {
	auto &fn = fnDefs[currentFn];
	assert(fn);
	switch (expr->getExprKind()) {
		case ExprKind::LITERAL:
		case ExprKind::ARRAY:
		case ExprKind::MAP: {
			fn->insertIns(Ins(Opcode::PUSH, { expr }));
			break;
		}
		case ExprKind::TERNARY: {
			auto falseLabel = "$TOP_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$TOP_END_" + std::to_string(fn->body.size());

			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			compileRightExpr(e->condition);

			fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });
			compileRightExpr(e->x);	 // True branch.
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });
			fn->insertLabel(falseLabel);
			compileRightExpr(e->y);	 // False branch.
			fn->insertLabel(endLabel);
			break;
		}
		case ExprKind::UNARY: {
			auto e = std::static_pointer_cast<UnaryOpExpr>(expr);
			Opcode opcode;

			if (!_unaryOp2opcodeMap.count(e->op))
				throw parser::syntax_error(expr->getLocation(), "Invalid operator detected");

			opcode = _unaryOp2opcodeMap.at(e->op);
			switch (e->op) {
				case UnaryOp::INC_B:
				case UnaryOp::INC_F:
				case UnaryOp::DEC_B:
				case UnaryOp::DEC_F:
					compileLeftExpr(e->x);
					break;
				default:
					compileRightExpr(e->x);
			}

			if (isSuffixUnaryOp(e->op)) {
				compileRightExpr(e->x);
			}

			fn->insertIns({ opcode, {} });
			break;
		}
		case ExprKind::BINARY: {
			auto e = std::static_pointer_cast<BinaryOpExpr>(expr);
			compileRightExpr(e->y);
			if (e->op != BinaryOp::ASSIGN)
				compileRightExpr(e->x);

			Opcode opcode;
			if (_binaryOp2opcodeMap.count(e->op)) {
				opcode = _binaryOp2opcodeMap.at(e->op);
				fn->insertIns({ opcode, {} });
			} else if (e->op != BinaryOp::ASSIGN)
				throw parser::syntax_error(e->getLocation(), "Invalid operator detected");

			if (isAssignment(e->op)) {
				compileLeftExpr(e->x);
				fn->insertIns({ Opcode::STORE, {} });
			}

			break;
		}
		case ExprKind::CALL: {
			auto calle = std::static_pointer_cast<CallExpr>(expr);
			auto t = std::static_pointer_cast<FnTypeName>(evalExprType(calle->target));
			if (t->kind != TypeNameKind::FN)
				throw parser::syntax_error(calle->target->getLocation(), "Expression is not callable");

			Opcode opcode = Opcode::CALL;
			opcode = calle->isAsync ? Opcode::ACALL : Opcode::CALL;

			if (!t->isStatic) {
				evalExprType(calle);
				if (calleeParent)
					compileRightExpr(calleeParent);
				else
					fn->insertIns({ Opcode::LTHIS, {} });
				fn->insertIns({ Opcode::STHIS, {} });
			}

			for (auto &i : *(calle->args)) {
				compileRightExpr(i);
				fn->insertIns({ Opcode::SARG, {} });
			}

			auto ce = evalConstExpr(calle->target);
			if (ce)
				fn->insertIns({ opcode, { ce } });
			else {
				compileRightExpr(calle->target);
				fn->insertIns({ opcode, {} });
			}

			fn->insertIns({ Opcode::LRET, {} });

			calleeParent.reset();
			break;
		}
		case ExprKind::REF: {
			auto ref = std::static_pointer_cast<RefExpr>(expr);

			//
			// Local variables are unavailable if the flag was set.
			//
			if (!(isRecursing) && context.lvars.count(ref->name)) {
				fn->insertIns({ context.lvars.at(ref->name).isParam ? Opcode::LARG : Opcode::LLOAD,
					{ std::make_shared<UIntLiteralExpr>(ref->getLocation(), context.lvars[ref->name].stackPos) } });

				if (ref->next) {
					auto savedScope = scope;

					auto &lvarType = context.lvars[ref->name].type;
					if (lvarType->kind != TypeNameKind::CUSTOM)
						throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
					{
						auto t = Scope::getCustomType(std::static_pointer_cast<CustomTypeName>(lvarType));
						if (!t)
							throw parser::syntax_error(expr->getLocation(), "Type was not defined");
						if (!t->getScope())
							throw parser::syntax_error(expr->getLocation(), "Accessing member `" + ref->name + "' with unsupported type");
						scope = t->getScope();
					}
					compileRightExpr(ref->next, true);

					scope = savedScope;
				}
				break;
			}

			if (evalExprType(ref)) {
				bool isStatic = ref->isStatic;
				if (!isRecursing) {
					if ((!isStatic) && (!context.lvars.count(ref->name))) {
						auto v = scope->getVar(ref->name);
						if (v && v->isStatic())
							isStatic = true;
					}

					if (isStatic) {
						auto fullRef = scope->resolve();
						if (fullRef) {
							fullRef->next = ref;
							ref = fullRef;
						}
					} else if (!scope->parent.expired())
						fn->insertIns({ Opcode::LTHIS, {} }), isRecursing = true;
				}
				fn->insertIns({ isRecursing ? Opcode::RLOAD : Opcode::LOAD, { ref } });
				if (isLastResolvedVar)
					fn->insertIns({ Opcode::LVALUE, {} });
				break;
			}

			throw parser::syntax_error(expr->getLocation(), "`" + ref->name + "' was undefined");
		}
		case ExprKind::NEW: {
			auto newExpr = std::static_pointer_cast<NewExpr>(expr);

			switch (newExpr->type->kind) {
				case TypeNameKind::CUSTOM: {
					auto customTypeName = std::static_pointer_cast<CustomTypeName>(newExpr->type);	// Class type name
					auto customType = customTypeName->resolveType();
					if (!customType)
						throw parser::syntax_error(customTypeName->getLocation(), "Type `" + std::to_string(*customType) + "' not found");

					// Check if the type is constructible.
					auto constructor = customType->getScope()->getFn("new");
					if (!constructor)
						throw parser::syntax_error(customTypeName->getLocation(), "Type `" + std::to_string(*customType) + "' is not constructible");


					auto nExprArgs = newExpr->args->size(), nConstructorArgs = constructor->params->size();

					if (constructor->hasVarArg()) {
						if (nExprArgs < nConstructorArgs - 1) {
							throw parser::syntax_error(newExpr->args->front()->getLocation(), "Too few arguments");
						}
					} else {
						if (nExprArgs > nConstructorArgs)
							throw parser::syntax_error(newExpr->args->front()->getLocation(), "Too many arguments");
						if (nExprArgs < nConstructorArgs) {
							throw parser::syntax_error(newExpr->args->front()->getLocation(), "Too few arguments");
						}
					}

					std::uint8_t j = 0;	 // Index of current argument of the new expression.
					for (auto i : *(newExpr->args)) {
						auto type = evalExprType(i);

						if ((!constructor->hasVarArg()) || (j < nConstructorArgs - 1)) {
							if (!isSameType((*constructor->params).at(j)->typeName, type))
								throw parser::syntax_error(i->getLocation(), "Incompatible argument type");
						}

						compileArg(i);
						j++;
					}
					fn->insertIns({ Opcode::NEW, { customTypeName->typeRef } });
					break;
				}
				case TypeNameKind::ARRAY: {
					auto nArgs = newExpr->args->size();
					auto typeName = std::static_pointer_cast<ArrayTypeName>(newExpr->type);
					if (nArgs == 1) {
						auto arg = newExpr->args->front();
						auto argType = evalExprType(arg);

						if (!(isConvertible(argType, std::make_shared<TypeName>(location(), TypeNameKind::USIZE)) ||
								argType->kind == TypeNameKind::ARRAY))
							throw parser::syntax_error(arg->getLocation(), "Incompatible argument type");

						compileArg(arg);
					} else if (nArgs == 2) {
						auto size = newExpr->args->front();
						auto sizeType = evalExprType(size);
						if (!isConvertible(sizeType, std::make_shared<TypeName>(location(), TypeNameKind::USIZE)))
							throw parser::syntax_error(size->getLocation(), "Incompatible argument type");

						compileArg(size);

						auto initValue = newExpr->args->at(1);
						auto initValueType = evalExprType(initValue);

						if (isConvertible(initValueType, typeName->type))
							;
						else if (initValueType->kind == TypeNameKind::ARRAY) {
							for (auto i : *(newExpr->args)) {
								if (!isConvertible(typeName->type, evalExprType(i)))
									throw parser::syntax_error(newExpr->args->front()->getLocation(), "Incompatible argument type");
							}
						} else
							throw parser::syntax_error(initValue->getLocation(), "Incompatible argument type");

						compileArg(initValue);
					} else if (nArgs >= 3)
						throw parser::syntax_error(newExpr->args->front()->getLocation(), "Too many arguments");
					break;
				}
				case TypeNameKind::MAP: {
					auto nArgs = newExpr->args->size();
					if (nArgs > 1)
						throw parser::syntax_error(newExpr->args->front()->getLocation(), "Too many arguments");

					if (nArgs) {
						auto initValue = std::static_pointer_cast<MapExpr>(newExpr->args->front());
						auto initValueType = std::static_pointer_cast<MapTypeName>(evalExprType(initValue));
						if (initValueType->kind != TypeNameKind::MAP)
							throw parser::syntax_error(initValue->getLocation(), "Incompatible argument type");

						for (auto i : *(initValue->pairs)) {
							if (!isConvertible(initValueType->keyType, evalExprType(i->first)))
								throw parser::syntax_error(newExpr->args->front()->getLocation(), "Incompatible key type");
							if (!isConvertible(initValueType->valueType, evalExprType(i->second)))
								throw parser::syntax_error(newExpr->args->front()->getLocation(), "Incompatible value type");
						}
						compileArg(initValue);
					}
					break;
				}
				default:
					throw parser::syntax_error(newExpr->type->getLocation(), "Type `" + std::to_string(*newExpr->type) + "' is not constructible");
			}
			break;
		}
		default:
			throw std::logic_error("Invalid expression type detected");
	}

	clearTempAttribs();
}
