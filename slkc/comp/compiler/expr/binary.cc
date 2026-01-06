#include "../../compiler.h"

using namespace slkc;

peff::Option<CompilationError> slkc::_compileSimpleBinaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> lhsType,
	AstNodePtr<TypeNameNode> desiredLhsType,
	ExprEvalPurpose lhsEvalPurpose,
	AstNodePtr<TypeNameNode> rhsType,
	AstNodePtr<TypeNameNode> desiredRhsType,
	ExprEvalPurpose rhsEvalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode,
	uint32_t idxSld) {
	peff::Option<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t lhsReg,
				rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, lhsEvalPurpose, desiredLhsType, expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileEnv->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				opcode,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

peff::Option<CompilationError> slkc::_compileSimpleAssignBinaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> lhsType,
	AstNodePtr<TypeNameNode> desiredLhsType,
	AstNodePtr<TypeNameNode> rhsType,
	AstNodePtr<TypeNameNode> desiredRhsType,
	ExprEvalPurpose rhsEvalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	uint32_t idxSld) {
	peff::Option<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			if ((e = _compileOrCastOperand(compileEnv, compilationContext, resultRegOut, ExprEvalPurpose::LValue, desiredLhsType, expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileEnv->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, resultRegOut), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			break;
		}
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t lhsReg,
				rhsReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

			if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::LValue, desiredLhsType, expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType); re) {
					if (!compileEnv->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::MOV,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

peff::Option<CompilationError> slkc::_compileSimpleLAndBinaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<BoolTypeNameNode> boolType,
	AstNodePtr<TypeNameNode> lhsType,
	AstNodePtr<TypeNameNode> rhsType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode,
	uint32_t idxSld) {
	peff::Option<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t lhsReg,
				rhsReg,
				tmpResultReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpResultReg));

			uint32_t cmpEndLabelId;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(cmpEndLabelId));

			if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
					if (!compileEnv->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}

			uint32_t postBranchLabelId;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(postBranchLabelId));

			uint32_t postBranchPhiSrcOff = compilationContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::Label, postBranchLabelId), slake::Value(slake::ValueType::Label, cmpEndLabelId) }));

			compilationContext->setLabelOffset(postBranchLabelId, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::LAND,
				tmpResultReg,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			uint32_t cmpEndPhiSrcOff = compilationContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cmpEndLabelId) }));

			compilationContext->setLabelOffset(cmpEndLabelId, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::PHI,
				tmpResultReg,
				{ slake::Value((uint32_t)postBranchPhiSrcOff), slake::Value(slake::ValueType::RegIndex, lhsReg),
					slake::Value((uint32_t)cmpEndPhiSrcOff), slake::Value(slake::ValueType::RegIndex, tmpResultReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

peff::Option<CompilationError> slkc::_compileSimpleLOrBinaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<BoolTypeNameNode> boolType,
	AstNodePtr<TypeNameNode> lhsType,
	AstNodePtr<TypeNameNode> rhsType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode,
	uint32_t idxSld) {
	peff::Option<CompilationError> e;

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::Stmt:
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t lhsReg,
				rhsReg,
				tmpResultReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpResultReg));

			uint32_t cmpEndLabelId;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(cmpEndLabelId));

			if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), expr->lhs, lhsType))) {
				if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
					if (!compileEnv->errors.pushBack(std::move(*e))) {
						return genOutOfMemoryCompError();
					}
					e.reset();
					return re;
				} else {
					return e;
				}
			}

			uint32_t postBranchLabelId;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(postBranchLabelId));

			uint32_t postBranchPhiSrcOff = compilationContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::BR,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::Label, cmpEndLabelId), slake::Value(slake::ValueType::Label, postBranchLabelId) }));

			compilationContext->setLabelOffset(postBranchLabelId, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, boolType.template castTo<TypeNameNode>(), expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::LAND,
				tmpResultReg,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			uint32_t cmpEndPhiSrcOff = compilationContext->getCurInsOff();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::JMP,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::Label, cmpEndLabelId) }));

			compilationContext->setLabelOffset(cmpEndLabelId, compilationContext->getCurInsOff());

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::PHI,
				tmpResultReg,
				{ slake::Value((uint32_t)postBranchPhiSrcOff), slake::Value(slake::ValueType::RegIndex, lhsReg),
					slake::Value((uint32_t)cmpEndPhiSrcOff), slake::Value(slake::ValueType::RegIndex, tmpResultReg) }));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::_compileSimpleBinaryAssignOpExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> lhsType,
	AstNodePtr<TypeNameNode> rhsType,
	AstNodePtr<TypeNameNode> desiredRhsType,
	ExprEvalPurpose rhsEvalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut,
	slake::Opcode opcode,
	uint32_t idxSld) {
	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::LValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t lhsReg,
				lhsValueReg,
				rhsReg,
				resultValueReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsValueReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(resultValueReg));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::LValue, lhsType, expr->lhs, lhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::LVALUE,
				lhsValueReg,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				opcode,
				resultValueReg,
				{ slake::Value(slake::ValueType::RegIndex, lhsValueReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, resultValueReg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::MOV,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg) }));
			break;
		}
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			[[fallthrough]];
		case ExprEvalPurpose::RValue: {
			CompileExprResult result(compileEnv->allocator.get());

			uint32_t lhsReg,
				lhsValueReg,
				rhsReg,
				resultValueReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsValueReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(resultValueReg));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::LValue, lhsType, expr->lhs, lhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::LVALUE,
				lhsValueReg,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				opcode,
				resultValueReg,
				{ slake::Value(slake::ValueType::RegIndex, lhsValueReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::STORE,
				UINT32_MAX,
				{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, resultValueReg) }));
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
				idxSld,
				slake::Opcode::MOV,
				resultRegOut,
				{ slake::Value(slake::ValueType::RegIndex, resultValueReg) }));
			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compileBinaryExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	AstNodePtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	AstNodePtr<TypeNameNode> lhsType, rhsType, decayedLhsType, decayedRhsType;

	if (auto e = evalExprType(compileEnv, compilationContext, expr->lhs, lhsType); e) {
		if (auto re = evalExprType(compileEnv, compilationContext, expr->rhs, rhsType); re) {
			if (!compileEnv->errors.pushBack(std::move(*e))) {
				return genOutOfMemoryCompError();
			}
			e.reset();
			return re;
		}
		return e;
	}
	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileEnv, compilationContext, expr->rhs, rhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(lhsType, decayedLhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(rhsType, decayedRhsType));

	uint32_t sldIndex;
	SLKC_RETURN_IF_COMP_ERROR(compilationContext->registerSourceLocDesc(tokenRangeToSld(expr->tokenRange), sldIndex));

	if (expr->binaryOp == BinaryOp::Comma) {
		CompileExprResult result(compileEnv->allocator.get());
		uint32_t tmpRegIndex;

		SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->lhs, ExprEvalPurpose::Stmt, {}, tmpRegIndex, result));
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->rhs, evalPurpose, {}, resultRegOut, resultOut));
		return {};
	}

	peff::SharedPtr<U32TypeNameNode> u32Type;
	peff::SharedPtr<I32TypeNameNode> i32Type;
	AstNodePtr<BoolTypeNameNode> boolType;

	if (!(u32Type = peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	if (!(i32Type = peff::makeSharedWithControlBlock<I32TypeNameNode, AstNodeControlBlock<I32TypeNameNode>>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
		return genOutOfMemoryCompError();
	}

	if (!(boolType = makeAstNode<BoolTypeNameNode>(
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document))) {
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
				AstNodePtr<MemberNode> clsNode, operatorSlot;

				SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileEnv, decayedRhsType->document->sharedFromThis(), decayedRhsType.template castTo<CustomTypeNameNode>(), clsNode));

				IdRefEntry e(compileEnv->allocator.get());

				std::string_view operatorName = getBinaryOperatorOverloadingName(expr->binaryOp);

				if (!e.name.build(operatorName)) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, compileEnv->document, clsNode, e, operatorSlot));

				if (!operatorSlot)
					return CompilationError(
						expr->tokenRange,
						CompilationErrorKind::OperatorNotFound);

				if (operatorSlot->getAstNodeType() != AstNodeType::Fn)
					std::terminate();

				peff::DynArray<AstNodePtr<FnOverloadingNode>> matchedOverloadingIndices(compileEnv->allocator.get());
				peff::DynArray<AstNodePtr<TypeNameNode>> operatorParamTypes(compileEnv->allocator.get());

				if (!operatorParamTypes.pushBack(AstNodePtr<TypeNameNode>(lhsType))) {
					return genOutOfMemoryCompError();
				}

				AstNodePtr<VoidTypeNameNode> voidType;

				if (!(voidType = makeAstNode<VoidTypeNameNode>(
						  compileEnv->allocator.get(),
						  compileEnv->allocator.get(),
						  compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				if (!operatorParamTypes.pushBack(voidType.template castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}

				SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, operatorSlot.template castTo<FnNode>(), operatorParamTypes.data(), operatorParamTypes.size(), false, matchedOverloadingIndices));

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

				bool accessible;
				SLKC_RETURN_IF_COMP_ERROR(isMemberAccessible(compileEnv, {}, matchedOverloading.castTo<MemberNode>(), accessible));
				if (!accessible)
					return CompilationError(
						expr->tokenRange,
						CompilationErrorKind::MemberIsNotAccessible);

				uint32_t rhsReg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));
				{
					CompileExprResult argResult(compileEnv->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->rhs, ExprEvalPurpose::RValue, decayedLhsType, rhsReg, argResult));
				}

				uint32_t operatorReg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(operatorReg));
				if (matchedOverloading->fnFlags & FN_VIRTUAL) {
					slake::HostObjectRef<slake::IdRefObject> idRefObject;

					if (!(idRefObject = slake::IdRefObject::alloc(compileEnv->runtime))) {
						return genOutOfRuntimeMemoryCompError();
					}

					slake::IdRefEntry e(compileEnv->runtime->getCurGenAlloc());

					if (!e.name.build(operatorName)) {
						return genOutOfRuntimeMemoryCompError();
					}

					if (!idRefObject->entries.pushBack(std::move(e))) {
						return genOutOfRuntimeMemoryCompError();
					}
					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, decayedRhsType, idRefObject->overridenType));

					idRefObject->paramTypes = peff::DynArray<slake::TypeRef>(compileEnv->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
						return genOutOfMemoryCompError();
					}

					for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
					}

					idRefObject->hasVarArgs = true;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::RLOAD, operatorReg, { slake::Value(slake::ValueType::RegIndex, rhsReg), slake::Value(slake::Reference::makeObjectRef(idRefObject.get())) }));
				} else {
					slake::HostObjectRef<slake::IdRefObject> idRefObject;

					IdRefPtr fullName;
					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), operatorSlot, fullName));

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, true, decayedRhsType, idRefObject));

					idRefObject->paramTypes = peff::DynArray<slake::TypeRef>(compileEnv->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
						return genOutOfMemoryCompError();
					}

					for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
					}

					idRefObject->hasVarArgs = true;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::LOAD, operatorReg, { slake::Value(slake::Reference::makeObjectRef(idRefObject.get())) }));
				}

				uint32_t reg;
				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));
				{
					CompileExprResult argResult(compileEnv->allocator.get());

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->params.at(0)->type, b));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->rhs, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, matchedOverloading->params.at(0)->type, reg, argResult));
				}

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::PUSHARG, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, reg) }));

				switch (evalPurpose) {
					case ExprEvalPurpose::EvalType:
						break;
					case ExprEvalPurpose::Stmt:
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));
						break;
					case ExprEvalPurpose::LValue: {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));
						if (!b) {
							return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));
						break;
					}
					case ExprEvalPurpose::RValue:
					case ExprEvalPurpose::Call: {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));

						if (b) {
							uint32_t tmpRegIndex;
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::LVALUE, resultRegOut, { slake::Value(slake::ValueType::RegIndex, tmpRegIndex) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));
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
		AstNodePtr<TypeNameNode> promotionalTypeName;

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
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Sub:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mul:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Div:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mod:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::And:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Or:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Xor:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLAndBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LAND,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLOrBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LOR,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Shl:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType,
								u32Type.template castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LSH,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Shr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType,
								u32Type.template castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::RSH,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AddAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::SubAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::MulAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::DivAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ModAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AndAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::OrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::XorAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShlAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileEnv->allocator.get(),
									compileEnv->allocator.get(),
									compileEnv->document)
									.template castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LSH,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileEnv->allocator.get(),
									compileEnv->allocator.get(),
									compileEnv->document)
									.template castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::RSH,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Lt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LT,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Gt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GT,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::LtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LTEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::GtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GTEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Cmp:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::CMP,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
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
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Sub:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mul:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Div:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Mod:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLAndBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LAND,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLOrBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LOR,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AddAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::ADD,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::SubAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::SUB,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::MulAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MUL,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::DivAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::DIV,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ModAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::MOD,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShlAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileEnv->allocator.get(),
									compileEnv->allocator.get(),
									compileEnv->document)
									.template castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LSH,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::ShrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType,
								peff::makeSharedWithControlBlock<U32TypeNameNode, AstNodeControlBlock<U32TypeNameNode>>(
									compileEnv->allocator.get(),
									compileEnv->allocator.get(),
									compileEnv->document)
									.template castTo<TypeNameNode>(),
								ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::RSH,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Lt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LT,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Gt:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GT,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::LtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::LTEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::GtEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::GTEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Cmp:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::CMP,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
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
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Or:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::Xor:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR,
								sldIndex));
						resultOut.evaluatedType = decayedLhsType;
						break;
					case BinaryOp::LAnd:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLAndBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LAND,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::LOr:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleLOrBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								boolType,
								decayedLhsType,
								decayedRhsType,
								resultRegOut,
								resultOut,
								slake::Opcode::LOR,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::AndAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::AND,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::OrAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::OR,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::XorAssign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryAssignOpExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::XOR,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
					case BinaryOp::Eq:
					case BinaryOp::StrictEq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Neq:
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
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
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
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
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::EQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::StrictNeq:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								decayedLhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								decayedRhsType, promotionalTypeName, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								slake::Opcode::NEQ,
								sldIndex));
						resultOut.evaluatedType = boolType.template castTo<TypeNameNode>();
						break;
					case BinaryOp::Subscript: {
						AstNodePtr<TypeNameNode> evaluatedType;
						if (!(evaluatedType = makeAstNode<RefTypeNameNode>(
								  compileEnv->allocator.get(),
								  compileEnv->allocator.get(),
								  compileEnv->document,
								  decayedLhsType.template castTo<ArrayTypeNameNode>()->elementType)
									.template castTo<TypeNameNode>())) {
							return genOutOfMemoryCompError();
						}
						peff::Option<CompilationError> e;

						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::LValue: {
								CompileExprResult result(compileEnv->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.template castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
										if (!compileEnv->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.template castTo<TypeNameNode>(), expr->rhs, rhsType));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									sldIndex,
									slake::Opcode::AT,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

								break;
							}
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
									CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compileEnv->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.template castTo<TypeNameNode>(), expr->rhs, rhsType); re) {
										if (!compileEnv->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, u32Type.template castTo<TypeNameNode>(), expr->rhs, rhsType));

								uint32_t tmpReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpReg));

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									sldIndex,
									slake::Opcode::AT,
									tmpReg,
									{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									sldIndex,
									slake::Opcode::LVALUE,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegIndex, tmpReg) }));

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
					case BinaryOp::Assign:
						SLKC_RETURN_IF_COMP_ERROR(
							_compileSimpleAssignBinaryExpr(
								compileEnv,
								compilationContext,
								expr,
								evalPurpose,
								lhsType, lhsType,
								decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
								resultRegOut,
								resultOut,
								sldIndex));
						resultOut.evaluatedType = lhsType;
						break;
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
						AstNodePtr<MemberNode> clsNode, operatorSlot;

						SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileEnv, decayedLhsType->document->sharedFromThis(), decayedLhsType.template castTo<CustomTypeNameNode>(), clsNode));

						IdRefEntry e(compileEnv->allocator.get());

						const char *operatorName = getBinaryOperatorOverloadingName(expr->binaryOp);

						if (!operatorName)
							return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);

						if (!e.name.build(operatorName)) {
							return genOutOfMemoryCompError();
						}

						SLKC_RETURN_IF_COMP_ERROR(resolveInstanceMember(compileEnv, compileEnv->document, clsNode, e, operatorSlot));

						if (!operatorSlot)
							return CompilationError(
								expr->tokenRange,
								CompilationErrorKind::OperatorNotFound);

						if (operatorSlot->getAstNodeType() != AstNodeType::Fn)
							std::terminate();

						peff::DynArray<AstNodePtr<FnOverloadingNode>> matchedOverloadingIndices(compileEnv->allocator.get());
						peff::DynArray<AstNodePtr<TypeNameNode>> operatorParamTypes(compileEnv->allocator.get());

						if (!operatorParamTypes.pushBack(AstNodePtr<TypeNameNode>(rhsType))) {
							return genOutOfMemoryCompError();
						}

						SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, operatorSlot.template castTo<FnNode>(), operatorParamTypes.data(), operatorParamTypes.size(), false, matchedOverloadingIndices));

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

						bool accessible;
						SLKC_RETURN_IF_COMP_ERROR(isMemberAccessible(compileEnv, {}, matchedOverloading.castTo<MemberNode>(), accessible));
						if (!accessible)
							return CompilationError(
								expr->tokenRange,
								CompilationErrorKind::MemberIsNotAccessible);

						uint32_t lhsReg;
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
						{
							CompileExprResult argResult(compileEnv->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->lhs, ExprEvalPurpose::RValue, decayedLhsType, lhsReg, argResult));
						}

						uint32_t operatorReg;
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(operatorReg));
						if (matchedOverloading->fnFlags & FN_VIRTUAL) {
							slake::HostObjectRef<slake::IdRefObject> idRefObject;

							if (!(idRefObject = slake::IdRefObject::alloc(compileEnv->runtime))) {
								return genOutOfRuntimeMemoryCompError();
							}

							slake::IdRefEntry e(compileEnv->runtime->getCurGenAlloc());

							if (!e.name.build(operatorName)) {
								return genOutOfRuntimeMemoryCompError();
							}

							if (!idRefObject->entries.pushBack(std::move(e))) {
								return genOutOfRuntimeMemoryCompError();
							}
							SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, decayedLhsType, idRefObject->overridenType));

							idRefObject->paramTypes = peff::DynArray<slake::TypeRef>(compileEnv->runtime->getCurGenAlloc());

							if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
								return genOutOfMemoryCompError();
							}

							for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
							}

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::RLOAD, operatorReg, { slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::Reference::makeObjectRef(idRefObject.get())) }));
						} else {
							slake::HostObjectRef<slake::IdRefObject> idRefObject;

							if (!(idRefObject = slake::IdRefObject::alloc(compileEnv->runtime))) {
								return genOutOfRuntimeMemoryCompError();
							}

							IdRefPtr fullName;
							SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), operatorSlot, fullName));

							SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullName->entries.data(), fullName->entries.size(), nullptr, 0, matchedOverloading->fnFlags & FN_VARG, {}, idRefObject));

							idRefObject->paramTypes = peff::DynArray<slake::TypeRef>(compileEnv->runtime->getCurGenAlloc());

							if (!idRefObject->paramTypes->resize(matchedOverloading->params.size())) {
								return genOutOfMemoryCompError();
							}

							for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, matchedOverloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
							}

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::LOAD, operatorReg, { slake::Value(slake::Reference::makeObjectRef(idRefObject.get())) }));
						}

						uint32_t reg;
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));
						{
							CompileExprResult argResult(compileEnv->allocator.get());

							bool b = false;
							SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->params.at(0)->type, b));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, expr->rhs, b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, matchedOverloading->params.at(0)->type, reg, argResult));
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::PUSHARG, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, reg) }));

						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, lhsReg) }));
								break;
							case ExprEvalPurpose::LValue: {
								bool b = false;
								SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));
								if (!b) {
									return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
								}

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, lhsReg) }));
								break;
							}
							case ExprEvalPurpose::RValue:
							case ExprEvalPurpose::Call: {
								bool b = false;
								SLKC_RETURN_IF_COMP_ERROR(isLValueType(matchedOverloading->returnType, b));

								if (b) {
									uint32_t tmpRegIndex;
									SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

									SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, lhsReg) }));

									SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::LVALUE, resultRegOut, { slake::Value(slake::ValueType::RegIndex, tmpRegIndex) }));
								} else {
									SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(sldIndex, slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegIndex, operatorReg), slake::Value(slake::ValueType::RegIndex, lhsReg) }));
								}

								break;
							}
						}
						resultOut.evaluatedType = matchedOverloading->returnType;
						break;
					}
					case BinaryOp::StrictEq:
						switch (evalPurpose) {
							case ExprEvalPurpose::EvalType:
								break;
							case ExprEvalPurpose::LValue:
								return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
							case ExprEvalPurpose::Stmt:
								SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
									CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compileEnv->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								peff::Option<CompilationError> e;

								if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType); re) {
										if (!compileEnv->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									sldIndex,
									slake::Opcode::EQ,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

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
								SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
									CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
								[[fallthrough]];
							case ExprEvalPurpose::RValue: {
								CompileExprResult result(compileEnv->allocator.get());

								uint32_t lhsReg,
									rhsReg;

								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(lhsReg));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(rhsReg));

								peff::Option<CompilationError> e;

								if ((e = _compileOrCastOperand(compileEnv, compilationContext, lhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->lhs, lhsType))) {
									if (auto re = _compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType); re) {
										if (!compileEnv->errors.pushBack(std::move(*e))) {
											return genOutOfMemoryCompError();
										}
										e.reset();
										return re;
									} else {
										return e;
									}
								}
								SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileEnv, compilationContext, rhsReg, ExprEvalPurpose::RValue, decayedLhsType, expr->rhs, rhsType));
								SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(
									sldIndex,
									slake::Opcode::NEQ,
									resultRegOut,
									{ slake::Value(slake::ValueType::RegIndex, lhsReg), slake::Value(slake::ValueType::RegIndex, rhsReg) }));

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
