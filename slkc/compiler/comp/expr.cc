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
	{ UnaryOp::LNot, { slake::Opcode::LNOT, false } },
	{ UnaryOp::Not, { slake::Opcode::NOT, false } },
	{ UnaryOp::IncF, { slake::Opcode::INCF, true } },
	{ UnaryOp::DecF, { slake::Opcode::DECF, true } },
	{ UnaryOp::IncB, { slake::Opcode::INCB, true } },
	{ UnaryOp::DecB, { slake::Opcode::DECB, true } }
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
	{ BinaryOp::Add, { slake::Opcode::ADD, false, false } },
	{ BinaryOp::Sub, { slake::Opcode::SUB, false, false } },
	{ BinaryOp::Mul, { slake::Opcode::MUL, false, false } },
	{ BinaryOp::Div, { slake::Opcode::DIV, false, false } },
	{ BinaryOp::Mod, { slake::Opcode::MOD, false, false } },
	{ BinaryOp::And, { slake::Opcode::AND, false, false } },
	{ BinaryOp::Or, { slake::Opcode::OR, false, false } },
	{ BinaryOp::Xor, { slake::Opcode::XOR, false, false } },
	{ BinaryOp::LAnd, { slake::Opcode::LAND, false, false } },
	{ BinaryOp::LOr, { slake::Opcode::LOR, false, false } },
	{ BinaryOp::Lsh, { slake::Opcode::LSH, false, false } },
	{ BinaryOp::Rsh, { slake::Opcode::RSH, false, false } },
	{ BinaryOp::Swap, { slake::Opcode::SWAP, true, true } },

	{ BinaryOp::Assign, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignAdd, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignSub, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignMul, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignDiv, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignMod, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignAnd, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignOr, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignXor, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignLsh, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignRsh, { slake::Opcode::NOP, true, false } },

	{ BinaryOp::Eq, { slake::Opcode::EQ, false, false } },
	{ BinaryOp::Neq, { slake::Opcode::NEQ, false, false } },
	{ BinaryOp::StrictEq, { slake::Opcode::SEQ, false, false } },
	{ BinaryOp::StrictNeq, { slake::Opcode::SNEQ, false, false } },
	{ BinaryOp::Lt, { slake::Opcode::LT, false, false } },
	{ BinaryOp::Gt, { slake::Opcode::GT, false, false } },
	{ BinaryOp::LtEq, { slake::Opcode::LTEQ, false, false } },
	{ BinaryOp::GtEq, { slake::Opcode::GTEQ, false, false } },
	{ BinaryOp::Subscript, { slake::Opcode::AT, false, false } }
};

static map<BinaryOp, BinaryOp> _assignBinaryOpToOrdinaryBinaryOpMap = {
	{ BinaryOp::Assign, BinaryOp::Assign },
	{ BinaryOp::AssignAdd, BinaryOp::Add },
	{ BinaryOp::AssignSub, BinaryOp::Sub },
	{ BinaryOp::AssignMul, BinaryOp::Mul },
	{ BinaryOp::AssignDiv, BinaryOp::Div },
	{ BinaryOp::AssignMod, BinaryOp::Mod },
	{ BinaryOp::AssignAnd, BinaryOp::And },
	{ BinaryOp::AssignOr, BinaryOp::Or },
	{ BinaryOp::AssignXor, BinaryOp::Xor },
	{ BinaryOp::AssignLsh, BinaryOp::Lsh },
	{ BinaryOp::AssignRsh, BinaryOp::Rsh }
};

