#include "../compiler.h"

using namespace slkc;

template <typename LE, typename DT, typename TN>
SLAKE_FORCEINLINE std::optional<CompilationError> _compileLiteralExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	const peff::SharedPtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<LE> e = expr.castTo<LE>();

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
				CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::RValue:
			if (resultRegOut != UINT32_MAX) {
				SLKC_RETURN_IF_COMP_ERROR(
					compilationContext->emitIns(
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
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
		default:
			std::terminate();
	}
	if (!(resultOut.evaluatedType = peff::makeSharedWithControlBlock<TN, AstNodeControlBlock<TN>>(
			  compileContext->allocator.get(),
			  compileContext->allocator.get(),
			  compileContext->document)
				.template castTo<TypeNameNode>())) {
		return genOutOfMemoryCompError();
	}
	return {};
}

SLKC_API std::optional<CompilationError> slkc::_compileOrCastOperand(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	uint32_t regOut,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> desiredType,
	peff::SharedPtr<ExprNode> operand,
	peff::SharedPtr<TypeNameNode> operandType) {
	bool whether;
	SLKC_RETURN_IF_COMP_ERROR(isSameType(desiredType, operandType, whether));
	if (whether) {
		CompileExprResult result(compileContext->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, operand, evalPurpose, desiredType, regOut, result));
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(operandType, desiredType, true, whether));
	if (whether) {
		CompileExprResult result(compileContext->allocator.get());

		peff::SharedPtr<CastExprNode> castExpr;

		if (!(castExpr = peff::makeSharedWithControlBlock<CastExprNode, AstNodeControlBlock<CastExprNode>>(
				  compileContext->allocator.get(),
				  compileContext->allocator.get(),
				  compileContext->document))) {
			return genOutOfMemoryCompError();
		}

		castExpr->targetType = desiredType;
		castExpr->source = operand;

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, castExpr.castTo<ExprNode>(), evalPurpose, desiredType, regOut, result));
		return {};
	}

	IncompatibleOperandErrorExData exData;
	exData.desiredType = desiredType;

	return CompilationError(operand->tokenRange, std::move(exData));
}

static std::optional<CompilationError> _loadTheRestOfIdRef(CompileContext *compileContext, CompilationContext *compilationContext, uint32_t resultRegOut, CompileExprResult &resultOut, IdRef *idRef, ResolvedIdRefPartList &parts, uint32_t initialMemberReg, size_t curIdx, peff::SharedPtr<FnTypeNameNode> extraFnArgs) {
	uint32_t idxReg;

	if (initialMemberReg == UINT32_MAX) {
		SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxReg));
	} else {
		idxReg = initialMemberReg;
	}

	if (parts.back().member->astNodeType == AstNodeType::Fn) {
		peff::SharedPtr<FnOverloadingNode> fn = parts.back().member.castTo<FnOverloadingNode>();

		if (parts.size() > 1) {
			if (fn->fnFlags & FN_VIRTUAL) {
				slake::HostObjectRef<slake::IdRefObject> idRefObject;
				// Is calling a virtual instance method.
				for (size_t i = curIdx; i < parts.size() - 1; ++i) {
					ResolvedIdRefPart &part = parts.at(i);

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

					if (part.isStatic) {
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
					} else {
						uint32_t idxNewReg;

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxNewReg));

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::RLOAD, idxNewReg, { slake::Value(slake::ValueType::RegRef, idxReg), slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
						idxReg = idxNewReg;
					}

					curIdx += part.nEntries;
				}

				{
					ResolvedIdRefPart &part = parts.back();
					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

					if (extraFnArgs) {
						idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

						if (!idRefObject->paramTypes->resize(extraFnArgs->paramTypes.size()))
							return genOutOfRuntimeMemoryCompError();

						for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
							SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes->at(i)));
						}

						if (extraFnArgs->hasVarArgs)
							idRefObject->hasVarArgs = true;
					}

					resultOut.idxThisRegOut = idxReg;
					uint32_t idxNewReg;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxNewReg));

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::RLOAD, idxNewReg, { slake::Value(slake::ValueType::RegRef, idxReg), slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
					idxReg = idxNewReg;
				}
			} else {
				// Is calling an instance method that is not virtual.
				slake::HostObjectRef<slake::IdRefObject> idRefObject;

				for (size_t i = curIdx; i < parts.size() - 1; ++i) {
					ResolvedIdRefPart &part = parts.at(i);

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

					if (part.isStatic) {
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
					} else {
						uint32_t idxNewReg;

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxNewReg));

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::RLOAD, idxNewReg, { slake::Value(slake::ValueType::RegRef, idxReg), slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
						idxReg = idxNewReg;
					}

					curIdx += part.nEntries;
				}

				IdRefPtr fullIdRef;

				resultOut.idxThisRegOut = idxReg;
				SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), fn.castTo<MemberNode>(), fullIdRef));

				idxReg;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxReg));

				SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));

				if (extraFnArgs) {
					idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(extraFnArgs->paramTypes.size()))
						return genOutOfRuntimeMemoryCompError();

					for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes->at(i)));
					}

					if (extraFnArgs->hasVarArgs)
						idRefObject->hasVarArgs = true;
				}

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
			}
		} else {
			// Is calling a static method.
			IdRefPtr fullIdRef;
			SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), fn.castTo<MemberNode>(), fullIdRef));

			slake::HostObjectRef<slake::IdRefObject> idRefObject;
			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));

			if (extraFnArgs) {
				idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

				if (!idRefObject->paramTypes->resize(extraFnArgs->paramTypes.size()))
					return genOutOfRuntimeMemoryCompError();

				for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes->at(i)));
				}

				if (extraFnArgs->hasVarArgs)
					idRefObject->hasVarArgs = true;
			}

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
		}
	} else {
		slake::HostObjectRef<slake::IdRefObject> idRefObject;

		if (parts.size() > 1) {
			for (size_t i = curIdx; i < parts.size(); ++i) {
				ResolvedIdRefPart &part = parts.at(i);

				SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

				if (part.isStatic) {
					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
				} else {
					uint32_t idxNewReg;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxNewReg));

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::RLOAD, idxNewReg, { slake::Value(slake::ValueType::RegRef, idxReg), slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
					idxReg = idxNewReg;
				}

				curIdx += part.nEntries;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MOV, resultRegOut, { slake::Value(slake::ValueType::RegRef, idxReg) }));
	return {};
};
auto selectSingleMatchingOverloading = [](CompileContext *compileContext, const TokenRange &tokenRange, peff::SharedPtr<MemberNode> &finalMember, peff::SharedPtr<TypeNameNode> desiredType, bool isStatic, CompileExprResult &resultOut) -> std::optional<CompilationError> {
	peff::SharedPtr<FnNode> m = finalMember.castTo<FnNode>();

	resultOut.callTargetFnSlot = m;

	if (m->overloadings.size() == 1) {
		finalMember = m->overloadings.back().castTo<MemberNode>();
		if (!resultOut.callTargetMatchedOverloadings.pushBack(peff::SharedPtr<FnOverloadingNode>(m->overloadings.back()))) {
			return genOutOfMemoryCompError();
		}
	} else {
		if (desiredType && (desiredType->typeNameKind == TypeNameKind::Fn)) {
			peff::SharedPtr<FnTypeNameNode> tn = desiredType.castTo<FnTypeNameNode>();
			peff::DynArray<peff::SharedPtr<FnOverloadingNode>> matchedOverloadingIndices(compileContext->allocator.get());

			// TODO: Check tn->isForAdl and do strictly equality check.
			SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext, m, tn->paramTypes.data(), tn->paramTypes.size(), isStatic, matchedOverloadingIndices));

			switch (matchedOverloadingIndices.size()) {
				case 0:
					return CompilationError(tokenRange, CompilationErrorKind::NoMatchingFnOverloading);
				case 1:
					break;
				default:
					return CompilationError(tokenRange, CompilationErrorKind::UnableToDetermineOverloading);
			}

			finalMember = m->overloadings.at(matchedOverloadingIndices.back()).castTo<MemberNode>();

			resultOut.callTargetMatchedOverloadings = std::move(matchedOverloadingIndices);
		} else {
			return CompilationError(tokenRange, CompilationErrorKind::UnableToDetermineOverloading);
		}
	}

	return {};
};

