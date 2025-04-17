#include "../compiler.h"

using namespace slkc;

template <typename LE, typename DT, typename TN>
SLAKE_FORCEINLINE std::optional<CompilationError> _compileLiteralExpr(
	CompileContext *compileContext,
	const peff::SharedPtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<LE> e = expr.castTo<LE>();

	switch (evalPurpose) {
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::RValue:
			if (resultRegOut != UINT32_MAX) {
				SLKC_RETURN_IF_COMP_ERROR(
					compileContext->emitIns(
						slake::Opcode::MOV,
						resultRegOut,
						{ slake::Value((DT)e->data) }));
			}
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
			break;
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
			break;
		default:
			std::terminate();
	}
	if (!(resultOut.evaluatedType = peff::makeShared<TN>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document)
				.castTo<TypeNameNode>())) {
		return genOutOfMemoryCompError();
	}
	return {};
}

SLKC_API std::optional<CompilationError> slkc::_compileOrCastOperand(
	CompileContext *compileContext,
	uint32_t regOut,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> desiredType,
	peff::SharedPtr<ExprNode> operand,
	peff::SharedPtr<TypeNameNode> operandType) {
	bool whether;
	SLKC_RETURN_IF_COMP_ERROR(isSameType(desiredType, operandType, whether));
	if (whether) {
		CompileExprResult result;
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, operand, evalPurpose, regOut, result));
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(operandType, desiredType, whether));
	if (whether) {
		CompileExprResult result;

		peff::SharedPtr<CastExprNode> castExpr;

		if (!(castExpr = peff::makeShared<CastExprNode>(
				  compileContext->allocator.get(),
				  compileContext->allocator.get(),
				  compileContext->document,
				  desiredType,
				  operand))) {
			return genOutOfMemoryCompError();
		}

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, castExpr.castTo<ExprNode>(), evalPurpose, regOut, result));
		return {};
	}

	IncompatibleOperandErrorExData exData;
	exData.desiredType = desiredType;

	return CompilationError(operand->tokenRange, std::move(exData));
}