void Compiler::compileExpr(shared_ptr<ExprNode> expr) {
	slxfmt::SourceLocDesc sld;
	sld.offIns = curFn->body.size();
	sld.line = expr->getLocation().line;
	sld.column = expr->getLocation().column;

	if (auto ce = evalConstExpr(expr); ce) {
		if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
			throw FatalCompilationError(
				Message(
					expr->getLocation(),
					MessageType::Error,
					"Expecting a lvalue expression"));

		curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, ce);
		return;
	}

	switch (expr->getExprType()) {
		case ExprType::Unary: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			uint32_t lhsRegIndex = allocReg();

			compileExpr(
				e->x,
				_unaryOpRegs.at(e->op).lvalueOperand ? EvalPurpose::LValue : EvalPurpose::RValue,
				make_shared<RegRefNode>(lhsRegIndex));
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::Stmt) {
				if (_unaryOpRegs.at(e->op).lvalueOperand) {
					curFn->insertIns(_unaryOpRegs.at(e->op).opcode, make_shared<RegRefNode>(lhsRegIndex), make_shared<RegRefNode>(lhsRegIndex));
				}
			} else {
				curFn->insertIns(_unaryOpRegs.at(e->op).opcode, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(lhsRegIndex, true));
			}

			break;
		}
		case ExprType::Binary: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			auto lhsType = evalExprType(e->lhs), rhsType = evalExprType(e->rhs);

			auto compileShortCircuitOperator = [this, e, lhsType, rhsType]() {
				uint32_t lhsRegIndex = allocReg(1);

				auto boolType = std::make_shared<BoolTypeNameNode>(e->lhs->getLocation());

				// Compile the LHS.
				// The LHS must be a boolean expression.
				if (!isSameType(lhsType, boolType)) {
					if (!isTypeNamesConvertible(lhsType, boolType))
						throw FatalCompilationError(
							{ e->lhs->getLocation(),
								MessageType::Error,
								"Incompatible operand types" });

					compileExpr(
						make_shared<CastExprNode>(e->lhs->getLocation(), boolType, e->lhs),
						_binaryOpRegs.at(e->op).isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						make_shared<RegRefNode>(lhsRegIndex));
				} else
					compileExpr(
						e->lhs,
						_binaryOpRegs.at(e->op).isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						make_shared<RegRefNode>(lhsRegIndex));

				Location loc = e->getLocation();
				string endLabel = "$short_circuit_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

				// Jump to the end if the left expression is enough to get the final result.
				curFn->insertIns(
					e->op == BinaryOp::LOr ? Opcode::JT : Opcode::JF,
					make_shared<LabelRefNode>(endLabel),
					make_shared<RegRefNode>(lhsRegIndex, true));

				// Compile the RHS.
				// The RHS also must be a boolean expression.
				uint32_t rhsRegIndex = allocReg(1);

				if (!isSameType(rhsType, boolType)) {
					if (!isTypeNamesConvertible(rhsType, boolType))
						throw FatalCompilationError(
							{ e->rhs->getLocation(),
								MessageType::Error,
								"Incompatible operand types" });

					compileExpr(
						make_shared<CastExprNode>(e->rhs->getLocation(), boolType, e->rhs),
						_binaryOpRegs.at(e->op).isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						make_shared<RegRefNode>(lhsRegIndex));
				} else
					compileExpr(
						e->rhs,
						_binaryOpRegs.at(e->op).isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						make_shared<RegRefNode>(rhsRegIndex));

				curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					make_shared<RegRefNode>(lhsRegIndex),
					make_shared<RegRefNode>(lhsRegIndex, true),
					make_shared<RegRefNode>(rhsRegIndex, true));

				curFn->insertLabel(endLabel);

				if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest,
						make_shared<RegRefNode>(lhsRegIndex, true));
			};
			auto execOpAndStoreResult = [this, e, lhsType](uint32_t lhsRegIndex, uint32_t rhsRegIndex) {
				if (isAssignBinaryOp(e->op)) {
					// RHS of the assignment expression.
					uint32_t lhsValueRegIndex = allocReg();

					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(lhsValueRegIndex),
						make_shared<RegRefNode>(lhsRegIndex, true));
					curFn->insertIns(
						_binaryOpRegs.at(_assignBinaryOpToOrdinaryBinaryOpMap.at(e->op)).opcode,
						make_shared<RegRefNode>(rhsRegIndex),
						make_shared<RegRefNode>(lhsValueRegIndex, true),
						make_shared<RegRefNode>(rhsRegIndex, true));

					// LHS of the assignment expression.
					curFn->insertIns(Opcode::STORE, make_shared<RegRefNode>(lhsRegIndex, true), make_shared<RegRefNode>(rhsRegIndex, true));

					if ((curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt) && (curMajorContext.curMinorContext.evalDest))
						// TODO: Make it returns LHS (reference to the left operand), not RHS.
						curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(rhsRegIndex, true));
				} else if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt) {
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
			};

			switch (lhsType->getTypeId()) {
				case Type::I8:
				case Type::I16:
				case Type::I32:
				case Type::I64:
				case Type::U8:
				case Type::U16:
				case Type::U32:
				case Type::U64:
				case Type::F32:
				case Type::F64: {
					switch (e->op) {
						case BinaryOp::LAnd:
						case BinaryOp::LOr:
							compileShortCircuitOperator();
							break;
						case BinaryOp::Lsh:
						case BinaryOp::Rsh:
						case BinaryOp::AssignLsh:
						case BinaryOp::AssignRsh: {
							uint32_t lhsRegIndex = allocReg(2),
									 rhsRegIndex = lhsRegIndex + 1;

							auto u32Type = std::make_shared<U32TypeNameNode>(e->rhs->getLocation());

							compileExpr(
								e->lhs,
								_binaryOpRegs.at(e->op).isLhsLvalue
									? EvalPurpose::LValue
									: EvalPurpose::RValue,
								make_shared<RegRefNode>(lhsRegIndex));

							if (!isSameType(rhsType, u32Type)) {
								if (!isTypeNamesConvertible(rhsType, u32Type))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									make_shared<CastExprNode>(e->rhs->getLocation(), u32Type, e->rhs),
									_binaryOpRegs.at(e->op).isRhsLvalue
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(lhsRegIndex));
							} else
								compileExpr(
									e->rhs,
									_binaryOpRegs.at(e->op).isRhsLvalue
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(rhsRegIndex));

							execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

							break;
						}
						default: {
							uint32_t lhsRegIndex = allocReg(2);
							uint32_t rhsRegIndex = lhsRegIndex + 1;

							compileExpr(
								e->lhs,
								_binaryOpRegs.at(e->op).isLhsLvalue
									? EvalPurpose::LValue
									: EvalPurpose::RValue,
								make_shared<RegRefNode>(lhsRegIndex));

							if (!isSameType(lhsType, rhsType)) {
								if (!isTypeNamesConvertible(rhsType, lhsType))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
									_binaryOpRegs.at(e->op).isRhsLvalue
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(rhsRegIndex));
							} else
								compileExpr(e->rhs, _binaryOpRegs.at(e->op).isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, make_shared<RegRefNode>(rhsRegIndex));

							execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

							break;
						}
					}

					break;
				}
				case Type::Bool: {
					switch (e->op) {
						case BinaryOp::LAnd:
						case BinaryOp::LOr:
							compileShortCircuitOperator();
							break;
						case BinaryOp::Eq:
						case BinaryOp::Neq: {
							uint32_t lhsRegIndex = allocReg(2);
							uint32_t rhsRegIndex = lhsRegIndex + 1;

							compileExpr(
								e->lhs,
								_binaryOpRegs.at(e->op).isLhsLvalue
									? EvalPurpose::LValue
									: EvalPurpose::RValue,
								make_shared<RegRefNode>(lhsRegIndex));

							if (!isSameType(lhsType, rhsType)) {
								if (!isTypeNamesConvertible(rhsType, lhsType))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
									_binaryOpRegs.at(e->op).isRhsLvalue
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(rhsRegIndex));
							} else
								compileExpr(e->rhs, _binaryOpRegs.at(e->op).isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, make_shared<RegRefNode>(rhsRegIndex));

							execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

							break;
						}
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}

					break;
				}
				case Type::String:
				case Type::WString: {
					switch (e->op) {
						case BinaryOp::Add:
							compileShortCircuitOperator();
							break;
						case BinaryOp::Subscript: {
							uint32_t lhsRegIndex = allocReg(2),
									 rhsRegIndex = lhsRegIndex + 1;

							auto u32Type = std::make_shared<U32TypeNameNode>(e->rhs->getLocation());

							if (!isSameType(rhsType, u32Type)) {
								if (!isTypeNamesConvertible(rhsType, u32Type))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									make_shared<CastExprNode>(e->rhs->getLocation(), u32Type, e->rhs),
									_binaryOpRegs.at(e->op).isRhsLvalue
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(lhsRegIndex));
							} else
								compileExpr(
									e->rhs,
									_binaryOpRegs.at(e->op).isRhsLvalue
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(rhsRegIndex));

							if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt) {
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
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
					break;
				}
				case Type::Custom: {
					auto node = resolveCustomTypeName(static_pointer_cast<CustomTypeNameNode>(lhsType).get());
					auto lhsType = evalExprType(e->lhs), rhsType = evalExprType(e->rhs);

					auto determineOverloading = [this, e, rhsType](shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> bool {
						if (auto it = n->scope->members.find(std::to_string(e->op));
							it != n->scope->members.end()) {
							assert(it->second->getNodeType() == NodeType::Fn);
							shared_ptr<FnNode> operatorNode = static_pointer_cast<FnNode>(it->second);
							shared_ptr<FnOverloadingNode> overloading;

							try {
								overloading = argDependentLookup(e->getLocation(), operatorNode.get(), { rhsType }, {});
							} catch (FatalCompilationError e) {
								return false;
							}

							compileExpr(
								e->lhs,
								EvalPurpose::RValue,
								make_shared<RegRefNode>(lhsRegIndex));

							Ref fullName;
							_getFullName(operatorNode.get(), fullName);

							// FIXME: The compiler does not generate the original type.
							fullName.back().name = mangleName(
								fullName.back().name,
								{ overloading->params[0].originalType
										? overloading->params[0].originalType
										: overloading->params[0].type },
								false);

							uint32_t tmpRegIndex = allocReg();

							if (auto ce = evalConstExpr(e->rhs); ce) {
								if (isLValueType(overloading->params[0].type))
									throw FatalCompilationError(
										Message(
											e->getLocation(),
											MessageType::Error,
											"Expecting a lvalue expression"));

								curFn->insertIns(Opcode::PUSHARG, ce);
							} else {
								compileExpr(
									e->rhs,
									isLValueType(overloading->params[0].type)
										? EvalPurpose::LValue
										: EvalPurpose::RValue,
									make_shared<RegRefNode>(tmpRegIndex));
								curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
							}

							curFn->insertIns(Opcode::LOAD, make_shared<RegRefNode>(tmpRegIndex), make_shared<RefExprNode>(fullName));
							curFn->insertIns(Opcode::MCALL, make_shared<RegRefNode>(tmpRegIndex, true), make_shared<RegRefNode>(lhsRegIndex, true));

							if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
								curFn->insertIns(Opcode::LRET, curMajorContext.curMinorContext.evalDest);

							return true;
						}
						return false;
					};

					switch (node->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface:
						case NodeType::Trait: {
							uint32_t lhsRegIndex = allocReg(1);

							switch (e->op) {
								case BinaryOp::Assign: {
									uint32_t rhsRegIndex = allocReg(1);

									compileExpr(
										e->lhs,
										EvalPurpose::LValue,
										make_shared<RegRefNode>(lhsRegIndex));

									compileExpr(
										e->rhs,
										EvalPurpose::RValue,
										make_shared<RegRefNode>(rhsRegIndex));

									if (!isSameType(lhsType, rhsType)) {
										if (!isTypeNamesConvertible(rhsType, lhsType))
											throw FatalCompilationError(
												{ e->rhs->getLocation(),
													MessageType::Error,
													"Incompatible operand types" });

										compileExpr(
											make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
											EvalPurpose::LValue,
											make_shared<RegRefNode>(rhsRegIndex));
									}

									curFn->insertIns(
										Opcode::STORE,
										make_shared<RegRefNode>(lhsRegIndex, true),
										make_shared<RegRefNode>(rhsRegIndex, true));

									break;
								}
								default: {
									shared_ptr<MemberNode> n = static_pointer_cast<MemberNode>(node);

									if (!determineOverloading(n, lhsRegIndex))
										throw FatalCompilationError(
											Message(
												e->getLocation(),
												MessageType::Error,
												"No matching operator"));
								}
							}

							break;
						}
						case NodeType::GenericParam: {
							uint32_t lhsRegIndex = allocReg(1);

							shared_ptr<GenericParamNode> n = static_pointer_cast<GenericParamNode>(node);

							switch (e->op) {
								case BinaryOp::Assign: {
									uint32_t rhsRegIndex = allocReg(1);

									compileExpr(
										e->lhs,
										EvalPurpose::LValue,
										make_shared<RegRefNode>(lhsRegIndex));

									compileExpr(
										e->rhs,
										EvalPurpose::RValue,
										make_shared<RegRefNode>(rhsRegIndex));

									if (!isSameType(lhsType, rhsType)) {
										if (!isTypeNamesConvertible(rhsType, lhsType))
											throw FatalCompilationError(
												{ e->rhs->getLocation(),
													MessageType::Error,
													"Incompatible operand types" });

										compileExpr(
											make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
											EvalPurpose::LValue,
											make_shared<RegRefNode>(rhsRegIndex));
									}

									curFn->insertIns(
										Opcode::STORE,
										make_shared<RegRefNode>(lhsRegIndex, true),
										make_shared<RegRefNode>(rhsRegIndex, true));

									break;
								}
								default: {
									shared_ptr<AstNode> curMember;

									curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

									if (curMember->getNodeType() != NodeType::Class)
										throw FatalCompilationError(
											Message(
												n->baseType->getLocation(),
												MessageType::Error,
												"Must be a class"));

									if (determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
										break;

									for (auto i : n->interfaceTypes) {
										curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

										if (curMember->getNodeType() != NodeType::Interface)
											throw FatalCompilationError(
												Message(
													n->baseType->getLocation(),
													MessageType::Error,
													"Must be an interface"));

										if (determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
											break;
									}

									for (auto i : n->traitTypes) {
										curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

										if (curMember->getNodeType() != NodeType::Interface)
											throw FatalCompilationError(
												Message(
													n->baseType->getLocation(),
													MessageType::Error,
													"Must be an interface"));

										if (determineOverloading(static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
											break;
									}

									throw FatalCompilationError(
										Message(
											e->getLocation(),
											MessageType::Error,
											"No matching operator"));
								}
							}
							break;
						}
						default:
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
					}
					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							e->getLocation(),
							MessageType::Error,
							"No matching operator"));
			}
			break;
		}
		case ExprType::Ternary: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			uint32_t conditionRegIndex = allocReg();

			auto loc = e->getLocation();
			string falseBranchLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_false",
				   endLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			compileExpr(e->condition, EvalPurpose::RValue, make_shared<RegRefNode>(conditionRegIndex));
			if (evalExprType(e->condition)->getTypeId() != Type::Bool)
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
			compileExpr(e->x, EvalPurpose::RValue, curMajorContext.curMinorContext.evalDest);
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

			// Compile the false expression.
			curFn->insertLabel(falseBranchLabel);
			compileExpr(e->y, EvalPurpose::RValue, curMajorContext.curMinorContext.evalDest);

			curFn->insertLabel(endLabel);
			break;
		}
		case ExprType::Match: {
			auto e = static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->getLocation();

			string labelPrefix = "$match_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			uint32_t matcheeRegIndex = allocReg();

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RValue, make_shared<RegRefNode>(matcheeRegIndex));

			pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>> defaultCase;

			for (auto i : e->cases) {
				string caseEndLabel = "$match_" + to_string(i.second->getLocation().line) + "_" + to_string(i.second->getLocation().column) + "_caseEnd";

				if (!i.first) {
					if (defaultCase.second)
						// The default case is already exist.
						throw FatalCompilationError(
							{ i.second->getLocation(),
								MessageType::Error,
								"Duplicated default case" });
					defaultCase = i;
				}

				uint32_t conditionRegIndex = allocReg();

				compileExpr(i.first, EvalPurpose::RValue, make_shared<RegRefNode>(conditionRegIndex));
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
		case ExprType::Closure: {
			auto e = static_pointer_cast<ClosureExprNode>(expr);
			break;
		}
		case ExprType::Call: {
			auto e = static_pointer_cast<CallExprNode>(expr);

			curMajorContext.curMinorContext.isArgTypesSet = true;
			curMajorContext.curMinorContext.argTypes = {};

			for (auto &i : e->args) {
				curMajorContext.curMinorContext.argTypes.push_back(evalExprType(i));
			}

			if (auto ce = evalConstExpr(e); ce) {
				curFn->insertIns(Opcode::CALL, ce);
			} else {
				uint32_t callTargetRegIndex = allocReg(2);
				uint32_t tmpRegIndex = callTargetRegIndex + 1;

				for (auto i : e->args) {
					if (auto ce = evalConstExpr(i); ce)
						curFn->insertIns(Opcode::PUSHARG, ce);
					else {
						compileExpr(i, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
						curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
					}
				}

				auto returnType = evalExprType(e->target);
				bool isReturnTypeLValue = isLValueType(returnType);

				compileExpr(e->target, EvalPurpose::Call, make_shared<RegRefNode>(callTargetRegIndex), make_shared<RegRefNode>(tmpRegIndex));

				if (curMajorContext.curMinorContext.isLastCallTargetStatic)
					curFn->insertIns(
						e->isAsync ? Opcode::ACALL : Opcode::CALL,
						make_shared<RegRefNode>(callTargetRegIndex, true));
				else
					curFn->insertIns(
						e->isAsync ? Opcode::AMCALL : Opcode::MCALL,
						make_shared<RegRefNode>(callTargetRegIndex, true),
						make_shared<RegRefNode>(tmpRegIndex, true));

				switch (curMajorContext.curMinorContext.evalPurpose) {
					case EvalPurpose::LValue: {
						if (!isReturnTypeLValue)
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"Expecting a lvalue expression"));

						curFn->insertIns(
							Opcode::LRET,
							curMajorContext.curMinorContext.evalDest);
						break;
					}
					case EvalPurpose::RValue:
					case EvalPurpose::Call: {
						if (isReturnTypeLValue) {
							uint32_t tmpRegIndex = allocReg();

							curFn->insertIns(
								Opcode::LRET,
								make_shared<RegRefNode>(tmpRegIndex));
							curFn->insertIns(
								Opcode::LVALUE,
								curMajorContext.curMinorContext.evalDest,
								make_shared<RegRefNode>(tmpRegIndex, true));
						} else {
							curFn->insertIns(
								Opcode::LRET,
								curMajorContext.curMinorContext.evalDest);
						}

						break;
					}
					case EvalPurpose::Stmt:
						break;
					default:
						assert(false);
				}
			}

			break;
		}
		case ExprType::Await: {
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
		case ExprType::New: {
			auto e = static_pointer_cast<NewExprNode>(expr);

			for (auto i : e->args) {
				if (auto ce = evalConstExpr(i); ce)
					curFn->insertIns(Opcode::PUSHARG, ce);
				else {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(i, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			auto node = resolveCustomTypeName((CustomTypeNameNode *)e->type.get());

			switch (node->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface:
				case NodeType::Trait: {
					shared_ptr<MemberNode> n = static_pointer_cast<MemberNode>(node);

					if (auto it = n->scope->members.find("new"); it != n->scope->members.end()) {
						deque<shared_ptr<TypeNameNode>> argTypes;

						for (auto &i : e->args) {
							argTypes.push_back(evalExprType(i));
						}

						assert(it->second->getNodeType() == NodeType::Fn);

						shared_ptr<FnOverloadingNode> overloading = argDependentLookup(expr->getLocation(), (FnNode *)it->second.get(), argTypes, {});

						deque<shared_ptr<TypeNameNode>> paramTypes;
						for (auto &j : overloading->params) {
							paramTypes.push_back(j.originalType ? j.originalType : j.type);
						}

						if (overloading->isVaridic())
							paramTypes.pop_back();

						Ref fullName = getFullName(it->second.get());
						fullName.back().name = mangleName("new", paramTypes, false);

						curFn->insertIns(Opcode::NEW, curMajorContext.curMinorContext.evalDest, e->type, make_shared<RefExprNode>(fullName));
					} else {
						curFn->insertIns(Opcode::NEW, curMajorContext.curMinorContext.evalDest, e->type, {});
					}

					break;
				}
				case NodeType::GenericParam:
					throw FatalCompilationError(
						Message(
							e->type->getLocation(),
							MessageType::Error,
							"Cannot instantiate a generic parameter"));
				default:
					assert(false);
			}

			break;
		}
		case ExprType::Typeof: {
			auto e = static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				curFn->insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, ce);
			} else {
				uint32_t tmpRegIndex = allocReg();

				compileExpr(e->target, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
				curFn->insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(tmpRegIndex));
			}

			break;
		}
		case ExprType::Cast: {
			auto e = static_pointer_cast<CastExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				if (isTypeNamesConvertible(evalExprType(ce), e->targetType)) {
					curFn->insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, ce);
				} else {
					throw FatalCompilationError({ e->getLocation(), MessageType::Error, "Invalid type conversion" });
				}
			} else {
				if (isTypeNamesConvertible(evalExprType(e->target), e->targetType)) {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(e->target, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, make_shared<RegRefNode>(tmpRegIndex, true));
				} else {
					throw FatalCompilationError({ e->getLocation(), MessageType::Error, "Invalid type conversion" });
				}
			}

			break;
		}
		case ExprType::HeadedRef: {
			auto e = static_pointer_cast<HeadedRefExprNode>(expr);

			break;
		}
		case ExprType::Ref: {
			auto e = static_pointer_cast<RefExprNode>(expr);

			deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;
			if (!resolveRef(e->ref, resolvedParts))
				// The default case is already exist.
				throw FatalCompilationError(
					{ e->getLocation(),
						MessageType::Error,
						"Identifier not found: `" + to_string(e->ref, this) + "'" });

			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::Call) {
				if (isDynamicMember(resolvedParts.back().second)) {
					// Load `this' for the method calling.
					Ref thisRef = e->ref;
					thisRef.pop_back();
					compileExpr(
						make_shared<RefExprNode>(thisRef),
						EvalPurpose::RValue,
						curMajorContext.curMinorContext.thisDest);
					curMajorContext.curMinorContext.isLastCallTargetStatic = false;
				} else
					curMajorContext.curMinorContext.isLastCallTargetStatic = true;
			}

			uint32_t tmpRegIndex = allocReg();

			auto determineOverloadingRegistry = [this, expr, &resolvedParts](shared_ptr<FnNode> x, const deque<shared_ptr<TypeNameNode>> &genericArgs) -> shared_ptr<FnOverloadingNode> {
				if ((resolvedParts.size() > 2) || (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Call)) {
					//
					// Reference to a overloaded function is always ambiguous,
					// because we cannot determine which overloading is the user wanted.
					//
					if (x->overloadingRegistries.size() > 1) {
						throw FatalCompilationError(
							Message(
								expr->getLocation(),
								MessageType::Error,
								"Reference to a overloaded function is ambiguous"));
					}

					return x->overloadingRegistries.front();
				}

				//
				// Find a proper overloading for the function calling expression.
				//
				return argDependentLookup(expr->getLocation(), x.get(), curMajorContext.curMinorContext.argTypes, genericArgs);
			};
			auto loadRest = [this, &determineOverloadingRegistry, &expr, &tmpRegIndex, &resolvedParts]() {
				for (size_t i = 1; i < resolvedParts.size(); ++i) {
					switch (resolvedParts[i].second->getNodeType()) {
						case NodeType::Var: {
							curFn->insertIns(
								Opcode::RLOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true),
								make_shared<RefExprNode>(resolvedParts[i].first));
							break;
						}
						case NodeType::Fn: {
							Ref ref = resolvedParts[i].first;

							shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(static_pointer_cast<FnNode>(resolvedParts[i].second), resolvedParts[i].first.back().genericArgs);

							deque<shared_ptr<TypeNameNode>> paramTypes;
							for (auto &j : overloading->params) {
								paramTypes.push_back(j.originalType ? j.originalType : j.type);
							}

							if (overloading->isVaridic())
								paramTypes.pop_back();

							ref.back().name = mangleName(
								resolvedParts[i].first.back().name,
								paramTypes,
								false);

							curFn->insertIns(
								Opcode::RLOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true),
								make_shared<RefExprNode>(ref));
							break;
						}
						default:
							assert(false);
					}
				}
			};

			auto &x = resolvedParts.front().second;
			switch (x->getNodeType()) {
				case NodeType::LocalVar:
					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(tmpRegIndex),
						make_shared<LocalVarRefNode>(
							static_pointer_cast<LocalVarNode>(x)->index,
							resolvedParts.size() > 1
								? true
								: curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue));

					loadRest();
					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
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
				case NodeType::ArgRef:
					static_pointer_cast<ArgRefNode>(x)->unwrapData = (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue);

					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(tmpRegIndex), x);
					loadRest();
					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							curFn->insertIns(
								Opcode::LVALUE,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true));
					}
					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				case NodeType::Var:
				case NodeType::Fn:
				case NodeType::ThisRef:
				case NodeType::BaseRef: {
					//
					// Resolve the head of the reference.
					// After that, we will use RLOAD instructions to load the members one by one.
					//
					switch (x->getNodeType()) {
						case NodeType::Var: {
							Ref ref = getFullName((VarNode *)x.get());
							curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RefExprNode>(ref));
							break;
						}
						case NodeType::Fn: {
							Ref ref;

							FnNode *fn = (FnNode *)x.get();
							_getFullName(fn, ref);

							shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(static_pointer_cast<FnNode>(x), ref.back().genericArgs);

							deque<shared_ptr<TypeNameNode>> paramTypes;
							for (auto i : overloading->params) {
								paramTypes.push_back(i.originalType ? i.originalType : i.type);
							}

							if (overloading->isVaridic())
								paramTypes.pop_back();

							ref.back().name = mangleName(
								resolvedParts.front().first.back().name,
								paramTypes,
								false);

							curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RefExprNode>(ref));
							break;
						}
						case NodeType::ThisRef:
							curFn->insertIns(Opcode::LTHIS, make_shared<RegRefNode>(tmpRegIndex));
							break;
						case NodeType::BaseRef:
							curFn->insertIns(Opcode::LOAD, make_shared<RegRefNode>(tmpRegIndex), make_shared<RefExprNode>(Ref{ RefEntry(e->getLocation(), "base") }));
							break;
						default:
							assert(false);
					}

					// Check if the target is static.
					loadRest();

					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
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

			// Resolve the rest of the reference.

			break;
		}
		case ExprType::I8:
		case ExprType::I16:
		case ExprType::I32:
		case ExprType::I64:
		case ExprType::U32:
		case ExprType::U64:
		case ExprType::F32:
		case ExprType::F64:
		case ExprType::String:
		case ExprType::Bool:
		case ExprType::Array:
		case ExprType::Map:
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->getLocation(),
						MessageType::Error,
						"Expecting a lvalue expression"));

			curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, expr);
			break;
		default:
			assert(false);
	}

	sld.nIns = curFn->body.size() - sld.offIns;
	curFn->srcLocDescs.push_back(sld);
}
