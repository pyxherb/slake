#include "../../compiler.h"

using namespace slake::slkc;

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
	{ BinaryOp::StrictEq, { slake::Opcode::EQ, false, false, false } },
	{ BinaryOp::StrictNeq, { slake::Opcode::NEQ, false, false, false } },
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
				tokenRangeToSourceLocation(e->lhs->tokenRange),
				MessageType::Error,
				"Error deducing type of the left operand"));
	if (!rhsType)
		throw FatalCompilationError(
			Message(
				tokenRangeToSourceLocation(e->rhs->tokenRange),
				MessageType::Error,
				"Error deducing type of the right operand"));

	std::shared_ptr<TypeNameNode> resultType;

	curMajorContext.curMinorContext.expectedType = lhsType;

	bool isLhsTypeLValue = isLValueType(lhsType), isRhsTypeLValue = isLValueType(rhsType);

	uint32_t resultRegIndex = allocReg();

	auto &opReg = _binaryOpRegs.at(e->op);

	auto compileOrCastOperand =
		[this, e](
			std::shared_ptr<ExprNode> operand,
			std::shared_ptr<TypeNameNode> operandType,
			std::shared_ptr<TypeNameNode> targetType,
			EvalPurpose evalPurpose,
			std::shared_ptr<AstNode> destOut) {
			if (!isSameType(operandType, targetType)) {
				if (!isTypeNamesConvertible(operandType, targetType))
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(e->rhs->tokenRange),
							MessageType::Error,
							"Incompatible operand types" });

				compileExpr(
					std::make_shared<CastExprNode>(targetType, operand),
					evalPurpose,
					destOut);
			} else
				compileExpr(e->rhs, evalPurpose, destOut);
		};
	auto compileShortCircuitOperator = [this, e, lhsType, rhsType, &opReg, resultRegIndex]() {
		uint32_t lhsRegIndex = allocReg();

		auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

		// Compile the LHS.
		// The LHS must be a boolean expression.
		if (!isSameType(lhsType, boolType)) {
			if (!isTypeNamesConvertible(lhsType, boolType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(e->lhs->tokenRange),
						MessageType::Error,
						"Incompatible operand types" });

			compileExpr(
				std::make_shared<CastExprNode>(boolType, e->lhs),
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

		SourceLocation loc = tokenRangeToSourceLocation(e->tokenRange);
		std::string endLabel = "$short_circuit_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

		// Jump to the end if the left expression is enough to get the final result.
		_insertIns(
			e->op == BinaryOp::LOr ? Opcode::JT : Opcode::JF,
			{},
			{ std::make_shared<LabelRefNode>(endLabel),
				std::make_shared<RegRefNode>(lhsRegIndex) });

		// Compile the RHS.
		// The RHS also must be a boolean expression.
		uint32_t rhsRegIndex = allocReg();

		if (!isSameType(rhsType, boolType)) {
			if (!isTypeNamesConvertible(rhsType, boolType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(e->rhs->tokenRange),
						MessageType::Error,
						"Incompatible operand types" });

			compileExpr(
				std::make_shared<CastExprNode>(boolType, e->rhs),
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
			std::make_shared<RegRefNode>(resultRegIndex),
			{ std::make_shared<RegRefNode>(lhsRegIndex),
				std::make_shared<RegRefNode>(rhsRegIndex) });

		_insertLabel(endLabel);
	};
	auto execOpAndStoreResult = [this, e, lhsType, &opReg, resultRegIndex](uint32_t lhsRegIndex, uint32_t rhsRegIndex) {
		if (isAssignBinaryOp(e->op)) {
			BinaryOp ordinaryOp = _assignBinaryOpToOrdinaryBinaryOpMap.at(e->op);

			if (ordinaryOp != BinaryOp::Assign) {
				uint32_t lhsValueRegIndex = allocReg();

				// Load value of the LHS.
				_insertIns(
					Opcode::LVALUE,
					std::make_shared<RegRefNode>(lhsValueRegIndex),
					{ std::make_shared<RegRefNode>(lhsRegIndex) });

				// Execute the operation.
				_insertIns(
					_binaryOpRegs.at(ordinaryOp).opcode,
					std::make_shared<RegRefNode>(resultRegIndex),
					{ std::make_shared<RegRefNode>(lhsValueRegIndex),
						std::make_shared<RegRefNode>(rhsRegIndex) });

				// Store the result to the LHS.
				_insertIns(
					Opcode::STORE,
					{},
					{ std::make_shared<RegRefNode>(lhsRegIndex),
						std::make_shared<RegRefNode>(resultRegIndex) });
			} else {
				// Ordinary assignment operation.
				_insertIns(
					Opcode::STORE,
					{},
					{ std::make_shared<RegRefNode>(lhsRegIndex),
						std::make_shared<RegRefNode>(rhsRegIndex) });
				_insertIns(
					Opcode::MOV,
					std::make_shared<RegRefNode>(resultRegIndex),
					{ std::make_shared<RegRefNode>(lhsRegIndex) });
			}
		} else {
			// Execute the operation.
			_insertIns(
				opReg.opcode,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex),
					std::make_shared<RegRefNode>(rhsRegIndex) });
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
		case TypeId::U64: {
			switch (e->op) {
				case BinaryOp::LAnd:
				case BinaryOp::LOr:
					compileShortCircuitOperator();

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				case BinaryOp::Eq:
				case BinaryOp::Neq:
				case BinaryOp::Lt:
				case BinaryOp::Gt:
				case BinaryOp::LtEq:
				case BinaryOp::GtEq: {
					uint32_t lhsRegIndex = allocReg();
					uint32_t rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

					break;
				}
				case BinaryOp::Lsh:
				case BinaryOp::Rsh:
				case BinaryOp::AssignLsh:
				case BinaryOp::AssignRsh: {
					uint32_t lhsRegIndex = allocReg(),
							 rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
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
					uint32_t lhsRegIndex = allocReg();
					uint32_t rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.isResultLvalue;

					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
							MessageType::Error,
							"No matching operator"));
			}

			break;
		}
		case TypeId::F32:
		case TypeId::F64: {
			switch (e->op) {
				case BinaryOp::LAnd:
				case BinaryOp::LOr:
					compileShortCircuitOperator();

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				case BinaryOp::Eq:
				case BinaryOp::Neq:
				case BinaryOp::Lt:
				case BinaryOp::Gt:
				case BinaryOp::LtEq:
				case BinaryOp::GtEq: {
					uint32_t lhsRegIndex = allocReg();
					uint32_t rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

					break;
				}
				case BinaryOp::Add:
				case BinaryOp::Sub:
				case BinaryOp::Mul:
				case BinaryOp::Div:
				case BinaryOp::Mod:
				case BinaryOp::Assign:
				case BinaryOp::AssignAdd:
				case BinaryOp::AssignSub:
				case BinaryOp::AssignMul:
				case BinaryOp::AssignDiv:
				case BinaryOp::AssignMod: {
					uint32_t lhsRegIndex = allocReg();
					uint32_t rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.isResultLvalue;

					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
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

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				case BinaryOp::Eq:
				case BinaryOp::Neq: {
					uint32_t lhsRegIndex = allocReg();
					uint32_t rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

					break;
				}
				case BinaryOp::Assign: {
					uint32_t lhsRegIndex = allocReg();
					uint32_t rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					execOpAndStoreResult(lhsRegIndex, rhsRegIndex);

					resultType = lhsType->duplicate<TypeNameNode>();
					resultType->isRef = opReg.isResultLvalue;

					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
							MessageType::Error,
							"No matching operator"));
			}

			break;
		}
		case TypeId::String: {
			switch (e->op) {
				case BinaryOp::Add: {
					uint32_t lhsRegIndex = allocReg(),
							 rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, lhsType,
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						{ std::make_shared<RegRefNode>(lhsRegIndex),
							std::make_shared<RegRefNode>(rhsRegIndex) });

					resultType = std::make_shared<StringTypeNameNode>(SIZE_MAX);

					break;
				}
				case BinaryOp::Subscript: {
					uint32_t lhsRegIndex = allocReg(),
							 rhsRegIndex = allocReg();

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						{ std::make_shared<RegRefNode>(lhsRegIndex),
							std::make_shared<RegRefNode>(rhsRegIndex) });

					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);

					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
							MessageType::Error,
							"No matching operator"));
			}
			break;
		}
		case TypeId::Array: {
			switch (e->op) {
				case BinaryOp::Subscript: {
					uint32_t lhsRegIndex = allocReg(),
							 rhsRegIndex = allocReg();

					auto u32Type = std::make_shared<U32TypeNameNode>(SIZE_MAX);

					compileExpr(
						e->lhs,
						opReg.isLhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					compileOrCastOperand(
						e->rhs,
						rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
						opReg.isRhsLvalue
							? EvalPurpose::LValue
							: EvalPurpose::RValue,
						std::make_shared<RegRefNode>(rhsRegIndex));

					_insertIns(
						opReg.opcode,
						std::make_shared<RegRefNode>(resultRegIndex),
						{ std::make_shared<RegRefNode>(lhsRegIndex),
							std::make_shared<RegRefNode>(rhsRegIndex) });

					auto arrayType = std::static_pointer_cast<ArrayTypeNameNode>(lhsType);

					resultType = std::static_pointer_cast<ArrayTypeNameNode>(lhsType)->elementType->duplicate<TypeNameNode>();
					resultType->isRef = true;

					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
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
						auto overloadings = argDependentLookup(operatorNode.get(), { rhsType }, {});
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
									tokenRangeToSourceLocation(e->tokenRange),
									MessageType::Error,
									"Expecting a lvalue expression"));

						_insertIns(Opcode::PUSHARG, {}, { ce, overloading->params[0]->type });
					} else {
						compileExpr(
							e->rhs,
							isLValueType(overloading->params[0]->type)
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(tmpRegIndex));
						_insertIns(Opcode::PUSHARG, {}, { std::make_shared<RegRefNode>(tmpRegIndex), overloading->params[0]->type });
					}

					uint32_t callTargetRegIndex = allocReg();
					if (overloading->isVirtual) {
						_insertIns(Opcode::RLOAD,
							std::make_shared<RegRefNode>(callTargetRegIndex),
							{ std::make_shared<RegRefNode>(lhsRegIndex),
								std::make_shared<IdRefExprNode>(operatorName) });
					} else {
						IdRef fullName = getFullName(overloading.get());
						_insertIns(Opcode::LOAD,
							std::make_shared<RegRefNode>(callTargetRegIndex),
							{ std::make_shared<IdRefExprNode>(fullName) });
					}
					_insertIns(Opcode::MCALL,
						{},
						{ std::make_shared<RegRefNode>(callTargetRegIndex), std::make_shared<RegRefNode>(lhsRegIndex) });

#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(e->idxOpToken, [this, &overloading](TokenInfo &tokenInfo) {
						tokenInfo.semanticInfo.correspondingMember = overloading;
					});
