#include "../compiler.h"
#include <type_traits>
#include <cmath>

using namespace slake::slkc;

struct UnaryOpRegistry {
	slake::Opcode opcode;
	bool lvalueOperand;

	inline UnaryOpRegistry(
		slake::Opcode opcode,
		bool lvalueOperand) : opcode(opcode), lvalueOperand(lvalueOperand) {}
};

static map<UnaryOp, UnaryOpRegistry> _unaryOpRegs = {
	{ OP_NOT, { slake::Opcode::NOT, false } },
	{ OP_REV, { slake::Opcode::REV, false } },
	{ OP_INCF, { slake::Opcode::INCF, true } },
	{ OP_DECF, { slake::Opcode::DECF, true } },
	{ OP_INCB, { slake::Opcode::INCB, true } },
	{ OP_DECB, { slake::Opcode::DECB, true } }
};

struct BinaryOpRegistry {
	slake::Opcode opcode;
	bool isLhsLvalue;
	bool isRhsLvalue;

	inline BinaryOpRegistry(
		slake::Opcode opcode,
		bool isLhsLvalue,
		bool isRhsLvalue)
		: opcode(opcode),
		  isLhsLvalue(isLhsLvalue),
		  isRhsLvalue(isRhsLvalue) {}
};

static map<BinaryOp, BinaryOpRegistry> _binaryOpRegs = {
	{ OP_ADD, { slake::Opcode::ADD, false, false } },
	{ OP_SUB, { slake::Opcode::SUB, false, false } },
	{ OP_MUL, { slake::Opcode::MUL, false, false } },
	{ OP_DIV, { slake::Opcode::DIV, false, false } },
	{ OP_MOD, { slake::Opcode::MOD, false, false } },
	{ OP_AND, { slake::Opcode::AND, false, false } },
	{ OP_OR, { slake::Opcode::OR, false, false } },
	{ OP_XOR, { slake::Opcode::XOR, false, false } },
	{ OP_LAND, { slake::Opcode::LAND, false, false } },
	{ OP_LOR, { slake::Opcode::LOR, false, false } },
	{ OP_LSH, { slake::Opcode::LSH, false, false } },
	{ OP_RSH, { slake::Opcode::RSH, false, false } },
	{ OP_SWAP, { slake::Opcode::SWAP, true, true } },

	{ OP_ASSIGN, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_ADD, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_SUB, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_MUL, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_DIV, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_MOD, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_AND, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_OR, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_XOR, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_LAND, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_LOR, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_LSH, { slake::Opcode::NOP, true, false } },
	{ OP_ASSIGN_RSH, { slake::Opcode::NOP, true, false } },

	{ OP_EQ, { slake::Opcode::EQ, false, false } },
	{ OP_NEQ, { slake::Opcode::NEQ, false, false } },
	{ OP_STRICTEQ, { slake::Opcode::SEQ, false, false } },
	{ OP_STRICTNEQ, { slake::Opcode::SNEQ, false, false } },
	{ OP_LT, { slake::Opcode::LT, false, false } },
	{ OP_GT, { slake::Opcode::GT, false, false } },
	{ OP_LTEQ, { slake::Opcode::LTEQ, false, false } },
	{ OP_GTEQ, { slake::Opcode::GTEQ, false, false } },
	{ OP_SUBSCRIPT, { slake::Opcode::AT, false, false } }
};

static map<BinaryOp, BinaryOp> _assignBinaryOpToOrdinaryBinaryOpMap = {
	{ OP_ASSIGN, OP_ASSIGN },
	{ OP_ASSIGN_ADD, OP_ADD },
	{ OP_ASSIGN_SUB, OP_SUB },
	{ OP_ASSIGN_MUL, OP_MUL },
	{ OP_ASSIGN_DIV, OP_DIV },
	{ OP_ASSIGN_MOD, OP_MOD },
	{ OP_ASSIGN_AND, OP_AND },
	{ OP_ASSIGN_OR, OP_OR },
	{ OP_ASSIGN_XOR, OP_XOR },
	{ OP_ASSIGN_LAND, OP_LAND },
	{ OP_ASSIGN_LOR, OP_LOR },
	{ OP_ASSIGN_LSH, OP_LSH },
	{ OP_ASSIGN_RSH, OP_RSH }
};

