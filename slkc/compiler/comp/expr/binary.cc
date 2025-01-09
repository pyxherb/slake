#include "../../compiler.h"

using namespace slake::slkc;

void Compiler::compileBinaryOpExpr(CompileContext *compileContext, std::shared_ptr<BinaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType, std::shared_ptr<TypeNameNode> rhsType) {
#if SLKC_WITH_LANGUAGE_SERVER
	updateTokenInfo(e->idxOpToken, [this, compileContext](TokenInfo &tokenInfo) {
		tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
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

	compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType = lhsType;

	bool isLhsTypeLValue = isLValueType(lhsType), isRhsTypeLValue = isLValueType(rhsType);

	uint32_t resultRegIndex = compileContext->allocReg();
	auto compileOrCastOperand =
		[this, e, &compileContext](
			std::shared_ptr<ExprNode> operand,
			std::shared_ptr<TypeNameNode> operandType,
			std::shared_ptr<TypeNameNode> targetType,
			EvalPurpose evalPurpose,
			std::shared_ptr<AstNode> destOut) {
			targetType = targetType->duplicate<TypeNameNode>();
			targetType->isRef = (evalPurpose == EvalPurpose::LValue);
			if (!isSameType(compileContext, operandType, targetType)) {
				if (!isTypeNamesConvertible(compileContext, operandType, targetType))
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(operand->tokenRange),
							MessageType::Error,
							"Incompatible operand types" });

				compileExpr(compileContext,
					std::make_shared<CastExprNode>(targetType, operand),
					evalPurpose,
					destOut);
			} else
				compileExpr(compileContext, operand, evalPurpose, destOut);
		};
	auto compileSimpleAssignOp =
		[this, e, &compileContext, compileOrCastOperand](
			uint32_t lhsRegIndex,
			std::shared_ptr<TypeNameNode> lhsType,
			uint32_t rhsRegIndex,
			std::shared_ptr<TypeNameNode> rhsType,
			uint32_t resultRegIndex,
			std::shared_ptr<TypeNameNode> expectedLhsType,
			std::shared_ptr<TypeNameNode> expectedRhsType) {
			compileExpr(compileContext,
				e->lhs,
				expectedLhsType->isRef ? EvalPurpose::LValue : EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));

			compileOrCastOperand(
				e->rhs,
				rhsType, expectedRhsType,
				expectedRhsType->isRef ? EvalPurpose::LValue : EvalPurpose::RValue,
				std::make_shared<RegRefNode>(rhsRegIndex));

			// Ordinary assignment operation.
			compileContext->_insertIns(
				Opcode::STORE,
				{},
				{ std::make_shared<RegRefNode>(lhsRegIndex),
					std::make_shared<RegRefNode>(rhsRegIndex) });
			compileContext->_insertIns(
				Opcode::MOV,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });
		};
	auto compileSimpleBinaryOp =
		[this, e, &compileContext, compileOrCastOperand](
			uint32_t lhsRegIndex,
			std::shared_ptr<TypeNameNode> lhsType,
			uint32_t rhsRegIndex,
			std::shared_ptr<TypeNameNode> rhsType,
			uint32_t resultRegIndex,
			std::shared_ptr<TypeNameNode> expectedLhsType,
			std::shared_ptr<TypeNameNode> expectedRhsType,
			Opcode opcode) {
			compileExpr(compileContext,
				e->lhs,
				expectedLhsType->isRef ? EvalPurpose::LValue : EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));

			compileOrCastOperand(
				e->rhs,
				rhsType, expectedRhsType,
				expectedRhsType->isRef ? EvalPurpose::LValue : EvalPurpose::RValue,
				std::make_shared<RegRefNode>(rhsRegIndex));

			compileContext->_insertIns(
				opcode,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex),
					std::make_shared<RegRefNode>(rhsRegIndex) });
		};
	auto compileSimpleAssignaryBinaryOp =
		[this, e, &compileContext, compileOrCastOperand](
			uint32_t lhsRegIndex,
			std::shared_ptr<TypeNameNode> lhsType,
			uint32_t rhsRegIndex,
			std::shared_ptr<TypeNameNode> rhsType,
			uint32_t resultRegIndex,
			std::shared_ptr<TypeNameNode> expectedLhsType,
			std::shared_ptr<TypeNameNode> expectedRhsType,
			Opcode opcode) {
			compileExpr(compileContext,
				e->lhs,
				expectedLhsType->isRef ? EvalPurpose::LValue : EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));

			compileOrCastOperand(
				e->rhs,
				rhsType, expectedRhsType,
				expectedRhsType->isRef ? EvalPurpose::LValue : EvalPurpose::RValue,
				std::make_shared<RegRefNode>(rhsRegIndex));

			uint32_t lhsValueRegIndex = compileContext->allocReg();

			// Load value of the LHS.
			compileContext->_insertIns(
				Opcode::LVALUE,
				std::make_shared<RegRefNode>(lhsValueRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			// Execute the operation.
			compileContext->_insertIns(
				opcode,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsValueRegIndex),
					std::make_shared<RegRefNode>(rhsRegIndex) });

			// Store the result to the LHS.
			compileContext->_insertIns(
				Opcode::STORE,
				{},
				{ std::make_shared<RegRefNode>(lhsRegIndex),
					std::make_shared<RegRefNode>(resultRegIndex) });
		};

	switch (lhsType->getTypeId()) {
		case TypeId::I8: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I8TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::I16: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I16TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::I32: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I32TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::I64: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<I64TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<I64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::U8: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U8TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U8TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
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
		case TypeId::U16: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U16TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U16TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::U32: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::U64: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::And: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Or: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Xor: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAnd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::AND);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignOr: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::OR);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignXor: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U64TypeNameNode>(SIZE_MAX),
						Opcode::XOR);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignLsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignRsh: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<U64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::F32: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F32TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F32TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F32TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX, true);
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
		case TypeId::F64: {
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F64TypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Sub: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mul: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Div: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Mod: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::LT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Gt: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::GT);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::LTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::GtEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::GTEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Lsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::LSH);
					break;
				}
				case BinaryOp::Rsh: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::RSH);
					break;
				}
				case BinaryOp::Cmp: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::CMP);
					resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignSub: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::SUB);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMul: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::MUL);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignDiv: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::DIV);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignMod: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<F64TypeNameNode>(SIZE_MAX, true),
						std::make_shared<F64TypeNameNode>(SIZE_MAX),
						Opcode::MOD);
					resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX, true);
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
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX, true),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::LAnd: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LAND);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::LOr: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::LOR);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq:
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq:
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						std::make_shared<BoolTypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
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
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<StringTypeNameNode>(SIZE_MAX, true),
						std::make_shared<StringTypeNameNode>(SIZE_MAX));
					resultType = std::make_shared<StringTypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<StringTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Eq: {
					// TODO: Implement this operation.
				}
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq: {
					// TODO: Implement this operation.
				}
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Subscript: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::AT);
					resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX, true);
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						std::make_shared<StringTypeNameNode>(SIZE_MAX, true),
						std::make_shared<StringTypeNameNode>(SIZE_MAX),
						Opcode::ADD);
					resultType = std::make_shared<StringTypeNameNode>(SIZE_MAX, true);
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
			uint32_t lhsRegIndex = compileContext->allocReg();
			uint32_t rhsRegIndex = compileContext->allocReg();

			std::shared_ptr<ArrayTypeNameNode>
				lvalueLhsType = lhsType->duplicate<ArrayTypeNameNode>(),
				nonLValueLhsType = lhsType->duplicate<ArrayTypeNameNode>();
			lvalueLhsType->isRef = true;
			nonLValueLhsType->isRef = false;

			switch (e->op) {
				case BinaryOp::Assign: {
					compileSimpleAssignOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						lvalueLhsType,
						nonLValueLhsType);
					resultType = lhsType;
					break;
				}
				case BinaryOp::Add: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						nonLValueLhsType,
						nonLValueLhsType,
						Opcode::ADD);
					resultType = nonLValueLhsType;
					break;
				}
				case BinaryOp::Eq: {
					// TODO: Implement this operation.
				}
				case BinaryOp::StrictEq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						nonLValueLhsType,
						nonLValueLhsType,
						Opcode::EQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Neq: {
					// TODO: Implement this operation.
				}
				case BinaryOp::StrictNeq: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						nonLValueLhsType,
						nonLValueLhsType,
						Opcode::NEQ);
					resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
				}
				case BinaryOp::Subscript: {
					compileSimpleBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						nonLValueLhsType,
						std::make_shared<U32TypeNameNode>(SIZE_MAX),
						Opcode::AT);
					auto elementType = std::static_pointer_cast<ArrayTypeNameNode>(lhsType)->elementType->duplicate<TypeNameNode>();
					elementType->isRef = true;
					resultType = elementType;
					break;
				}
				case BinaryOp::AssignAdd: {
					compileSimpleAssignaryBinaryOp(
						lhsRegIndex, lhsType,
						rhsRegIndex, rhsType,
						resultRegIndex,
						lvalueLhsType,
						nonLValueLhsType,
						Opcode::ADD);
					resultType = lhsType;
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
			auto node = resolveCustomTypeName(compileContext, std::static_pointer_cast<CustomTypeNameNode>(lhsType).get());
			auto lhsType = evalExprType(compileContext, e->lhs), rhsType = evalExprType(compileContext, e->rhs);

			auto determineOverloading = [this, e, rhsType, resultRegIndex, &resultType, &compileContext](std::shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> bool {
				if (auto it = n->scope->members.find("operator" + std::to_string(e->op));
					it != n->scope->members.end()) {
					assert(it->second->getNodeType() == NodeType::Fn);
					std::shared_ptr<FnNode> operatorNode = std::static_pointer_cast<FnNode>(it->second);
					std::shared_ptr<FnOverloadingNode> overloading;

					{
						auto overloadings = argDependentLookup(compileContext, operatorNode.get(), { rhsType }, {});
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

					compileExpr(compileContext,
						e->lhs,
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(lhsRegIndex));

					std::shared_ptr<IdRefNode> operatorName = std::make_shared<IdRefNode>(IdRefEntries{ overloading->getName() });

					uint32_t tmpRegIndex = compileContext->allocReg();

					if (auto ce = evalConstExpr(compileContext, e->rhs); ce) {
						// Check if the parameter requires a lvalue argument.
						if (isLValueType(overloading->params[0]->type))
							throw FatalCompilationError(
								Message(
									tokenRangeToSourceLocation(e->tokenRange),
									MessageType::Error,
									"Expecting a lvalue expression"));

						compileContext->_insertIns(Opcode::PUSHARG, {}, { ce });
					} else {
						compileExpr(compileContext,
							e->rhs,
							isLValueType(overloading->params[0]->type)
								? EvalPurpose::LValue
								: EvalPurpose::RValue,
							std::make_shared<RegRefNode>(tmpRegIndex));
						compileContext->_insertIns(Opcode::PUSHARG, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
					}

					uint32_t callTargetRegIndex = compileContext->allocReg();
					if (overloading->isVirtual) {
						compileContext->_insertIns(Opcode::RLOAD,
							std::make_shared<RegRefNode>(callTargetRegIndex),
							{ std::make_shared<RegRefNode>(lhsRegIndex),
								std::make_shared<IdRefExprNode>(operatorName) });
					} else {
						auto fullName = getFullName(overloading.get());
						compileContext->_insertIns(Opcode::LOAD,
							std::make_shared<RegRefNode>(callTargetRegIndex),
							{ std::make_shared<IdRefExprNode>(fullName) });
					}
					compileContext->_insertIns(Opcode::MCALL,
						std::make_shared<RegRefNode>(resultRegIndex),
						{ std::make_shared<RegRefNode>(callTargetRegIndex), std::make_shared<RegRefNode>(lhsRegIndex) });

#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(e->idxOpToken, [this, &overloading](TokenInfo &tokenInfo) {
						tokenInfo.semanticInfo.correspondingMember = overloading;
					});
#endif

					resultType = overloading->returnType;

					return true;
				}
				return false;
			};

			switch (node->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface: {
					uint32_t lhsRegIndex = compileContext->allocReg();

					switch (e->op) {
						case BinaryOp::Assign: {
							uint32_t rhsRegIndex = compileContext->allocReg();

							compileExpr(compileContext,
								e->lhs,
								EvalPurpose::LValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							compileOrCastOperand(
								e->rhs,
								rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(rhsRegIndex));

							compileContext->_insertIns(
								Opcode::STORE,
								{},
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });
							compileContext->_insertIns(
								Opcode::MOV,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex) });

							resultType = lhsType->duplicate<TypeNameNode>();
							resultType->isRef = true;
							break;
						}
						case BinaryOp::StrictEq: {
							uint32_t rhsRegIndex = compileContext->allocReg();

							compileExpr(compileContext,
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

							compileContext->_insertIns(
								Opcode::EQ,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });

							resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
							break;
						}
						case BinaryOp::StrictNeq: {
							uint32_t rhsRegIndex = compileContext->allocReg();

							compileExpr(compileContext,
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

							compileContext->_insertIns(
								Opcode::NEQ,
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
					uint32_t lhsRegIndex = compileContext->allocReg();

					std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

					switch (e->op) {
						case BinaryOp::Assign: {
							uint32_t rhsRegIndex = compileContext->allocReg();

							compileExpr(compileContext,
								e->lhs,
								EvalPurpose::LValue,
								std::make_shared<RegRefNode>(lhsRegIndex));

							compileOrCastOperand(
								e->rhs,
								rhsType, std::make_shared<U32TypeNameNode>(SIZE_MAX),
								EvalPurpose::RValue,
								std::make_shared<RegRefNode>(rhsRegIndex));

							compileContext->_insertIns(
								Opcode::STORE,
								{},
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });
							compileContext->_insertIns(
								Opcode::MOV,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex) });

							resultType = lhsType->duplicate<TypeNameNode>();
							resultType->isRef = true;
							break;
						}
						case BinaryOp::StrictEq: {
							uint32_t rhsRegIndex = compileContext->allocReg();

							compileExpr(compileContext,
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

							compileContext->_insertIns(
								Opcode::NEQ,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });

							resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
							break;
						}
						case BinaryOp::StrictNeq: {
							uint32_t rhsRegIndex = compileContext->allocReg();

							compileExpr(compileContext,
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

							compileContext->_insertIns(
								Opcode::EQ,
								std::make_shared<RegRefNode>(resultRegIndex),
								{ std::make_shared<RegRefNode>(lhsRegIndex),
									std::make_shared<RegRefNode>(rhsRegIndex) });

							resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
							break;
						}
						default: {
							std::shared_ptr<AstNode> curMember;

							if (n->baseType) {
								curMember = resolveCustomTypeName(compileContext, (CustomTypeNameNode *)n->baseType.get());

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
								curMember = resolveCustomTypeName(compileContext, (CustomTypeNameNode *)i.get());

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

	if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue) {
		if (!isLValueType(resultType))
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Expecting a lvalue expression"));
	} else {
		if (isLValueType(resultType)) {
			uint32_t newResultRegIndex = compileContext->allocReg();
			compileContext->_insertIns(
				Opcode::LVALUE,
				std::make_shared<RegRefNode>(newResultRegIndex),
				{ std::make_shared<RegRefNode>(resultRegIndex) });
			resultRegIndex = newResultRegIndex;
		}
	}

	if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
		compileContext->_insertIns(
			Opcode::MOV,
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
			{ std::make_shared<RegRefNode>(resultRegIndex) });

	compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = resultType;
}
