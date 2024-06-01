#include "../compiler.h"
#include <type_traits>
#include <cmath>

using namespace slake::slkc;

std::map<UnaryOp, Compiler::UnaryOpRegistry> Compiler::_unaryOpRegs = {
	{ UnaryOp::LNot, { slake::Opcode::LNOT, false, false } },
	{ UnaryOp::Not, { slake::Opcode::NOT, false, false } },
	{ UnaryOp::IncF, { slake::Opcode::INCF, true, false } },
	{ UnaryOp::DecF, { slake::Opcode::DECF, true, false } },
	{ UnaryOp::IncB, { slake::Opcode::INCB, true, false } },
	{ UnaryOp::DecB, { slake::Opcode::DECB, true, false } },
	{ UnaryOp::Neg, { slake::Opcode::NEG, false, false } }
};

std::map<BinaryOp, Compiler::BinaryOpRegistry> Compiler::_binaryOpRegs = {
	{ BinaryOp::Add, { slake::Opcode::ADD, false, false, false } },
	{ BinaryOp::Sub, { slake::Opcode::SUB, false, false, false } },
	{ BinaryOp::Mul, { slake::Opcode::MUL, false, false, false } },
	{ BinaryOp::Div, { slake::Opcode::DIV, false, false, false } },
	{ BinaryOp::Mod, { slake::Opcode::MOD, false, false, false } },
	{ BinaryOp::And, { slake::Opcode::AND, false, false, false } },
	{ BinaryOp::Or, { slake::Opcode::OR, false, false, false } },
	{ BinaryOp::Xor, { slake::Opcode::XOR, false, false, false } },
	{ BinaryOp::LAnd, { slake::Opcode::LAND, false, false, false } },
	{ BinaryOp::LOr, { slake::Opcode::LOR, false, false, false } },
	{ BinaryOp::Lsh, { slake::Opcode::LSH, false, false, false } },
	{ BinaryOp::Rsh, { slake::Opcode::RSH, false, false, false } },

	{ BinaryOp::Assign, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignAdd, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignSub, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignMul, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignDiv, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignMod, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignAnd, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignOr, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignXor, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignLsh, { slake::Opcode::NOP, true, false, true } },
	{ BinaryOp::AssignRsh, { slake::Opcode::NOP, true, false, true } },

	{ BinaryOp::Eq, { slake::Opcode::EQ, false, false, false } },
	{ BinaryOp::Neq, { slake::Opcode::NEQ, false, false, false } },
	{ BinaryOp::StrictEq, { slake::Opcode::SEQ, false, false, false } },
	{ BinaryOp::StrictNeq, { slake::Opcode::SNEQ, false, false, false } },
	{ BinaryOp::Lt, { slake::Opcode::LT, false, false, false } },
	{ BinaryOp::Gt, { slake::Opcode::GT, false, false, false } },
	{ BinaryOp::LtEq, { slake::Opcode::LTEQ, false, false, false } },
	{ BinaryOp::GtEq, { slake::Opcode::GTEQ, false, false, false } },
	{ BinaryOp::Subscript, { slake::Opcode::AT, false, false, true } }
};

