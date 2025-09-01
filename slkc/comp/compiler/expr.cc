#include "../compiler.h"

using namespace slkc;

template <typename LE, typename DT, typename TN>
SLAKE_FORCEINLINE std::optional<CompilationError> _compileLiteralExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	const AstNodePtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	peff::SharedPtr<LE> e = expr.castTo<LE>();

	switch (evalPurpose) {
		case ExprEvalPurpose::EvalType:
			break;
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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
			  compileEnv->allocator.get(),
			  compileEnv->allocator.get(),
			  compileEnv->document)
				.template castTo<TypeNameNode>())) {
		return genOutOfMemoryCompError();
	}
	return {};
}

SLKC_API std::optional<CompilationError> slkc::_compileOrCastOperand(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	uint32_t regOut,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> desiredType,
	AstNodePtr<ExprNode> operand,
	AstNodePtr<TypeNameNode> operandType) {
	bool whether;
	SLKC_RETURN_IF_COMP_ERROR(isSameType(desiredType, operandType, whether));
	if (whether) {
		CompileExprResult result(compileEnv->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, operand, evalPurpose, desiredType, regOut, result));
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(operandType, desiredType, true, whether));
	if (whether) {
		CompileExprResult result(compileEnv->allocator.get());

		AstNodePtr<CastExprNode> castExpr;

		if (!(castExpr = makeAstNode<CastExprNode>(
				  compileEnv->allocator.get(),
				  compileEnv->allocator.get(),
				  compileEnv->document))) {
			return genOutOfMemoryCompError();
		}

		castExpr->targetType = desiredType;
		castExpr->source = operand;

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, castExpr.castTo<ExprNode>(), evalPurpose, desiredType, regOut, result));
		return {};
	}

	IncompatibleOperandErrorExData exData;
	exData.desiredType = desiredType;

	return CompilationError(operand->tokenRange, std::move(exData));
}