void Compiler::compileExpr(shared_ptr<ExprNode> expr) {
	if (auto ce = evalConstExpr(expr); ce) {
		context.curFn->insertIns(Opcode::STORE, context.evalDest, ce);
		return;
	}

	switch (expr->getExprType()) {
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			bool restoreR0 = preserveRegister(RegId::R0);

			compileExpr(
				e->x,
				_unaryOpRegs.at(e->op).lvalueOperand ? EvalPurpose::LVALUE : EvalPurpose::RVALUE,
				make_shared<RegRefNode>(RegId::R0));
			if (context.evalPurpose == EvalPurpose::STMT) {
				if (_unaryOpRegs.at(e->op).lvalueOperand) {
					context.curFn->insertIns(_unaryOpRegs.at(e->op).opcode, make_shared<RegRefNode>(RegId::R0), make_shared<RegRefNode>(RegId::R0));
				}
			} else {
				context.curFn->insertIns(_unaryOpRegs.at(e->op).opcode, context.evalDest, make_shared<RegRefNode>(RegId::R0));
			}

			if (restoreR0)
				restoreRegister(RegId::R0);
			break;
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			bool restoreR0 = preserveRegister(RegId::R0),
				 restoreR1 = preserveRegister(RegId::R1);

			auto lhsType = evalExprType(e->lhs), rhsType = evalExprType(e->rhs);

			compileExpr(e->lhs, _binaryOpRegs.at(e->op).isLhsLvalue ? EvalPurpose::LVALUE : EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R0));

			setRegisterPreserved(RegId::R0);
			if (!isSameType(lhsType, rhsType)) {
				if (!areTypesConvertible(rhsType, lhsType))
					throw FatalCompilationError(
						{ e->rhs->getLocation(),
							MSG_ERROR,
							"Incompatible initial value type" });

				compileExpr(make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs), _binaryOpRegs.at(e->op).isRhsLvalue ? EvalPurpose::LVALUE : EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R1));
			} else
				compileExpr(e->rhs, _binaryOpRegs.at(e->op).isRhsLvalue ? EvalPurpose::LVALUE : EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R1));
			unsetRegisterPreserved(RegId::R0);

			if (isAssignBinaryOp(e->op)) {
				// RHS of the assignment expression.
				if (auto opcode = _binaryOpRegs.at(_assignBinaryOpToOrdinaryBinaryOpMap.at(e->op)).opcode; opcode != Opcode::NOP) {
					context.curFn->insertIns(
						Opcode::LVALUE,
						make_shared<RegRefNode>(RegId::TMP0),
						make_shared<RegRefNode>(RegId::R0));
					context.curFn->insertIns(
						opcode,
						make_shared<RegRefNode>(RegId::R1),
						make_shared<RegRefNode>(RegId::TMP0),
						make_shared<RegRefNode>(RegId::R1));
				}

				// LHS of the assignment expression.
				context.curFn->insertIns(Opcode::ISTORE, make_shared<RegRefNode>(RegId::R0), make_shared<RegRefNode>(RegId::R1));

				if ((context.evalPurpose != EvalPurpose::STMT) && (context.evalDest))
					context.curFn->insertIns(Opcode::STORE, static_pointer_cast<RegRefNode>(context.evalDest), make_shared<RegRefNode>(RegId::R1));
			} else if (context.evalPurpose != EvalPurpose::STMT) {
				context.curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					context.evalDest,
					make_shared<RegRefNode>(RegId::R0),
					make_shared<RegRefNode>(RegId::R1));
			}

			if (restoreR0)
				restoreRegister(RegId::R0);
			if (restoreR1)
				restoreRegister(RegId::R1);

			break;
		}
		case EXPR_TERNARY: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			auto loc = e->getLocation();
			string falseBranchLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_false",
				   endLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			bool restoreR0 = preserveRegister(RegId::R0);

			compileExpr(e->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R0));
			if (evalExprType(e->condition)->getTypeId() != TYPE_BOOL)
				context.curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(RegId::R0),
					make_shared<BoolTypeNameNode>(e->getLocation(), true), make_shared<RegRefNode>(RegId::R0));

			context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(falseBranchLabel), make_shared<RegRefNode>(RegId::R0));

			// Compile the true expression.
			compileExpr(e->x, EvalPurpose::RVALUE, context.evalDest);
			context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

			// Compile the false expression.
			context.curFn->insertLabel(falseBranchLabel);
			compileExpr(e->y, EvalPurpose::RVALUE, context.evalDest);

			context.curFn->insertLabel(endLabel);

			if (restoreR0)
				restoreRegister(RegId::R0);

			break;
		}
		case EXPR_MATCH: {
			auto e = static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->getLocation();

			string labelPrefix = "$match_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			bool restoreR2 = preserveRegister(RegId::R2);

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::R2));
			auto condVar = (context.localVars[condLocalVarName] = make_shared<LocalVarNode>(context.localVars.size(), make_shared<AnyTypeNameNode>(loc, false)));

			pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>> defaultCase;

			for (auto i : e->cases) {
				string caseEndLabel = "$match_" + to_string(i.second->getLocation().line) + "_" + to_string(i.second->getLocation().column) + "_caseEnd";

				if (!i.first) {
					if (defaultCase.second)
						// The default case is already exist.
						throw FatalCompilationError(
							{ i.second->getLocation(),
								MSG_ERROR,
								"Duplicated default case" });
					defaultCase = i;
				}

				compileExpr(i.first, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::EQ, make_shared<RegRefNode>(RegId::TMP0), make_shared<RegRefNode>(RegId::R2), make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(caseEndLabel), make_shared<RegRefNode>(RegId::TMP0));

				// Leave the minor stack that is created for the local variable.
				compileExpr(i.second, context.evalPurpose, context.evalDest);
				;

				context.curFn->insertLabel(caseEndLabel);
				context.curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			if (defaultCase.second)
				compileExpr(defaultCase.second);

			context.curFn->insertLabel(endLabel);

			if (restoreR2)
				restoreRegister(RegId::R2);

			break;
		}
		case EXPR_CLOSURE: {
			auto e = static_pointer_cast<ClosureExprNode>(expr);
			break;
		}
		case EXPR_CALL: {
			auto e = static_pointer_cast<CallExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				context.curFn->insertIns(Opcode::CALL, ce);
			} else {
				bool restoreR0 = preserveRegister(RegId::R0);

				for (auto i : e->args) {
					if (auto ce = evalConstExpr(i); ce)
						context.curFn->insertIns(Opcode::PUSHARG, ce);
					else {
						compileExpr(i, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
						context.curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(RegId::TMP0));
					}
				}

				compileExpr(e->target, EvalPurpose::CALL, make_shared<RegRefNode>(RegId::R0));

				if (context.isLastCallTargetStatic)
					context.curFn->insertIns(
						e->isAsync ? Opcode::ACALL : Opcode::CALL,
						make_shared<RegRefNode>(RegId::R0));
				else
					context.curFn->insertIns(
						e->isAsync ? Opcode::AMCALL : Opcode::MCALL,
						make_shared<RegRefNode>(RegId::R0),
						make_shared<RegRefNode>(RegId::TMP1));

				if (context.evalPurpose != EvalPurpose::STMT) {
					context.curFn->insertIns(
						Opcode::STORE,
						context.evalDest,
						make_shared<RegRefNode>(RegId::RR));
				}

				if (restoreR0)
					restoreRegister(RegId::R0);
			}

			break;
		}
		case EXPR_AWAIT: {
			auto e = static_pointer_cast<AwaitExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				context.curFn->insertIns(Opcode::AWAIT, ce);
			} else {
				bool restoreR0 = preserveRegister(RegId::R0);

				// TODO: Check if the target is a method.
				compileExpr(e->target, context.evalPurpose, make_shared<RegRefNode>(RegId::R0));
				context.curFn->insertIns(Opcode::AWAIT, make_shared<RegRefNode>(RegId::R0));

				if (restoreR0)
					restoreRegister(RegId::R0);
			}

			break;
		}
		case EXPR_NEW: {
			auto e = static_pointer_cast<NewExprNode>(expr);

			for (auto i : e->args) {
				if (auto ce = evalConstExpr(i); ce)
					context.curFn->insertIns(Opcode::PUSHARG, ce);
				else {
					compileExpr(i, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
					context.curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(RegId::TMP0));
				}
			}

			context.curFn->insertIns(Opcode::NEW, context.evalDest, e->type);
			break;
		}
		case EXPR_TYPEOF: {
			auto e = static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				context.curFn->insertIns(Opcode::TYPEOF, context.evalDest, ce);
			} else {
				compileExpr(e->target, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
				context.curFn->insertIns(Opcode::TYPEOF, context.evalDest, make_shared<RegRefNode>(RegId::TMP0));
			}

			break;
		}
		case EXPR_CAST: {
			auto e = static_pointer_cast<CastExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				if (areTypesConvertible(evalExprType(ce), e->targetType)) {
					context.curFn->insertIns(Opcode::CAST, context.evalDest, e->targetType, ce);
				} else {
					throw FatalCompilationError({ e->getLocation(), MSG_ERROR, "Invalid type conversion" });
				}
			} else {
				if (areTypesConvertible(evalExprType(e->target), e->targetType)) {
					compileExpr(e->target, EvalPurpose::RVALUE, make_shared<RegRefNode>(RegId::TMP0));
					context.curFn->insertIns(Opcode::CAST, context.evalDest, e->targetType, make_shared<RegRefNode>(RegId::TMP0));
				} else {
					throw FatalCompilationError({ e->getLocation(), MSG_ERROR, "Invalid type conversion" });
				}
			}

			break;
		}
		case EXPR_REF: {
			auto e = static_pointer_cast<RefExprNode>(expr);

			deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;
			if (!resolveRef(e->ref, resolvedParts))
				// The default case is already exist.
				throw FatalCompilationError(
					{ e->getLocation(),
						MSG_ERROR,
						"Identifier not found: `" + to_string(e->ref) + "'" });

			if (context.evalPurpose == EvalPurpose::CALL) {
				if (isDynamicMember(resolvedParts.back().second)) {
					Ref thisRef = e->ref;
					thisRef.pop_back();
					compileExpr(
						make_shared<RefExprNode>(thisRef),
						EvalPurpose::RVALUE,
						make_shared<RegRefNode>(RegId::TMP1));
					context.isLastCallTargetStatic = false;
				} else
					context.isLastCallTargetStatic = true;
			}

			auto &x = resolvedParts.front().second;
			switch (x->getNodeType()) {
				case AST_LOCAL_VAR:
					context.curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(RegId::TMP0),
						make_shared<LocalVarRefNode>(
							static_pointer_cast<LocalVarNode>(x)->index,
							resolvedParts.size() > 1
								? true
								: context.evalPurpose == EvalPurpose::LVALUE));

					resolvedParts.pop_front();
					for (auto i : resolvedParts) {
						context.curFn->insertIns(
							Opcode::RLOAD,
							make_shared<RegRefNode>(RegId::TMP0),
							make_shared<RegRefNode>(RegId::TMP0),
							make_shared<RefExprNode>(i.first));
					}

					if (context.evalPurpose == EvalPurpose::RVALUE)
						context.curFn->insertIns(
							Opcode::LVALUE,
							make_shared<RegRefNode>(RegId::TMP0),
							make_shared<RegRefNode>(RegId::TMP0));

					context.curFn->insertIns(
						Opcode::STORE,
						context.evalDest,
						make_shared<RegRefNode>(RegId::TMP0));
					break;
				case AST_ARG_REF:
					static_pointer_cast<ArgRefNode>(x)->unwrapData = (context.evalPurpose == EvalPurpose::RVALUE);
					context.curFn->insertIns(
						Opcode::STORE,
						context.evalDest, x);
					break;
				case AST_VAR:
				case AST_FN:
				case AST_REG_REF: {
					switch (x->getNodeType()) {
						case AST_VAR:
							context.curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(RegId::TMP0),
								make_shared<RefExprNode>(resolvedParts.front().first));
							break;
						case AST_FN:
							context.curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(RegId::TMP0),
								make_shared<RefExprNode>(resolvedParts.front().first));
							break;
						case AST_REG_REF:
							context.curFn->insertIns(
								Opcode::STORE,
								make_shared<RegRefNode>(RegId::TMP0),
								x);
							break;
						default:
							assert(false);
					}

					// Check if the target is static.
					resolvedParts.pop_front();
					for (auto i : resolvedParts) {
						context.curFn->insertIns(
							Opcode::RLOAD,
							make_shared<RegRefNode>(RegId::TMP0),
							make_shared<RegRefNode>(RegId::TMP0),
							make_shared<RefExprNode>(i.first));
					}

					if (context.evalPurpose == EvalPurpose::RVALUE)
						context.curFn->insertIns(
							Opcode::LVALUE,
							make_shared<RegRefNode>(RegId::TMP0),
							make_shared<RegRefNode>(RegId::TMP0));

					context.curFn->insertIns(
						Opcode::STORE,
						context.evalDest,
						make_shared<RegRefNode>(RegId::TMP0));
					break;
				}
				default:
					assert(false);
			}

			break;
		}
		case EXPR_I8:
		case EXPR_I16:
		case EXPR_I32:
		case EXPR_I64:
		case EXPR_U32:
		case EXPR_U64:
		case EXPR_F32:
		case EXPR_F64:
		case EXPR_STRING:
		case EXPR_BOOL:
		case EXPR_ARRAY:
		case EXPR_MAP:
			context.curFn->insertIns(Opcode::STORE, context.evalDest, expr);
			break;
		default:
			assert(false);
	}
}
