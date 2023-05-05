#include <slkparse.hh>

#include "misc.hh"

using namespace Slake;
using namespace Slake::Compiler;

void State::compileLeftExpr(std::shared_ptr<Expr> expr, bool isRecursing) {
	auto &fn = fnDefs[currentFn];
	assert(fn);
	switch (expr->getExprKind()) {
		case ExprKind::TERNARY: {
			auto falseLabel = "$TOP_FALSE_" + std::to_string(fn->body.size()),
				 endLabel = "$TOP_END_" + std::to_string(fn->body.size());

			auto e = std::static_pointer_cast<TernaryOpExpr>(expr);
			compileRightExpr(e->condition);

			fn->insertIns({ Opcode::JF, { std::make_shared<LabelExpr>(falseLabel) } });
			compileLeftExpr(e->x);	// True branch.
			fn->insertIns({ Opcode::JMP, { std::make_shared<LabelExpr>(endLabel) } });
			fn->insertLabel(falseLabel);
			compileLeftExpr(e->y);	// False branch.
			fn->insertLabel(endLabel);
			break;
		}
		case ExprKind::UNARY: {
			auto e = std::static_pointer_cast<UnaryOpExpr>(expr);

			switch (e->op) {
				case UnaryOp::INC_F:
				case UnaryOp::INC_B:
				case UnaryOp::DEC_F:
				case UnaryOp::DEC_B: {
					compileLeftExpr(e->x);
					fn->insertIns({ _unaryOp2opcodeMap.at(e->op), {} });
					break;
				}
				case UnaryOp::NEG:
				case UnaryOp::NOT:
				case UnaryOp::REV: {
					compileRightExpr(e->x);
					fn->insertIns({ _unaryOp2opcodeMap.at(e->op), {} });
					break;
				}
			}
			throw parser::syntax_error(expr->getLocation(), "Expected a left value");
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
					compileLeftExpr(ref->next, true);

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
				break;
			}

			throw parser::syntax_error(expr->getLocation(), "`" + ref->name + "' was undefined");
		}
		default:
			throw parser::syntax_error(expr->getLocation(), "Expected a left value");
	}

	clearTempAttribs();
}