static std::optional<CompilationError> _determineNodeType(CompileContext *compileContext, peff::SharedPtr<MemberNode> node, peff::SharedPtr<TypeNameNode> &typeNameOut) {
	switch (node->astNodeType) {
		case AstNodeType::This: {
			auto m = node.castTo<ThisNode>()->thisType;

			IdRefPtr fullIdRef;

			SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), m, fullIdRef));

			auto tn = peff::makeSharedWithControlBlock<CustomTypeNameNode, AstNodeControlBlock<CustomTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document);

			if (!tn) {
				return genOutOfMemoryCompError();
			}
			tn->contextNode = compileContext->document->rootModule.castTo<MemberNode>();

			tn->idRefPtr = std::move(fullIdRef);

			typeNameOut = tn.castTo<TypeNameNode>();
			break;
		}
		case AstNodeType::Var: {
			auto originalType = node.castTo<VarNode>()->type;
			if (originalType->typeNameKind != TypeNameKind::Ref) {
				peff::SharedPtr<RefTypeNameNode> t;

				if (!(t = peff::makeSharedWithControlBlock<RefTypeNameNode, AstNodeControlBlock<RefTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, peff::SharedPtr<TypeNameNode>()))) {
					return genOutOfMemoryCompError();
				}
				t->referencedType = node.castTo<VarNode>()->type;
				typeNameOut = t.castTo<TypeNameNode>();
			} else {
				typeNameOut = originalType;
			}
			break;
		}
		case AstNodeType::Fn: {
			peff::SharedPtr<FnTypeNameNode> tn;
			SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileContext, node.castTo<FnOverloadingNode>(), tn));
			typeNameOut = tn.castTo<TypeNameNode>();
			break;
		}
		case AstNodeType::FnSlot:
			typeNameOut = {};
			break;
		case AstNodeType::Module:
		case AstNodeType::Class:
		case AstNodeType::Struct:
		case AstNodeType::Enum:
		case AstNodeType::Interface:
			typeNameOut = {};
			break;
		default:
			std::terminate();
	}

	SLKC_RETURN_IF_COMP_ERROR(simplifyType(typeNameOut, typeNameOut));

	return {};
};