static std::map<BinaryOp, BinaryOp> _assignBinaryOpToOrdinaryBinaryOpMap = {
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

void slake::slkc::Compiler::compileUnaryOpExpr(std::shared_ptr<UnaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType) {
	uint32_t resultRegIndex = allocReg(),
			 lhsRegIndex = allocReg();

	if (!lhsType)
		throw FatalCompilationError(
			Message(
				e->x->getLocation(),
				MessageType::Error,
				"Error deducing type of the operand"));

	auto &opReg = _unaryOpRegs.at(e->op);

	std::shared_ptr<TypeNameNode> resultType;

	switch (lhsType->getTypeId()) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
			switch (e->op) {
				case UnaryOp::LNot:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true));

					resultType = std::make_shared<BoolTypeNameNode>(e->getLocation(), SIZE_MAX);
					break;
				case UnaryOp::Not:
				case UnaryOp::IncF:
				case UnaryOp::DecF:
				case UnaryOp::IncB:
				case UnaryOp::DecB:
				case UnaryOp::Neg:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true));

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.lvalueResult;
					break;
				default:
					throw FatalCompilationError(
						Message(
							e->getLocation(),
							MessageType::Error,
							"No matching operator"));
			}
			break;
		case TypeId::Bool:
			switch (e->op) {
				case UnaryOp::LNot:
				case UnaryOp::Not:
					compileExpr(
						e->x,
						opReg.lvalueOperand
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true));

					resultType = std::make_shared<BoolTypeNameNode>(e->getLocation(), SIZE_MAX);
					break;
				default:
					throw FatalCompilationError(
						Message(
							e->getLocation(),
							MessageType::Error,
							"No matching operator"));
			}

			break;
		case TypeId::Custom: {
			auto node = resolveCustomTypeName(std::static_pointer_cast<CustomTypeNameNode>(lhsType).get());

			auto determineOverloading = [this, e, resultRegIndex, &resultType](std::shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> bool {
				if (auto it = n->scope->members.find("operator" + std::to_string(e->op));
					it != n->scope->members.end()) {
					assert(it->second->getNodeType() == NodeType::Fn);
					std::shared_ptr<FnNode> operatorNode = std::static_pointer_cast<FnNode>(it->second);
					std::shared_ptr<FnOverloadingNode> overloading;

					{
						auto overloadings = argDependentLookup(e->getLocation(), operatorNode.get(), {}, {});
						if (overloadings.size() != 1)
							return false;
						overloading = overloadings[0];
					}

#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(e->idxOpToken, [&overloading](TokenInfo &tokenInfo) {
						tokenInfo.semanticInfo.correspondingMember = overloading;
					});
#endif

					compileExpr(
						e->x,
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					IdRef operatorName = { overloading->getName() };

					uint32_t tmpRegIndex = allocReg();

					_insertIns(Opcode::RLOAD, std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(lhsRegIndex, true), std::make_shared<IdRefExprNode>(operatorName));
					_insertIns(Opcode::MCALL, std::make_shared<RegRefNode>(tmpRegIndex, true), std::make_shared<RegRefNode>(lhsRegIndex, true));

					_insertIns(Opcode::LRET, std::make_shared<RegRefNode>(resultRegIndex));

					resultType = overloading->returnType;

					return true;
				}
				return false;
			};

			switch (node->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface:
				case NodeType::Trait: {
					uint32_t lhsRegIndex = allocReg(1);

					std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

					if (!determineOverloading(n, lhsRegIndex))
						throw FatalCompilationError(
							Message(
								e->getLocation(),
								MessageType::Error,
								"No matching operator"));

					break;
				}
				case NodeType::GenericParam: {
					uint32_t lhsRegIndex = allocReg(1);

					std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

					std::shared_ptr<AstNode> curMember;

					curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

					if (curMember->getNodeType() != NodeType::Class)
						throw FatalCompilationError(
							Message(
								n->baseType->getLocation(),
								MessageType::Error,
								"Must be a class"));

					if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
						break;

					for (auto i : n->interfaceTypes) {
						curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

						if (curMember->getNodeType() != NodeType::Interface)
							throw FatalCompilationError(
								Message(
									n->baseType->getLocation(),
									MessageType::Error,
									"Must be an interface"));

						if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
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

						if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
							break;
					}

					throw FatalCompilationError(
						Message(
							e->getLocation(),
							MessageType::Error,
							"No matching operator"));
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

	assert(resultType);

	if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue) {
		if (!isLValueType(resultType))
			throw FatalCompilationError(
				Message(
					e->getLocation(),
					MessageType::Error,
					"Expecting a lvalue expression"));
	} else {
		if (isLValueType(resultType)) {
			_insertIns(
				Opcode::LVALUE,
				std::make_shared<RegRefNode>(resultRegIndex),
				std::make_shared<RegRefNode>(resultRegIndex, true));
		}
	}

	if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
		_insertIns(
			Opcode::STORE,
			curMajorContext.curMinorContext.evalDest,
			std::make_shared<RegRefNode>(resultRegIndex, true));

	curMajorContext.curMinorContext.evaluatedType = resultType;
}

void Compiler::compileBinaryOpExpr(std::shared_ptr<BinaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType, std::shared_ptr<TypeNameNode> rhsType) {
#if SLKC_WITH_LANGUAGE_SERVER
	updateTokenInfo(e->idxOpToken, [this](TokenInfo &tokenInfo) {
		tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
		tokenInfo.semanticType = SemanticType::Operator;
	});
	updateCompletionContext(e->idxOpToken, CompletionContext::Expr);
#endif

	if (!lhsType)
		throw FatalCompilationError(
			Message(
				e->lhs->getLocation(),
				MessageType::Error,
				"Error deducing type of the left operand"));
	if (!rhsType)
		throw FatalCompilationError(
			Message(
				e->rhs->getLocation(),
				MessageType::Error,
				"Error deducing type of the right operand"));

	std::shared_ptr<TypeNameNode> resultType;

	curMajorContext.curMinorContext.expectedType = lhsType;

	bool isLhsTypeLValue = isLValueType(lhsType), isRhsTypeLValue = isLValueType(rhsType);

	uint32_t resultRegIndex = allocReg();

	auto &opReg = _binaryOpRegs.at(e->op);

	auto compileShortCircuitOperator = [this, e, lhsType, rhsType, &opReg, resultRegIndex]() {
		uint32_t lhsRegIndex = allocReg(1);

		auto boolType = std::make_shared<BoolTypeNameNode>(e->lhs->getLocation(), SIZE_MAX);

		// Compile the LHS.
		// The LHS must be a boolean expression.
		if (!isSameType(lhsType, boolType)) {
			if (!isTypeNamesConvertible(lhsType, boolType))
				throw FatalCompilationError(
					{ e->lhs->getLocation(),
						MessageType::Error,
						"Incompatible operand types" });

			compileExpr(
				std::make_shared<CastExprNode>(e->lhs->getLocation(), boolType, e->lhs),
				opReg.isLhsLvalue
					? EvalPurpose::LValue
					: EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
		} else
			compileExpr(
				e->lhs,
				opReg.isLhsLvalue
					? EvalPurpose::LValue
					: EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));

		Location loc = e->getLocation();
		std::string endLabel = "$short_circuit_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_end";

		// Jump to the end if the left expression is enough to get the final result.
		_insertIns(
			e->op == BinaryOp::LOr ? Opcode::JT : Opcode::JF,
			std::make_shared<LabelRefNode>(endLabel),
			std::make_shared<RegRefNode>(lhsRegIndex, true));

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
				std::make_shared<CastExprNode>(e->rhs->getLocation(), boolType, e->rhs),
				opReg.isRhsLvalue
					? EvalPurpose::LValue
					: EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
		} else
			compileExpr(
				e->rhs,
				opReg.isRhsLvalue
					? EvalPurpose::LValue
					: EvalPurpose::RValue,
				std::make_shared<RegRefNode>(rhsRegIndex));

		_insertIns(
			opReg.opcode,
			std::make_shared<RegRefNode>(lhsRegIndex),
			std::make_shared<RegRefNode>(lhsRegIndex, true),
			std::make_shared<RegRefNode>(rhsRegIndex, true));

		_insertLabel(endLabel);

		_insertIns(
			Opcode::STORE,
			std::make_shared<RegRefNode>(resultRegIndex),
			std::make_shared<RegRefNode>(lhsRegIndex, true));
	};
	auto execOpAndStoreResult = [this, e, lhsType, &opReg, resultRegIndex](uint32_t lhsRegIndex, uint32_t rhsRegIndex) {
		if (isAssignBinaryOp(e->op)) {
			BinaryOp ordinaryOp = _assignBinaryOpToOrdinaryBinaryOpMap.at(e->op);

			if (ordinaryOp != BinaryOp::Assign) {
				uint32_t lhsValueRegIndex = allocReg(),
						 opResultRegIndex = allocReg();

				// Load value of the LHS.
				_insertIns(
					Opcode::LVALUE,
					std::make_shared<RegRefNode>(lhsValueRegIndex),
					std::make_shared<RegRefNode>(lhsRegIndex, true));

				_insertIns(
					_binaryOpRegs.at(ordinaryOp).opcode,
					std::make_shared<RegRefNode>(opResultRegIndex),
					std::make_shared<RegRefNode>(lhsValueRegIndex, true),
					std::make_shared<RegRefNode>(rhsRegIndex, true));

				// Store the result to the LHS.
				_insertIns(Opcode::STORE, std::make_shared<RegRefNode>(lhsRegIndex, true), std::make_shared<RegRefNode>(opResultRegIndex, true));
				_insertIns(Opcode::STORE, std::make_shared<RegRefNode>(resultRegIndex), std::make_shared<RegRefNode>(lhsRegIndex, true));
			} else {
				_insertIns(Opcode::STORE, std::make_shared<RegRefNode>(lhsRegIndex, true), std::make_shared<RegRefNode>(rhsRegIndex, true));
				_insertIns(Opcode::STORE, std::make_shared<RegRefNode>(resultRegIndex), std::make_shared<RegRefNode>(lhsRegIndex, true));
			}
		} else {
			_insertIns(
				opReg.opcode,
				std::make_shared<RegRefNode>(resultRegIndex),
				std::make_shared<RegRefNode>(lhsRegIndex, true),
				std::make_shared<RegRefNode>(rhsRegIndex, true));
		}
	};

	switch (lhsType->getTypeId()) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64: {
			switch (e->op) {
				case BinaryOp::LAnd:
				case BinaryOp::LOr:
					compileShortCircuitOperator();

					resultType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);
					break;
				case BinaryOp::Eq:
				case BinaryOp::Neq:
				case BinaryOp::Lt:
				case BinaryOp::Gt:
				case BinaryOp::LtEq:
				case BinaryOp::GtEq: {
					uint32_t lhsRegIndex = allocReg(2);
					uint32_t rhsRegIndex = lhsRegIndex + 1;

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(lhsType, rhsType)) {
						if (!isTypeNamesConvertible(rhsType, lhsType))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(e->rhs, opReg.isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);

					break;
				}
				case BinaryOp::Lsh:
				case BinaryOp::Rsh:
				case BinaryOp::AssignLsh:
				case BinaryOp::AssignRsh: {
					uint32_t lhsRegIndex = allocReg(2),
							 rhsRegIndex = lhsRegIndex + 1;

					auto u32Type = std::make_shared<U32TypeNameNode>(e->rhs->getLocation(), SIZE_MAX);

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(rhsType, u32Type)) {
						if (!isTypeNamesConvertible(rhsType, u32Type))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), u32Type, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(lhsRegIndex));
					} else
						compileExpr(
							e->rhs,
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.isResultLvalue;

					break;
				}
				case BinaryOp::Add:
				case BinaryOp::Sub:
				case BinaryOp::Mul:
				case BinaryOp::Div:
				case BinaryOp::Mod:
				case BinaryOp::And:
				case BinaryOp::Or:
				case BinaryOp::Xor:
				case BinaryOp::Assign:
				case BinaryOp::AssignAdd:
				case BinaryOp::AssignSub:
				case BinaryOp::AssignMul:
				case BinaryOp::AssignDiv:
				case BinaryOp::AssignMod:
				case BinaryOp::AssignAnd:
				case BinaryOp::AssignOr:
				case BinaryOp::AssignXor: {
					uint32_t lhsRegIndex = allocReg(2);
					uint32_t rhsRegIndex = lhsRegIndex + 1;

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(lhsType, rhsType)) {
						if (!isTypeNamesConvertible(rhsType, lhsType))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(e->rhs, opReg.isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.isResultLvalue;

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
		case TypeId::Bool: {
			switch (e->op) {
				case BinaryOp::LAnd:
				case BinaryOp::LOr:
					compileShortCircuitOperator();

					resultType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);
					break;
				case BinaryOp::Eq:
				case BinaryOp::Neq: {
					uint32_t lhsRegIndex = allocReg(2);
					uint32_t rhsRegIndex = lhsRegIndex + 1;

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(lhsType, rhsType)) {
						if (!isTypeNamesConvertible(rhsType, lhsType))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(e->rhs, opReg.isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);

					break;
				}
				case BinaryOp::Assign: {
					uint32_t lhsRegIndex = allocReg(2);
					uint32_t rhsRegIndex = lhsRegIndex + 1;

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(lhsType, rhsType)) {
						if (!isTypeNamesConvertible(rhsType, lhsType))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(e->rhs, opReg.isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.isResultLvalue;

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
		case TypeId::String: {
			switch (e->op) {
				case BinaryOp::Add: {
					uint32_t lhsRegIndex = allocReg(2),
							 rhsRegIndex = lhsRegIndex + 1;

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(lhsType, rhsType)) {
						if (!isTypeNamesConvertible(rhsType, lhsType))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(
							e->rhs,
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true),
						std::make_shared<RegRefNode>(rhsRegIndex, true));

					resultType = std::make_shared<StringTypeNameNode>(Location(), SIZE_MAX);

					break;
				}
				case BinaryOp::Subscript: {
					uint32_t lhsRegIndex = allocReg(2),
							 rhsRegIndex = lhsRegIndex + 1;

					auto u32Type = std::make_shared<U32TypeNameNode>(e->rhs->getLocation(), SIZE_MAX);

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(rhsType, u32Type)) {
						if (!isTypeNamesConvertible(rhsType, u32Type))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), u32Type, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(
							e->rhs,
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true),
						std::make_shared<RegRefNode>(rhsRegIndex, true));

					resultType = std::make_shared<U8TypeNameNode>(Location(), SIZE_MAX, true);

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
		case TypeId::WString: {
			switch (e->op) {
				case BinaryOp::Add: {
					uint32_t lhsRegIndex = allocReg(2),
							 rhsRegIndex = lhsRegIndex + 1;

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(lhsType, rhsType)) {
						if (!isTypeNamesConvertible(rhsType, lhsType))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(
							e->rhs,
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true),
						std::make_shared<RegRefNode>(rhsRegIndex, true));

					resultType = std::make_shared<WStringTypeNameNode>(Location(), SIZE_MAX);

					break;
				}
				case BinaryOp::Subscript: {
					uint32_t lhsRegIndex = allocReg(2),
							 rhsRegIndex = lhsRegIndex + 1;

					auto u32Type = std::make_shared<U32TypeNameNode>(e->rhs->getLocation(), SIZE_MAX);

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(rhsType, u32Type)) {
						if (!isTypeNamesConvertible(rhsType, u32Type))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), u32Type, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(
							e->rhs,
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true),
						std::make_shared<RegRefNode>(rhsRegIndex, true));

					resultType = std::make_shared<U32TypeNameNode>(Location(), SIZE_MAX, true);

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
		case TypeId::Array: {
			switch (e->op) {
				case BinaryOp::Subscript: {
					uint32_t lhsRegIndex = allocReg(2),
							 rhsRegIndex = lhsRegIndex + 1;

					auto u32Type = std::make_shared<U32TypeNameNode>(e->rhs->getLocation(), SIZE_MAX);

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					if (!isSameType(rhsType, u32Type)) {
						if (!isTypeNamesConvertible(rhsType, u32Type))
							throw FatalCompilationError(
								{ e->rhs->getLocation(),
									MessageType::Error,
									"Incompatible operand types" });

						compileExpr(
							std::make_shared<CastExprNode>(e->rhs->getLocation(), u32Type, e->rhs),
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));
					} else
						compileExpr(
							e->rhs,
							opReg.isRhsLvalue
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex, true),
						std::make_shared<RegRefNode>(rhsRegIndex, true));

					resultType = std::static_pointer_cast<ArrayTypeNameNode>(lhsType)->elementType->duplicate<TypeNameNode>();
					resultType->isRef = true;

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
		case TypeId::Custom: {
			auto node = resolveCustomTypeName(std::static_pointer_cast<CustomTypeNameNode>(lhsType).get());
			auto lhsType = evalExprType(e->lhs), rhsType = evalExprType(e->rhs);

			auto determineOverloading = [this, e, rhsType, resultRegIndex, &resultType](std::shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> bool {
				if (auto it = n->scope->members.find("operator" + std::to_string(e->op));
					it != n->scope->members.end()) {
					assert(it->second->getNodeType() == NodeType::Fn);
					std::shared_ptr<FnNode> operatorNode = std::static_pointer_cast<FnNode>(it->second);
					std::shared_ptr<FnOverloadingNode> overloading;

					{
						auto overloadings = argDependentLookup(e->getLocation(), operatorNode.get(), { rhsType }, {});
						if (overloadings.size() != 1)
							return false;
						overloading = overloadings[0];

#if SLKC_WITH_LANGUAGE_SERVER
						updateTokenInfo(e->idxOpToken, [&overloading](TokenInfo &tokenInfo) {
							tokenInfo.semanticInfo.correspondingMember = overloading;
						});
						updateTokenInfo(e->idxClosingToken, [&overloading](TokenInfo &tokenInfo) {
							tokenInfo.semanticInfo.correspondingMember = overloading;
						});
#endif
					}

					compileExpr(
						e->lhs,
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					IdRef operatorName = { overloading->getName() };

					uint32_t tmpRegIndex = allocReg();

					if (auto ce = evalConstExpr(e->rhs); ce) {
						// Check if the parameter requires a lvalue argument.
						if (isLValueType(overloading->params[0]->type))
							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"Expecting a lvalue expression"));

						_insertIns(Opcode::PUSHARG, ce, overloading->params[0]->type);
					} else {
						compileExpr(
							e->rhs,
							isLValueType(overloading->params[0]->type)
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(tmpRegIndex));
						_insertIns(Opcode::PUSHARG, std::make_shared<RegRefNode>(tmpRegIndex, true), overloading->params[0]->type);
					}

					_insertIns(Opcode::RLOAD, std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<RegRefNode>(lhsRegIndex, true), std::make_shared<IdRefExprNode>(operatorName));
					_insertIns(Opcode::MCALL, std::make_shared<RegRefNode>(tmpRegIndex, true), std::make_shared<RegRefNode>(lhsRegIndex, true));

#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(e->idxOpToken, [this, &overloading](TokenInfo &tokenInfo) {
						tokenInfo.semanticInfo.correspondingMember = overloading;
					});
#endif

					_insertIns(Opcode::LRET, std::make_shared<RegRefNode>(resultRegIndex));

					resultType = overloading->returnType;

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
								std::make_shared<RegRefNode>(lhsRegIndex));

							if (!isSameType(lhsType, rhsType)) {
								if (!isTypeNamesConvertible(rhsType, lhsType))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));
							} else
								compileExpr(
									e->rhs,
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								Opcode::STORE,
								std::make_shared<RegRefNode>(lhsRegIndex, true),
								std::make_shared<RegRefNode>(rhsRegIndex, true));
							_insertIns(
								Opcode::STORE,
								std::make_shared<RegRefNode>(resultRegIndex),
								std::make_shared<RegRefNode>(lhsRegIndex, true));

							resultType = lhsType->duplicate<TypeNameNode>();
							resultType->isRef = true;
							break;
						}
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq: {
							uint32_t rhsRegIndex = allocReg(1);

							compileExpr(
								e->lhs,
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							lhsType = lhsType->duplicate<TypeNameNode>();
							lhsType->isRef = false;

							if (!isSameType(lhsType, rhsType)) {
								if (!isTypeNamesConvertible(rhsType, lhsType))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));
							} else
								compileExpr(
									e->rhs,
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								opReg.opcode,
								std::make_shared<RegRefNode>(resultRegIndex),
								std::make_shared<RegRefNode>(lhsRegIndex, true),
								std::make_shared<RegRefNode>(rhsRegIndex, true));

							resultType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);
							break;
						}
						default: {
							std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

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

					std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

					switch (e->op) {
						case BinaryOp::Assign: {
							uint32_t rhsRegIndex = allocReg(1);

							compileExpr(
								e->lhs,
								EvalPurpose::LValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							if (!isSameType(lhsType, rhsType)) {
								if (!isTypeNamesConvertible(rhsType, lhsType))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));
							} else
								compileExpr(
									e->rhs,
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								Opcode::STORE,
								std::make_shared<RegRefNode>(lhsRegIndex, true),
								std::make_shared<RegRefNode>(rhsRegIndex, true));
							_insertIns(
								Opcode::STORE,
								std::make_shared<RegRefNode>(resultRegIndex),
								std::make_shared<RegRefNode>(lhsRegIndex, true));

							resultType = lhsType->duplicate<TypeNameNode>();
							resultType->isRef = true;
							break;
						}
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq: {
							uint32_t rhsRegIndex = allocReg(1);

							compileExpr(
								e->lhs,
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							lhsType = lhsType->duplicate<TypeNameNode>();
							lhsType->isRef = false;

							if (!isSameType(lhsType, rhsType)) {
								if (!isTypeNamesConvertible(rhsType, lhsType))
									throw FatalCompilationError(
										{ e->rhs->getLocation(),
											MessageType::Error,
											"Incompatible operand types" });

								compileExpr(
									std::make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));
							} else
								compileExpr(
									e->rhs,
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								opReg.opcode,
								std::make_shared<RegRefNode>(resultRegIndex),
								std::make_shared<RegRefNode>(lhsRegIndex, true),
								std::make_shared<RegRefNode>(rhsRegIndex, true));

							resultType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);
							break;
						}
						default: {
							std::shared_ptr<AstNode> curMember;

							if (n->baseType) {
								curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

								if (curMember->getNodeType() != NodeType::Class)
									throw FatalCompilationError(
										Message(
											n->baseType->getLocation(),
											MessageType::Error,
											"Must be a class"));

								if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
									goto genericParamOperatorFound;
							}

							for (auto &i : n->interfaceTypes) {
								curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

								if (curMember->getNodeType() != NodeType::Interface)
									throw FatalCompilationError(
										Message(
											n->baseType->getLocation(),
											MessageType::Error,
											"Must be an interface"));

								if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
									goto genericParamOperatorFound;
							}

							for (auto &i : n->traitTypes) {
								curMember = resolveCustomTypeName((CustomTypeNameNode *)i.get());

								if (curMember->getNodeType() != NodeType::Interface)
									throw FatalCompilationError(
										Message(
											n->baseType->getLocation(),
											MessageType::Error,
											"Must be an interface"));

								if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
									goto genericParamOperatorFound;
							}

							throw FatalCompilationError(
								Message(
									e->getLocation(),
									MessageType::Error,
									"No matching operator"));
						}
					}
				genericParamOperatorFound:
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

	assert(resultType);

	if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue) {
		if (!isLValueType(resultType))
			throw FatalCompilationError(
				Message(
					e->getLocation(),
					MessageType::Error,
					"Expecting a lvalue expression"));
	} else {
		if (isLValueType(resultType)) {
			_insertIns(
				Opcode::LVALUE,
				std::make_shared<RegRefNode>(resultRegIndex),
				std::make_shared<RegRefNode>(resultRegIndex, true));
		}
	}

	if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
		_insertIns(
			Opcode::STORE,
			curMajorContext.curMinorContext.evalDest,
			std::make_shared<RegRefNode>(resultRegIndex, true));

	curMajorContext.curMinorContext.evaluatedType = resultType;
}