static std::optional<CompilationError> _loadTheRestOfIdRef(CompileEnvironment *compileEnv, CompilationContext *compilationContext, uint32_t resultRegOut, CompileExprResult &resultOut, IdRef *idRef, ResolvedIdRefPartList &parts, uint32_t initialMemberReg, size_t curIdx, AstNodePtr<FnTypeNameNode> extraFnArgs) {
	uint32_t idxReg;

	if (initialMemberReg == UINT32_MAX) {
		SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxReg));
	} else {
		idxReg = initialMemberReg;
	}

	if (parts.back().member->astNodeType == AstNodeType::Fn) {
		AstNodePtr<FnOverloadingNode> fn = parts.back().member.castTo<FnOverloadingNode>();

		if (parts.size() > 1) {
			if (fn->fnFlags & FN_VIRTUAL) {
				slake::HostObjectRef<slake::IdRefObject> idRefObject;
				// Is calling a virtual instance method.
				for (size_t i = curIdx; i < parts.size() - 1; ++i) {
					ResolvedIdRefPart &part = parts.at(i);

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

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
					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

					if (extraFnArgs) {
						idRefObject->paramTypes = peff::DynArray<slake::Type>(compileEnv->runtime->getCurGenAlloc());

						if (!idRefObject->paramTypes->resize(extraFnArgs->paramTypes.size()))
							return genOutOfRuntimeMemoryCompError();

						for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
							SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes->at(i)));
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

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

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
				SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), fn.castTo<MemberNode>(), fullIdRef));

				idxReg;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxReg));

				SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));

				if (extraFnArgs) {
					idRefObject->paramTypes = peff::DynArray<slake::Type>(compileEnv->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(extraFnArgs->paramTypes.size()))
						return genOutOfRuntimeMemoryCompError();

					for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes->at(i)));
					}

					if (extraFnArgs->hasVarArgs)
						idRefObject->hasVarArgs = true;
				}

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));
			}
		} else {
			// Is calling a static method.
			IdRefPtr fullIdRef;
			SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), fn.castTo<MemberNode>(), fullIdRef));

			slake::HostObjectRef<slake::IdRefObject> idRefObject;
			SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));

			if (extraFnArgs) {
				idRefObject->paramTypes = peff::DynArray<slake::Type>(compileEnv->runtime->getCurGenAlloc());

				if (!idRefObject->paramTypes->resize(extraFnArgs->paramTypes.size()))
					return genOutOfRuntimeMemoryCompError();

				for (size_t i = 0; i < idRefObject->paramTypes->size(); ++i) {
					SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes->at(i)));
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

				SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

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
auto selectSingleMatchingOverloading = [](CompileEnvironment *compileEnv, const TokenRange &tokenRange, AstNodePtr<MemberNode> &finalMember, AstNodePtr<TypeNameNode> desiredType, bool isStatic, CompileExprResult &resultOut) -> std::optional<CompilationError> {
	AstNodePtr<FnNode> m = finalMember.castTo<FnNode>();

	resultOut.callTargetFnSlot = m;

	if (m->overloadings.size() == 1) {
		finalMember = m->overloadings.back().castTo<MemberNode>();
		if (!resultOut.callTargetMatchedOverloadings.pushBack(AstNodePtr<FnOverloadingNode>(m->overloadings.back()))) {
			return genOutOfMemoryCompError();
		}
	} else {
		if (desiredType && (desiredType->typeNameKind == TypeNameKind::Fn)) {
			AstNodePtr<FnTypeNameNode> tn = desiredType.castTo<FnTypeNameNode>();
			peff::DynArray<AstNodePtr<FnOverloadingNode>> matchedOverloadingIndices(compileEnv->allocator.get());

			// TODO: Check tn->isForAdl and do strictly equality check.
			SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv, m, tn->paramTypes.data(), tn->paramTypes.size(), isStatic, matchedOverloadingIndices));

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

static std::optional<CompilationError> _determineNodeType(CompileEnvironment *compileEnv, AstNodePtr<MemberNode> node, AstNodePtr<TypeNameNode> &typeNameOut) {
	switch (node->astNodeType) {
		case AstNodeType::This: {
			auto m = node.castTo<ThisNode>()->thisType;

			IdRefPtr fullIdRef;

			SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), m, fullIdRef));

			auto tn = makeAstNode<CustomTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document);

			if (!tn) {
				return genOutOfMemoryCompError();
			}
			tn->contextNode = compileEnv->document->rootModule.castTo<MemberNode>();

			tn->idRefPtr = std::move(fullIdRef);

			typeNameOut = tn.castTo<TypeNameNode>();
			break;
		}
		case AstNodeType::Var: {
			auto originalType = node.castTo<VarNode>()->type;

			AstNodePtr<TypeNameNode> unpackedTypeNameNode;

			SLKC_RETURN_IF_COMP_ERROR(getUnpackedTypeOf(originalType, unpackedTypeNameNode));

			if ((originalType->typeNameKind != TypeNameKind::Ref) && (!unpackedTypeNameNode)) {
				AstNodePtr<RefTypeNameNode> t;

				if (!(t = makeAstNode<RefTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, AstNodePtr<TypeNameNode>()))) {
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
			AstNodePtr<FnTypeNameNode> tn;
			SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileEnv, node.castTo<FnOverloadingNode>(), tn));
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

	SLKC_RETURN_IF_COMP_ERROR(simplifyParamListTypeNameTree(typeNameOut, compileEnv->allocator.get(), typeNameOut));

	return {};
};