SLKC_API std::optional<CompilationError> slkc::compileExpr(
	CompileContext *compileContext,
	CompilationContext *compilationContext,
	const peff::SharedPtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	peff::SharedPtr<TypeNameNode> desiredType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	switch (expr->exprKind) {
		case ExprKind::Unary:
			SLKC_RETURN_IF_COMP_ERROR(compileUnaryExpr(compileContext, compilationContext, expr.castTo<UnaryExprNode>(), evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Binary:
			SLKC_RETURN_IF_COMP_ERROR(compileBinaryExpr(compileContext, compilationContext, expr.castTo<BinaryExprNode>(), evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::HeadedIdRef: {
			peff::SharedPtr<HeadedIdRefExprNode> e = expr.castTo<HeadedIdRefExprNode>();

			CompileExprResult result(compileContext->allocator.get());

			uint32_t headRegister;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(headRegister));

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->head, ExprEvalPurpose::RValue, {}, headRegister, result));

			peff::SharedPtr<MemberNode> initialMember;

			peff::SharedPtr<TypeNameNode> tn = result.evaluatedType;

		determineInitialMember:
			switch (tn->typeNameKind) {
				case TypeNameKind::Void:
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::U64:
				case TypeNameKind::F32:
				case TypeNameKind::F64:
				case TypeNameKind::ISize:
				case TypeNameKind::USize:
				case TypeNameKind::String:
				case TypeNameKind::Bool:
				case TypeNameKind::Any:
				case TypeNameKind::Object:
					return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
				case TypeNameKind::Custom:
					SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(tn->document->sharedFromThis(), tn.castTo<CustomTypeNameNode>(), initialMember));
					break;
				case TypeNameKind::Fn:
				case TypeNameKind::Array:
					return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
				case TypeNameKind::Ref:
					tn = tn.castTo<RefTypeNameNode>()->referencedType;

					goto determineInitialMember;
				case TypeNameKind::TempRef:
					tn = tn.castTo<TempRefTypeNameNode>()->referencedType;

					goto determineInitialMember;
				case TypeNameKind::Bad:
					break;
				default:
					std::terminate();
			}

			if (!initialMember) {
				return CompilationError(e->head->tokenRange, CompilationErrorKind::IdNotFound);
			}

			peff::SharedPtr<MemberNode> finalMember;

			uint32_t finalRegister;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(finalRegister));

			ResolvedIdRefPartList parts(compileContext->document->allocator.get());
			{
				ResolvedIdRefPart initialPart;

				initialPart.isStatic = false;
				initialPart.nEntries = 1;
				initialPart.member = initialMember;

				if (!parts.pushBack(std::move(initialPart))) {
					return genOutOfMemoryCompError();
				}

				peff::Set<peff::SharedPtr<MemberNode>> walkedMemberNodes(compileContext->document->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(
					resolveIdRefWithScopeNode(
						compileContext,
						compileContext->document,
						walkedMemberNodes,
						initialMember,
						e->idRefPtr->entries.data(),
						e->idRefPtr->entries.size(),
						finalMember,
						&parts,
						false));

				if (!finalMember) {
					return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
				}
			}

			if (!finalMember) {
				return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
			}

			if (finalMember->astNodeType == AstNodeType::FnSlot) {
				SLKC_RETURN_IF_COMP_ERROR(selectSingleMatchingOverloading(compileContext, e->idRefPtr->tokenRange, finalMember, desiredType, false, resultOut));

				peff::SharedPtr<FnTypeNameNode> fnType;
				SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileContext, finalMember.castTo<FnOverloadingNode>(), fnType));

				parts.back().member = finalMember;

				SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileContext, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, headRegister, 0, fnType));
			} else {
				SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileContext, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, headRegister, 0, {}));
			}

			SLKC_RETURN_IF_COMP_ERROR(_determineNodeType(compileContext, finalMember, resultOut.evaluatedType));

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					if (!resultOut.evaluatedType) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::ExpectingId);
					}
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if (!b) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					}

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							slake::Opcode::MOV,
							resultRegOut,
							{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					break;
				}
				case ExprEvalPurpose::RValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if ((b) && (initialMember != finalMember)) {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::LVALUE,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					}
					break;
				}
				case ExprEvalPurpose::Call: {
					peff::SharedPtr<TypeNameNode> decayedTargetType;

					SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(resultOut.evaluatedType, decayedTargetType));

					if (decayedTargetType->typeNameKind != TypeNameKind::Fn) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if ((b) && (initialMember != finalMember)) {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::LVALUE,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					}
					break;
				}
				case ExprEvalPurpose::Unpacking:
					return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
					break;
				default:
					std::terminate();
			}

			break;
		}
		case ExprKind::IdRef: {
			peff::SharedPtr<IdRefExprNode> e = expr.castTo<IdRefExprNode>();

			peff::SharedPtr<MemberNode> initialMember, finalMember;
			IdRefEntry &initialEntry = e->idRefPtr->entries.at(0);
			ExprEvalPurpose initialMemberEvalPurpose;

			uint32_t initialMemberReg;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(initialMemberReg));
			bool isStatic = false;

			uint32_t finalRegister;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(finalRegister));

			if (!initialEntry.genericArgs.size()) {
				if (e->idRefPtr->entries.at(0).name == "this") {
					if (!compileContext->thisNode)
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::InvalidThisUsage);

					initialMember = compileContext->thisNode.castTo<MemberNode>();

					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::EvalType:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
								CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue:
							return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LTHIS, initialMemberReg, {}));
							break;
						}
						case ExprEvalPurpose::Unpacking:
							return CompilationError(e->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
					}
					goto initialMemberResolved;
				}

				// Check if the entry refers to a local variable.
				if (auto it = compilationContext->lookupLocalVar(e->idRefPtr->entries.at(0).name);
					it) {
					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					initialMember = it.castTo<MemberNode>();

					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::EvalType:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
								CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue: {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MOV, initialMemberReg, { slake::Value(slake::ValueType::RegRef, it->idxReg) }));
							break;
						}
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LVALUE, initialMemberReg, { slake::Value(slake::ValueType::RegRef, it->idxReg) }));
							break;
						}
						case ExprEvalPurpose::Unpacking:
							return CompilationError(e->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
					}
					goto initialMemberResolved;
				}

				// Check if the entry refers to a function parameter.
				if (auto it = compileContext->curOverloading->paramIndices.find(e->idRefPtr->entries.at(0).name);
					it != compileContext->curOverloading->paramIndices.end()) {
					initialMember = compileContext->curOverloading->params.at(it.value()).castTo<MemberNode>();

					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::EvalType:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
								CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
							break;
						case ExprEvalPurpose::LValue: {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LARG, initialMemberReg, { slake::Value((uint32_t)it.value()) }));
							break;
						}
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							uint32_t tmpReg;

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpReg));

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LARG, tmpReg, { slake::Value((uint32_t)it.value()) }));
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LVALUE, initialMemberReg, { slake::Value(slake::ValueType::RegRef, tmpReg) }));

							break;
						}
						case ExprEvalPurpose::Unpacking:
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LAPARG, initialMemberReg, { slake::Value((uint32_t)it.value()) }));

							break;
					}
					goto initialMemberResolved;
				}
			}

		initialMemberResolved:

			if (initialMember) {
				if (e->idRefPtr->entries.size() > 1) {
					size_t curIdx = 1;

					ResolvedIdRefPartList parts(compileContext->document->allocator.get());
					{
						ResolvedIdRefPart initialPart;

						initialPart.isStatic = false;
						initialPart.nEntries = 1;
						initialPart.member = initialMember;

						if (!parts.pushBack(std::move(initialPart))) {
							return genOutOfMemoryCompError();
						}

						peff::Set<peff::SharedPtr<MemberNode>> walkedMemberNodes(compileContext->document->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(
							resolveIdRefWithScopeNode(
								compileContext,
								compileContext->document,
								walkedMemberNodes,
								initialMember,
								e->idRefPtr->entries.data() + 1,
								e->idRefPtr->entries.size() - 1,
								finalMember,
								&parts,
								false));

						if (!finalMember) {
							return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
						}
					}

					if (finalMember->astNodeType == AstNodeType::FnSlot) {
						SLKC_RETURN_IF_COMP_ERROR(selectSingleMatchingOverloading(compileContext, e->idRefPtr->tokenRange, finalMember, desiredType, false, resultOut));

						peff::SharedPtr<FnTypeNameNode> fnType;
						SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileContext, finalMember.castTo<FnOverloadingNode>(), fnType));

						parts.back().member = finalMember;

						SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileContext, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, initialMemberReg, 1, fnType));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileContext, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, initialMemberReg, 1, {}));
					}
				} else {
					finalMember = initialMember;

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							slake::Opcode::MOV,
							finalRegister,
							{ slake::Value(slake::ValueType::RegRef, initialMemberReg) }));
				}
			} else {
				size_t curIdx = 0;

				ResolvedIdRefPartList parts(compileContext->document->allocator.get());
				{
					peff::Set<peff::SharedPtr<MemberNode>> walkedMemberNodes(compileContext->document->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(
						resolveIdRefWithScopeNode(
							compileContext,
							compileContext->document,
							walkedMemberNodes,
							compileContext->curOverloading.castTo<MemberNode>(),
							e->idRefPtr->entries.data(),
							e->idRefPtr->entries.size(),
							finalMember,
							&parts));
				}

				if (!finalMember) {
					return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
				}

				if (finalMember->astNodeType == AstNodeType::FnSlot) {
					SLKC_RETURN_IF_COMP_ERROR(selectSingleMatchingOverloading(compileContext, e->idRefPtr->tokenRange, finalMember, desiredType, false, resultOut));

					peff::SharedPtr<FnTypeNameNode> fnType;
					SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileContext, finalMember.castTo<FnOverloadingNode>(), fnType));

					parts.back().member = finalMember;

					SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileContext, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, UINT32_MAX, 0, fnType));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileContext, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, UINT32_MAX, 0, {}));
				}
			}

			SLKC_RETURN_IF_COMP_ERROR(_determineNodeType(compileContext, finalMember, resultOut.evaluatedType));

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					if (!resultOut.evaluatedType) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::ExpectingId);
					}
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if (!b) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					}

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							slake::Opcode::MOV,
							resultRegOut,
							{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					break;
				}
				case ExprEvalPurpose::RValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if ((b) && (initialMember != finalMember)) {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::LVALUE,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					}
					break;
				}
				case ExprEvalPurpose::Call: {
					peff::SharedPtr<TypeNameNode> decayedTargetType;

					SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(resultOut.evaluatedType, decayedTargetType));

					if (decayedTargetType->typeNameKind != TypeNameKind::Fn) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if ((b) && (initialMember != finalMember)) {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::LVALUE,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					}
					break;
				}
				case ExprEvalPurpose::Unpacking: {
					peff::SharedPtr<TypeNameNode> decayedTargetType;

					SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(resultOut.evaluatedType, decayedTargetType));

					peff::SharedPtr<TypeNameNode> unpackedType;

					SLKC_RETURN_IF_COMP_ERROR(getUnpackedTypeOf(decayedTargetType, unpackedType));

					if (!unpackedType) {
						return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotUnpackable);
					}

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							slake::Opcode::MOV,
							resultRegOut,
							{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					break;
				}
				default:
					std::terminate();
			}

			break;
		}
		case ExprKind::I8:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I8LiteralExprNode, int8_t, I8TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I16:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I16LiteralExprNode, int16_t, I16TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I32LiteralExprNode, int32_t, I32TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I64LiteralExprNode, int64_t, I64TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U8:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U8LiteralExprNode, uint8_t, U8TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U16:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U16LiteralExprNode, uint16_t, U16TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U32LiteralExprNode, uint32_t, U32TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U64LiteralExprNode, uint64_t, U64TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::F32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<F32LiteralExprNode, float, F32TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::F64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<F64LiteralExprNode, double, F64TypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Bool:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<BoolLiteralExprNode, bool, BoolTypeNameNode>(compileContext, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::String: {
			peff::SharedPtr<StringLiteralExprNode> e = expr.castTo<StringLiteralExprNode>();

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					slake::HostObjectRef<slake::StringObject> sl;

					{
						if (!(sl = slake::StringObject::alloc(compileContext->runtime))) {
							return genOutOfRuntimeMemoryCompError();
						}

						if (!sl->data.build(e->data)) {
							return genOutOfRuntimeMemoryCompError();
						}
					}

					if (resultRegOut != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(slake::EntityRef::makeObjectRef(sl.get())) }));
					}
					break;
				}
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					break;
				case ExprEvalPurpose::Call:
					return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
					break;
				default:
					std::terminate();
			}
			if (!(resultOut.evaluatedType = peff::makeSharedWithControlBlock<StringTypeNameNode, AstNodeControlBlock<StringTypeNameNode>>(
					  compileContext->allocator.get(),
					  compileContext->allocator.get(),
					  compileContext->document)
						.castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}
			break;
		}
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
							compilationContext->emitIns(
								slake::Opcode::MOV,
								resultRegOut,
								{ slake::Value(slake::EntityRef::makeObjectRef(nullptr)) }));
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

			if (!(resultOut.evaluatedType = peff::makeSharedWithControlBlock<ObjectTypeNameNode, AstNodeControlBlock<ObjectTypeNameNode>>(
					  compileContext->allocator.get(),
					  compileContext->allocator.get(),
					  compileContext->document)
						.castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}
			break;
		}
		case ExprKind::InitializerList: {
			peff::SharedPtr<InitializerListExprNode> e = expr.castTo<InitializerListExprNode>();

			peff::SharedPtr<TypeNameNode> tn;
			if (!desiredType) {
				for (auto i : e->elements) {
					peff::SharedPtr<TypeNameNode> t;
					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, i, t));

					if (t) {
						if (tn) {
							SLKC_RETURN_IF_COMP_ERROR(determinePromotionalType(t, tn, tn));
						} else {
							tn = t;
						}
					}
				}

				if (!tn) {
					return CompilationError(expr->tokenRange, CompilationErrorKind::ErrorDeducingInitializerListType);
				}

				if (!(resultOut.evaluatedType = peff::makeSharedWithControlBlock<ArrayTypeNameNode, AstNodeControlBlock<ArrayTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, tn).castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}
			} else {
				switch (desiredType->typeNameKind) {
					case TypeNameKind::Array:
						tn = desiredType.castTo<ArrayTypeNameNode>()->elementType;
						break;
					default:
						return CompilationError(expr->tokenRange, CompilationErrorKind::InvalidInitializerListUsage);
				}

				if (!(resultOut.evaluatedType = desiredType)) {
					return genOutOfMemoryCompError();
				}
			}

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					if (resultRegOut != UINT32_MAX) {
						slake::Type elementType;

						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, tn, elementType));
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::ARRNEW,
								resultRegOut,
								{ slake::Value(elementType), slake::Value((uint32_t)e->elements.size()) }));

						for (size_t i = 0; i < e->elements.size(); ++i) {
							uint32_t curElementSlotRegIndex;

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(curElementSlotRegIndex));

							uint32_t curElementRegIndex;

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(curElementRegIndex));

							SLKC_RETURN_IF_COMP_ERROR(
								compilationContext->emitIns(
									slake::Opcode::AT,
									curElementSlotRegIndex,
									{ slake::Value(slake::ValueType::RegRef, resultRegOut), slake::Value((uint32_t)i) }));

							CompileExprResult result(compileContext->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->elements.at(i), ExprEvalPurpose::RValue, tn.castTo<TypeNameNode>(), curElementRegIndex, result));

							SLKC_RETURN_IF_COMP_ERROR(
								compilationContext->emitIns(
									slake::Opcode::STORE,
									UINT32_MAX,
									{ slake::Value(slake::ValueType::RegRef, curElementSlotRegIndex), slake::Value(slake::ValueType::RegRef, curElementRegIndex) }));
						}
					} else {
						// Just simply evaluate each element.
						for (size_t i = 0; i < e->elements.size(); ++i) {
							CompileExprResult result(compileContext->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->elements.at(i), ExprEvalPurpose::RValue, tn.castTo<TypeNameNode>(), UINT32_MAX, result));
						}
					}
					break;
				}
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
				case ExprEvalPurpose::Call:
					return CompilationError(expr->tokenRange, CompilationErrorKind::TargetIsNotCallable);
				default:
					std::terminate();
			}

			break;
		}
		case ExprKind::Call: {
			peff::SharedPtr<CallExprNode> e = expr.castTo<CallExprNode>();

			uint32_t targetReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(targetReg));

			peff::DynArray<peff::SharedPtr<TypeNameNode>> argTypes(compileContext->allocator.get());

			if (!argTypes.resize(e->args.size())) {
				return genOutOfMemoryCompError();
			}

			CompileExprResult result(compileContext->allocator.get());
			peff::SharedPtr<TypeNameNode> fnType;
			if (auto error = evalExprType(compileContext, compilationContext, e->target, fnType); error) {
				switch (error->errorKind) {
					case CompilationErrorKind::OutOfMemory:
					case CompilationErrorKind::OutOfRuntimeMemory:
						return error;
					default:
						break;
				}

				for (size_t i = 0, j = 0; i < e->args.size(); ++i, ++j) {
					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->args.at(i), argTypes.at(j)));

					if (argTypes.at(j)->typeNameKind == TypeNameKind::UnpackedParams) {
						peff::SharedPtr<UnpackedParamsTypeNameNode> t = argTypes.at(i).castTo<UnpackedParamsTypeNameNode>();

						if (!argTypes.resize(argTypes.size() + t->paramTypes.size() - 1)) {
							return genOutOfMemoryCompError();
						}

						--j;
						for (size_t k = 0; k < t->paramTypes.size(); ++k, ++j) {
							argTypes.at(j) = t->paramTypes.at(k);
						}

						if (t->hasVarArgs) {
							// Varidic arguments must be the trailing one.
							if (i + 1 != e->args.size()) {
								return CompilationError(expr->tokenRange, CompilationErrorKind::CannotBeUnpackedInThisContext);
							}
						}
					}
				}

				peff::SharedPtr<FnTypeNameNode> fnPrototype;

				if (!(fnPrototype = peff::makeSharedWithControlBlock<FnTypeNameNode, AstNodeControlBlock<FnTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
					return genOutOfMemoryCompError();
				}

				fnPrototype->paramTypes = std::move(argTypes);
				fnPrototype->isForAdl = true;

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->target, ExprEvalPurpose::Call, fnPrototype.castTo<TypeNameNode>(), targetReg, result));

				argTypes = std::move(fnPrototype->paramTypes);
				fnType = result.evaluatedType;
			} else {
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->target, ExprEvalPurpose::Call, {}, targetReg, result));

				fnType = result.evaluatedType;

				peff::SharedPtr<FnTypeNameNode> tn = fnType.castTo<FnTypeNameNode>();

				for (size_t i = 0, j = 0; i < e->args.size(); ++i, ++j) {
					if (i < tn->paramTypes.size()) {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->args.at(i), argTypes.at(j), tn->paramTypes.at(i)));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->args.at(i), argTypes.at(j), {}));
					}

					if (argTypes.at(j)->typeNameKind == TypeNameKind::UnpackedParams) {
						peff::SharedPtr<UnpackedParamsTypeNameNode> t = argTypes.at(i).castTo<UnpackedParamsTypeNameNode>();

						if (!argTypes.resize(argTypes.size() + t->paramTypes.size() - 1)) {
							return genOutOfMemoryCompError();
						}

						--j;
						for (size_t k = 0; k < t->paramTypes.size(); ++k, ++j) {
							argTypes.at(j) = t->paramTypes.at(k);
						}

						if (t->hasVarArgs) {
							// Varidic arguments must be the trailing one.
							if (i + 1 != e->args.size()) {
								return CompilationError(expr->tokenRange, CompilationErrorKind::CannotBeUnpackedInThisContext);
							}
						}
					}

					assert(argTypes.at(j));
				}
			}

			if (result.callTargetFnSlot) {
				peff::DynArray<peff::SharedPtr<FnOverloadingNode>> matchedOverloadingIndices(compileContext->allocator.get());
				auto matchedOverloading = result.callTargetMatchedOverloadings.back();
				SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext,
					result.callTargetFnSlot,
					argTypes.data(),
					argTypes.size(),
					matchedOverloading->accessModifier & slake::ACCESS_STATIC,
					matchedOverloadingIndices));
				if (!matchedOverloadingIndices.size()) {
					return CompilationError(e->tokenRange, CompilationErrorKind::ArgsMismatched);
				}
			}

			auto tn = fnType.castTo<FnTypeNameNode>();

			for (size_t i = 0; i < e->args.size(); ++i) {
				CompileExprResult argResult(compileContext->allocator.get());

				uint32_t reg;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

				if (i < tn->paramTypes.size()) {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(tn->paramTypes.at(i), b));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, tn->paramTypes.at(i), reg, argResult));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->args.at(i), ExprEvalPurpose::RValue, {}, reg, argResult));
				}

				switch (argResult.evaluatedType->typeNameKind) {
					case TypeNameKind::UnpackedParams:
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::PUSHAP,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegRef, reg) }));
						break;
					default:
						SLKC_RETURN_IF_COMP_ERROR(
							compilationContext->emitIns(
								slake::Opcode::PUSHARG,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegRef, reg) }));
				}
			}

			uint32_t thisReg = UINT32_MAX;

			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(fnType, fnType));

			if (fnType.castTo<FnTypeNameNode>()->thisType) {
				if (result.idxThisRegOut == UINT32_MAX) {
					if (!e->withObject) {
						return CompilationError(expr->tokenRange, CompilationErrorKind::MissingBindingObject);
					}

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(thisReg));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->withObject, ExprEvalPurpose::RValue, fnType.castTo<FnTypeNameNode>()->thisType, thisReg, result));
				} else {
					thisReg = result.idxThisRegOut;
				}
			} else {
				if (e->withObject) {
					return CompilationError(expr->tokenRange, CompilationErrorKind::RedundantWithObject);
				}
			}

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					break;
				case ExprEvalPurpose::Stmt:
					if (thisReg != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, targetReg) }));
					}
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(fnType.castTo<FnTypeNameNode>()->returnType, b));
					if (!b) {
						return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					}

					if (thisReg != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg) }));
					}
					break;
				}
				case ExprEvalPurpose::RValue:
				case ExprEvalPurpose::Call: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(fnType.castTo<FnTypeNameNode>()->returnType, b));

					if (b) {
						uint32_t tmpRegIndex;

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(tmpRegIndex));

						if (result.idxThisRegOut != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, tmpRegIndex, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CALL, tmpRegIndex, { slake::Value(slake::ValueType::RegRef, targetReg) }));
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LVALUE, resultRegOut, { slake::Value(slake::ValueType::RegRef, tmpRegIndex) }));
					} else {
						if (result.idxThisRegOut != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg) }));
						}
					}

					break;
				}
			}

			if (auto rt = fnType.castTo<FnTypeNameNode>()->returnType; rt) {
				resultOut.evaluatedType = fnType.castTo<FnTypeNameNode>()->returnType;
			} else {
				if (!(resultOut.evaluatedType = peff::makeSharedWithControlBlock<VoidTypeNameNode, AstNodeControlBlock<VoidTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document).castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}
			}
			break;
		}
		case ExprKind::New: {
			peff::SharedPtr<NewExprNode> e = expr.castTo<NewExprNode>();

			uint32_t targetReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(targetReg));

			if (e->targetType->typeNameKind != TypeNameKind::Custom) {
				return CompilationError(e->targetType->tokenRange, CompilationErrorKind::TypeIsNotConstructible);
			}

			peff::SharedPtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileContext->document, e->targetType.castTo<CustomTypeNameNode>(), m));

			if (m->astNodeType != AstNodeType::Class) {
				return CompilationError(e->targetType->tokenRange, CompilationErrorKind::TypeIsNotConstructible);
			}

			peff::SharedPtr<ClassNode> c = m.castTo<ClassNode>();

			slake::Type type;
			{
				IdRefPtr fullIdRef;
				SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), c.castTo<MemberNode>(), fullIdRef));

				auto tn = peff::makeSharedWithControlBlock<CustomTypeNameNode, AstNodeControlBlock<CustomTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document);

				if (!tn) {
					return genOutOfMemoryCompError();
				}
				tn->contextNode = compileContext->document->rootModule.castTo<MemberNode>();

				tn->idRefPtr = std::move(fullIdRef);

				tn->tokenRange = e->targetType->tokenRange;

				auto e = compileTypeName(compileContext, tn.castTo<TypeNameNode>(), type);
				/* if (e) {
					std::terminate();
				}*/
			}
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::NEW, resultRegOut, { slake::Value(type) }));

			if (auto it = c->memberIndices.find("new"); it != c->memberIndices.end()) {
				if (c->members.at(it.value())->astNodeType != AstNodeType::FnSlot) {
					return CompilationError(e->targetType->tokenRange, CompilationErrorKind::TypeIsNotConstructible);
				}
				peff::SharedPtr<FnNode> constructor = c->members.at(it.value()).castTo<FnNode>();

				peff::DynArray<peff::SharedPtr<TypeNameNode>> argTypes(compileContext->allocator.get());

				if (!argTypes.resize(e->args.size())) {
					return genOutOfMemoryCompError();
				}

				peff::SharedPtr<FnOverloadingNode> overloading;
				if (constructor->overloadings.size() == 1) {
					overloading = constructor->overloadings.back();

					for (size_t i = 0; i < e->args.size(); ++i) {
						if (i < overloading->params.size()) {
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->args.at(i), argTypes.at(i), overloading->params.at(i)->type));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->args.at(i), argTypes.at(i), {}));
						}
					}

					peff::DynArray<peff::SharedPtr<FnOverloadingNode>> matchedOverloadingIndices(compileContext->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext,
						constructor,
						argTypes.data(),
						argTypes.size(),
						false,
						matchedOverloadingIndices));
					if (!matchedOverloadingIndices.size()) {
						return CompilationError(e->tokenRange, CompilationErrorKind::ArgsMismatched);
					}
				} else {
					for (size_t i = 0; i < e->args.size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->args.at(i), argTypes.at(i), {}));
					}

					peff::DynArray<peff::SharedPtr<FnOverloadingNode>> matchedOverloadingIndices(compileContext->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileContext,
						constructor,
						argTypes.data(),
						argTypes.size(),
						false,
						matchedOverloadingIndices));
					switch (matchedOverloadingIndices.size()) {
						case 0:
							return CompilationError(e->tokenRange, CompilationErrorKind::NoMatchingFnOverloading);
						case 1:
							break;
						default:
							return CompilationError(e->tokenRange, CompilationErrorKind::UnableToDetermineOverloading);
					}

					overloading = matchedOverloadingIndices.back();
				}

				slake::HostObjectRef<slake::IdRefObject> idRefObject;
				{
					IdRefPtr fullIdRef;
					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), constructor.castTo<MemberNode>(), fullIdRef));

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));

					idRefObject->paramTypes = peff::DynArray<slake::Type>(compileContext->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(overloading->params.size()))
						return genOutOfRuntimeMemoryCompError();

					for (size_t i = 0; i < overloading->params.size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, overloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
					}

					if (overloading->fnFlags & FN_VARG)
						idRefObject->hasVarArgs = true;
				}

				uint32_t ctorCallTarget;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(ctorCallTarget));

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, ctorCallTarget, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));

				for (size_t i = 0; i < e->args.size(); ++i) {
					CompileExprResult argResult(compileContext->allocator.get());

					uint32_t reg;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

					if (i < overloading->params.size()) {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(overloading->params.at(i)->type, b));

						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, overloading->params.at(i)->type, reg, argResult));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->args.at(i), ExprEvalPurpose::RValue, {}, reg, argResult));
					}

					SLKC_RETURN_IF_COMP_ERROR(
						compilationContext->emitIns(
							slake::Opcode::PUSHARG,
							UINT32_MAX,
							{ slake::Value(slake::ValueType::RegRef, reg) }));
				}

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CTORCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, ctorCallTarget), slake::Value(slake::ValueType::RegRef, resultRegOut) }));
			} else {
			}

			resultOut.evaluatedType = e->targetType;
			break;
		}
		case ExprKind::Cast: {
			peff::SharedPtr<CastExprNode> e = expr.castTo<CastExprNode>();

			peff::SharedPtr<TypeNameNode> exprType;

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, e->source, exprType, e->targetType));

			bool b;
			SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(exprType, e->targetType, false, b));

			if (!b) {
				return CompilationError(e->source->tokenRange, CompilationErrorKind::InvalidCast);
			}

			bool leftValue;

			SLKC_RETURN_IF_COMP_ERROR(isLValueType(e->targetType, leftValue));

			peff::SharedPtr<TypeNameNode> decayedExprType;

			bool exprLeftValue;

			SLKC_RETURN_IF_COMP_ERROR(isLValueType(exprType, exprLeftValue));

			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(exprType, decayedExprType));

			bool sameType;
			SLKC_RETURN_IF_COMP_ERROR(isSameType(decayedExprType, e->targetType, sameType));
			if (!sameType) {
				uint32_t idxReg;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxReg));

				CompileExprResult result(compileContext->allocator.get());
				if (!leftValue) {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->source, ExprEvalPurpose::RValue, {}, idxReg, result));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->source, ExprEvalPurpose::LValue, {}, idxReg, result));
				}

				slake::Type type;
				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, e->targetType, type));

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CAST, resultRegOut, { slake::Value(type), slake::Value(slake::ValueType::RegRef, idxReg) }));
			} else {
				CompileExprResult result(compileContext->allocator.get());
				if (!leftValue) {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->source, ExprEvalPurpose::RValue, {}, resultRegOut, result));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->source, ExprEvalPurpose::LValue, {}, resultRegOut, result));
				}
			}

			resultOut.evaluatedType = e->targetType;
			break;
		}
		case ExprKind::Match: {
			peff::SharedPtr<MatchExprNode> e = expr.castTo<MatchExprNode>();

			peff::SharedPtr<TypeNameNode> returnType = e->returnType;

			if (!returnType) {
				if (desiredType)
					returnType = desiredType;
				else {
					peff::SharedPtr<TypeNameNode> mostPromotionalType, t;

					for (auto &i : e->cases) {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, i.second, t));

						if (t) {
							if (mostPromotionalType) {
								SLKC_RETURN_IF_COMP_ERROR(determinePromotionalType(t, mostPromotionalType, mostPromotionalType));
							} else {
								mostPromotionalType = t;
							}
						}
					}

					if (!mostPromotionalType)
						return CompilationError(expr->tokenRange, CompilationErrorKind::ErrorDeducingMatchResultType);
				}
			}

			bool isLValue;

			SLKC_RETURN_IF_COMP_ERROR(isLValueType(returnType, isLValue));

			switch (evalPurpose) {
				case ExprEvalPurpose::LValue:
					if (!isLValue)
						return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					break;
				default:
					break;
			}

			uint32_t conditionReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(conditionReg));

			CompileExprResult result(compileContext->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, e->condition, ExprEvalPurpose::RValue, {}, conditionReg, result));

			if (!result.evaluatedType)
				return CompilationError(expr->tokenRange, CompilationErrorKind::ErrorDeducingMatchConditionType);

			peff::SharedPtr<TypeNameNode> conditionType = result.evaluatedType;

			uint32_t endLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(endLabel));

			uint32_t defaultLabel;

			bool isDefaultSet = false;

			if (e->isConst) {
				// Key = jump source, value = value register
				peff::DynArray<uint32_t> matchValueEvalLabels(compileContext->allocator.get());
				peff::DynArray<std::pair<uint32_t, uint32_t>> phiRegisterValueMap(compileContext->allocator.get());

				if (!matchValueEvalLabels.resize(e->cases.size())) {
					return genOutOfMemoryCompError();
				}

				if (!phiRegisterValueMap.resize(e->cases.size())) {
					return genOutOfMemoryCompError();
				}

				size_t idxDefaultBranchCase;

				for (size_t i = 0; i < e->cases.size(); ++i) {
					auto &curCase = e->cases.at(i);

					uint32_t evalValueLabel;
					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(evalValueLabel));

					if (curCase.first) {
						peff::SharedPtr<ExprNode> resultExpr;

						SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileContext, compilationContext, curCase.first, resultExpr));

						if (!resultExpr) {
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::ErrorEvaluatingConstMatchCaseCondition);
						}

						peff::SharedPtr<TypeNameNode> resultExprType;

						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, compilationContext, resultExpr, resultExprType));

						if (!resultExprType) {
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::MismatchedMatchCaseConditionType);
						}

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(isSameType(conditionType, resultExprType, b));

						if (!b)
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::MismatchedMatchCaseConditionType);

						peff::SharedPtr<BinaryExprNode> cmpExpr;

						if (!(cmpExpr = peff::makeSharedWithControlBlock<BinaryExprNode, AstNodeControlBlock<BinaryExprNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->binaryOp = BinaryOp::Eq;

						cmpExpr->tokenRange = curCase.first->tokenRange;

						if (!(cmpExpr->lhs = peff::makeSharedWithControlBlock<RegRefExprNode, AstNodeControlBlock<RegRefExprNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, conditionReg, conditionType).castTo<ExprNode>())) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->rhs = curCase.first;

						peff::SharedPtr<BoolTypeNameNode> boolTypeName;

						if (!(boolTypeName = peff::makeSharedWithControlBlock<BoolTypeNameNode, AstNodeControlBlock<BoolTypeNameNode>>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document)))
							return genOutOfMemoryCompError();

						uint32_t cmpResultReg;
						{
							CompileExprResult cmpExprResult(compileContext->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(cmpResultReg));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, cmpExpr.castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.castTo<TypeNameNode>(), cmpResultReg, cmpExprResult));
						}

						SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JT, UINT32_MAX, { slake::Value(slake::ValueType::Label, evalValueLabel), slake::Value(slake::ValueType::RegRef, cmpResultReg) }));
					} else {
						if (isDefaultSet)
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::DuplicatedMatchCaseBranch);

						defaultLabel = evalValueLabel;

						idxDefaultBranchCase = i;

						isDefaultSet = true;
					}

					matchValueEvalLabels.at(i) = evalValueLabel;
				}

				if (!isDefaultSet)
					return CompilationError(e->tokenRange, CompilationErrorKind::MissingDefaultMatchCaseBranch);

				for (size_t i = 0; i < e->cases.size(); ++i) {
					if (defaultLabel == matchValueEvalLabels.at(i))
						continue;

					auto &curCase = e->cases.at(i);

					compilationContext->setLabelOffset(matchValueEvalLabels.at(i), compilationContext->getCurInsOff());

					uint32_t exprValueRegister;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(exprValueRegister));

					CompileExprResult resultExprResult(compileContext->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, curCase.second, evalPurpose, returnType, exprValueRegister, result));

					phiRegisterValueMap.at(i) = { compilationContext->getCurInsOff(), exprValueRegister };

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, endLabel) }));
				}

				{
					auto &defaultCase = e->cases.at(idxDefaultBranchCase);

					compilationContext->setLabelOffset(matchValueEvalLabels.at(idxDefaultBranchCase), compilationContext->getCurInsOff());

					uint32_t exprValueRegister;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(exprValueRegister));

					CompileExprResult resultExprResult(compileContext->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, compilationContext, defaultCase.second, evalPurpose, returnType, exprValueRegister, result));

					phiRegisterValueMap.at(idxDefaultBranchCase) = { UINT32_MAX, exprValueRegister };

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, endLabel) }));
				}

				compilationContext->setLabelOffset(endLabel, compilationContext->getCurInsOff());

				peff::DynArray<slake::Value> operands(compileContext->allocator.get());

				if (!operands.resize(phiRegisterValueMap.size() * 2)) {
					return genOutOfMemoryCompError();
				}

				for (size_t i = 0, j = 0; i < phiRegisterValueMap.size(); ++i, j += 2) {
					operands.at(j) = phiRegisterValueMap.at(i).first;
					operands.at(j + 1) = slake::Value(slake::ValueType::RegRef, phiRegisterValueMap.at(i).second);
				}

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::PHI, resultRegOut, operands.data(), operands.size()));
			} else {
			}

			resultOut.evaluatedType = returnType;
			break;
		}
		case ExprKind::RegRef: {
			auto e = expr.castTo<RegRefExprNode>();

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::MOV, resultRegOut, { slake::Value(slake::ValueType::RegRef, e->reg) }));

			resultOut.evaluatedType = e->type;

			break;
		}
		case ExprKind::Wrapper:
			return compileExpr(compileContext, compilationContext, expr.castTo<WrapperExprNode>()->target, evalPurpose, desiredType, resultRegOut, resultOut);
		default:
			std::terminate();
	}

	return {};
}
