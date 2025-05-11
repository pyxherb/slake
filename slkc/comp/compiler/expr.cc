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
		case ExprEvalPurpose::EvalType:
			break;
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
				.template castTo<TypeNameNode>())) {
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
		CompileExprResult result(compileContext->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, operand, evalPurpose, desiredType, regOut, result));
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(operandType, desiredType, true, whether));
	if (whether) {
		CompileExprResult result(compileContext->allocator.get());

		peff::SharedPtr<CastExprNode> castExpr;

		if (!(castExpr = peff::makeShared<CastExprNode>(
				  compileContext->allocator.get(),
				  compileContext->allocator.get(),
				  compileContext->document,
				  desiredType,
				  operand))) {
			return genOutOfMemoryCompError();
		}

		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, castExpr.castTo<ExprNode>(), evalPurpose, desiredType, regOut, result));
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
	peff::SharedPtr<TypeNameNode> desiredType,
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
			bool isStatic = false;

			uint32_t finalRegister = compileContext->allocReg();

			if (!initialEntry.genericArgs.size()) {
				if (e->idRefPtr->entries.at(0).name == "this") {
					if (!compileContext->fnCompileContext.thisNode)
						return CompilationError(e->idRefPtr->tokenRange, CompilationErrorKind::InvalidThisUsage);

					initialMember = compileContext->fnCompileContext.thisNode.castTo<MemberNode>();

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
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LTHIS, initialMemberReg, {}));
							break;
						}
					}
					goto initialMemberResolved;
				}

				// Check if the entry refers to a local variable.
				for (auto i = compileContext->fnCompileContext.blockCompileContexts.beginReversed(); i != compileContext->fnCompileContext.blockCompileContexts.endReversed(); ++i) {
					if (auto it = (*i)->localVars.find(e->idRefPtr->entries.at(0).name);
						it != (*i)->localVars.end()) {
						if (e->idRefPtr->entries.size() > 1) {
							initialMemberEvalPurpose = ExprEvalPurpose::RValue;
						} else {
							initialMemberEvalPurpose = evalPurpose;
						}

						initialMember = it.value().castTo<MemberNode>();
						switch (initialMemberEvalPurpose) {
							case ExprEvalPurpose::EvalType:
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
						case ExprEvalPurpose::EvalType:
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
			auto loadTheRest = [](CompileContext *compileContext, uint32_t resultRegOut, CompileExprResult &resultOut, IdRef *idRef, ResolvedIdRefPartList &parts, size_t curIdx, peff::SharedPtr<FnTypeNameNode> extraFnArgs) -> std::optional<CompilationError> {
				uint32_t idxReg = compileContext->allocReg();

				if (parts.back().member->astNodeType == AstNodeType::Fn) {
					peff::SharedPtr<FnOverloadingNode> fn = parts.back().member.castTo<FnOverloadingNode>();

					if (parts.size() > 1) {
						if (!(fn->fnFlags & FN_VIRTUAL)) {
							slake::HostObjectRef<slake::IdRefObject> idRefObject;
							// Is calling a virtual instance method.
							for (size_t i = 0; i < parts.size() - 1; ++i) {
								ResolvedIdRefPart &part = parts.at(i);

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

							{
								ResolvedIdRefPart &part = parts.back();
								SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, part.nEntries, nullptr, 0, false, idRefObject));

								resultOut.idxThisRegOut = idxReg;
								uint32_t idxNewReg = compileContext->allocReg();
								SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::RLOAD, idxNewReg, { slake::Value(slake::ValueType::RegRef, idxReg), slake::Value(slake::ValueType::EntityRef, slake::EntityRef::makeObjectRef(idRefObject.get())) }));
								idxReg = idxNewReg;
							}

							if (extraFnArgs) {
								if (!idRefObject->paramTypes.resize(extraFnArgs->paramTypes.size()))
									return genOutOfRuntimeMemoryCompError();

								for (size_t i = 0; i < idRefObject->paramTypes.size(); ++i) {
									SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes.at(i)));
								}

								if (extraFnArgs->hasVarArgs)
									idRefObject->hasVarArgs = true;
							}
						} else {
							// Is calling an instance method that is not virtual.
							slake::HostObjectRef<slake::IdRefObject> idRefObject;
							IdRefPtr fullIdRef;

							for (size_t i = 0; i < parts.size() - 1; ++i) {
								ResolvedIdRefPart &part = parts.at(i);

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

							resultOut.idxThisRegOut = idxReg;
							SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), fn.castTo<MemberNode>(), fullIdRef));

							idxReg = compileContext->allocReg();
							SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, fullIdRef->entries.data(), fullIdRef->entries.size(), nullptr, 0, false, idRefObject));
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::ValueType::EntityRef, slake::EntityRef::makeObjectRef(idRefObject.get())) }));

							if (extraFnArgs) {
								if (!idRefObject->paramTypes.resize(extraFnArgs->paramTypes.size()))
									return genOutOfRuntimeMemoryCompError();

								for (size_t i = 0; i < idRefObject->paramTypes.size(); ++i) {
									SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes.at(i)));
								}

								if (extraFnArgs->hasVarArgs)
									idRefObject->hasVarArgs = true;
							}
						}
					} else {
						// Is calling a static method.
						slake::HostObjectRef<slake::IdRefObject> idRefObject;
						SLKC_RETURN_IF_COMP_ERROR(compileIdRef(compileContext, idRef->entries.data() + curIdx, parts.back().nEntries, nullptr, 0, false, idRefObject));

						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LOAD, idxReg, { slake::Value(slake::ValueType::EntityRef, slake::EntityRef::makeObjectRef(idRefObject.get())) }));

						if (extraFnArgs) {
							if (!idRefObject->paramTypes.resize(extraFnArgs->paramTypes.size()))
								return genOutOfRuntimeMemoryCompError();

							for (size_t i = 0; i < idRefObject->paramTypes.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, extraFnArgs->paramTypes.at(i), idRefObject->paramTypes.at(i)));
							}

							if (extraFnArgs->hasVarArgs)
								idRefObject->hasVarArgs = true;
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MOV, resultRegOut, { slake::Value(slake::ValueType::RegRef, idxReg) }));
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

			auto determineNodeType = [](CompileContext *compileContext, peff::SharedPtr<MemberNode> node, peff::SharedPtr<TypeNameNode> &typeNameOut) -> std::optional<CompilationError> {
				switch (node->astNodeType) {
					case AstNodeType::This: {
						auto m = node.castTo<ThisNode>()->thisType;

						IdRefPtr fullIdRef;

						SLKC_RETURN_IF_COMP_ERROR(getFullIdRef(compileContext->allocator.get(), m, fullIdRef));

						auto tn = peff::makeShared<CustomTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document);

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

							if (!(t = peff::makeShared<RefTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document, peff::SharedPtr<TypeNameNode>()))) {
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

				return {};
			};

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

						SLKC_RETURN_IF_COMP_ERROR(loadTheRest(compileContext, finalRegister, resultOut, e->idRefPtr.get(), parts, 1, fnType));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(loadTheRest(compileContext, finalRegister, resultOut, e->idRefPtr.get(), parts, 1, {}));
					}
				} else {
					finalMember = initialMember;
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
							compileContext->fnCompileContext.currentFn.castTo<MemberNode>(),
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

					SLKC_RETURN_IF_COMP_ERROR(loadTheRest(compileContext, finalRegister, resultOut, e->idRefPtr.get(), parts, 0, fnType));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(loadTheRest(compileContext, finalRegister, resultOut, e->idRefPtr.get(), parts, 0, {}));
				}
			}

			SLKC_RETURN_IF_COMP_ERROR(determineNodeType(compileContext, finalMember, resultOut.evaluatedType));

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
						compileContext->emitIns(
							slake::Opcode::MOV,
							resultRegOut,
							{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					break;
				}
				case ExprEvalPurpose::RValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(resultOut.evaluatedType, b));
					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(
							compileContext->emitIns(
								slake::Opcode::LVALUE,
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
					if (b) {
						SLKC_RETURN_IF_COMP_ERROR(
							compileContext->emitIns(
								slake::Opcode::LVALUE,
								resultRegOut,
								{ slake::Value(slake::ValueType::RegRef, finalRegister) }));
					}
					break;
				}
				default:
					std::terminate();
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
						peff::String s(&compileContext->runtime->globalHeapPoolAlloc);

						if (!s.build(e->data)) {
							return genOutOfRuntimeMemoryCompError();
						}

						if (!(sl = slake::StringObject::alloc(compileContext->runtime, std::move(s)))) {
							return genOutOfRuntimeMemoryCompError();
						}
					}

					if (resultRegOut != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR(
							compileContext->emitIns(
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
			if (!(resultOut.evaluatedType = peff::makeShared<StringTypeNameNode>(
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
		case ExprKind::InitializerList: {
			peff::SharedPtr<InitializerListExprNode> e = expr.castTo<InitializerListExprNode>();

			switch (evalPurpose) {
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR(compileContext->pushWarning(
						CompilationWarning(e->tokenRange, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					if (!desiredType)
						return CompilationError(expr->tokenRange, CompilationErrorKind::InvalidInitializerListUsage);
					switch (desiredType->typeNameKind) {
						case TypeNameKind::Array: {
							peff::SharedPtr<ArrayTypeNameNode> tn = desiredType.castTo<ArrayTypeNameNode>();

							if (resultRegOut != UINT32_MAX) {
								slake::Type elementType;

								SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, tn->elementType, elementType));
								SLKC_RETURN_IF_COMP_ERROR(
									compileContext->emitIns(
										slake::Opcode::ARRNEW,
										resultRegOut,
										{ slake::Value(elementType), slake::Value((uint32_t)e->elements.size()) }));

								for (size_t i = 0; i < e->elements.size(); ++i) {
									uint32_t curElementSlotRegIndex = compileContext->allocReg();
									uint32_t curElementRegIndex = compileContext->allocReg();

									SLKC_RETURN_IF_COMP_ERROR(
										compileContext->emitIns(
											slake::Opcode::AT,
											curElementSlotRegIndex,
											{ slake::Value(slake::ValueType::RegRef, resultRegOut), slake::Value((uint32_t)i) }));

									CompileExprResult result(compileContext->allocator.get());

									SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->elements.at(i), ExprEvalPurpose::RValue, tn->elementType.castTo<TypeNameNode>(), curElementRegIndex, result));

									SLKC_RETURN_IF_COMP_ERROR(
										compileContext->emitIns(
											slake::Opcode::STORE,
											UINT32_MAX,
											{ slake::Value(slake::ValueType::RegRef, curElementSlotRegIndex), slake::Value(slake::ValueType::RegRef, curElementRegIndex) }));
								}
							} else {
								// Just simply evaluate each element.
								for (size_t i = 0; i < e->elements.size(); ++i) {
									CompileExprResult result(compileContext->allocator.get());

									SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->elements.at(i), ExprEvalPurpose::RValue, tn->elementType.castTo<TypeNameNode>(), UINT32_MAX, result));
								}
							}

							if (!(resultOut.evaluatedType = desiredType)) {
								return genOutOfMemoryCompError();
							}
							break;
						}
						default:
							return CompilationError(expr->tokenRange, CompilationErrorKind::InvalidInitializerListUsage);
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

			uint32_t targetReg = compileContext->allocReg();

			peff::DynArray<peff::SharedPtr<TypeNameNode>> argTypes(compileContext->allocator.get());

			if (!argTypes.resize(e->args.size())) {
				return genOutOfMemoryCompError();
			}

			CompileExprResult result(compileContext->allocator.get());
			peff::SharedPtr<TypeNameNode> fnType;
			if (auto error = evalExprType(compileContext, e->target, fnType); error) {
				switch (error->errorKind) {
					case CompilationErrorKind::OutOfMemory:
					case CompilationErrorKind::OutOfRuntimeMemory:
						return error;
					default:
						break;
				}

				for (size_t i = 0; i < e->args.size(); ++i) {
					SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->args.at(i), argTypes.at(i)));
				}

				peff::SharedPtr<FnTypeNameNode> fnPrototype;

				if (!(fnPrototype = peff::makeShared<FnTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document))) {
					return genOutOfMemoryCompError();
				}

				fnPrototype->paramTypes = std::move(argTypes);
				fnPrototype->isForAdl = true;

				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->target, ExprEvalPurpose::Call, fnPrototype.castTo<TypeNameNode>(), targetReg, result));

				argTypes = std::move(fnPrototype->paramTypes);
				fnType = result.evaluatedType;
			} else {
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->target, ExprEvalPurpose::Call, {}, targetReg, result));

				fnType = result.evaluatedType;

				peff::SharedPtr<FnTypeNameNode> tn = fnType.castTo<FnTypeNameNode>();

				for (size_t i = 0; i < e->args.size(); ++i) {
					if (i < tn->paramTypes.size()) {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->args.at(i), argTypes.at(i), tn->paramTypes.at(i)));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->args.at(i), argTypes.at(i), {}));
					}

					assert(argTypes.at(i));
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

				uint32_t reg = compileContext->allocReg();

				bool b = false;
				SLKC_RETURN_IF_COMP_ERROR(isLValueType(argTypes.at(i), b));

				if (i < tn->paramTypes.size()) {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, tn->paramTypes.at(i), reg, argResult));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, {}, reg, argResult));
				}
			}

			uint32_t thisReg = UINT32_MAX;

			SLKC_RETURN_IF_COMP_ERROR(removeRefOfType(fnType, fnType));

			if (fnType.castTo<FnTypeNameNode>()->thisType) {
				if (result.idxThisRegOut == UINT32_MAX) {
					if (!e->withObject) {
						return CompilationError(expr->tokenRange, CompilationErrorKind::MissingBindingObject);
					}

					thisReg = compileContext->allocReg();

					SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->withObject, ExprEvalPurpose::RValue, fnType.castTo<FnTypeNameNode>()->thisType, thisReg, result));
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
						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::CALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, targetReg) }));
					}
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(fnType.castTo<FnTypeNameNode>()->returnType, b));
					if (!b) {
						return CompilationError(expr->tokenRange, CompilationErrorKind::ExpectingLValueExpr);
					}

					if (thisReg != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::CALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg) }));
					}
					break;
				}
				case ExprEvalPurpose::RValue:
				case ExprEvalPurpose::Call: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(isLValueType(fnType.castTo<FnTypeNameNode>()->returnType, b));

					if (b) {
						uint32_t tmpRegIndex = compileContext->allocReg();

						if (result.idxThisRegOut != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MCALL, tmpRegIndex, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::CALL, tmpRegIndex, { slake::Value(slake::ValueType::RegRef, targetReg) }));
						}

						SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LVALUE, resultRegOut, { slake::Value(slake::ValueType::RegRef, tmpRegIndex) }));
					} else {
						if (result.idxThisRegOut != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::MCALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg), slake::Value(slake::ValueType::RegRef, result.idxThisRegOut) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::CALL, resultRegOut, { slake::Value(slake::ValueType::RegRef, targetReg) }));
						}
					}

					break;
				}
			}

			if (auto rt = fnType.castTo<FnTypeNameNode>()->returnType; rt) {
				resultOut.evaluatedType = fnType.castTo<FnTypeNameNode>()->returnType;
			} else {
				if (!(resultOut.evaluatedType = peff::makeShared<VoidTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document).castTo<TypeNameNode>())) {
					return genOutOfMemoryCompError();
				}
			}
			break;
		}
		case ExprKind::New: {
			peff::SharedPtr<NewExprNode> e = expr.castTo<NewExprNode>();

			uint32_t targetReg = compileContext->allocReg();

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

				auto tn = peff::makeShared<CustomTypeNameNode>(compileContext->allocator.get(), compileContext->allocator.get(), compileContext->document);

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
			SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::NEW, resultRegOut, { slake::Value(type) }));

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
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->args.at(i), argTypes.at(i), overloading->params.at(i)->type));
						} else {
							SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->args.at(i), argTypes.at(i), {}));
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
						SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->args.at(i), argTypes.at(i), {}));
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

					if (!idRefObject->paramTypes.resize(overloading->params.size()))
						return genOutOfRuntimeMemoryCompError();

					for (size_t i = 0; i < overloading->params.size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, overloading->params.at(i)->type, idRefObject->paramTypes.at(i)));
					}

					if (overloading->fnFlags & FN_VARG)
						idRefObject->hasVarArgs = true;
				}

				uint32_t ctorCallTarget = compileContext->allocReg();

				SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::LOAD, ctorCallTarget, { slake::Value(slake::ValueType::EntityRef, slake::EntityRef::makeObjectRef(idRefObject.get())) }));
				SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::CTORCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegRef, ctorCallTarget), slake::Value(slake::ValueType::RegRef, resultRegOut) }));
			} else {
			}

			resultOut.evaluatedType = e->targetType;
			break;
		}
		case ExprKind::Cast: {
			peff::SharedPtr<CastExprNode> e = expr.castTo<CastExprNode>();

			peff::SharedPtr<TypeNameNode> tn;

			SLKC_RETURN_IF_COMP_ERROR(evalExprType(compileContext, e->source, tn, e->targetType));

			bool b;
			SLKC_RETURN_IF_COMP_ERROR(isTypeConvertible(tn, e->targetType, false, b));

			if (!b) {
				return CompilationError(e->source->tokenRange, CompilationErrorKind::InvalidCast);
			}

			uint32_t idxReg = compileContext->allocReg();

			SLKC_RETURN_IF_COMP_ERROR(isLValueType(e->targetType, b));

			CompileExprResult result(compileContext->allocator.get());
			if (!b) {
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->source, ExprEvalPurpose::RValue, {}, idxReg, result));
			} else {
				SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, e->source, ExprEvalPurpose::LValue, {}, idxReg, result));
			}

			slake::Type type;
			SLKC_RETURN_IF_COMP_ERROR(compileTypeName(compileContext, e->targetType, type));

			SLKC_RETURN_IF_COMP_ERROR(compileContext->emitIns(slake::Opcode::CAST, resultRegOut, { slake::Value(type), slake::Value(slake::ValueType::RegRef, idxReg) }));

			resultOut.evaluatedType = e->targetType;
			break;
		}
		default:
			std::terminate();
	}

	return {};
}