SLKC_API std::optional<CompilationError> slkc::compileExpr(
	CompileEnvironment *compileEnv,
	CompilationContext *compilationContext,
	const AstNodePtr<ExprNode> &expr,
	ExprEvalPurpose evalPurpose,
	AstNodePtr<TypeNameNode> desiredType,
	uint32_t resultRegOut,
	CompileExprResult &resultOut) {
	switch (expr->exprKind) {
		case ExprKind::Unary:
			SLKC_RETURN_IF_COMP_ERROR(compileUnaryExpr(compileEnv, compilationContext, expr.castTo<UnaryExprNode>(), evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Binary:
			SLKC_RETURN_IF_COMP_ERROR(compileBinaryExpr(compileEnv, compilationContext, expr.castTo<BinaryExprNode>(), evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::HeadedIdRef: {
			AstNodePtr<HeadedIdRefExprNode> e = expr.castTo<HeadedIdRefExprNode>();

			CompileExprResult result(compileEnv->allocator.get());

			uint32_t headRegister;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(headRegister));

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->head, ExprEvalPurpose::RValue, {}, headRegister, result));

			AstNodePtr<MemberNode> initialMember;

			AstNodePtr<TypeNameNode> tn = result.evaluatedType;

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

			AstNodePtr<MemberNode> finalMember;

			uint32_t finalRegister;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(finalRegister));

			ResolvedIdRefPartList parts(compileEnv->document->allocator.get());
			{
				ResolvedIdRefPart initialPart;

				initialPart.isStatic = false;
				initialPart.nEntries = 1;
				initialPart.member = initialMember;

				if (!parts.pushBack(std::move(initialPart))) {
					return genOutOfMemoryCompError();
				}

				peff::Set<AstNodePtr<MemberNode>> walkedMemberNodes(compileEnv->document->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(
					resolveIdRefWithScopeNode(
						compileEnv,
						compileEnv->document,
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
				SLKC_RETURN_IF_COMP_ERROR(selectSingleMatchingOverloading(compileEnv, e->idRefPtr->tokenRange, finalMember, desiredType, false, resultOut));

				AstNodePtr<FnTypeNameNode> fnType;
				SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileEnv, finalMember.castTo<FnOverloadingNode>(), fnType));

				parts.back().member = finalMember;

				SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileEnv, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, headRegister, 0, fnType));
			} else {
				SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileEnv, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, headRegister, 0, {}));
			}

			SLKC_RETURN_IF_COMP_ERROR(_determineNodeType(compileEnv, finalMember, resultOut.evaluatedType));

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					if (!resultOut.evaluatedType) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::ExpectingId);
					}
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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
					AstNodePtr<TypeNameNode> decayedTargetType;

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
			AstNodePtr<IdRefExprNode> e = expr.castTo<IdRefExprNode>();

			AstNodePtr<MemberNode> initialMember, finalMember;
			IdRefEntry &initialEntry = e->idRefPtr->entries.at(0);
			ExprEvalPurpose initialMemberEvalPurpose;

			uint32_t initialMemberReg;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(initialMemberReg));
			bool isStatic = false;

			uint32_t finalRegister;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(finalRegister));

			if (!initialEntry.genericArgs.size()) {
				if (e->idRefPtr->entries.at(0).name == "this") {
					if (!compileEnv->thisNode)
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::InvalidThisUsage);

					initialMember = compileEnv->thisNode.castTo<MemberNode>();

					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::EvalType:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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
				if (auto it = compileEnv->curOverloading->paramIndices.find(e->idRefPtr->entries.at(0).name);
					it != compileEnv->curOverloading->paramIndices.end()) {
					initialMember = compileEnv->curOverloading->params.at(it.value()).castTo<MemberNode>();

					if (e->idRefPtr->entries.size() > 1) {
						initialMemberEvalPurpose = ExprEvalPurpose::RValue;
					} else {
						initialMemberEvalPurpose = evalPurpose;
					}

					switch (initialMemberEvalPurpose) {
						case ExprEvalPurpose::EvalType:
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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

					ResolvedIdRefPartList parts(compileEnv->document->allocator.get());
					{
						ResolvedIdRefPart initialPart;

						initialPart.isStatic = false;
						initialPart.nEntries = 1;
						initialPart.member = initialMember;

						if (!parts.pushBack(std::move(initialPart))) {
							return genOutOfMemoryCompError();
						}

						peff::Set<AstNodePtr<MemberNode>> walkedMemberNodes(compileEnv->document->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(
							resolveIdRefWithScopeNode(
								compileEnv,
								compileEnv->document,
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
						SLKC_RETURN_IF_COMP_ERROR(selectSingleMatchingOverloading(compileEnv, e->idRefPtr->tokenRange, finalMember, desiredType, false, resultOut));

						AstNodePtr<FnTypeNameNode> fnType;
						SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileEnv, finalMember.castTo<FnOverloadingNode>(), fnType));

						parts.back().member = finalMember;

						SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileEnv, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, initialMemberReg, 1, fnType));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileEnv, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, initialMemberReg, 1, {}));
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

				ResolvedIdRefPartList parts(compileEnv->document->allocator.get());
				{
					peff::Set<AstNodePtr<MemberNode>> walkedMemberNodes(compileEnv->document->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(
						resolveIdRefWithScopeNode(
							compileEnv,
							compileEnv->document,
							walkedMemberNodes,
							compileEnv->curOverloading.castTo<MemberNode>(),
							e->idRefPtr->entries.data(),
							e->idRefPtr->entries.size(),
							finalMember,
							&parts));
				}

				if (!finalMember) {
					return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::IdNotFound);
				}

				if (finalMember->astNodeType == AstNodeType::FnSlot) {
					SLKC_RETURN_IF_COMP_ERROR(selectSingleMatchingOverloading(compileEnv, e->idRefPtr->tokenRange, finalMember, desiredType, false, resultOut));

					AstNodePtr<FnTypeNameNode> fnType;
					SLKC_RETURN_IF_COMP_ERROR(fnToTypeName(compileEnv, finalMember.castTo<FnOverloadingNode>(), fnType));

					parts.back().member = finalMember;

					SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileEnv, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, UINT32_MAX, 0, fnType));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(_loadTheRestOfIdRef(compileEnv, compilationContext, finalRegister, resultOut, e->idRefPtr.get(), parts, UINT32_MAX, 0, {}));
				}
			}

			SLKC_RETURN_IF_COMP_ERROR(_determineNodeType(compileEnv, finalMember, resultOut.evaluatedType));

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					if (!resultOut.evaluatedType) {
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::ExpectingId);
					}
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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
					AstNodePtr<TypeNameNode> decayedTargetType;

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
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I8LiteralExprNode, int8_t, I8TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I16:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I16LiteralExprNode, int16_t, I16TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I32LiteralExprNode, int32_t, I32TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::I64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<I64LiteralExprNode, int64_t, I64TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U8:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U8LiteralExprNode, uint8_t, U8TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U16:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U16LiteralExprNode, uint16_t, U16TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U32LiteralExprNode, uint32_t, U32TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::U64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<U64LiteralExprNode, uint64_t, U64TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::F32:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<F32LiteralExprNode, float, F32TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::F64:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<F64LiteralExprNode, double, F64TypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::Bool:
			SLKC_RETURN_IF_COMP_ERROR(_compileLiteralExpr<BoolLiteralExprNode, bool, BoolTypeNameNode>(compileEnv, compilationContext, expr, evalPurpose, resultRegOut, resultOut));
			break;
		case ExprKind::String: {
			AstNodePtr<StringLiteralExprNode> e = expr.castTo<StringLiteralExprNode>();

			switch (evalPurpose) {
				case ExprEvalPurpose::EvalType:
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					slake::HostObjectRef<slake::StringObject> sl;

					{
						if (!(sl = slake::StringObject::alloc(compileEnv->runtime))) {
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
			if (!(resultOut.evaluatedType = makeAstNode<StringTypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(),
					  compileEnv->document)
						.castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}
			break;
		}
		case ExprKind::Null: {
			AstNodePtr<NullLiteralExprNode> e = expr.castTo<NullLiteralExprNode>();

			switch (evalPurpose) {
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
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

			if (!(resultOut.evaluatedType = makeAstNode<ObjectTypeNameNode>(
					  compileEnv->allocator.get(),
					  compileEnv->allocator.get(),
					  compileEnv->document)
						.castTo<TypeNameNode>())) {
				return genOutOfMemoryCompError();
			}
			break;
		}
		case ExprKind::InitializerList: {
			AstNodePtr<InitializerListExprNode> e = expr.castTo<InitializerListExprNode>();

			AstNodePtr<TypeNameNode> tn;
			if (!desiredType) {
				for (auto i : e->elements) {
					AstNodePtr<TypeNameNode> t;
					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, i, t));

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

				if (!(resultOut.evaluatedType = makeAstNode<ArrayTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, tn).castTo<TypeNameNode>())) {
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
					SLKC_RETURN_IF_COMP_ERROR(compileEnv->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					if (resultRegOut != UINT32_MAX) {
						slake::Type elementType;

						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, tn, elementType));
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

							CompileExprResult result(compileEnv->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->elements.at(i), ExprEvalPurpose::RValue, tn.castTo<TypeNameNode>(), curElementRegIndex, result));

							SLKC_RETURN_IF_COMP_ERROR(
								compilationContext->emitIns(
									slake::Opcode::STORE,
									UINT32_MAX,
									{ slake::Value(slake::ValueType::RegRef, curElementSlotRegIndex), slake::Value(slake::ValueType::RegRef, curElementRegIndex) }));
						}
					} else {
						// Just simply evaluate each element.
						for (size_t i = 0; i < e->elements.size(); ++i) {
							CompileExprResult result(compileEnv->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->elements.at(i), ExprEvalPurpose::RValue, tn.castTo<TypeNameNode>(), UINT32_MAX, result));
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
			AstNodePtr<CallExprNode> e = expr.castTo<CallExprNode>();

			uint32_t targetReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(targetReg));

			peff::DynArray<AstNodePtr<TypeNameNode>> argTypes(compileEnv->allocator.get());

			if (!argTypes.resize(e->args.size())) {
				return genOutOfMemoryCompError();
			}

			CompileExprResult result(compileEnv->allocator.get());
			AstNodePtr<TypeNameNode> fnType;
			if (auto error = evalExprType(compileEnv, compilationContext, e->target, fnType); error) {
				switch (error->errorKind) {
					case CompilationErrorKind::OutOfMemory:
					case CompilationErrorKind::OutOfRuntimeMemory:
						return error;
					default:
						break;
				}

				for (size_t i = 0, j = 0; i < e->args.size(); ++i, ++j) {
					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->args.at(i), argTypes.at(j)));

					if (argTypes.at(j)->typeNameKind == TypeNameKind::UnpackedArgs) {
						AstNodePtr<UnpackedArgsTypeNameNode> t = argTypes.at(i).castTo<UnpackedArgsTypeNameNode>();

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

				AstNodePtr<FnTypeNameNode> fnPrototype;

				if (!(fnPrototype = makeAstNode<FnTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
					return genOutOfMemoryCompError();
				}

				fnPrototype->paramTypes = std::move(argTypes);
				fnPrototype->isForAdl = true;

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->target, ExprEvalPurpose::Call, fnPrototype.castTo<TypeNameNode>(), targetReg, result));

				argTypes = std::move(fnPrototype->paramTypes);
				fnType = result.evaluatedType;
			} else {
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->target, ExprEvalPurpose::Call, {}, targetReg, result));

				fnType = result.evaluatedType;

				AstNodePtr<FnTypeNameNode> tn = fnType.castTo<FnTypeNameNode>();

				for (size_t i = 0, j = 0; i < e->args.size(); ++i, ++j) {
					if (i < tn->paramTypes.size()) {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->args.at(i), argTypes.at(j), tn->paramTypes.at(i)));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->args.at(i), argTypes.at(j), {}));
					}

					if (!argTypes.at(j)) {
						return CompilationError(e->args.at(i)->tokenRange, CompilationErrorKind::ErrorDeducingArgType);
					}

					//SLKC_RETURN_IF_COMP_ERROR(simplifyParamListTypeNameTree(argTypes.at(j), compileEnv->allocator.get(), argTypes.at(j)));

					if (argTypes.at(j)->typeNameKind == TypeNameKind::UnpackedArgs) {
						AstNodePtr<UnpackedArgsTypeNameNode> t = argTypes.at(i).castTo<UnpackedArgsTypeNameNode>();

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
				peff::DynArray<AstNodePtr<FnOverloadingNode>> matchedOverloadingIndices(compileEnv->allocator.get());
				auto matchedOverloading = result.callTargetMatchedOverloadings.back();
				SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv,
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
			 	CompileExprResult argResult(compileEnv->allocator.get());

				uint32_t reg;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

				if (i < tn->paramTypes.size()) {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(tn->paramTypes.at(i), b));

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, tn->paramTypes.at(i), reg, argResult));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->args.at(i), ExprEvalPurpose::RValue, {}, reg, argResult));
				}

				AstNodePtr<TypeNameNode> argType = argResult.evaluatedType;

				//SLKC_RETURN_IF_COMP_ERROR(simplifyParamListTypeNameTree(argResult.evaluatedType, compileEnv->allocator.get(), argType));

				switch (argType->typeNameKind) {
					case TypeNameKind::UnpackedArgs:
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

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->withObject, ExprEvalPurpose::RValue, fnType.castTo<FnTypeNameNode>()->thisType, thisReg, result));
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
				if (!(resultOut.evaluatedType = makeAstNode<VoidTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document).castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}
			}
			break;
		}
		case ExprKind::New: {
			AstNodePtr<NewExprNode> e = expr.castTo<NewExprNode>();

			uint32_t targetReg;

			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(targetReg));

			if (e->targetType->typeNameKind != TypeNameKind::Custom) {
				return CompilationError(e->targetType->tokenRange, CompilationErrorKind::TypeIsNotConstructible);
			}

			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolveCustomTypeName(compileEnv->document, e->targetType.castTo<CustomTypeNameNode>(), m));

			if (m->astNodeType != AstNodeType::Class) {
				return CompilationError(e->targetType->tokenRange, CompilationErrorKind::TypeIsNotConstructible);
			}

			AstNodePtr<ClassNode> c = m.castTo<ClassNode>();

			slake::Type type;
			{
				IdRefPtr fullIdRef;
				SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), c.castTo<MemberNode>(), fullIdRef));

				auto tn = makeAstNode<CustomTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document);

				if (!tn) {
					return genOutOfMemoryCompError();
				}
				tn->contextNode = compileEnv->document->rootModule.castTo<MemberNode>();

				tn->idRefPtr = std::move(fullIdRef);

				tn->tokenRange = e->targetType->tokenRange;

				auto e = compileTypeName(compileEnv, compilationContext, tn.castTo<TypeNameNode>(), type);
				/* if (e) {
					std::terminate();
				}*/
			}
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::NEW, resultRegOut, { slake::Value(type) }));

			if (auto it = c->memberIndices.find("new"); it != c->memberIndices.end()) {
				if (c->members.at(it.value())->astNodeType != AstNodeType::FnSlot) {
					return CompilationError(e->targetType->tokenRange, CompilationErrorKind::TypeIsNotConstructible);
				}
				AstNodePtr<FnNode> constructor = c->members.at(it.value()).castTo<FnNode>();

				peff::DynArray<AstNodePtr<TypeNameNode>> argTypes(compileEnv->allocator.get());

				if (!argTypes.resize(e->args.size())) {
					return genOutOfMemoryCompError();
				}

				AstNodePtr<FnOverloadingNode> overloading;
				if (constructor->overloadings.size() == 1) {
					overloading = constructor->overloadings.back();

					for (size_t i = 0; i < e->args.size(); ++i) {
						if (i < overloading->params.size()) {
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->args.at(i), argTypes.at(i), overloading->params.at(i)->type));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->args.at(i), argTypes.at(i), {}));
						}
					}

					peff::DynArray<AstNodePtr<FnOverloadingNode>> matchedOverloadingIndices(compileEnv->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv,
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
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->args.at(i), argTypes.at(i), {}));
					}

					peff::DynArray<AstNodePtr<FnOverloadingNode>> matchedOverloadingIndices(compileEnv->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(determineFnOverloading(compileEnv,
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
					SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileEnv->allocator.get(), constructor.castTo<MemberNode>(), fullIdRef));

					SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileEnv, compilationContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));

					idRefObject->paramTypes = peff::DynArray<slake::Type>(compileEnv->runtime->getCurGenAlloc());

					if (!idRefObject->paramTypes->resize(overloading->params.size()))
						return genOutOfRuntimeMemoryCompError();

					for (size_t i = 0; i < overloading->params.size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, overloading->params.at(i)->type, idRefObject->paramTypes->at(i)));
					}

					if (overloading->fnFlags & FN_VARG)
						idRefObject->hasVarArgs = true;
				}

				uint32_t ctorCallTarget;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(ctorCallTarget));

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::LOAD, ctorCallTarget, { slake::Value(slake::EntityRef::makeObjectRef(idRefObject.get())) }));

				for (size_t i = 0; i < e->args.size(); ++i) {
					CompileExprResult argResult(compileEnv->allocator.get());

					uint32_t reg;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(reg));

					if (i < overloading->params.size()) {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR(isLValueType(overloading->params.at(i)->type, b));

						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, overloading->params.at(i)->type, reg, argResult));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->args.at(i), ExprEvalPurpose::RValue, {}, reg, argResult));
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
			AstNodePtr<CastExprNode> e = expr.castTo<CastExprNode>();

			AstNodePtr<TypeNameNode> exprType, targetType;

			SLKC_RETURN_IF_COMP_ERROR(simplifyGenericParamFacadeTypeNameTree(e->targetType, compileEnv->allocator.get(), targetType));

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, e->source, exprType, targetType));

			bool b;
			SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(exprType, targetType, false, b));

			if (!b) {
				return CompilationError(e->source->tokenRange, CompilationErrorKind::InvalidCast);
			}

			bool leftValue;

			SLKC_RETURN_IF_COMP_ERROR(isLValueType(targetType, leftValue));

			AstNodePtr<TypeNameNode> decayedExprType;

			bool exprLeftValue;

			SLKC_RETURN_IF_COMP_ERROR(isLValueType(exprType, exprLeftValue));

			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(exprType, decayedExprType));

			bool sameType;
			SLKC_RETURN_IF_COMP_ERROR(isSameType(decayedExprType, targetType, sameType));
			if (!sameType) {
				uint32_t idxReg;

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(idxReg));

				CompileExprResult result(compileEnv->allocator.get());
				if (!leftValue) {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->source, ExprEvalPurpose::RValue, {}, idxReg, result));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->source, ExprEvalPurpose::LValue, {}, idxReg, result));
				}

				slake::Type type;
				SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileEnv, compilationContext, targetType, type));

				SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::CAST, resultRegOut, { slake::Value(type), slake::Value(slake::ValueType::RegRef, idxReg) }));
			} else {
				CompileExprResult result(compileEnv->allocator.get());
				if (!leftValue) {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->source, ExprEvalPurpose::RValue, {}, resultRegOut, result));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->source, ExprEvalPurpose::LValue, {}, resultRegOut, result));
				}
			}

			resultOut.evaluatedType = targetType;
			break;
		}
		case ExprKind::Match: {
			AstNodePtr<MatchExprNode> e = expr.castTo<MatchExprNode>();

			AstNodePtr<TypeNameNode> returnType;

			SLKC_RETURN_IF_COMP_ERROR(simplifyGenericParamFacadeTypeNameTree(e->returnType, compileEnv->allocator.get(), returnType));

			if (!returnType) {
				if (desiredType)
					returnType = desiredType;
				else {
					AstNodePtr<TypeNameNode> mostPromotionalType, t;

					for (auto &i : e->cases) {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, i.second, t));

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

			CompileExprResult result(compileEnv->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, e->condition, ExprEvalPurpose::RValue, {}, conditionReg, result));

			if (!result.evaluatedType)
				return CompilationError(expr->tokenRange, CompilationErrorKind::ErrorDeducingMatchConditionType);

			AstNodePtr<TypeNameNode> conditionType = result.evaluatedType;

			uint32_t endLabel;
			SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocLabel(endLabel));

			uint32_t defaultLabel;

			bool isDefaultSet = false;

			if (e->isConst) {
				// Key = jump source, value = value register
				peff::DynArray<uint32_t> matchValueEvalLabels(compileEnv->allocator.get());
				peff::DynArray<std::pair<uint32_t, uint32_t>> phiRegisterValueMap(compileEnv->allocator.get());

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
						AstNodePtr<ExprNode> resultExpr;

						SLKC_RETURN_IF_COMP_ERROR(evalConstExpr(compileEnv, compilationContext, curCase.first, resultExpr));

						if (!resultExpr) {
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::ErrorEvaluatingConstMatchCaseCondition);
						}

						AstNodePtr<TypeNameNode> resultExprType;

						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileEnv, compilationContext, resultExpr, resultExprType));

						if (!resultExprType) {
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::MismatchedMatchCaseConditionType);
						}

						bool b;

						SLKC_RETURN_IF_COMP_ERROR(isSameType(conditionType, resultExprType, b));

						if (!b)
							return CompilationError(curCase.first->tokenRange, CompilationErrorKind::MismatchedMatchCaseConditionType);

						AstNodePtr<BinaryExprNode> cmpExpr;

						if (!(cmpExpr = makeAstNode<BinaryExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document))) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->binaryOp = BinaryOp::Eq;

						cmpExpr->tokenRange = curCase.first->tokenRange;

						if (!(cmpExpr->lhs = makeAstNode<RegRefExprNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document, conditionReg, conditionType).castTo<ExprNode>())) {
							return genOutOfMemoryCompError();
						}

						cmpExpr->rhs = curCase.first;

						AstNodePtr<BoolTypeNameNode> boolTypeName;

						if (!(boolTypeName = makeAstNode<BoolTypeNameNode>(compileEnv->allocator.get(), compileEnv->allocator.get(), compileEnv->document)))
							return genOutOfMemoryCompError();

						uint32_t cmpResultReg;
						{
							CompileExprResult cmpExprResult(compileEnv->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(cmpResultReg));

							SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, cmpExpr.castTo<ExprNode>(), ExprEvalPurpose::RValue, boolTypeName.castTo<TypeNameNode>(), cmpResultReg, cmpExprResult));
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

					CompileExprResult resultExprResult(compileEnv->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, curCase.second, evalPurpose, returnType, exprValueRegister, result));

					phiRegisterValueMap.at(i) = { compilationContext->getCurInsOff(), exprValueRegister };

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, endLabel) }));
				}

				{
					auto &defaultCase = e->cases.at(idxDefaultBranchCase);

					compilationContext->setLabelOffset(matchValueEvalLabels.at(idxDefaultBranchCase), compilationContext->getCurInsOff());

					uint32_t exprValueRegister;

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->allocReg(exprValueRegister));

					CompileExprResult resultExprResult(compileEnv->allocator.get());

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileEnv, compilationContext, defaultCase.second, evalPurpose, returnType, exprValueRegister, result));

					phiRegisterValueMap.at(idxDefaultBranchCase) = { UINT32_MAX, exprValueRegister };

					SLKC_RETURN_IF_COMP_ERROR(compilationContext->emitIns(slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, endLabel) }));
				}

				compilationContext->setLabelOffset(endLabel, compilationContext->getCurInsOff());

				peff::DynArray<slake::Value> operands(compileEnv->allocator.get());

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
			return compileExpr(compileEnv, compilationContext, expr.castTo<WrapperExprNode>()->target, evalPurpose, desiredType, resultRegOut, resultOut);
		default:
			std::terminate();
	}

	SLKC_RETURN_IF_COMP_ERROR(simplifyGenericParamFacadeTypeNameTree(resultOut.evaluatedType, compileEnv->allocator.get(), resultOut.evaluatedType));

	return {};
}