void Compiler::compileExpr(std::shared_ptr<ExprNode> expr) {
	slxfmt::SourceLocDesc sld;
	sld.offIns = curFn->body.size();
	sld.line = expr->getLocation().line;
	sld.column = expr->getLocation().column;

	if (!curMajorContext.curMinorContext.dryRun) {
		if (auto ce = evalConstExpr(expr); ce) {
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->getLocation(),
						MessageType::Error,
						"Expecting a lvalue expression"));

			_insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, ce);
			return;
		}
	}

	switch (expr->getExprType()) {
		case ExprType::Unary: {
			auto e = std::static_pointer_cast<UnaryOpExprNode>(expr);

			compileUnaryOpExpr(e, evalExprType(e->x));
			break;
		}
		case ExprType::Binary: {
			auto e = std::static_pointer_cast<BinaryOpExprNode>(expr);

			compileBinaryOpExpr(e, evalExprType(e->lhs), evalExprType(e->rhs));
			break;
		}
		case ExprType::Ternary: {
			auto e = std::static_pointer_cast<TernaryOpExprNode>(expr);

			uint32_t conditionRegIndex = allocReg(),
					 resultRegIndex = allocReg();

			auto loc = e->getLocation();
			std::string falseBranchLabel = "$ternary_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_false",
						endLabel = "$ternary_" + std::to_string(loc.line) + "_" + std::to_string(loc.column) + "_end";

			auto conditionType = evalExprType(e->condition),
				 trueBranchType = evalExprType(e->x),
				 falseBranchType = evalExprType(e->y);
			auto boolType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);

			if (!conditionType)
				throw FatalCompilationError(
					Message(
						e->getLocation(),
						MessageType::Error,
						"Error deducing type of the condition expression"));
			if (!trueBranchType)
				throw FatalCompilationError(
					Message(
						e->getLocation(),
						MessageType::Error,
						"Error deducing type of the true branch expression"));
			if (!falseBranchType)
				throw FatalCompilationError(
					Message(
						e->getLocation(),
						MessageType::Error,
						"Error deducing type of the true branch expression"));

			auto resultType = getStrongerTypeName(trueBranchType, falseBranchType);

			if (!isSameType(conditionType, boolType)) {
				if (!isTypeNamesConvertible(conditionType, boolType))
					throw FatalCompilationError(
						{ e->condition->getLocation(),
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(
					std::make_shared<CastExprNode>(
						e->condition->getLocation(),
						boolType,
						e->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(conditionRegIndex));
			} else
				compileExpr(e->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));

			_insertIns(
				Opcode::JF,
				std::make_shared<LabelRefNode>(falseBranchLabel),
				std::make_shared<RegRefNode>(conditionRegIndex, true));

			// Compile the true expression.
			if (isSameType(trueBranchType, resultType)) {
				if (!isTypeNamesConvertible(trueBranchType, resultType))
					throw FatalCompilationError(
						{ e->x->getLocation(),
							MessageType::Error,
							"Incompatible operand types" });
				compileExpr(
					std::make_shared<CastExprNode>(e->x->getLocation(), resultType, e->x),
					curMajorContext.curMinorContext.evalPurpose,
					std::make_shared<RegRefNode>(resultRegIndex));
			} else
				compileExpr(e->x, curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(resultRegIndex));
			_insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(endLabel));

			// Compile the false expression.
			if (isSameType(falseBranchType, resultType)) {
				if (!isTypeNamesConvertible(falseBranchType, resultType))
					throw FatalCompilationError(
						{ e->y->getLocation(),
							MessageType::Error,
							"Incompatible operand types" });
				compileExpr(
					std::make_shared<CastExprNode>(e->y->getLocation(), resultType, e->y),
					curMajorContext.curMinorContext.evalPurpose,
					std::make_shared<RegRefNode>(resultRegIndex));
			} else
				compileExpr(e->y, curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(resultRegIndex));

			_insertLabel(endLabel);

			curMajorContext.curMinorContext.evaluatedType = resultType;
			break;
		}
		case ExprType::Match: {
			auto e = std::static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->getLocation();

			std::string labelPrefix = "$match_" + std::to_string(loc.line) + "_" + std::to_string(loc.column),
						condLocalVarName = labelPrefix + "_cond",
						defaultLabel = labelPrefix + "_label",
						endLabel = labelPrefix + "_end";

			uint32_t matcheeRegIndex = allocReg();

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(matcheeRegIndex));

			std::pair<std::shared_ptr<ExprNode>, std::shared_ptr<ExprNode>> defaultCase;

			for (auto i : e->cases) {
				std::string caseEndLabel = "$match_" + std::to_string(i.second->getLocation().line) + "_" + std::to_string(i.second->getLocation().column) + "_caseEnd";

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

				compileExpr(i.first, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));
				_insertIns(
					Opcode::EQ,
					std::make_shared<RegRefNode>(conditionRegIndex),
					std::make_shared<RegRefNode>(matcheeRegIndex, true),
					std::make_shared<RegRefNode>(conditionRegIndex, true));
				_insertIns(Opcode::JF, std::make_shared<LabelRefNode>(caseEndLabel), std::make_shared<RegRefNode>(conditionRegIndex, true));

				// Leave the minor stack that is created for the local variable.
				compileExpr(i.second, curMajorContext.curMinorContext.evalPurpose, curMajorContext.curMinorContext.evalDest);

				_insertLabel(caseEndLabel);
				_insertIns(Opcode::JMP, std::make_shared<LabelRefNode>(endLabel));
			}

			if (defaultCase.second)
				compileExpr(defaultCase.second);

			_insertLabel(endLabel);

			// TODO: Set evaluated type.

			break;
		}
		case ExprType::Call: {
			auto e = std::static_pointer_cast<CallExprNode>(expr);

			curMajorContext.curMinorContext.isArgTypesSet = true;
			curMajorContext.curMinorContext.argTypes = {};

			for (auto &i : e->args) {
				auto type = evalExprType(i);
				if (!type)
					throw FatalCompilationError(
						Message(
							i->getLocation(),
							MessageType::Error,
							"Error deducing type of the argument"));
				curMajorContext.curMinorContext.argTypes.push_back(type);
			}

			uint32_t callTargetRegIndex = allocReg(),
					 tmpRegIndex = allocReg(),
					 thisRegIndex = allocReg();

			compileExpr(e->target, EvalPurpose::Call, std::make_shared<RegRefNode>(callTargetRegIndex), std::make_shared<RegRefNode>(thisRegIndex));

			auto returnType = curMajorContext.curMinorContext.lastCallTargetReturnType;
			if (!returnType)
				throw FatalCompilationError(
					Message(
						e->target->getLocation(),
						MessageType::Error,
						"Error deducing return type"));

			for (size_t i = 0; i < e->args.size(); ++i) {
				EvalPurpose evalPurpose = EvalPurpose::RValue;
				if (i < curMajorContext.curMinorContext.lastCallTargetParams.size()) {
					if (isLValueType(curMajorContext.curMinorContext.lastCallTargetParams[i]->type))
						evalPurpose = EvalPurpose::LValue;
				}

				compileExpr(e->args[i], evalPurpose, std::make_shared<RegRefNode>(tmpRegIndex));
				_insertIns(
					Opcode::PUSHARG,
					std::make_shared<RegRefNode>(tmpRegIndex, true),
					i < curMajorContext.curMinorContext.lastCallTargetParams.size()
						? curMajorContext.curMinorContext.lastCallTargetParams[i]->type
						: nullptr);
			}

			if (curMajorContext.curMinorContext.isLastCallTargetStatic)
				_insertIns(
					e->isAsync ? Opcode::ACALL : Opcode::CALL,
					std::make_shared<RegRefNode>(callTargetRegIndex, true));
			else
				_insertIns(
					e->isAsync ? Opcode::AMCALL : Opcode::MCALL,
					std::make_shared<RegRefNode>(callTargetRegIndex, true),
					std::make_shared<RegRefNode>(thisRegIndex, true));

			switch (curMajorContext.curMinorContext.evalPurpose) {
				case EvalPurpose::LValue: {
					if (!isLValueType(returnType))
						throw FatalCompilationError(
							Message(
								e->getLocation(),
								MessageType::Error,
								"Expecting a lvalue expression"));

					_insertIns(
						Opcode::LRET,
						curMajorContext.curMinorContext.evalDest);
					break;
				}
				case EvalPurpose::RValue:
				case EvalPurpose::Call: {
					if (isLValueType(returnType)) {
						uint32_t tmpRegIndex = allocReg();

						_insertIns(
							Opcode::LRET,
							std::make_shared<RegRefNode>(tmpRegIndex));
						_insertIns(
							Opcode::LVALUE,
							curMajorContext.curMinorContext.evalDest,
							std::make_shared<RegRefNode>(tmpRegIndex, true));
					} else {
						_insertIns(
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

			curMajorContext.curMinorContext.evaluatedType = returnType;

			break;
		}
		case ExprType::Await: {
			uint32_t awaitTargetRegIndex = allocReg();

			auto e = std::static_pointer_cast<AwaitExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				_insertIns(Opcode::AWAIT, ce);
			} else {
				// TODO: Check if the target is a method.
				// compileExpr(e->target, curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(RegId::R0));
				// _insertIns(Opcode::AWAIT, std::make_shared<RegRefNode>(RegId::R0));
			}

			// TODO: Set evaluated type.

			break;
		}
		case ExprType::New: {
			auto e = std::static_pointer_cast<NewExprNode>(expr);

			switch (e->type->getTypeId()) {
				case TypeId::Array: {
					//
					// Construct a new array.
					//
					auto t = std::static_pointer_cast<ArrayTypeNameNode>(e->type);

					if (!t->elementType) {
						throw FatalCompilationError(
							Message(
								t->elementType->getLocation(),
								MessageType::Error,
								"Cannot deduce type of elements"));
					}

					if (e->args.size() != 1) {
						throw FatalCompilationError(
							Message(
								e->type->getLocation(),
								MessageType::Error,
								"Invalid argument number for array constructor"));
					}

					auto sizeArg = e->args[0];
					auto sizeArgType = evalExprType(sizeArg);

					auto u32Type = std::make_shared<U32TypeNameNode>(Location(), SIZE_MAX);

					if (!sizeArgType) {
						throw FatalCompilationError(
							Message(
								sizeArg->getLocation(),
								MessageType::Error,
								"Cannot deduce type of the argument"));
					}

					uint32_t sizeArgRegIndex = allocReg(1);

					if (!isSameType(sizeArgType, u32Type)) {
						if (!isTypeNamesConvertible(sizeArgType, u32Type))
							throw FatalCompilationError(
								{ sizeArg->getLocation(),
									MessageType::Error,
									"Incompatible argument type" });
						compileExpr(
							std::make_shared<CastExprNode>(sizeArg->getLocation(), u32Type, sizeArg),
							EvalPurpose::RValue,
							std::make_shared<RegRefNode>(sizeArgRegIndex));
					} else {
						compileExpr(
							sizeArg,
							EvalPurpose::RValue,
							std::make_shared<RegRefNode>(sizeArgRegIndex));
					}

					_insertIns(Opcode::ARRNEW, curMajorContext.curMinorContext.evalDest, t->elementType, std::make_shared<RegRefNode>(sizeArgRegIndex, true));

					break;
				}
				case TypeId::Custom: {
					//
					// Instantiate a custom type.
					//
					auto node = resolveCustomTypeName((CustomTypeNameNode *)e->type.get());

					switch (node->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface:
						case NodeType::Trait: {
							std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

							if (auto it = n->scope->members.find("new"); it != n->scope->members.end()) {
								std::deque<std::shared_ptr<TypeNameNode>> argTypes;

								for (auto &i : e->args) {
									auto t = evalExprType(i);
									if (!t)
										throw FatalCompilationError(
											Message(
												i->getLocation(),
												MessageType::Error,
												"Error deducing type of the argument"));
									argTypes.push_back(t);
								}

								assert(it->second->getNodeType() == NodeType::Fn);

								std::shared_ptr<FnOverloadingNode> overloading;

								{
									auto overloadings = argDependentLookup(expr->getLocation(), (FnNode *)it->second.get(), argTypes, {});
									if (!overloadings.size()) {
										throw FatalCompilationError(
											Message(
												expr->getLocation(),
												MessageType::Error,
												"No matching function was found"));
									} else if (overloadings.size() > 1) {
										for (auto i : overloadings) {
											messages.push_back(
												Message(
													i->loc,
													MessageType::Note,
													"Matched here"));
										}
										throw FatalCompilationError(
											Message(
												expr->getLocation(),
												MessageType::Error,
												"Ambiguous function call"));
									}

									overloading = overloadings[0];
								}

								std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
								for (auto &j : overloading->params) {
									paramTypes.push_back(j->type);
								}

								if (overloading->isVaridic())
									paramTypes.pop_back();

								for (size_t i = 0; i < argTypes.size(); ++i) {
									if (i < paramTypes.size()) {
										if (isSameType(argTypes[i], paramTypes[i])) {
											if (auto ce = evalConstExpr(e->args[i]); ce)
												_insertIns(Opcode::PUSHARG, ce, paramTypes[i]);
											else {
												uint32_t tmpRegIndex = allocReg();

												compileExpr(e->args[i], EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
												_insertIns(Opcode::PUSHARG, std::make_shared<RegRefNode>(tmpRegIndex, true), paramTypes[i]);
											}
										} else {
											uint32_t tmpRegIndex = allocReg();

											compileExpr(
												std::make_shared<CastExprNode>(e->args[i]->getLocation(), argTypes[i], e->args[i]),
												EvalPurpose::RValue,
												std::make_shared<RegRefNode>(tmpRegIndex));
											_insertIns(Opcode::PUSHARG, std::make_shared<RegRefNode>(tmpRegIndex, true), paramTypes[i]);
										}
									} else {
										if (auto ce = evalConstExpr(e->args[i]); ce)
											_insertIns(Opcode::PUSHARG, ce, {});
										else {
											uint32_t tmpRegIndex = allocReg();

											compileExpr(e->args[i], EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
											_insertIns(Opcode::PUSHARG, std::make_shared<RegRefNode>(tmpRegIndex, true), {});
										}
									}
								}

								IdRef fullName = getFullName(overloading.get());

								_insertIns(Opcode::NEW, curMajorContext.curMinorContext.evalDest, e->type, std::make_shared<IdRefExprNode>(fullName));
							} else {
								_insertIns(Opcode::NEW, curMajorContext.curMinorContext.evalDest, e->type, {});
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
				default:
					throw FatalCompilationError(
						Message(
							e->type->getLocation(),
							MessageType::Error,
							"Specified type is not constructible"));
			}

			curMajorContext.curMinorContext.evaluatedType = e->type;

			break;
		}
		case ExprType::Typeof: {
			auto e = std::static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				_insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, ce);
			} else {
				uint32_t tmpRegIndex = allocReg();

				compileExpr(e->target, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				_insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, std::make_shared<RegRefNode>(tmpRegIndex));
			}

			// TODO: Set evaluated type.

			break;
		}
		case ExprType::Cast: {
			auto e = std::static_pointer_cast<CastExprNode>(expr);

			curMajorContext.curMinorContext.expectedType = e->targetType;

			if (auto ce = evalConstExpr(e->target); ce) {
				if (isTypeNamesConvertible(evalExprType(ce), e->targetType)) {
					_insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, ce);
				} else {
					throw FatalCompilationError({ e->getLocation(), MessageType::Error, "Invalid type conversion" });
				}
			} else {
				auto originalType = evalExprType(e->target);

				if (!originalType)
					break;

				if (isTypeNamesConvertible(originalType, e->targetType)) {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(e->target, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
					_insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, std::make_shared<RegRefNode>(tmpRegIndex, true));
				} else {
					throw FatalCompilationError({ e->getLocation(), MessageType::Error, "Invalid type conversion" });
				}
			}

			curMajorContext.curMinorContext.evaluatedType = e->targetType;

			break;
		}
		case ExprType::HeadedRef: {
			auto e = std::static_pointer_cast<HeadedIdRefExprNode>(expr);

			break;
		}
		case ExprType::IdRef: {
			auto e = std::static_pointer_cast<IdRefExprNode>(expr);

#if SLKC_WITH_LANGUAGE_SERVER
			updateCompletionContext(e->ref, CompletionContext::Expr);
#endif

			std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> resolvedParts;
			if (!resolveIdRef(e->ref, resolvedParts))
				throw FatalCompilationError(
					{ e->getLocation(),
						MessageType::Error,
						"Identifier not found: `" + std::to_string(e->ref, this) + "'" });

			uint32_t tmpRegIndex = allocReg();

			auto determineOverloadingRegistry = [this, expr, &resolvedParts](std::shared_ptr<FnNode> x, const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs) -> std::shared_ptr<FnOverloadingNode> {
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
				{
					auto overloadings = argDependentLookup(expr->getLocation(), x.get(), curMajorContext.curMinorContext.argTypes, genericArgs);

					if (!overloadings.size()) {
						throw FatalCompilationError(
							Message(
								expr->getLocation(),
								MessageType::Error,
								"No matching function was found"));
					} else if (overloadings.size() > 1) {
						for (auto i : overloadings) {
							messages.push_back(
								Message(
									i->loc,
									MessageType::Note,
									"Matched here"));
						}
						throw FatalCompilationError(
							Message(
								expr->getLocation(),
								MessageType::Error,
								"Ambiguous function call"));
					}

					curMajorContext.curMinorContext.lastCallTargetParams = overloadings[0]->params;
					// stub, TODO: Set a flag to hint that the last call target has varidic parameters.
					if (overloadings[0]->isVaridic())
						curMajorContext.curMinorContext.lastCallTargetParams.pop_back();

					curMajorContext.curMinorContext.lastCallTargetReturnType =
						overloadings[0]->returnType
							? overloadings[0]->returnType
							: std::make_shared<AnyTypeNameNode>(Location(), SIZE_MAX);
					curMajorContext.curMinorContext.isLastCallTargetStatic = overloadings[0]->access & ACCESS_STATIC;
					return overloadings[0];
				}
			};

			auto loadRest = [this, &determineOverloadingRegistry, &expr, &tmpRegIndex, &resolvedParts]() {
				for (size_t i = 1; i < resolvedParts.size(); ++i) {
					switch (resolvedParts[i].second->getNodeType()) {
						case NodeType::Var: {
							auto varNode = std::static_pointer_cast<VarNode>(resolvedParts[i].second);

							_insertIns(
								Opcode::RLOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(tmpRegIndex, true),
								std::make_shared<IdRefExprNode>(resolvedParts[i].first));

							if (varNode->type ? isLValueType(varNode->type) : false) {
								// Load once more for reference types.
								_insertIns(
									Opcode::LVALUE,
									std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<RegRefNode>(tmpRegIndex, true));
							}

							// Intermediate scopes should always be loaded as rvalue.
							if ((i + 1 < resolvedParts.size()))
								_insertIns(
									Opcode::LVALUE,
									std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<RegRefNode>(tmpRegIndex, true));
							else {
								curMajorContext.curMinorContext.evaluatedType = varNode->type->duplicate<TypeNameNode>();
								curMajorContext.curMinorContext.evaluatedType->isRef = true;
							}
							break;
						}
						case NodeType::Fn: {
							IdRef ref = resolvedParts[i].first;

							std::shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(std::static_pointer_cast<FnNode>(resolvedParts[i].second), resolvedParts[i].first.back().genericArgs);

							if (!curMajorContext.curMinorContext.isLastCallTargetStatic) {
								_insertIns(
									Opcode::STORE,
									curMajorContext.curMinorContext.thisDest,
									std::make_shared<RegRefNode>(tmpRegIndex, true));
							}

#if SLKC_WITH_LANGUAGE_SERVER
							updateTokenInfo(resolvedParts[i].first.back().idxToken, [&overloading](TokenInfo &tokenInfo) {
								tokenInfo.semanticInfo.correspondingMember = overloading;
							});
#endif

							std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
							for (auto &j : overloading->params) {
								paramTypes.push_back(j->type);
							}

							if (overloading->isVaridic())
								paramTypes.pop_back();

							curMajorContext.curMinorContext.evaluatedType =
								std::make_shared<FnTypeNameNode>(
									overloading->loc,
									overloading->returnType,
									paramTypes);

							_insertIns(
								Opcode::RLOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(tmpRegIndex, true),
								std::make_shared<IdRefExprNode>(ref));
							break;
						}
						default:
							assert(false);
					}
				}
			};

			auto &x = resolvedParts.front().second;
			switch (x->getNodeType()) {
				case NodeType::LocalVar: {
					auto localVarNode = std::static_pointer_cast<LocalVarNode>(x);

					_insertIns(
						Opcode::STORE,
						std::make_shared<RegRefNode>(tmpRegIndex),
						std::make_shared<LocalVarRefNode>(
							localVarNode->index,
							resolvedParts.size() > 1
								? true
								: curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue));

					if (resolvedParts.size() == 1) {
						curMajorContext.curMinorContext.evaluatedType = localVarNode->type->duplicate<TypeNameNode>();
						curMajorContext.curMinorContext.evaluatedType->isRef = true;
					}

					loadRest();
					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							_insertIns(
								Opcode::LVALUE,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(tmpRegIndex, true));
					}

					if (curMajorContext.curMinorContext.evalDest)
						_insertIns(
							Opcode::STORE,
							curMajorContext.curMinorContext.evalDest,
							std::make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				}
				case NodeType::ArgRef: {
					auto argRef = std::static_pointer_cast<ArgRefNode>(x);

					argRef->unwrapData = (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue);
					_insertIns(
						Opcode::STORE,
						std::make_shared<RegRefNode>(tmpRegIndex), x);

					auto paramNode = curFn->params[argRef->index];

					if (paramNode->type ? isLValueType(paramNode->type) : false) {
						// Load once more for reference types.
						_insertIns(
							Opcode::LVALUE,
							std::make_shared<RegRefNode>(tmpRegIndex),
							std::make_shared<RegRefNode>(tmpRegIndex, true));
					}

					if (resolvedParts.size() == 1) {
						curMajorContext.curMinorContext.evaluatedType = paramNode->type->duplicate<TypeNameNode>();
						curMajorContext.curMinorContext.evaluatedType->isRef = true;
					}

					loadRest();
					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							_insertIns(
								Opcode::LVALUE,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(tmpRegIndex, true));
					}

					if (curMajorContext.curMinorContext.evalDest)
						_insertIns(
							Opcode::STORE,
							curMajorContext.curMinorContext.evalDest, std::make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				}
				case NodeType::Var:
				case NodeType::Fn:
				case NodeType::ThisRef:
				case NodeType::BaseRef:
				case NodeType::Class:
				case NodeType::Interface:
				case NodeType::Trait:
				case NodeType::Module: {
					//
					// Resolve the head of the reference.
					// After that, we will use RLOAD instructions to load the members one by one.
					//
					switch (x->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface:
						case NodeType::Trait:
						case NodeType::Module: {
							IdRef ref = getFullName((MemberNode *)x.get());
							_insertIns(
								Opcode::LOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<IdRefExprNode>(ref));

							if (resolvedParts.size() == 1) {
								throw FatalCompilationError(
									Message(
										e->getLocation(),
										MessageType::Error,
										"`" + std::to_string(e->ref, this) + "' is a type"));
							}
							break;
						}
						case NodeType::Var: {
							auto varNode = (VarNode *)x.get();

							IdRef ref = getFullName(varNode);
							_insertIns(
								Opcode::LOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<IdRefExprNode>(ref));

							if (resolvedParts.size() == 1) {
								curMajorContext.curMinorContext.evaluatedType = varNode->type->duplicate<TypeNameNode>();
								curMajorContext.curMinorContext.evaluatedType->isRef = true;
							}
							break;
						}
						case NodeType::Fn: {
							IdRef ref;

							FnNode *fn = (FnNode *)x.get();

							std::shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(std::static_pointer_cast<FnNode>(x), resolvedParts.front().first.back().genericArgs);
							_getFullName(overloading.get(), ref);

#if SLKC_WITH_LANGUAGE_SERVER
							updateTokenInfo(resolvedParts.front().first.back().idxToken, [&overloading](TokenInfo &tokenInfo) {
								tokenInfo.semanticInfo.correspondingMember = overloading;
							});
#endif

							std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
							for (auto i : overloading->params) {
								paramTypes.push_back(i->type);
							}

							if (overloading->isVaridic())
								paramTypes.pop_back();

							if (resolvedParts.size() == 1) {
								curMajorContext.curMinorContext.evaluatedType =
									std::make_shared<FnTypeNameNode>(
										overloading->loc,
										overloading->returnType,
										paramTypes);
							}

							_insertIns(
								Opcode::LOAD,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<IdRefExprNode>(ref));
							break;
						}
						case NodeType::ThisRef: {
							auto owner = curMajorContext.curMinorContext.curScope->owner;

							switch (owner->getNodeType()) {
								case NodeType::Class:
								case NodeType::Interface:
								case NodeType::Trait: {
									_insertIns(Opcode::LTHIS, std::make_shared<RegRefNode>(tmpRegIndex));
									curMajorContext.curMinorContext.evaluatedType = curMajorContext.thisType;
									break;
								}
								default:
									throw FatalCompilationError(
										Message(
											e->getLocation(),
											MessageType::Error,
											"Cannot use this reference in this context"));
							}

							break;
						}
						case NodeType::BaseRef:
							_insertIns(Opcode::LOAD, std::make_shared<RegRefNode>(tmpRegIndex), std::make_shared<IdRefExprNode>(IdRef{ IdRefEntry(e->getLocation(), SIZE_MAX, "base") }));
							break;
						default:
							assert(false);
					}

					// Check if the target is static.
					loadRest();

					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							_insertIns(
								Opcode::LVALUE,
								std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(tmpRegIndex, true));
					}

					if (curMajorContext.curMinorContext.evalDest)
						_insertIns(
							Opcode::STORE,
							curMajorContext.curMinorContext.evalDest,
							std::make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				}
				default:
					assert(false);
			}

			break;
		}
		case ExprType::Array: {
			auto e = std::static_pointer_cast<ArrayExprNode>(expr);

			if ((!curMajorContext.curMinorContext.expectedType) ||
				curMajorContext.curMinorContext.expectedType->getTypeId() != TypeId::Array)
				throw FatalCompilationError(
					Message(
						e->getLocation(),
						MessageType::Error,
						"Error deducing type of the expression"));

			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->getLocation(),
						MessageType::Error,
						"Expecting a lvalue expression"));

			auto type = std::static_pointer_cast<ArrayTypeNameNode>(curMajorContext.curMinorContext.expectedType);

#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(e->idxLBraceToken, [this](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.completionContext = CompletionContext::Expr;
			});

			for (size_t i = 0; i < e->idxCommaTokens.size(); ++i) {
				updateTokenInfo(e->idxCommaTokens[i], [this](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.completionContext = CompletionContext::Expr;
				});
			}
#endif

			// Check type of the members.
			for (auto &i : e->elements) {
				auto t = evalExprType(i);
				if (!t)
					throw FatalCompilationError(
						Message(
							i->getLocation(),
							MessageType::Error,
							"Error deducing the element type"));

				if (!isSameType(t, type->elementType)) {
					if (!isTypeNamesConvertible(t, type->elementType))
						throw FatalCompilationError(
							Message(
								i->getLocation(),
								MessageType::Error,
								"Incompatible element type"));
				}
			}

			curMajorContext.curMinorContext.evaluatedType = type;

			if (auto ce = evalConstExpr(e); ce) {
				_insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, ce);
			} else {
				auto initArray = std::make_shared<ArrayExprNode>(e->getLocation());
				initArray->elements.resize(e->elements.size());

				initArray->evaluatedElementType = type->elementType;

				// We use nullptr to represent non-constexpr expressions.
				for (size_t i = 0; i < e->elements.size(); ++i) {
					initArray->elements[i] = evalConstExpr(initArray->elements[i]);
				}

				size_t idxTmpArrayRegIndex = allocReg(),
					   idxTmpElementRegIndex = allocReg();

				_insertIns(
					Opcode::STORE,
					std::make_shared<RegRefNode>(idxTmpArrayRegIndex),
					initArray);

				// Assign non-constexpr expressions to corresponding elements.
				for (size_t i = 0; i < initArray->elements.size(); ++i) {
					if (!initArray->elements[i]) {
						_insertIns(
							Opcode::AT,
							std::make_shared<RegRefNode>(idxTmpElementRegIndex),
							std::make_shared<RegRefNode>(idxTmpArrayRegIndex, true),
							std::make_shared<U32LiteralExprNode>(Location(), idxTmpElementRegIndex, SIZE_MAX));
						compileExpr(
							e->elements[i],
							EvalPurpose::RValue,
							std::make_shared<RegRefNode>(idxTmpElementRegIndex, true));
					}
				}
			}

			break;
		}
		case ExprType::I8:
		case ExprType::I16:
		case ExprType::I32:
		case ExprType::I64:
		case ExprType::U8:
		case ExprType::U16:
		case ExprType::U32:
		case ExprType::U64:
		case ExprType::F32:
		case ExprType::F64:
		case ExprType::String:
		case ExprType::Bool: {
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->getLocation(),
						MessageType::Error,
						"Expecting a lvalue expression"));

			_insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, expr);

			switch (expr->getExprType()) {
				case ExprType::I8:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I8TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::I16:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I16TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::I32:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I32TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::I64:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I64TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::U8:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U8TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::U16:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U16TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::U32:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U32TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::U64:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U64TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::F32:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<F32TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::F64:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<F64TypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::String:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<StringTypeNameNode>(Location(), SIZE_MAX);
					break;
				case ExprType::Bool:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<BoolTypeNameNode>(Location(), SIZE_MAX);
					break;
			}
			break;
		}
		case ExprType::Null:
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->getLocation(),
						MessageType::Error,
						"Expecting a lvalue expression"));

			_insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, expr);

			curMajorContext.curMinorContext.evaluatedType = std::make_shared<AnyTypeNameNode>(Location(), SIZE_MAX);
			break;
		case ExprType::Bad:
			break;
		default:
			assert(false);
	}

	if (!curMajorContext.curMinorContext.dryRun) {
		sld.nIns = curFn->body.size() - sld.offIns;
		curFn->srcLocDescs.push_back(sld);
	}
}
