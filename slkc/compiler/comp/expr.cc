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
	slxfmt::SourceLocDesc sld;
	sld.offIns = curFn->body.size();
	sld.line = expr->getLocation().line;
	sld.column = expr->getLocation().column;

	if (auto ce = evalConstExpr(expr); ce) {
		curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, ce);
		return;
	}

	switch (expr->getExprType()) {
		case EXPR_UNARY: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			uint32_t lhsRegIndex = allocReg();

			compileExpr(
				e->x,
				_unaryOpRegs.at(e->op).lvalueOperand ? EvalPurpose::LVALUE : EvalPurpose::RVALUE,
				make_shared<RegRefNode>(lhsRegIndex));
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::STMT) {
				if (_unaryOpRegs.at(e->op).lvalueOperand) {
					curFn->insertIns(_unaryOpRegs.at(e->op).opcode, make_shared<RegRefNode>(lhsRegIndex), make_shared<RegRefNode>(lhsRegIndex));
				}
			} else {
				curFn->insertIns(_unaryOpRegs.at(e->op).opcode, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(lhsRegIndex, true));
			}

			break;
		}
		case EXPR_BINARY: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			uint32_t lhsRegIndex = allocReg(),
					 rhsRegIndex = allocReg();

			auto lhsType = evalExprType(e->lhs), rhsType = evalExprType(e->rhs);

			compileExpr(
				e->lhs,
				_binaryOpRegs.at(e->op).isLhsLvalue
					? EvalPurpose::LVALUE
					: EvalPurpose::RVALUE,
				make_shared<RegRefNode>(lhsRegIndex));

			if (!isSameType(lhsType, rhsType)) {
				if (!areTypesConvertible(rhsType, lhsType))
					throw FatalCompilationError(
						{ e->rhs->getLocation(),
							MSG_ERROR,
							"Incompatible initial value type" });

				compileExpr(
					make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
					_binaryOpRegs.at(e->op).isRhsLvalue
						? EvalPurpose::LVALUE
						: EvalPurpose::RVALUE,
					make_shared<RegRefNode>(rhsRegIndex));
			} else
				compileExpr(e->rhs, _binaryOpRegs.at(e->op).isRhsLvalue ? EvalPurpose::LVALUE : EvalPurpose::RVALUE, make_shared<RegRefNode>(rhsRegIndex));

			if (isAssignBinaryOp(e->op)) {
				// RHS of the assignment expression.
				if (auto opcode = _binaryOpRegs.at(_assignBinaryOpToOrdinaryBinaryOpMap.at(e->op)).opcode; opcode != Opcode::NOP) {
					uint32_t lhsValueRegIndex = allocReg();

					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(lhsValueRegIndex),
						make_shared<RegRefNode>(lhsRegIndex, true));
					curFn->insertIns(
						opcode,
						make_shared<RegRefNode>(rhsRegIndex),
						make_shared<RegRefNode>(lhsValueRegIndex, true),
						make_shared<RegRefNode>(rhsRegIndex, true));
				}

				// LHS of the assignment expression.
				curFn->insertIns(Opcode::STORE, make_shared<RegRefNode>(lhsRegIndex, true), make_shared<RegRefNode>(rhsRegIndex, true));

				if ((curMajorContext.curMinorContext.evalPurpose != EvalPurpose::STMT) && (curMajorContext.curMinorContext.evalDest))
					curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(rhsRegIndex, true));
			} else if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::STMT) {
				curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					curMajorContext.curMinorContext.evalDest,
					make_shared<RegRefNode>(lhsRegIndex, true),
					make_shared<RegRefNode>(rhsRegIndex, true));
			} else
				curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					make_shared<RegRefNode>(lhsRegIndex),
					make_shared<RegRefNode>(lhsRegIndex, true),
					make_shared<RegRefNode>(rhsRegIndex, true));

			break;
		}
		case EXPR_TERNARY: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			uint32_t conditionRegIndex = allocReg();

			auto loc = e->getLocation();
			string falseBranchLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_false",
				   endLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			compileExpr(e->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(conditionRegIndex));
			if (evalExprType(e->condition)->getTypeId() != TYPE_BOOL)
				curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(conditionRegIndex),
					make_shared<BoolTypeNameNode>(e->getLocation(), true),
					make_shared<RegRefNode>(conditionRegIndex, true));

			curFn->insertIns(
				Opcode::JF,
				make_shared<LabelRefNode>(falseBranchLabel),
				make_shared<RegRefNode>(conditionRegIndex, true));

			// Compile the true expression.
			compileExpr(e->x, EvalPurpose::RVALUE, curMajorContext.curMinorContext.evalDest);
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

			// Compile the false expression.
			curFn->insertLabel(falseBranchLabel);
			compileExpr(e->y, EvalPurpose::RVALUE, curMajorContext.curMinorContext.evalDest);

			curFn->insertLabel(endLabel);

			break;
		}
		case EXPR_MATCH: {
			auto e = static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->getLocation();

			string labelPrefix = "$match_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			uint32_t matcheeRegIndex = allocReg();

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RVALUE, make_shared<RegRefNode>(matcheeRegIndex));

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

				uint32_t conditionRegIndex = allocReg();

				compileExpr(i.first, EvalPurpose::RVALUE, make_shared<RegRefNode>(conditionRegIndex));
				curFn->insertIns(
					Opcode::EQ,
					make_shared<RegRefNode>(conditionRegIndex),
					make_shared<RegRefNode>(matcheeRegIndex, true),
					make_shared<RegRefNode>(conditionRegIndex, true));
				curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(caseEndLabel), make_shared<RegRefNode>(conditionRegIndex, true));

				// Leave the minor stack that is created for the local variable.
				compileExpr(i.second, curMajorContext.curMinorContext.evalPurpose, curMajorContext.curMinorContext.evalDest);

				curFn->insertLabel(caseEndLabel);
				curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			if (defaultCase.second)
				compileExpr(defaultCase.second);

			curFn->insertLabel(endLabel);

			break;
		}
		case EXPR_CLOSURE: {
			auto e = static_pointer_cast<ClosureExprNode>(expr);
			break;
		}
		case EXPR_CALL: {
			auto e = static_pointer_cast<CallExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				curFn->insertIns(Opcode::CALL, ce);
			} else {
				uint32_t callTargetRegIndex = allocReg();
				uint32_t tmpRegIndex = allocReg();

				for (auto i : e->args) {
					if (auto ce = evalConstExpr(i); ce)
						curFn->insertIns(Opcode::PUSHARG, ce);
					else {
						compileExpr(i, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
						curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
					}
				}

				compileExpr(e->target, EvalPurpose::CALL, make_shared<RegRefNode>(callTargetRegIndex), make_shared<RegRefNode>(tmpRegIndex));

				if (curMajorContext.curMinorContext.isLastCallTargetStatic)
					curFn->insertIns(
						e->isAsync ? Opcode::ACALL : Opcode::CALL,
						make_shared<RegRefNode>(callTargetRegIndex, true));
				else
					curFn->insertIns(
						e->isAsync ? Opcode::AMCALL : Opcode::MCALL,
						make_shared<RegRefNode>(callTargetRegIndex, true),
						make_shared<RegRefNode>(tmpRegIndex));

				if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::STMT) {
					curFn->insertIns(
						Opcode::LRET,
						curMajorContext.curMinorContext.evalDest);
				}
			}

			break;
		}
		case EXPR_AWAIT: {
			uint32_t awaitTargetRegIndex = allocReg();

			auto e = static_pointer_cast<AwaitExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				curFn->insertIns(Opcode::AWAIT, ce);
			} else {
				// TODO: Check if the target is a method.
				// compileExpr(e->target, curMajorContext.curMinorContext.evalPurpose, make_shared<RegRefNode>(RegId::R0));
				// curFn->insertIns(Opcode::AWAIT, make_shared<RegRefNode>(RegId::R0));
			}

			break;
		}
		case EXPR_NEW: {
			auto e = static_pointer_cast<NewExprNode>(expr);

			for (auto i : e->args) {
				if (auto ce = evalConstExpr(i); ce)
					curFn->insertIns(Opcode::PUSHARG, ce);
				else {
					uint32_t tmpRegIndex = allocReg();
					compileExpr(i, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			curFn->insertIns(Opcode::NEW, curMajorContext.curMinorContext.evalDest, e->type);
			break;
		}
		case EXPR_TYPEOF: {
			auto e = static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				curFn->insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, ce);
			} else {
				uint32_t tmpRegIndex = allocReg();

				compileExpr(e->target, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
				curFn->insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(tmpRegIndex));
			}

			break;
		}
		case EXPR_CAST: {
			auto e = static_pointer_cast<CastExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				if (areTypesConvertible(evalExprType(ce), e->targetType)) {
					curFn->insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, ce);
				} else {
					throw FatalCompilationError({ e->getLocation(), MSG_ERROR, "Invalid type conversion" });
				}
			} else {
				if (areTypesConvertible(evalExprType(e->target), e->targetType)) {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(e->target, EvalPurpose::RVALUE, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, make_shared<RegRefNode>(tmpRegIndex, true));
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

			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::CALL) {
				if (isDynamicMember(resolvedParts.back().second)) {
					Ref thisRef = e->ref;
					thisRef.pop_back();
					compileExpr(
						make_shared<RefExprNode>(thisRef),
						EvalPurpose::RVALUE,
						curMajorContext.curMinorContext.thisDest);
					curMajorContext.curMinorContext.isLastCallTargetStatic = false;
				} else
					curMajorContext.curMinorContext.isLastCallTargetStatic = true;
			}

			uint32_t tmpRegIndex = allocReg();

			auto &x = resolvedParts.front().second;
			switch (x->getNodeType()) {
				case AST_LOCAL_VAR:
					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(tmpRegIndex),
						make_shared<LocalVarRefNode>(
							static_pointer_cast<LocalVarNode>(x)->index,
							resolvedParts.size() > 1
								? true
								: curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LVALUE));

					resolvedParts.pop_front();

					if (resolvedParts.size()) {
						for (auto i : resolvedParts) {
							curFn->insertIns(
								Opcode::RLOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true),
								make_shared<RefExprNode>(i.first));
						}

						if (resolvedParts.back().second->getNodeType() == AST_VAR) {
							if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RVALUE) {
								curFn->insertIns(
									Opcode::LVALUE,
									make_shared<RegRefNode>(tmpRegIndex),
									make_shared<RegRefNode>(tmpRegIndex, true));
							}
						}
					}

					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest,
						make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				case AST_ARG_REF:
					static_pointer_cast<ArgRefNode>(x)->unwrapData = (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RVALUE);
					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest, x);
					break;
				case AST_VAR:
				case AST_FN:
				case AST_THIS_REF: {
					switch (x->getNodeType()) {
						case AST_VAR: {
							Ref ref;
							_getFullName((VarNode *)resolvedParts.front().second.get(), ref);
							curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RefExprNode>(ref));
							break;
						}
						case AST_FN: {
							Ref ref;
							_getFullName((FnNode *)resolvedParts.front().second.get(), ref);
							curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RefExprNode>(ref));
							break;
						}
						case AST_THIS_REF:
							curFn->insertIns(Opcode::LTHIS, make_shared<RegRefNode>(tmpRegIndex));
							break;
						default:
							assert(false);
					}

					// Check if the target is static.
					resolvedParts.pop_front();
					for (auto i : resolvedParts) {
						curFn->insertIns(
							Opcode::RLOAD,
							make_shared<RegRefNode>(tmpRegIndex),
							make_shared<RegRefNode>(tmpRegIndex, true),
							make_shared<RefExprNode>(i.first));
					}

					if (resolvedParts.size()) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RVALUE)
							curFn->insertIns(
								Opcode::LVALUE,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true));
					}

					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest,
						make_shared<RegRefNode>(tmpRegIndex, true));
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
			curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, expr);
			break;
		default:
			assert(false);
	}

	sld.nIns = curFn->body.size() - sld.offIns;
	curFn->srcLocDescs.push_back(sld);
}
