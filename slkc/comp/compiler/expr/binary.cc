#include "../../compiler.h"

using namespace slkc;

std::optional<CompilationError> slkc::_compileSimpleBinaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> lhsType,
	peff::SharedPtr<TypeNameNode> desiredLhsType,
	ExprEvalPurpose lhsEvalPurpose,
	peff::SharedPtr<TypeNameNode> rhsType,
	peff::SharedPtr<TypeNameNode> desiredRhsType,
	ExprEvalPurpose rhsEvalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode) {
	std::optional<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t lhsReg,
				rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, lhsEvalPurpose, desiredLhsType, expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				opcode,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

std::optional<CompilationError> slkc::_compileSimpleAssignBinaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> lhsType,
	peff::SharedPtr<TypeNameNode> desiredLhsType,
	peff::SharedPtr<TypeNameNode> rhsType,
	peff::SharedPtr<TypeNameNode> desiredRhsType,
	ExprEvalPurpose rhsEvalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	std::optional<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			if ((e = _compileOrCastOperand(compileContext, compilationContext, resultRegOut, ExprEvalPurpose::LValue, desiredLhsType, expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegRef, resultRegOut), slake::Value(slake::ValueType::RegRef, rhsReg) }));

			break;
		}
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t lhsReg,
				rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::LValue, desiredLhsType, expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::MOV,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegRef, rhsReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

