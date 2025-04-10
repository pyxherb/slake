#include "../../compiler.h"

using namespace slkc;

std::optional<CompilationError> Compiler::_compileSimpleBinaryExpr(
	TopLevelCompileContext *compileContext,
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
	switch (evalPurpose) {
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::RValue: {
			CompileExprResult result;

			uint32_t lhsReg = compileContext->allocReg(),
					 rhsReg = compileContext->allocReg();

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, lhsReg, lhsEvalPurpose, desiredLhsType, expr->lhs, lhsType));
			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushIns(
				emitIns(
					opcode,
					resultRegOut,
					{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) })));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API std::optional<CompilationError> Compiler::_compileSimpleBinaryAssignOpExpr(
	TopLevelCompileContext *compileContext,
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
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(expr->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
		case ExprEvalPurpose::RValue: {
			CompileExprResult result;

			uint32_t lhsReg = compileContext->allocReg(),
					 lhsValueReg = compileContext->allocReg(),
					 rhsReg = compileContext->allocReg();

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, lhsReg, ExprEvalPurpose::LValue, lhsType, expr->lhs, lhsType));
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushIns(
				emitIns(
					slake::Opcode::LVALUE,
					lhsValueReg,
					{ slake::Value(slake::ValueType::RegRef, lhsReg) })));

			SLKC_RETURN_IF_COMP_ERROR(_compileOrCastOperand(compileContext, rhsReg, rhsEvalPurpose, desiredRhsType, expr->rhs, rhsType));
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushIns(
				emitIns(
					opcode,
					resultRegOut,
					{ slake::Value(slake::ValueType::RegRef, lhsReg), slake::Value(slake::ValueType::RegRef, rhsReg) })));

			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushIns(
				emitIns(
					opcode,
					resultRegOut,
					{ slake::Value(slake::ValueType::RegRef, lhsValueReg), slake::Value(slake::ValueType::RegRef, rhsReg) })));

			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushIns(
				emitIns(
					slake::Opcode::STORE,
					lhsReg,
					{ slake::Value(slake::ValueType::RegRef, resultRegOut) })));

			break;
		}
		case ExprEvalPurpose::Call:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
	}

	return {};
}

SLKC_API std::optional<CompilationError> Compiler::compileBinaryExpr(
	TopLevelCompileContext *compileContext,
	peff::SharedPtr<BinaryExprNode> expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<TypeNameNode> lhsType, rhsType, decayedLhsType, decayedRhsType;

	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileContext, expr->lhs, lhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		evalExprType(compileContext, expr->rhs, rhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(compileContext, lhsType, decayedLhsType));
	SLKC_RETURN_IF_COMP_ERROR(
		removeRefOfType(compileContext, rhsType, decayedRhsType));

	if (expr->binaryOp == BinaryOp::Comma) {
		CompileExprResult result;
		uint32_t tmpRegIndex = compileContext->allocReg();
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, expr->lhs, ExprEvalPurpose::Stmt, tmpRegIndex, result));
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, expr->rhs, evalPurpose, resultRegOut, resultOut));
		return {};
	}

	peff::SharedPtr<U32TypeNameNode> u32Type;
	peff::SharedPtr<I32TypeNameNode> i32Type;
	peff::SharedPtr<BoolTypeNameNode> boolType;

	if (!(u32Type = peff::makeShared<U32TypeNameNode>(
		compileContext->allocator.get(),
		compileContext->allocator.get(),
		compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	if (!(i32Type = peff::makeShared<I32TypeNameNode>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	if (!(boolType = peff::makeShared<BoolTypeNameNode>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document))) {
		return genOutOfMemoryCompError();
	}

	switch (decayedLhsType->typeNameKind) {
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::ADD));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Sub:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::SUB));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Mul:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MUL));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Div:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::DIV));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Mod:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MOD));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::And:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::AND));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Or:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::OR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Xor:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::XOR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::LAnd:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LAND));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::LOr:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LOR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Shl:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType, lhsType, ExprEvalPurpose::LValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::STORE));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::AddAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::ADD));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::SubAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::SUB));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::MulAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MUL));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::DivAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::DIV));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::ModAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MOD));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::AndAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::AND));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::OrAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::OR));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::XorAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::XOR));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::ShlAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType,
							peff::makeShared<U32TypeNameNode>(
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
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType,
							peff::makeShared<U32TypeNameNode>(
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::NEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Lt:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LT));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Gt:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::GT));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::LtEq:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LTEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::GtEq:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::GTEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Cmp:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::ADD));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Sub:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::SUB));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Mul:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MUL));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Div:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::DIV));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Mod:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MOD));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::LAnd:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LAND));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::LOr:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LOR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Assign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType, lhsType, ExprEvalPurpose::LValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::STORE));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::AddAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::ADD));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::SubAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::SUB));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::MulAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MUL));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::DivAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::DIV));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::ModAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::MOD));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::ShlAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType,
							peff::makeShared<U32TypeNameNode>(
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
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType,
							peff::makeShared<U32TypeNameNode>(
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::NEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Lt:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LT));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Gt:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::GT));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::LtEq:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LTEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::GtEq:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::GTEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Cmp:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::AND));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Or:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::OR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Xor:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::XOR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::LAnd:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LAND));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::LOr:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::LOR));
					resultOut.evaluatedType = decayedLhsType;
					break;
				case BinaryOp::Assign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType, lhsType, ExprEvalPurpose::LValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::STORE));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::AndAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::AND));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::OrAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::OR));
					resultOut.evaluatedType = lhsType;
					break;
				case BinaryOp::XorAssign:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryAssignOpExpr(
							compileContext,
							expr,
							evalPurpose,
							lhsType,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::EQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::StrictNeq:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
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
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::EQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::StrictNeq:
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, decayedLhsType, ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::NEQ));
					resultOut.evaluatedType = boolType.castTo<TypeNameNode>();
					break;
				case BinaryOp::Subscript: {
					peff::SharedPtr<TypeNameNode> evaluatedType;
					if (!(evaluatedType = peff::makeShared<RefTypeNameNode>(
						compileContext->allocator.get(),
						compileContext->allocator.get(),
						compileContext->document,
						decayedLhsType.castTo<ArrayTypeNameNode>()->elementType).castTo<TypeNameNode>())) {
						return genOutOfMemoryCompError();
					}
					SLKC_RETURN_IF_COMP_ERROR(
						_compileSimpleBinaryExpr(
							compileContext,
							expr,
							evalPurpose,
							decayedLhsType, decayedLhsType, ExprEvalPurpose::RValue,
							decayedRhsType, u32Type.castTo<TypeNameNode>(), ExprEvalPurpose::RValue,
							resultRegOut,
							resultOut,
							slake::Opcode::NEQ));
					resultOut.evaluatedType = evaluatedType;
					break;
				}
				default:
					return CompilationError(expr->tokenRange, CompilationErrorKind::OperatorNotFound);
			}
			break;
		}
		default:
			return CompilationError(
				expr->tokenRange,
				CompilationErrorKind::OperatorNotFound);
	}

	return {};
}