#endif

					_insertIns(Opcode::LRET, std::make_shared<RegRefNode>(resultRegIndex), {});

					resultType = overloading->returnType;

					return true;
				}
				return false;
			};

			switch (node->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface: {
					uint32_t lhsRegIndex = allocReg();

					switch (e->op) {
						case BinaryOp::Assign: {
							uint32_t rhsRegIndex = allocReg();

							compileExpr(
								e->lhs,
								EvalPurpose::LValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							compileOrCastOperand(
								e->rhs,
								rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								Opcode::STORE,
								{},
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });
							_insertIns(
								Opcode::MOV,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex) });

							resultType = lhsType->duplicate<TypeNameNode>();
							resultType->isRef = true;
							break;
						}
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq: {
							uint32_t rhsRegIndex = allocReg();

							compileExpr(
								e->lhs,
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							lhsType = lhsType->duplicate<TypeNameNode>();
							lhsType->isRef = false;

							compileOrCastOperand(
								e->rhs,
								rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								opReg.opcode,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });

							resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
							break;
						}
						default: {
							std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

							if (!determineOverloading(n, lhsRegIndex))
								throw FatalCompilationError(
									Message(
										tokenRangeToSourceLocation(e->tokenRange),
										MessageType::Error,
										"No matching operator"));
						}
					}

					break;
				}
				case NodeType::GenericParam: {
					uint32_t lhsRegIndex = allocReg();

					std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

					switch (e->op) {
						case BinaryOp::Assign: {
							uint32_t rhsRegIndex = allocReg();

							compileExpr(
								e->lhs,
								EvalPurpose::LValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							compileOrCastOperand(
								e->rhs,
								rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								Opcode::STORE,
								{},
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });
							_insertIns(
								Opcode::MOV,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex) });

							resultType = lhsType->duplicate<TypeNameNode>();
							resultType->isRef = true;
							break;
						}
						case BinaryOp::StrictEq:
						case BinaryOp::StrictNeq: {
							uint32_t rhsRegIndex = allocReg();

							compileExpr(
								e->lhs,
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							lhsType = lhsType->duplicate<TypeNameNode>();
							lhsType->isRef = false;

							compileOrCastOperand(
								e->rhs,
								rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(rhsRegIndex));

							_insertIns(
								opReg.opcode,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });

							resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
							break;
						}
						default: {
							std::shared_ptr<AstNode> curMember;

							if (n->baseType) {
								curMember = resolveCustomTypeName((CustomTypeNameNode *)n->baseType.get());

								if (curMember->getNodeType() != NodeType::Class)
									throw FatalCompilationError(
										Message(
											tokenRangeToSourceLocation(n->baseType->tokenRange),
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
											tokenRangeToSourceLocation(n->baseType->tokenRange),
											MessageType::Error,
											"Must be an interface"));

								if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
									goto genericParamOperatorFound;
							}

							throw FatalCompilationError(
								Message(
									tokenRangeToSourceLocation(e->tokenRange),
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
							tokenRangeToSourceLocation(e->tokenRange),
							MessageType::Error,
							"No matching operator"));
			}
			break;
		}
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
	}

	assert(resultType);

	if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue) {
		if (!isLValueType(resultType))
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Expecting a lvalue expression"));
	} else {
		if (isLValueType(resultType)) {
			uint32_t newResultRegIndex = allocReg();
			_insertIns(
				Opcode::LVALUE,
				std::make_shared<RegRefNode>(newResultRegIndex),
				{ std::make_shared<RegRefNode>(resultRegIndex) });
			resultRegIndex = newResultRegIndex;
		}
	}

	if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
		_insertIns(
			Opcode::MOV,
			curMajorContext.curMinorContext.evalDest,
			{ std::make_shared<RegRefNode>(resultRegIndex) });

	curMajorContext.curMinorContext.evaluatedType = resultType;
}