std::optional<CompilationError> slkc::_compileSimpleLAndBinaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<BoolTypeNameNode> boolType,
	peff::SharedPtr<TypeNameNode> lhsType,
	peff::SharedPtr<TypeNameNode> rhsType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode) {
	std::optional<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t lhsReg,
				rhsReg,
				tmpResultReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpResultReg));

			uint32_t cmpEndLabelId;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(cmpEndLabelId));

			if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}

			uint32_t phiSrcOff = compilationContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::JF,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, rhsReg), slake::Value(slake::ValueType::RegRef, lhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::LAND,
				tmpResultReg,
				{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

			compilationContext->setLabelOffset(cmpEndLabelId, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::PHI,
				tmpResultReg,
				{ slake::Value((uint32_t)phiSrcOff), slake::Value(slake::ValueType::RegRef, lhsReg),
					slake::Value((uint32_t)UINT32_MAX), slake::Value(slake::ValueType::RegRef, tmpResultReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

std::optional<CompilationError> slkc::_compileSimpleLOrBinaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<BoolTypeNameNode> boolType,
	peff::SharedPtr<TypeNameNode> lhsType,
	peff::SharedPtr<TypeNameNode> rhsType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode) {
	std::optional<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t lhsReg,
				rhsReg,
				tmpResultReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpResultReg));

			uint32_t cmpEndLabelId;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(cmpEndLabelId));

			if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}

			uint32_t phiSrcOff = compilationContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::JF,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cmpEndLabelId), slake::Value(slake::ValueType::RegRef, lhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.castTo<TypeNameNode>(), expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::LAND,
				tmpResultReg,
				{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

			compilationContext->setLabelOffset(cmpEndLabelId, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::PHI,
				tmpResultReg,
				{ slake::Value((uint32_t)phiSrcOff), slake::Value(slake::ValueType::RegRef, lhsReg),
					slake::Value((uint32_t)UINT32_MAX), slake::Value(slake::ValueType::RegRef, tmpResultReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::_compileSimpleBinaryAssignOpExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> lhsType,
	peff::SharedPtr<TypeNameNode> rhsType,
	peff::SharedPtr<TypeNameNode> desiredRhsType,
	ExprEvalPurpose rhsEvalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode) {
	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t lhsValueReg,
				rhsReg,
				resultValueReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsValueReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(resultValueReg));

			if (auto e = _compileOrCastOperand(compileContext, compilationContext, resultRegOut, ExprEvalPurpose::LValue, lhsType, expr->lhs, lhsType); e) {
				if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileContext->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::LVALUE,
				lhsValueReg,
				{ slake::Value(slake::ValueType::RegRef, resultRegOut) }));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				opcode,
				resultValueReg,
				{ slake::Value(slake::ValueType::RegRef, lhsValueReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::STORE,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegRef, resultValueReg) }));
			break;
		}
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileContext->allocator.get());

			uint32_t lhsReg,
				lhsValueReg,
				rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsValueReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::LValue, lhsType, expr->lhs, lhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::LVALUE,
				lhsValueReg,
				{ slake::Value(slake::ValueType::RegRef, lhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				opcode,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				slake::Opcode::STORE,
				lhsReg,
				{ slake::Value(slake::ValueType::RegRef, resultRegOut) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API std::optional<CompilationError> slkc::compileBinaryExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<TypeNameNode> lhsType, rhsType, decayedLhsType, decayedRhsType;

	if (auto e = evalExprType(compileContext, compilationContext, expr->lhs, lhsType); e) {
		if (auto re = evalExprType(compileContext, compilationContext, expr->rhs, rhsType); re) {
			if (!compileContext->errors.pushBack(std::move(*e))) {
				return genOutOfMemoryCompError();
			}
			e.reset();
			return re;
		}
		return e;
	}
	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileContext, compilationContext, expr->rhs, rhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(lhsType, decayedLhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(rhsType, decayedRhsType));

	if (expr->binaryOp == BinaryOp::Comma) {
		CompileExprResult result(compileContext->allocator.get());
		uint32_t tmpRegIndex;

		SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->lhs, ExprEvalPurpose::Stmt, {}, tmpRegIndex, result));
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->rhs, evalPurpose, {}, resultRegOut, resultOut));
		return {};
	}

	peff::SharedPtr<U32TypeNameNode> u32Type;
	peff::SharedPtr<I32TypeNameNode> i32Type;
	peff::SharedPtr<BoolTypeNameNode> boolType;

	if (!(u32Type = peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	if (!(i32Type = peff::makeSharedWithControlBlock<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	if (!(boolType = peff::makeSharedWithControlBlock<BoolTypeNameNode, AstNodeControlBlock<BoolTypeNameNode>>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	// Deal with the RHS to LHS user binary operator.
	if ((decayedRhsType->typeNameKind == TypeNameKind::Custom) &&
		(decayedLhsType->typeNameKind != TypeNameKind::Custom)) {
		switch (expr->binaryOp) {
			case BinaryOp::Add:
			case BinaryOp::Sub:
			case BinaryOp::Mul:
			case BinaryOp::Div:
			case BinaryOp::Mod:
			case BinaryOp::And:
			case BinaryOp::Or:
			case BinaryOp::Xor:
			case BinaryOp::LAnd:
			case BinaryOp::LOr:
			case BinaryOp::Shl:
			case BinaryOp::Shr:
			case BinaryOp::Assign:
			case BinaryOp::AddAssign:
			case BinaryOp::SubAssign:
			case BinaryOp::MulAssign:
			case BinaryOp::DivAssign:
			case BinaryOp::ModAssign:
			case BinaryOp::AndAssign:
			case BinaryOp::OrAssign:
			case BinaryOp::XorAssign:
			case BinaryOp::ShlAssign:
			case BinaryOp::ShrAssign:
			case BinaryOp::Eq:
			case BinaryOp::Neq:
			case BinaryOp::Lt:
			case BinaryOp::Gt:
			case BinaryOp::LtEq:
			case BinaryOp::GtEq:
			case BinaryOp::Cmp: {
				peff::SharedPtr<MemberNode> clsNode, operatorSlot;

				SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(decayedRhsType->document->sharedFromThis(), decayedRhsType.castTo<CustomTypeNameNode>(), clsNode));

				IdRefEntry e(compileContext->allocator.get());

				std::string_view operatorName = getBinaryOperatorOverloadingName(expr->binaryOp);

				if (!e.name.build(operatorName)) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, compileContext->document, clsNode, e, operatorSlot));

				if (!operatorSlot)
					return CompilationError(
						expr->tokenRange,
						CompilationErrorKind::OperatorNotFound);

				if (operatorSlot->astNodeType != AstNodeType::FnSlot)
					std::terminate();

				peff::DynArray<peff::SharedPtr<FnOverloadingNode>> matchedOverloadingIndices(compileContext->allocator.get());
				peff::DynArray<peff::SharedPtr<TypeNameNode>> operatorParamTypes(compileContext->allocator.get());

				if (!operatorParamTypes.pushBack(peff::SharedPtr<TypeNameNode>(lhsType))) {
					return genOutOfMemoryCompError();
				}

				peff::SharedPtr<VoidTypeNameNode> voidType;

				if (!(voidType = peff::makeSharedWithControlBlock<VoidTypeNameNode, AstNodeControlBlock<VoidTypeNameNode>>(
						  compileContext->allocator.get(),
						  compileContext->allocator.get(),
						  compileContext->document))) {
					return genOutOfMemoryCompError();
				}

				if (!operatorParamTypes.pushBack(voidType.castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, operatorSlot.castTo<FnNode>(), operatorParamTypes.data(), operatorParamTypes.size(), false, matchedOverloadingIndices));

				switch (matchedOverloadingIndices.size()) {
					case 0:
						return CompilationError(
							expr->tokenRange,
							CompilationErrorKind::OperatorNotFound);
					case 1:
						break;
					default:
						return CompilationError(
							expr->tokenRange,
							CompilationErrorKind::AmbiguousOperatorCall);
				}

				auto matchedOverloading = matchedOverloadingIndices.back();

				uint32_t rhsReg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
				{
					CompileExprResult argResult(compileContext->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->rhs, ExprEvalPurpose::RValue, decayedLhsType, rhsReg, argResult));
				}

				uint32_t operatorReg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(operatorReg));
				if (matchedOverloading->fnFlags & FN_VIRTUAL) {
					slake::HostObjectRef<slake::IdRefObject> idRefObject;

					if (!(idRefObject = slake::IdRefObject::alloc(compileContext->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					slake::IdRefEntry e(compileContext->runtime->getCurGenAlloc());

					if (!e.name.build(operatorName)) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!idRefObject->entries.pushBack(std::move(e))) {
						return genOutOfRuntimeMemoryCompError();
					}

					idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
						return genOutOfMemoryCompError();
					}

					for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
					}

					idRefObject->hasVarArgs = true;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::RLOAD, operatorReg, { slake::Value(slake::ValueType::RegRef, rhsReg), slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
				} else {
					slake::HostObjectRef<slake::IdRefObject> idRefObject;

					IdRefPtr fullName;
					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), operatorSlot, fullName));

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, true, idRefObject));

					idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
						return genOutOfMemoryCompError();
					}

					for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
					}

					idRefObject->hasVarArgs = true;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, operatorReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
				}

				uint32_t reg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));
				{
					CompileExprResult argResult(compileContext->allocator.get());

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->params.at(0)->type, b));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->rhs, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, matchedOverloading->params.at(0)->type, reg, argResult));
				}

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::PUSHARG, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, reg) }));

				switch (evalPurpose) {
					case ExprEvalPurpose::EvalType:
						break;
					case ExprEvalPurpose::Stmt:
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));
						break;
					case ExprEvalPurpose::LValue: {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));
						if (!b) {
							return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));
						break;
					}
					case ExprEvalPurpose::RValue:
					case ExprEvalPurpose::Call: {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));

						if (b) {
							uint32_t tmpRegIndex;
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LVALUE, resultRegOut, { slake::Value(slake::ValueType::RegRef, tmpRegIndex) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));
						}

						break;
					}
				}

				goto rhsToLhsCustomOpExprResolved;
			}
			default:
				break;
		}
	}

	{
		peff::SharedPtr<TypeNameNode> promotionalTypeName;

		SLKC_RETURN_IF_COMP_ERROR(determinePromotionalType(decayedLhsType, decayedRhsType, promotionalTypeName));

		switch (promotionalTypeName->typeNameKind) {
			case TypeNameKind::I8:
			case TypeNameKind::I16:
			case TypeNameKind::I32:
			case TypeNameKind::I64:
			case TypeNameKind::U8:
			case TypeNameKind::U16:
			case TypeNameKind::U32:
			case TypeNameKind::U64: {
				switch (expr->binaryOp) {
					case BinaryOp::Add:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Sub:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mul:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Div:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mod:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::And:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Or:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Xor:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLAndBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LAND));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLOrBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LOR));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Shl:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType,
								u32Type.castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LSH));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Shr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType,
								u32Type.castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::RSH));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AddAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::SubAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::MulAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::DivAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ModAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AndAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::OrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::XorAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShlAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileContext->allocator.get(),
									compileContext->allocator.get(),
									compileContext->document)
									.castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LSH));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileContext->allocator.get(),
									compileContext->allocator.get(),
									compileContext->document)
									.castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::RSH));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Lt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LT));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Gt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GT));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::LtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LTEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::GtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GTEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Cmp:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::CMP));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::F32:
			case TypeNameKind::F64: {
				switch (expr->binaryOp) {
					case BinaryOp::Add:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Sub:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mul:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Div:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mod:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLAndBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LAND));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLOrBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LOR));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AddAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::SubAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::MulAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::DivAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ModAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShlAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileContext->allocator.get(),
									compileContext->allocator.get(),
									compileContext->document)
									.castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LSH));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileContext->allocator.get(),
									compileContext->allocator.get(),
									compileContext->document)
									.castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::RSH));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Lt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LT));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Gt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GT));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::LtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LTEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::GtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GTEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Cmp:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::CMP));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Bool: {
				switch (expr->binaryOp) {
					case BinaryOp::And:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Or:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Xor:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLAndBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LAND));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLOrBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LOR));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AndAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::OrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::XorAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Object: {
				switch (expr->binaryOp) {
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Array: {
				switch (expr->binaryOp) {
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileContext,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ));
						resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
						break;
					case BinaryOp::Subscript: {
						peff::SharedPtr<TypeNameNode> evaluatedType;
						if (!(evaluatedType = peff::makeSharedWithControlBlock<RefTypeNameNode, AstNodeControlBlock<RefTypeNameNode>>(
								  compileContext->allocator.get(),
								  compileContext->allocator.get(),
								  compileContext->document,
								  decayedLhsType.castTo<ArrayTypeNameNode>()->elementType)
									.castTo<TypeNameNode>())) {
							return genOutOfMemoryCompError();
						}
						std::optional<CompilationError> e;

						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::LValue: {
								CompileExprResult result(compileContext->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
										if (!compileContext->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.castTo<TypeNameNode>(), expr->rhs, rhsType));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									slake::Opcode::AT,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

								break;
							}
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
									CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compileContext->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
										if (!compileContext->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.castTo<TypeNameNode>(), expr->rhs, rhsType));

								uint32_t tmpReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpReg));

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									slake::Opcode::AT,
									tmpReg,
									{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									slake::Opcode::LVALUE,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegRef, tmpReg) }));

								break;
							}
							case ExprEvalPurpose::Call:
								return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
						}

						resultOut.evaluatedType = evaluatedType;
						break;
					}
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Custom: {
				switch (expr->binaryOp) {
					case BinaryOp::Add:
					case BinaryOp::Sub:
					case BinaryOp::Mul:
					case BinaryOp::Div:
					case BinaryOp::Mod:
					case BinaryOp::And:
					case BinaryOp::Or:
					case BinaryOp::Xor:
					case BinaryOp::LAnd:
					case BinaryOp::LOr:
					case BinaryOp::Shl:
					case BinaryOp::Shr:
					case BinaryOp::Assign:
					case BinaryOp::AddAssign:
					case BinaryOp::SubAssign:
					case BinaryOp::MulAssign:
					case BinaryOp::DivAssign:
					case BinaryOp::ModAssign:
					case BinaryOp::AndAssign:
					case BinaryOp::OrAssign:
					case BinaryOp::XorAssign:
					case BinaryOp::ShlAssign:
					case BinaryOp::ShrAssign:
					case BinaryOp::Eq:
					case BinaryOp::Neq:
					case BinaryOp::Lt:
					case BinaryOp::Gt:
					case BinaryOp::LtEq:
					case BinaryOp::GtEq:
					case BinaryOp::Cmp: {
						peff::SharedPtr<MemberNode> clsNode, operatorSlot;

						SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(decayedLhsType->document->sharedFromThis(), decayedLhsType.castTo<CustomTypeNameNode>(), clsNode));

						IdRefEntry e(compileContext->allocator.get());

						std::string_view operatorName = getBinaryOperatorOverloadingName(expr->binaryOp);

						if (!e.name.build(operatorName)) {
							return genOutOfMemoryCompError();
						}

						SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileContext, compileContext->document, clsNode, e, operatorSlot));

						if (!operatorSlot)
							return CompilationError(
								expr->tokenRange,
								CompilationErrorKind::OperatorNotFound);

						if (operatorSlot->astNodeType != AstNodeType::FnSlot)
							std::terminate();

						peff::DynArray<peff::SharedPtr<FnOverloadingNode>> matchedOverloadingIndices(compileContext->allocator.get());
						peff::DynArray<peff::SharedPtr<TypeNameNode>> operatorParamTypes(compileContext->allocator.get());

						if (!operatorParamTypes.pushBack(peff::SharedPtr<TypeNameNode>(rhsType))) {
							return genOutOfMemoryCompError();
						}

						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, operatorSlot.castTo<FnNode>(), operatorParamTypes.data(), operatorParamTypes.size(), false, matchedOverloadingIndices));

						switch (matchedOverloadingIndices.size()) {
							case 0:
								return CompilationError(
									expr->tokenRange,
									CompilationErrorKind::OperatorNotFound);
							case 1:
								break;
							default:
								return CompilationError(
									expr->tokenRange,
									CompilationErrorKind::AmbiguousOperatorCall);
						}

						auto matchedOverloading = matchedOverloadingIndices.back();

						uint32_t lhsReg;
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
						{
							CompileExprResult argResult(compileContext->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->lhs, ExprEvalPurpose::RValue, decayedLhsType, lhsReg, argResult));
						}

						uint32_t operatorReg;
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(operatorReg));
						if (matchedOverloading->fnFlags & FN_VIRTUAL) {
							slake::HostObjectRef<slake::IdRefObject> idRefObject;

							if (!(idRefObject = slake::IdRefObject::alloc(compileContext->runtime))) {
								return genOutOfRuntimeMemoryCompError();
							}

							slake::IdRefEntry e(compileContext->runtime->getCurGenAlloc());

							if (!e.name.build(operatorName)) {
								return genOutOfRuntimeMemoryCompError();
							}

							if (!idRefObject->entries.pushBack(std::move(e))) {
								return genOutOfRuntimeMemoryCompError();
							}

							idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

							if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
								return genOutOfMemoryCompError();
							}

							for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
							}

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::RLOAD, operatorReg, { slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
						} else {
							slake::HostObjectRef<slake::IdRefObject> idRefObject;

							if (!(idRefObject = slake::IdRefObject::alloc(compileContext->runtime))) {
								return genOutOfRuntimeMemoryCompError();
							}

							IdRefPtr fullName;
							SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), operatorSlot, fullName));

							idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

							if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
								return genOutOfMemoryCompError();
							}

							for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
							}

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, operatorReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
						}

						uint32_t reg;
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));
						{
							CompileExprResult argResult(compileContext->allocator.get());

							bool b = false;
							SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->params.at(0)->type, b));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, expr->rhs, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, matchedOverloading->params.at(0)->type, reg, argResult));
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::PUSHARG, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, reg) }));

						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, lhsReg) }));
								break;
							case ExprEvalPurpose::LValue: {
								bool b = false;
								SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));
								if (!b) {
									return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
								}

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, lhsReg) }));
								break;
							}
							case ExprEvalPurpose::RValue:
							case ExprEvalPurpose::Call: {
								bool b = false;
								SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));

								if (b) {
									uint32_t tmpRegIndex;
									SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

									SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, lhsReg) }));

									SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LVALUE, resultRegOut, { slake::Value(slake::ValueType::RegRef, tmpRegIndex) }));
								} else {
									SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, operatorReg), slake::Value(slake::ValueType::RegRef, lhsReg) }));
								}

								break;
							}
						}

						break;
					}
					case BinaryOp::StrictEq:
						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::LValue:
								return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
									CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compileContext->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								std::optional<CompilationError> e;

								if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType); re) {
										if (!compileContext->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									slake::Opcode::EQ,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

								break;
							}
							case ExprEvalPurpose::Call:
								return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
						}
						break;
					case BinaryOp::StrictNeq:
						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::LValue:
								return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
									CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compileContext->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								std::optional<CompilationError> e;

								if ((e = _compileOrCastOperand(compileContext, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType); re) {
										if (!compileContext->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									slake::Opcode::NEQ,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) }));

								break;
							}
							case ExprEvalPurpose::Call:
								return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
						}
						break;
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
				}
				break;
			}
			case TypeNameKind::Ref:
				std::terminate();
			default:
				return CompilationError(
					expr->tokenRange,
					CompilationErrorKind::OperatorNotFound);
		}
	}

rhsToLhsCustomOpExprResolved:
	return {};
}
