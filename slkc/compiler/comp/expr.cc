#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileExpr(CompileContext *compileContext, std::shared_ptr<ExprNode> expr) {
	slxfmt::SourceLocDesc sld;
	if (expr->tokenRange) {
		sld.offIns = compileContext->curFn->body.size();
		sld.line = tokenRangeToSourceLocation(expr->tokenRange).beginPosition.line;
		sld.column = tokenRangeToSourceLocation(expr->tokenRange).beginPosition.column;
	}

	if (!compileContext->curCollectiveContext.curMajorContext.curMinorContext.dryRun) {
		if (auto ce = evalConstExpr(compileContext, expr); ce) {
			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(expr->tokenRange),
						MessageType::Error,
						"Expecting a lvalue expression"));

			compileContext->_insertIns(Opcode::MOV, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { ce });
			return;
		}
	}

	switch (expr->getExprType()) {
	case ExprType::Unary: {
		auto e = std::static_pointer_cast<UnaryOpExprNode>(expr);

		compileUnaryOpExpr(compileContext, e, evalExprType(compileContext, e->x));
		break;
	}
	case ExprType::Binary: {
		auto e = std::static_pointer_cast<BinaryOpExprNode>(expr);

		compileBinaryOpExpr(compileContext, e, evalExprType(compileContext, e->lhs), evalExprType(compileContext, e->rhs));
		break;
	}
	case ExprType::Ternary: {
		auto e = std::static_pointer_cast<TernaryOpExprNode>(expr);

		uint32_t conditionRegIndex = compileContext->allocReg(),
				 resultRegIndex = compileContext->allocReg();

		auto loc = tokenRangeToSourceLocation(e->tokenRange);
		std::string falseBranchLabel = "$ternary_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_false",
					endLabel = "$ternary_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

		auto conditionType = evalExprType(compileContext, e->condition),
			 trueBranchType = evalExprType(compileContext, e->x),
			 falseBranchType = evalExprType(compileContext, e->y);
		auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

		if (!conditionType)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Error deducing type of the condition expression"));
		if (!trueBranchType)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Error deducing type of the true branch expression"));
		if (!falseBranchType)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Error deducing type of the true branch expression"));

		auto resultType = getStrongerTypeName(trueBranchType, falseBranchType);

		if (!isSameType(compileContext, conditionType, boolType)) {
			if (!isTypeNamesConvertible(compileContext, conditionType, boolType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(e->condition->tokenRange),
						MessageType::Error,
						"Expecting a boolean expression" });

			compileExpr(compileContext,
				std::make_shared<CastExprNode>(
					boolType,
					e->condition),
				EvalPurpose::RValue,
				std::make_shared<RegRefNode>(conditionRegIndex));
		} else
			compileExpr(compileContext, e->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));

		compileContext->_insertIns(
			Opcode::JF,
			{},
			{ std::make_shared<LabelRefNode>(falseBranchLabel),
				std::make_shared<RegRefNode>(conditionRegIndex) });

		// Compile the true expression.
		if (isSameType(compileContext, trueBranchType, resultType)) {
			if (!isTypeNamesConvertible(compileContext, trueBranchType, resultType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(e->x->tokenRange),
						MessageType::Error,
						"Incompatible operand types" });
			compileExpr(compileContext,
				std::make_shared<CastExprNode>(resultType, e->x),
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose,
				std::make_shared<RegRefNode>(resultRegIndex));
		} else
			compileExpr(compileContext, e->x, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(resultRegIndex));
		compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });

		// Compile the false expression.
		if (isSameType(compileContext, falseBranchType, resultType)) {
			if (!isTypeNamesConvertible(compileContext, falseBranchType, resultType))
				throw FatalCompilationError(
					{ tokenRangeToSourceLocation(e->y->tokenRange),
						MessageType::Error,
						"Incompatible operand types" });
			compileExpr(compileContext,
				std::make_shared<CastExprNode>(resultType, e->y),
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose,
				std::make_shared<RegRefNode>(resultRegIndex));
		} else
			compileExpr(compileContext, e->y, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(resultRegIndex));

		compileContext->_insertLabel(endLabel);

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = resultType;
		break;
	}
	case ExprType::Match: {
		auto e = std::static_pointer_cast<MatchExprNode>(expr);

		auto loc = tokenRangeToSourceLocation(e->tokenRange);

		std::string labelPrefix = "$match_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column),
					condLocalVarName = labelPrefix + "_cond",
					defaultLabel = labelPrefix + "_label",
					endLabel = labelPrefix + "_end";

		uint32_t matcheeRegIndex = compileContext->allocReg();

		// Create a local variable to store result of the condition expression.
		compileExpr(compileContext, e->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(matcheeRegIndex));

		std::pair<std::shared_ptr<ExprNode>, std::shared_ptr<ExprNode>> defaultCase;

		for (auto i : e->cases) {
			std::string caseEndLabel =
				"$match_" +
				std::to_string(tokenRangeToSourceLocation(i.second->tokenRange).beginPosition.line) +
				"_" +
				std::to_string(tokenRangeToSourceLocation(i.second->tokenRange).beginPosition.column) +
				"_caseEnd";

			if (!i.first) {
				if (defaultCase.second)
					// The default case is already exist.
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(i.second->tokenRange),
							MessageType::Error,
							"Duplicated default case" });
				defaultCase = i;
			}

			uint32_t conditionRegIndex = compileContext->allocReg();

			compileExpr(compileContext, i.first, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));

			uint32_t eqResultRegIndex = compileContext->allocReg();
			compileContext->_insertIns(
				Opcode::EQ,
				std::make_shared<RegRefNode>(eqResultRegIndex),
				{ std::make_shared<RegRefNode>(matcheeRegIndex),
					std::make_shared<RegRefNode>(conditionRegIndex) });
			compileContext->_insertIns(
				Opcode::JF,
				{},
				{ std::make_shared<LabelRefNode>(caseEndLabel),
					std::make_shared<RegRefNode>(eqResultRegIndex) });

			// Leave the minor stack that is created for the local variable.
			compileExpr(compileContext, i.second, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest);

			compileContext->_insertLabel(caseEndLabel);
			compileContext->_insertIns(Opcode::JMP, {}, { std::make_shared<LabelRefNode>(endLabel) });
		}

		if (defaultCase.second)
			compileExpr(compileContext, defaultCase.second);

		compileContext->_insertLabel(endLabel);

		// TODO: Set evaluated type.

		break;
	}
	case ExprType::Call: {
		auto e = std::static_pointer_cast<CallExprNode>(expr);

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.isArgTypesSet = true;
		compileContext->curCollectiveContext.curMajorContext.curMinorContext.argTypes = {};

		for (auto &i : e->args) {
			auto type = evalExprType(compileContext, i);
			if (!type)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(i->tokenRange),
						MessageType::Error,
						"Error deducing type of the argument"));
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.argTypes.push_back(type);
		}

		uint32_t callTargetRegIndex = compileContext->allocReg();

		// Note that the register allocated for `thisRegIndex' may end up useless for static methods.
		// The optimizer will (or should) remove it if the register is useless.
		uint32_t thisRegIndex = compileContext->allocReg();

		compileExpr(compileContext, e->target, EvalPurpose::Call, std::make_shared<RegRefNode>(callTargetRegIndex), std::make_shared<RegRefNode>(thisRegIndex));

		auto returnType = compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetReturnType;
		if (!returnType)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->target->tokenRange),
					MessageType::Error,
					"Error deducing return type"));

		for (size_t i = 0; i < e->args.size(); ++i) {
			compileContext->pushMinorContext();

#if SLKC_WITH_LANGUAGE_SERVER
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.curCorrespondingArgIndex = i;
			if (i < compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams.size()) {
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.curCorrespondingParam = compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams[i];
				if (!i) {
					updateCompletionContext(e->idxLParentheseToken, CompletionContext::Expr);
					updateTokenInfo(e->idxLParentheseToken, [this, &i, &compileContext](TokenInfo &info) {
						info.semanticInfo.correspondingParam = compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams[i];
					});
				} else {
					updateCompletionContext(e->idxCommaTokens[i - 1], CompletionContext::Expr);
					updateTokenInfo(e->idxCommaTokens[i - 1], [this, &i, &compileContext](TokenInfo &info) {
						info.semanticInfo.correspondingParam = compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams[i];
					});
				}
			}
#endif

			EvalPurpose evalPurpose = EvalPurpose::RValue;
			if (i < compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams.size()) {
				if (isLValueType(compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams[i]->type))
					evalPurpose = EvalPurpose::LValue;
			}

			uint32_t tmpRegIndex = compileContext->allocReg();

			compileExpr(compileContext, e->args[i], evalPurpose, std::make_shared<RegRefNode>(tmpRegIndex));
			compileContext->_insertIns(
				Opcode::PUSHARG,
				{},
				{ std::make_shared<RegRefNode>(tmpRegIndex) });

			compileContext->popMinorContext();
		}

		switch (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose) {
		case EvalPurpose::LValue: {
			if (!isLValueType(returnType))
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(e->tokenRange),
						MessageType::Error,
						"Expecting a lvalue expression"));

			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.isLastCallTargetStatic)
				compileContext->_insertIns(
					Opcode::CALL,
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
					{ std::make_shared<RegRefNode>(callTargetRegIndex) });
			else
				compileContext->_insertIns(
					Opcode::MCALL,
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
					{ std::make_shared<RegRefNode>(callTargetRegIndex),
						std::make_shared<RegRefNode>(thisRegIndex) });
			break;
		}
		case EvalPurpose::RValue:
		case EvalPurpose::Call: {
			if (isLValueType(returnType)) {
				uint32_t tmpRegIndex = compileContext->allocReg();

				if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.isLastCallTargetStatic)
					compileContext->_insertIns(
						Opcode::CALL,
						std::make_shared<RegRefNode>(tmpRegIndex),
						{ std::make_shared<RegRefNode>(callTargetRegIndex) });
				else
					compileContext->_insertIns(
						Opcode::MCALL,
						std::make_shared<RegRefNode>(tmpRegIndex),
						{ std::make_shared<RegRefNode>(callTargetRegIndex),
							std::make_shared<RegRefNode>(thisRegIndex) });
				compileContext->_insertIns(
					Opcode::LVALUE,
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
					{ std::make_shared<RegRefNode>(tmpRegIndex) });
			} else {
				if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.isLastCallTargetStatic)
					compileContext->_insertIns(
						Opcode::CALL,
						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
						{ std::make_shared<RegRefNode>(callTargetRegIndex) });
				else
					compileContext->_insertIns(
						Opcode::MCALL,
						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
						{ std::make_shared<RegRefNode>(callTargetRegIndex),
							std::make_shared<RegRefNode>(thisRegIndex) });
			}

			break;
		}
		case EvalPurpose::Stmt:
			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.isLastCallTargetStatic)
				compileContext->_insertIns(
					Opcode::CALL,
					{},
					{ std::make_shared<RegRefNode>(callTargetRegIndex) });
			else
				compileContext->_insertIns(
					Opcode::MCALL,
					{},
					{ std::make_shared<RegRefNode>(callTargetRegIndex),
						std::make_shared<RegRefNode>(thisRegIndex) });
			break;
		default:
			assert(false);
		}

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = returnType;

		break;
	}
	case ExprType::Await: {
		// stub

		break;
	}
	case ExprType::New: {
		auto e = std::static_pointer_cast<NewExprNode>(expr);

		switch (e->type->getTypeId()) {
		case TypeId::Array: {
			//
			// Construct a new array.
			//
			auto t = std::static_pointer_cast<ArrayTypeNameNode>(e->type);

			if (!t->elementType) {
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(t->elementType->tokenRange),
						MessageType::Error,
						"Cannot deduce type of elements"));
			}

			if (e->args.size() != 1) {
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(e->type->tokenRange),
						MessageType::Error,
						"Invalid argument number for array constructor"));
			}

			auto sizeArg = e->args[0];
			auto sizeArgType = evalExprType(compileContext, sizeArg);

			auto u32Type = std::make_shared<U32TypeNameNode>(SIZE_MAX);

			if (!sizeArgType) {
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(sizeArg->tokenRange),
						MessageType::Error,
						"Cannot deduce type of the argument"));
			}

			uint32_t sizeArgRegIndex = compileContext->allocReg();

			if (!isSameType(compileContext, sizeArgType, u32Type)) {
				if (!isTypeNamesConvertible(compileContext, sizeArgType, u32Type))
					throw FatalCompilationError(
						{ tokenRangeToSourceLocation(sizeArg->tokenRange),
							MessageType::Error,
							"Incompatible argument type" });
				compileExpr(compileContext,
					std::make_shared<CastExprNode>(u32Type, sizeArg),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(sizeArgRegIndex));
			} else {
				compileExpr(compileContext,
					sizeArg,
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(sizeArgRegIndex));
			}

			compileContext->_insertIns(
				Opcode::ARRNEW,
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
				{ t->elementType,
					std::make_shared<RegRefNode>(sizeArgRegIndex) });

			break;
		}
		case TypeId::Custom: {
			//
			// Instantiate a custom type.
			//
			auto node = resolveCustomTypeName(compileContext, (CustomTypeNameNode *)e->type.get());

			switch (node->getNodeType()) {
			case NodeType::Class:
			case NodeType::Interface: {
				std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

				if (auto it = n->scope->members.find("new"); it != n->scope->members.end()) {
					std::deque<std::shared_ptr<TypeNameNode>> argTypes;

					for (auto &i : e->args) {
						auto t = evalExprType(compileContext, i);
						if (!t)
							throw FatalCompilationError(
								Message(
									tokenRangeToSourceLocation(i->tokenRange),
									MessageType::Error,
									"Error deducing type of the argument"));
						argTypes.push_back(t);
					}

					assert(it->second->getNodeType() == NodeType::Fn);

					std::shared_ptr<FnOverloadingNode> overloading;

					{
						auto overloadings = argDependentLookup(compileContext, (FnNode *)it->second.get(), argTypes, {});
						if (!overloadings.size()) {
							throw FatalCompilationError(
								Message(
									tokenRangeToSourceLocation(expr->tokenRange),
									MessageType::Error,
									"No matching function was found"));
						} else if (overloadings.size() > 1) {
							for (auto i : overloadings) {
								pushMessage(
									curDocName,
									Message(
										tokenRangeToSourceLocation(i->tokenRange),
										MessageType::Note,
										"Matched here"));
							}
							throw FatalCompilationError(
								Message(
									tokenRangeToSourceLocation(expr->tokenRange),
									MessageType::Error,
									"Ambiguous function call"));
						}

						overloading = overloadings[0];
					}

					std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
					for (auto &j : overloading->params) {
						paramTypes.push_back(j->type);
					}

					if (overloading->isVaridic())
						paramTypes.pop_back();

					for (size_t i = 0; i < argTypes.size(); ++i) {
						if (i < paramTypes.size()) {
							if (isSameType(compileContext, argTypes[i], paramTypes[i])) {
								if (auto ce = evalConstExpr(compileContext, e->args[i]); ce)
									compileContext->_insertIns(Opcode::PUSHARG, {}, { ce });
								else {
									uint32_t tmpRegIndex = compileContext->allocReg();

									compileExpr(compileContext, e->args[i], EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
									compileContext->_insertIns(Opcode::PUSHARG, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
								}
							} else {
								uint32_t tmpRegIndex = compileContext->allocReg();

								compileExpr(compileContext,
									std::make_shared<CastExprNode>(argTypes[i], e->args[i]),
									EvalPurpose::RValue,
									std::make_shared<RegRefNode>(tmpRegIndex));
								compileContext->_insertIns(Opcode::PUSHARG, {}, { std::make_shared<RegRefNode>(tmpRegIndex) });
							}
						} else {
							if (auto ce = evalConstExpr(compileContext, e->args[i]); ce)
								compileContext->_insertIns(Opcode::PUSHARG, {}, { ce, {} });
							else {
								uint32_t tmpRegIndex = compileContext->allocReg();

								compileExpr(compileContext, e->args[i], EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
								compileContext->_insertIns(Opcode::PUSHARG, {}, { std::make_shared<RegRefNode>(tmpRegIndex), {} });
							}
						}
					}

					auto fullName = getFullName(overloading.get());
					uint32_t ctorIndex = compileContext->allocReg();
					compileContext->_insertIns(
						Opcode::LOAD,
						std::make_shared<RegRefNode>(ctorIndex),
						{ std::make_shared<IdRefExprNode>(fullName) });

					compileContext->_insertIns(
						Opcode::NEW,
						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
						{ e->type });
					compileContext->_insertIns(
						Opcode::CTORCALL,
						{},
						{ std::make_shared<RegRefNode>(ctorIndex), compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest });
				} else {
					compileContext->_insertIns(
						Opcode::NEW,
						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
						{ e->type });
				}

				break;
			}
			case NodeType::GenericParam:
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(e->type->tokenRange),
						MessageType::Error,
						"Cannot instantiate a generic parameter"));
			default:
				assert(false);
			}

			break;
		}
		default:
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->type->tokenRange),
					MessageType::Error,
					"Specified type is not constructible"));
		}

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = e->type;

		break;
	}
	case ExprType::Typeof: {
		auto e = std::static_pointer_cast<TypeofExprNode>(expr);

		if (auto ce = evalConstExpr(compileContext, e->target); ce) {
			compileContext->_insertIns(
				Opcode::TYPEOF,
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
				{ ce });
		} else {
			uint32_t tmpRegIndex = compileContext->allocReg();

			compileExpr(compileContext, e->target, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
			compileContext->_insertIns(
				Opcode::TYPEOF,
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
				{ std::make_shared<RegRefNode>(tmpRegIndex) });
		}

		// TODO: Set evaluated type.

		break;
	}
	case ExprType::Cast: {
		auto e = std::static_pointer_cast<CastExprNode>(expr);

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType = e->targetType;

		if (auto ce = evalConstExpr(compileContext, e->target); ce) {
			if (isTypeNamesConvertible(compileContext, evalExprType(compileContext, ce), e->targetType)) {
				compileContext->_insertIns(Opcode::CAST, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { e->targetType, ce });
			} else {
				throw FatalCompilationError({ tokenRangeToSourceLocation(e->tokenRange), MessageType::Error, "Invalid type conversion" });
			}
		} else {
			auto originalType = evalExprType(compileContext, e->target);

			if (!originalType)
				break;

			if (isTypeNamesConvertible(compileContext, originalType, e->targetType)) {
				uint32_t tmpRegIndex = compileContext->allocReg();

				compileExpr(compileContext, e->target, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				compileContext->_insertIns(Opcode::CAST, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { e->targetType, std::make_shared<RegRefNode>(tmpRegIndex) });
			} else {
				throw FatalCompilationError({ tokenRangeToSourceLocation(e->tokenRange), MessageType::Error, "Invalid type conversion" });
			}
		}

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = e->targetType;

		break;
	}
	case ExprType::HeadedRef: {
		auto e = std::static_pointer_cast<HeadedIdRefExprNode>(expr);

		break;
	}
	case ExprType::IdRef: {
		auto e = std::static_pointer_cast<IdRefExprNode>(expr);

#if SLKC_WITH_LANGUAGE_SERVER
		updateCompletionContext(e->ref, CompletionContext::Expr);
#endif

		IdRefResolvedParts resolvedParts;
		bool isStatic;
		if (!resolveIdRef(compileContext, e->ref, resolvedParts, isStatic))
			throw FatalCompilationError(
				{ tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Identifier not found: `" + std::to_string(e->ref->entries, this) + "'" });

		uint32_t tmpRegIndex = compileContext->allocReg();

		auto determineOverloadingRegistry = [this, expr, &resolvedParts, &compileContext, &isStatic](std::shared_ptr<FnNode> x, const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs) -> std::shared_ptr<FnOverloadingNode> {
			if ((resolvedParts.size() > 2) || (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Call)) {
				//
				// Reference to a overloaded function is always ambiguous,
				// because we cannot determine which overloading is the user wanted.
				//
				if (x->overloadingRegistries.size() > 1) {
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(expr->tokenRange),
							MessageType::Error,
							"Reference to a overloaded function is ambiguous"));
				}

				return x->overloadingRegistries.front();
			}

			//
			// Find a proper overloading for the function calling expression.
			//
			{
				auto overloadings = argDependentLookup(
					compileContext,
					x.get(),
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.argTypes,
					genericArgs,
					isStatic);

				if (!overloadings.size()) {
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(expr->tokenRange),
							MessageType::Error,
							"No matching function was found"));
				} else if (overloadings.size() > 1) {
					for (auto i : overloadings) {
						pushMessage(
							curDocName,
							Message(
								tokenRangeToSourceLocation(i->tokenRange),
								MessageType::Note,
								"Matched here"));
					}
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(expr->tokenRange),
							MessageType::Error,
							"Ambiguous function call"));
				}

				compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetParams = overloadings[0]->params;
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.hasVarArgs = overloadings[0]->isVaridic();

				compileContext->curCollectiveContext.curMajorContext.curMinorContext.lastCallTargetReturnType =
					overloadings[0]->returnType
						? overloadings[0]->returnType
						: std::make_shared<AnyTypeNameNode>(SIZE_MAX);
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.isLastCallTargetStatic = overloadings[0]->access & ACCESS_STATIC;
				return overloadings[0];
			}
		};

		auto loadRest = [this, &determineOverloadingRegistry, &expr, &tmpRegIndex, &resolvedParts, &compileContext]() -> uint32_t {
			for (size_t i = 1; i < resolvedParts.size(); ++i) {
				switch (resolvedParts[i].second->getNodeType()) {
				case NodeType::Var: {
					auto varNode = std::static_pointer_cast<VarNode>(resolvedParts[i].second);

					uint32_t resultRegIndex = compileContext->allocReg();
					compileContext->_insertIns(
						Opcode::RLOAD,
						std::make_shared<RegRefNode>(resultRegIndex),
						{ std::make_shared<RegRefNode>(tmpRegIndex),
							std::make_shared<IdRefExprNode>(resolvedParts[i].first) });

					if (varNode->type ? isLValueType(varNode->type) : false) {
						uint32_t newResultRegIndex = compileContext->allocReg();

						// Load once more for reference types.
						compileContext->_insertIns(
							Opcode::LVALUE,
							std::make_shared<RegRefNode>(newResultRegIndex),
							{ std::make_shared<RegRefNode>(resultRegIndex) });

						resultRegIndex = newResultRegIndex;
					}

					// Intermediate scopes should always be loaded as rvalue.
					if ((i + 1 < resolvedParts.size())) {
						uint32_t newResultRegIndex = compileContext->allocReg();
						compileContext->_insertIns(
							Opcode::LVALUE,
							std::make_shared<RegRefNode>(newResultRegIndex),
							{ std::make_shared<RegRefNode>(resultRegIndex) });
						resultRegIndex = newResultRegIndex;
					} else {
						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = varNode->type->duplicate<TypeNameNode>();
						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType->isRef = true;
					}

					tmpRegIndex = resultRegIndex;
					break;
				}
				case NodeType::Fn: {
					std::shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(
						std::static_pointer_cast<FnNode>(resolvedParts[i].second),
						resolvedParts[i].first->entries.back().genericArgs);

					if (!compileContext->curCollectiveContext.curMajorContext.curMinorContext.isLastCallTargetStatic) {
						compileContext->_insertIns(
							Opcode::MOV,
							compileContext->curCollectiveContext.curMajorContext.curMinorContext.thisDest,
							{ std::make_shared<RegRefNode>(tmpRegIndex) });
					}

#if SLKC_WITH_LANGUAGE_SERVER
					updateTokenInfo(resolvedParts[i].first->entries.back().idxToken, [&overloading](TokenInfo &tokenInfo) {
						tokenInfo.semanticInfo.correspondingMember = overloading;
					});
#endif

					{
						std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
						for (auto &j : overloading->params) {
							paramTypes.push_back(j->type);
						}

						if (overloading->isVaridic())
							paramTypes.pop_back();

						compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType =
							std::make_shared<FnTypeNameNode>(
								overloading->returnType,
								paramTypes);
					}

					uint32_t newRegIndex = compileContext->allocReg();
					if (overloading->isVirtual) {
						// Copy the parameter types to the reference.
						resolvedParts[i].first->entries.back().hasParamTypes = true;
						resolvedParts[i].first->entries.back().paramTypes = compileContext->curCollectiveContext.curMajorContext.curMinorContext.argTypes;
						resolvedParts[i].first->entries.back().hasVarArg = compileContext->curCollectiveContext.curMajorContext.curMinorContext.hasVarArgs;

						compileContext->_insertIns(
							Opcode::RLOAD,
							std::make_shared<RegRefNode>(newRegIndex),
							{ std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<IdRefExprNode>(resolvedParts[i].first) });
					} else {
						auto fullRef = getFullName(overloading.get());
						compileContext->_insertIns(
							Opcode::LOAD,
							std::make_shared<RegRefNode>(newRegIndex),
							{ std::make_shared<IdRefExprNode>(fullRef) });
					}

					tmpRegIndex = newRegIndex;
					break;
				}
				default:
					assert(false);
				}
			}

			return tmpRegIndex;
		};

		auto &x = resolvedParts.front().second;
		switch (x->getNodeType()) {
		case NodeType::LocalVar: {
			auto localVarNode = std::static_pointer_cast<LocalVarNode>(x);

			compileContext->_insertIns(
				Opcode::LLOAD,
				std::make_shared<RegRefNode>(tmpRegIndex),
				{ std::make_shared<U32LiteralExprNode>(localVarNode->index) });

			bool unwrap = false;
			if (resolvedParts.size() > 1) {
				unwrap = true;
			} else {
				unwrap = compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue;
			}

			if (unwrap) {
				uint32_t newTmpRegIndex = compileContext->allocReg();
				compileContext->_insertIns(
					Opcode::LVALUE,
					std::make_shared<RegRefNode>(newTmpRegIndex),
					{ std::make_shared<RegRefNode>(tmpRegIndex) });
				tmpRegIndex = newTmpRegIndex;
			}

			if (resolvedParts.size() == 1) {
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = localVarNode->type->duplicate<TypeNameNode>();
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType->isRef = true;
			}

			uint32_t resultRegIndex = loadRest();
			if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
				if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue) {
					uint32_t newResultRegIndex = compileContext->allocReg();
					compileContext->_insertIns(
						Opcode::LVALUE,
						std::make_shared<RegRefNode>(newResultRegIndex),
						{ std::make_shared<RegRefNode>(resultRegIndex) });
					resultRegIndex = newResultRegIndex;
				}
			}

			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest)
				compileContext->_insertIns(
					Opcode::MOV,
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
					{ std::make_shared<RegRefNode>(resultRegIndex) });
			break;
		}
		case NodeType::Param: {
			auto paramNode = std::static_pointer_cast<ParamNode>(x);

			compileContext->_insertIns(
				Opcode::LARG,
				std::make_shared<RegRefNode>(tmpRegIndex),
				{ std::make_shared<U32LiteralExprNode>(compileContext->curFn->paramIndices[paramNode->name]) });

			if (resolvedParts.size() > 1) {
				uint32_t newTmpRegIndex = compileContext->allocReg();
				compileContext->_insertIns(
					Opcode::LVALUE,
					std::make_shared<RegRefNode>(newTmpRegIndex),
					{ std::make_shared<RegRefNode>(tmpRegIndex) });
				tmpRegIndex = newTmpRegIndex;
			}

			if (paramNode->type && isLValueType(paramNode->type)) {
				uint32_t newTmpRegIndex = compileContext->allocReg();
				// Load once more for reference types.
				compileContext->_insertIns(
					Opcode::LVALUE,
					std::make_shared<RegRefNode>(newTmpRegIndex),
					{ std::make_shared<RegRefNode>(tmpRegIndex) });
				tmpRegIndex = newTmpRegIndex;
			}

			if (resolvedParts.size() == 1) {
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = paramNode->type->duplicate<TypeNameNode>();
				compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType->isRef = true;
			}

			uint32_t resultRegIndex = loadRest();
			switch (resolvedParts.back().second->getNodeType()) {
			case NodeType::Var:
			case NodeType::Param:
				if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue) {
					uint32_t newResultRegIndex = compileContext->allocReg();
					compileContext->_insertIns(
						Opcode::LVALUE,
						std::make_shared<RegRefNode>(newResultRegIndex),
						{ std::make_shared<RegRefNode>(resultRegIndex) });
					resultRegIndex = newResultRegIndex;
				}
				break;
			default:
				break;
			}

			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest)
				compileContext->_insertIns(
					Opcode::MOV,
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
					{ std::make_shared<RegRefNode>(resultRegIndex) });
			break;
		}
		case NodeType::Var:
		case NodeType::Fn:
		case NodeType::ThisRef:
		case NodeType::BaseRef:
		case NodeType::Class:
		case NodeType::Interface:
		case NodeType::Module: {
			//
			// Resolve the head of the reference.
			// After that, we will use RLOAD instructions to load the members one by one.
			//
			switch (x->getNodeType()) {
			case NodeType::Class:
			case NodeType::Interface:
			case NodeType::Module: {
				auto ref = getFullName((MemberNode *)x.get());
				compileContext->_insertIns(
					Opcode::LOAD,
					std::make_shared<RegRefNode>(tmpRegIndex),
					{ std::make_shared<IdRefExprNode>(ref) });

				if (resolvedParts.size() == 1) {
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
							MessageType::Error,
							"`" + std::to_string(e->ref->entries, this) + "' is a type"));
				}
				break;
			}
			case NodeType::Var: {
				auto varNode = (VarNode *)x.get();

				auto ref = getFullName(varNode);
				compileContext->_insertIns(
					Opcode::LOAD,
					std::make_shared<RegRefNode>(tmpRegIndex),
					{ std::make_shared<IdRefExprNode>(ref) });

				if (resolvedParts.size() == 1) {
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = varNode->type->duplicate<TypeNameNode>();
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType->isRef = true;
				}
				break;
			}
			case NodeType::Fn: {
				FnNode *fn = (FnNode *)x.get();

				std::shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(
					std::static_pointer_cast<FnNode>(x),
					resolvedParts.front().first->entries.back().genericArgs);
				auto ref = getFullName(overloading.get());

#if SLKC_WITH_LANGUAGE_SERVER
				updateTokenInfo(resolvedParts.front().first->entries.back().idxToken, [&overloading](TokenInfo &tokenInfo) {
					tokenInfo.semanticInfo.correspondingMember = overloading;
				});
#endif

				std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
				for (auto i : overloading->params) {
					paramTypes.push_back(i->type);
				}

				if (resolvedParts.size() == 1) {
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType =
						std::make_shared<FnTypeNameNode>(
							overloading->returnType,
							paramTypes);
				}

				compileContext->_insertIns(
					Opcode::LOAD,
					std::make_shared<RegRefNode>(tmpRegIndex),
					{ std::make_shared<IdRefExprNode>(ref) });
				break;
			}
			case NodeType::ThisRef: {
				auto owner = compileContext->curCollectiveContext.curMajorContext.curMinorContext.curScope->owner;

				switch (owner->getNodeType()) {
				case NodeType::Class:
				case NodeType::Interface: {
					compileContext->_insertIns(Opcode::LTHIS, std::make_shared<RegRefNode>(tmpRegIndex), {});
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = compileContext->curCollectiveContext.curMajorContext.thisType;
					break;
				}
				default:
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(e->tokenRange),
							MessageType::Error,
							"Cannot use this reference in this context"));
				}

				break;
			}
			case NodeType::BaseRef:
				compileContext->_insertIns(
					Opcode::LOAD,
					std::make_shared<RegRefNode>(tmpRegIndex),
					{ std::make_shared<IdRefExprNode>(std::make_shared<IdRefNode>(IdRefEntries{ IdRefEntry(e->tokenRange, SIZE_MAX, "base") })) });
				break;
			default:
				assert(false);
			}

			// Check if the target is static.
			uint32_t resultRegIndex = loadRest();

			if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
				if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue) {
					uint32_t newResultRegIndex = compileContext->allocReg();
					compileContext->_insertIns(
						Opcode::LVALUE,
						std::make_shared<RegRefNode>(newResultRegIndex),
						{ std::make_shared<RegRefNode>(resultRegIndex) });
					resultRegIndex = newResultRegIndex;
				}
			}

			if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest)
				compileContext->_insertIns(
					Opcode::MOV,
					compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest,
					{ std::make_shared<RegRefNode>(resultRegIndex) });
			break;
		}
		default:
			assert(false);
		}

		break;
	}
	case ExprType::VarArg: {
		auto e = std::static_pointer_cast<VarArgExprNode>(expr);

		if (!compileContext->curFn->hasVarArgs) {
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Cannot reference varidic arguments in a function without the varidic parameter"));
		}

		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(expr->tokenRange),
					MessageType::Error,
					"Expecting a lvalue expression"));

		uint32_t tmpRegIndex = compileContext->allocReg();
		compileContext->_insertIns(Opcode::LARG, std::make_shared<RegRefNode>(tmpRegIndex), { std::make_shared<U32LiteralExprNode>(compileContext->curFn->params.size()) });
		compileContext->_insertIns(Opcode::LVALUE, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { std::make_shared<RegRefNode>(tmpRegIndex) });

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<ArrayTypeNameNode>(std::make_shared<AnyTypeNameNode>(SIZE_MAX));

		break;
	}
	case ExprType::Array: {
		auto e = std::static_pointer_cast<ArrayExprNode>(expr);

		if ((!compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType) ||
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType->getTypeId() != TypeId::Array)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(e->tokenRange),
					MessageType::Error,
					"Error deducing type of the expression"));

		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(expr->tokenRange),
					MessageType::Error,
					"Expecting a lvalue expression"));

		auto type = std::static_pointer_cast<ArrayTypeNameNode>(compileContext->curCollectiveContext.curMajorContext.curMinorContext.expectedType);

