#include "../../compiler.h"

using namespace slake::slkc;

void slake::slkc::Compiler::compileUnaryOpExpr(CompileContext *compileContext, std::shared_ptr<UnaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType) {
	uint32_t resultRegIndex = compileContext->allocReg(),
			 lhsRegIndex = compileContext->allocReg();

	if (!lhsType)
		throw FatalCompilationError(
			Message(
				tokenRangeToSourceLocation(e->x->tokenRange),
				MessageType::Error,
				"Error deducing type of the operand"));

	std::shared_ptr<TypeNameNode> resultType;

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

	switch (lhsType->getTypeId()) {
	case TypeId::I8:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I8TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I8TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::I16:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I16TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I16TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::I32:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I32TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I32TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::I64:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I64TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<I64TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::U8:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U8TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U8TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::U16:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U16TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U16TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::U32:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U32TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U32TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::U64:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U64TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<U64TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::F32:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<F32TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::F64:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<F64TypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::Bool:
		switch (e->op) {
		case UnaryOp::LNot:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::LNOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Not:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NOT,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		case UnaryOp::Neg:
			compileOrCastOperand(
				e->x,
				lhsType,
				std::make_shared<BoolTypeNameNode>(SIZE_MAX),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(lhsRegIndex));
			compileContext->_insertIns(
				Opcode::NEG,
				std::make_shared<RegRefNode>(resultRegIndex),
				{ std::make_shared<RegRefNode>(lhsRegIndex) });

			resultType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
		}
		break;
	case TypeId::Custom: {
		auto node = resolveCustomTypeName(compileContext, std::static_pointer_cast<CustomTypeNameNode>(lhsType).get());

		auto determineOverloading = [this, e, resultRegIndex, &resultType, &compileContext](std::shared_ptr<MemberNode> n, uint32_t lhsRegIndex) -> bool {
			if (auto it = n->scope->members.find("operator" + std::to_string(e->op));
				it != n->scope->members.end()) {
				assert(it->second->getNodeType() == NodeType::Fn);
				std::shared_ptr<FnNode> operatorNode = std::static_pointer_cast<FnNode>(it->second);
				std::shared_ptr<FnOverloadingNode> overloading;

				{
					auto overloadings = argDependentLookup(compileContext, operatorNode.get(), {}, {});
					if (overloadings.size() != 1)
						return false;
					overloading = overloadings[0];
				}

#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(e->idxOpToken, [&overloading](TokenInfo &tokenInfo) {
					tokenInfo.semanticInfo.correspondingMember = overloading;
				});
#endif

				compileExpr(compileContext,
					e->x,
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(lhsRegIndex));

				std::shared_ptr<IdRefNode> operatorName = std::make_shared<IdRefNode>(IdRefEntries{ overloading->getName() });

				uint32_t callTargetRegIndex = compileContext->allocReg();

				if (overloading->isVirtual)
					compileContext->_insertIns(
						Opcode::RLOAD,
						std::make_shared<RegRefNode>(callTargetRegIndex),
						{ std::make_shared<RegRefNode>(lhsRegIndex),
							std::make_shared<IdRefExprNode>(operatorName) });
				else {
					std::shared_ptr<IdRefNode> fullName = getFullName(overloading.get());
					compileContext->_insertIns(
						Opcode::LOAD,
						std::make_shared<RegRefNode>(callTargetRegIndex),
						{ std::make_shared<IdRefExprNode>(fullName) });
				}
				compileContext->_insertIns(
					Opcode::MCALL,
					std::make_shared<RegRefNode>(resultRegIndex),
					{ std::make_shared<RegRefNode>(callTargetRegIndex),
						std::make_shared<RegRefNode>(lhsRegIndex) });

				resultType = overloading->returnType;

				return true;
			}
			return false;
		};

		switch (node->getNodeType()) {
		case NodeType::Class:
		case NodeType::Interface: {
			uint32_t lhsRegIndex = compileContext->allocReg();

			std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

			if (!determineOverloading(n, lhsRegIndex))
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(e->tokenRange),
						MessageType::Error,
						"No matching operator"));

			break;
		}
		case NodeType::GenericParam: {
			uint32_t lhsRegIndex = compileContext->allocReg();

			std::shared_ptr<GenericParamNode> n = std::static_pointer_cast<GenericParamNode>(node);

			std::shared_ptr<AstNode> curMember;

			curMember = resolveCustomTypeName(compileContext, (CustomTypeNameNode *)n->baseType.get());

			if (curMember->getNodeType() != NodeType::Class)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(n->baseType->tokenRange),
						MessageType::Error,
						"Must be a class"));

			if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
				break;

			for (auto i : n->interfaceTypes) {
				curMember = resolveCustomTypeName(compileContext, (CustomTypeNameNode *)i.get());

				if (curMember->getNodeType() != NodeType::Interface)
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(n->baseType->tokenRange),
							MessageType::Error,
							"Must be an interface"));

				if (determineOverloading(std::static_pointer_cast<MemberNode>(curMember), lhsRegIndex))
					break;
			}

			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"No matching operator"));
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

	if (compileContext->curTopLevelContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue) {
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
		}
	}

	if (compileContext->curTopLevelContext.curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt)
		compileContext->_insertIns(
			Opcode::MOV,
			compileContext->curTopLevelContext.curMajorContext.curMinorContext.evalDest,
			{ std::make_shared<RegRefNode>(resultRegIndex) });

	compileContext->curTopLevelContext.curMajorContext.curMinorContext.evaluatedType = resultType;
}