SLKC_API std::optional<CompilationError> slkc::compileExpr(
	CompileContext *compileContext,
	const peff::SharedPtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<BlockCompileContext> blockCompileContext = compileContext->fnCompileContext.blockCompileContexts.back();

	switch (expr->exprKind) {
		case ExprKind::Unary:
			SLKC_RETURN_IF_COMP_ERROR(compileUnaryExpr(compileContext, expr.castTo<UnaryExprNode>(), evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Binary:
			SLKC_RETURN_IF_COMP_ERROR(compileBinaryExpr(compileContext, expr.castTo<BinaryExprNode>(), evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::IdRef: {
			peff::SharedPtr<IdRefExprNode> e = expr.castTo<IdRefExprNode>();

			peff::SharedPtr<MemberNode> initialMember, finalMember;
			IdRefEntry &initialEntry = e->idRefPtr->entries.at(0);
			ExprEvalPurpose initialMemberEvalPurpose;

			uint32_t initialMemberReg = compileContext->allocReg();

			if (!initialEntry.genericArgs.size()) {
				// Check if the entry refers to a local variable.
				if (auto it = blockCompileContext->localVars.find(e->idRefPtr->entries.at(0).name);
					it != blockCompileContext->localVars.end()) {
					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					initialMember = it.value().castTo<MemberNode>();
					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::None:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
								CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue: {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MOV, initialMemberReg, { slake::Value(slake::ValueType::RegRef, it.value()->idxReg) }));
							break;
						}
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LVALUE, initialMemberReg, { slake::Value(slake::ValueType::RegRef, it.value()->idxReg) }));
							break;
						}
					}
					goto initialMemberResolved;
				}

				// Check if the entry refers to a function parameter.
				if (auto it = compileContext->fnCompileContext.currentFn->paramIndices.find(e->idRefPtr->entries.at(0).name);
					it != compileContext->fnCompileContext.currentFn->paramIndices.end()) {
					initialMember = compileContext->fnCompileContext.currentFn->params.at(it.value()).castTo<MemberNode>();

					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::None:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
								CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue: {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LARG, initialMemberReg, { slake::Value(slake::ValueType::RegRef, it.value()) }));
							break;
						}
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							uint32_t tmpReg = compileContext->allocReg();
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LARG, tmpReg, { slake::Value(slake::ValueType::RegRef, it.value()) }));
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LVALUE, initialMemberReg, { slake::Value(slake::ValueType::RegRef, tmpReg) }));
							break;
						}
					}
					goto initialMemberResolved;
				}
			}

		initialMemberResolved:
			auto loadTheRest = [compileContext](uint32_t resultRegOut, IdRef *idRef, ResolvedIdRefPartList &parts, size_t curIdx) -> std::optional<CompilationError> {
				uint32_t idxReg = compileContext->allocReg();

				for (size_t i = 0; i < parts.size(); ++i) {
					ResolvedIdRefPart &part = parts.at(i);

					slake::HostObjectRef<slake::IdRefObject> idRefObject;

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

					if (part.isStatic) {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::ValueType::EntityRef, slake::EntityRef::makeObjectRef(idRefObject.get())) }));
					} else {
						uint32_t idxNewReg = compileContext->allocReg();
						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::RLOAD, idxNewReg, { slake::Value(slake::ValueType::RegRef, idxReg), slake::Value(slake::ValueType::EntityRef, slake::EntityRef::makeObjectRef(idRefObject.get())) }));
						idxReg = idxNewReg;
					}

					curIdx += part.nEntries;
				}

				SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MOV, resultRegOut, { slake::Value(slake::ValueType::RegRef, idxReg) }));
			};
			if (initialMember) {
				if (e->idRefPtr->entries.size() > 1) {
					size_t curIdx = 1;

					ResolvedIdRefPartList parts(compileContext->document->allocator.get());
					{
						peff::Set<peff::SharedPtr<MemberNode>> walkedMemberNodes(compileContext->document->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(
							resolveIdRefWithScopeNode(
								compileContext->document,
								walkedMemberNodes,
								initialMember,
								e->idRefPtr->entries.data() + 1,
								e->idRefPtr->entries.size() - 1,
								finalMember,
								&parts));
					}

					SLKC_RETURN_IF_COMP_ERROR(loadTheRest(resultRegOut, e->idRefPtr.get(), parts, 1));
				} else {
					finalMember = initialMember;
				}
			} else {
				size_t curIdx = 1;

				ResolvedIdRefPartList parts(compileContext->document->allocator.get());
				{
					peff::Set<peff::SharedPtr<MemberNode>> walkedMemberNodes(compileContext->document->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(
						resolveIdRefWithScopeNode(
							compileContext->document,
							walkedMemberNodes,
							initialMember,
							e->idRefPtr->entries.data(),
							e->idRefPtr->entries.size(),
							finalMember,
							&parts));
				}

				SLKC_RETURN_IF_COMP_ERROR(loadTheRest(resultRegOut, e->idRefPtr.get(), parts, 0));
			}
			break;
		}
		case ExprKind::I8:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I8LiteralExprNode, int8_t, I8TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I16:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I16LiteralExprNode, int16_t, I16TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I32LiteralExprNode, int32_t, I32TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I64LiteralExprNode, int64_t, I64TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U8:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U8LiteralExprNode, uint8_t, U8TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U16:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U16LiteralExprNode, uint16_t, U16TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U32LiteralExprNode, uint32_t, U32TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U64LiteralExprNode, uint64_t, U64TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::F32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<F32LiteralExprNode, float, F32TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::F64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<F64LiteralExprNode, double, F64TypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Bool:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<BoolLiteralExprNode, bool, BoolTypeNameNode>(compileContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Null: {
			peff::SharedPtr<NullLiteralExprNode> e = expr.castTo<NullLiteralExprNode>();

			switch (evalPurpose) {
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue:
					if (resultRegOut != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR(
							compileContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(nullptr) }));
					}
					break;
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					break;
				case ExprEvalPurpose::Call:
					return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
					break;
				default:
					std::terminate();
			}

			if (!(resultOut.evaluatedType = peff::makeShared<ObjectTypeNameNode>(
					  compileContext->allocator.get(),
					  compileContext->allocator.get(),
					  compileContext->document)
						.castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}
			break;
		}
	}

	return {};
}