#if SLKC_WITH_LANGUAGE_SERVER
		updateTokenInfo(e->idxLBraceToken, [this, &compileContext](TokenInfo &tokenInfo) {
			tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
			tokenInfo.completionContext = CompletionContext::Expr;
		});

		for (size_t i = 0; i < e->idxCommaTokens.size(); ++i) {
			updateTokenInfo(e->idxCommaTokens[i], [this, &compileContext](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(compileContext->curFn, compileContext->curCollectiveContext.curMajorContext);
				tokenInfo.completionContext = CompletionContext::Expr;
			});
		}
#endif

		// Check type of the members.
		for (auto &i : e->elements) {
			auto t = evalExprType(compileContext, i);
			if (!t)
				throw FatalCompilationError(
					Message(
						tokenRangeToSourceLocation(i->tokenRange),
						MessageType::Error,
						"Error deducing the element type"));

			if (!isSameType(compileContext, t, type->elementType)) {
				if (!isTypeNamesConvertible(compileContext, t, type->elementType))
					throw FatalCompilationError(
						Message(
							tokenRangeToSourceLocation(i->tokenRange),
							MessageType::Error,
							"Incompatible element type"));
			}
		}

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = type;

		if (auto ce = evalConstExpr(compileContext, e); ce) {
			compileContext->_insertIns(Opcode::MOV, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { ce });
		} else {
			auto initArray = std::make_shared<ArrayExprNode>();
			initArray->tokenRange = e->tokenRange;
			initArray->elements.resize(e->elements.size());

			initArray->evaluatedElementType = type->elementType;

			// We use nullptr to represent non-constexpr expressions.
			for (size_t i = 0; i < e->elements.size(); ++i) {
				initArray->elements[i] = evalConstExpr(compileContext, initArray->elements[i]);
			}

			uint32_t idxTmpArrayRegIndex = compileContext->allocReg(),
					 idxTmpElementRegIndex = compileContext->allocReg();

			compileContext->_insertIns(
				Opcode::MOV,
				std::make_shared<RegRefNode>(idxTmpArrayRegIndex),
				{ initArray });

			// Assign non-constexpr expressions to corresponding elements.
			for (size_t i = 0; i < initArray->elements.size(); ++i) {
				if (!initArray->elements[i]) {
					compileContext->_insertIns(
						Opcode::AT,
						std::make_shared<RegRefNode>(idxTmpElementRegIndex),
						{ std::make_shared<RegRefNode>(idxTmpArrayRegIndex),
							std::make_shared<U32LiteralExprNode>(idxTmpElementRegIndex, SIZE_MAX) });
					compileExpr(compileContext,
						e->elements[i],
						EvalPurpose::RValue,
						std::make_shared<RegRefNode>(idxTmpElementRegIndex));
				}
			}
		}

		break;
	}
	case ExprType::I8:
	case ExprType::I16:
	case ExprType::I32:
	case ExprType::I64:
	case ExprType::U8:
	case ExprType::U16:
	case ExprType::U32:
	case ExprType::U64:
	case ExprType::F32:
	case ExprType::F64:
	case ExprType::String:
	case ExprType::Bool: {
		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(expr->tokenRange),
					MessageType::Error,
					"Expecting a lvalue expression"));

		compileContext->_insertIns(Opcode::MOV, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { expr });

		switch (expr->getExprType()) {
		case ExprType::I8:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::I16:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::I32:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::I64:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::U8:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::U16:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::U32:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::U64:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::F32:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::F64:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
			break;
		case ExprType::String:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<StringTypeNameNode>(SIZE_MAX);
			break;
		case ExprType::Bool:
			compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
			break;
		default:;
		}
		break;
	}
	case ExprType::Null:
		if (compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
			throw FatalCompilationError(
				Message(
					tokenRangeToSourceLocation(expr->tokenRange),
					MessageType::Error,
					"Expecting a lvalue expression"));

		compileContext->_insertIns(Opcode::MOV, compileContext->curCollectiveContext.curMajorContext.curMinorContext.evalDest, { expr });

		compileContext->curCollectiveContext.curMajorContext.curMinorContext.evaluatedType = std::make_shared<AnyTypeNameNode>(SIZE_MAX);
		break;
	case ExprType::Bad:
		break;
	default:
		assert(false);
	}

	if (!compileContext->curCollectiveContext.curMajorContext.curMinorContext.dryRun) {
		if (expr->tokenRange) {
			sld.nIns = compileContext->curFn->body.size() - sld.offIns;
			compileContext->curFn->srcLocDescs.push_back(sld);
		}
	}
}
