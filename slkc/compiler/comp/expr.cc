#include "../compiler.h"

using namespace slake::slkc;

void Compiler::compileExpr(std::shared_ptr<ExprNode> expr) {
	slxfmt::SourceLocDesc sld;
	sld.offIns = curFn->body.size();
	sld.line = expr->sourceLocation.beginPosition.line;
	sld.column = expr->sourceLocation.beginPosition.column;

	if (!curMajorContext.curMinorContext.dryRun) {
		if (auto ce = evalConstExpr(expr); ce) {
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->sourceLocation,
						MessageType::Error,
						"Expecting a lvalue expression"));

			_insertIns(Opcode::STORE, { curMajorContext.curMinorContext.evalDest, ce });
			return;
		}
	}

	switch (expr->getExprType()) {
		case ExprType::Unary: {
			auto e = std::static_pointer_cast<UnaryOpExprNode>(expr);

			compileUnaryOpExpr(e, evalExprType(e->x));
			break;
		}
		case ExprType::Binary: {
			auto e = std::static_pointer_cast<BinaryOpExprNode>(expr);

			compileBinaryOpExpr(e, evalExprType(e->lhs), evalExprType(e->rhs));
			break;
		}
		case ExprType::Ternary: {
			auto e = std::static_pointer_cast<TernaryOpExprNode>(expr);

			uint32_t conditionRegIndex = allocReg(),
					 resultRegIndex = allocReg();

			auto loc = e->sourceLocation;
			std::string falseBranchLabel = "$ternary_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_false",
						endLabel = "$ternary_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column) + "_end";

			auto conditionType = evalExprType(e->condition),
				 trueBranchType = evalExprType(e->x),
				 falseBranchType = evalExprType(e->y);
			auto boolType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);

			if (!conditionType)
				throw FatalCompilationError(
					Message(
						e->sourceLocation,
						MessageType::Error,
						"Error deducing type of the condition expression"));
			if (!trueBranchType)
				throw FatalCompilationError(
					Message(
						e->sourceLocation,
						MessageType::Error,
						"Error deducing type of the true branch expression"));
			if (!falseBranchType)
				throw FatalCompilationError(
					Message(
						e->sourceLocation,
						MessageType::Error,
						"Error deducing type of the true branch expression"));

			auto resultType = getStrongerTypeName(trueBranchType, falseBranchType);

			if (!isSameType(conditionType, boolType)) {
				if (!isTypeNamesConvertible(conditionType, boolType))
					throw FatalCompilationError(
						{ e->condition->sourceLocation,
							MessageType::Error,
							"Expecting a boolean expression" });

				compileExpr(
					std::make_shared<CastExprNode>(
						boolType,
						e->condition),
					EvalPurpose::RValue,
					std::make_shared<RegRefNode>(conditionRegIndex));
			} else
				compileExpr(e->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));

			_insertIns(
				Opcode::JF,
				{ std::make_shared<LabelRefNode>(falseBranchLabel),
					std::make_shared<RegRefNode>(conditionRegIndex, true) });

			// Compile the true expression.
			if (isSameType(trueBranchType, resultType)) {
				if (!isTypeNamesConvertible(trueBranchType, resultType))
					throw FatalCompilationError(
						{ e->x->sourceLocation,
							MessageType::Error,
							"Incompatible operand types" });
				compileExpr(
					std::make_shared<CastExprNode>(resultType, e->x),
					curMajorContext.curMinorContext.evalPurpose,
					std::make_shared<RegRefNode>(resultRegIndex));
			} else
				compileExpr(e->x, curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(resultRegIndex));
			_insertIns(Opcode::JMP, { std::make_shared<LabelRefNode>(endLabel) });

			// Compile the false expression.
			if (isSameType(falseBranchType, resultType)) {
				if (!isTypeNamesConvertible(falseBranchType, resultType))
					throw FatalCompilationError(
						{ e->y->sourceLocation,
							MessageType::Error,
							"Incompatible operand types" });
				compileExpr(
					std::make_shared<CastExprNode>(resultType, e->y),
					curMajorContext.curMinorContext.evalPurpose,
					std::make_shared<RegRefNode>(resultRegIndex));
			} else
				compileExpr(e->y, curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(resultRegIndex));

			_insertLabel(endLabel);

			curMajorContext.curMinorContext.evaluatedType = resultType;
			break;
		}
		case ExprType::Match: {
			auto e = std::static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->sourceLocation;

			std::string labelPrefix = "$match_" + std::to_string(loc.beginPosition.line) + "_" + std::to_string(loc.beginPosition.column),
						condLocalVarName = labelPrefix + "_cond",
						defaultLabel = labelPrefix + "_label",
						endLabel = labelPrefix + "_end";

			uint32_t matcheeRegIndex = allocReg();

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RValue, std::make_shared<RegRefNode>(matcheeRegIndex));

			std::pair<std::shared_ptr<ExprNode>, std::shared_ptr<ExprNode>> defaultCase;

			for (auto i : e->cases) {
				std::string caseEndLabel =
					"$match_" + std::to_string(i.second->sourceLocation.beginPosition.line) + "_" + std::to_string(i.second->sourceLocation.beginPosition.column) + "_caseEnd";

				if (!i.first) {
					if (defaultCase.second)
						// The default case is already exist.
						throw FatalCompilationError(
							{ i.second->sourceLocation,
								MessageType::Error,
								"Duplicated default case" });
					defaultCase = i;
				}

				uint32_t conditionRegIndex = allocReg();

				compileExpr(i.first, EvalPurpose::RValue, std::make_shared<RegRefNode>(conditionRegIndex));
				_insertIns(
					Opcode::EQ,
					{ std::make_shared<RegRefNode>(conditionRegIndex),
						std::make_shared<RegRefNode>(matcheeRegIndex, true),
						std::make_shared<RegRefNode>(conditionRegIndex, true) });
				_insertIns(
					Opcode::JF,
					{ std::make_shared<LabelRefNode>(caseEndLabel),
						std::make_shared<RegRefNode>(conditionRegIndex, true) });

				// Leave the minor stack that is created for the local variable.
				compileExpr(i.second, curMajorContext.curMinorContext.evalPurpose, curMajorContext.curMinorContext.evalDest);

				_insertLabel(caseEndLabel);
				_insertIns(Opcode::JMP, { std::make_shared<LabelRefNode>(endLabel) });
			}

			if (defaultCase.second)
				compileExpr(defaultCase.second);

			_insertLabel(endLabel);

			// TODO: Set evaluated type.

			break;
		}
		case ExprType::Call: {
			auto e = std::static_pointer_cast<CallExprNode>(expr);

			curMajorContext.curMinorContext.isArgTypesSet = true;
			curMajorContext.curMinorContext.argTypes = {};

			for (auto &i : e->args) {
				auto type = evalExprType(i);
				if (!type)
					throw FatalCompilationError(
						Message(
							i->sourceLocation,
							MessageType::Error,
							"Error deducing type of the argument"));
				curMajorContext.curMinorContext.argTypes.push_back(type);
			}

			uint32_t callTargetRegIndex = allocReg(),
					 tmpRegIndex = allocReg(),
					 thisRegIndex = allocReg();

			compileExpr(e->target, EvalPurpose::Call, std::make_shared<RegRefNode>(callTargetRegIndex), std::make_shared<RegRefNode>(thisRegIndex));

			auto returnType = curMajorContext.curMinorContext.lastCallTargetReturnType;
			if (!returnType)
				throw FatalCompilationError(
					Message(
						e->target->sourceLocation,
						MessageType::Error,
						"Error deducing return type"));

			for (size_t i = 0; i < e->args.size(); ++i) {
				EvalPurpose evalPurpose = EvalPurpose::RValue;
				if (i < curMajorContext.curMinorContext.lastCallTargetParams.size()) {
					if (isLValueType(curMajorContext.curMinorContext.lastCallTargetParams[i]->type))
						evalPurpose = EvalPurpose::LValue;
				}

				compileExpr(e->args[i], evalPurpose, std::make_shared<RegRefNode>(tmpRegIndex));
				_insertIns(
					Opcode::PUSHARG,
					{ std::make_shared<RegRefNode>(tmpRegIndex, true),
						i < curMajorContext.curMinorContext.lastCallTargetParams.size()
							? curMajorContext.curMinorContext.lastCallTargetParams[i]->type
							: nullptr });
			}

			if (curMajorContext.curMinorContext.isLastCallTargetStatic)
				_insertIns(
					e->isAsync ? Opcode::ACALL : Opcode::CALL,
					{ std::make_shared<RegRefNode>(callTargetRegIndex, true) });
			else
				_insertIns(
					e->isAsync ? Opcode::AMCALL : Opcode::MCALL,
					{ std::make_shared<RegRefNode>(callTargetRegIndex, true),
						std::make_shared<RegRefNode>(thisRegIndex, true) });

			switch (curMajorContext.curMinorContext.evalPurpose) {
				case EvalPurpose::LValue: {
					if (!isLValueType(returnType))
						throw FatalCompilationError(
							Message(
								e->sourceLocation,
								MessageType::Error,
								"Expecting a lvalue expression"));

					_insertIns(
						Opcode::LRET,
						{ curMajorContext.curMinorContext.evalDest });
					break;
				}
				case EvalPurpose::RValue:
				case EvalPurpose::Call: {
					if (isLValueType(returnType)) {
						uint32_t tmpRegIndex = allocReg();

						_insertIns(
							Opcode::LRET,
							{ std::make_shared<RegRefNode>(tmpRegIndex) });
						_insertIns(
							Opcode::LVALUE,
							{ curMajorContext.curMinorContext.evalDest,
								std::make_shared<RegRefNode>(tmpRegIndex, true) });
					} else {
						_insertIns(
							Opcode::LRET,
							{ curMajorContext.curMinorContext.evalDest });
					}

					break;
				}
				case EvalPurpose::Stmt:
					break;
				default:
					assert(false);
			}

			curMajorContext.curMinorContext.evaluatedType = returnType;

			break;
		}
		case ExprType::Await: {
			uint32_t awaitTargetRegIndex = allocReg();

			auto e = std::static_pointer_cast<AwaitExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				_insertIns(Opcode::AWAIT, { ce });
			} else {
				// TODO: Check if the target is a method.
				// compileExpr(e->target, curMajorContext.curMinorContext.evalPurpose, std::make_shared<RegRefNode>(RegId::R0));
				// _insertIns(Opcode::AWAIT, std::make_shared<RegRefNode>(RegId::R0));
			}

			// TODO: Set evaluated type.

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
								t->elementType->sourceLocation,
								MessageType::Error,
								"Cannot deduce type of elements"));
					}

					if (e->args.size() != 1) {
						throw FatalCompilationError(
							Message(
								e->type->sourceLocation,
								MessageType::Error,
								"Invalid argument number for array constructor"));
					}

					auto sizeArg = e->args[0];
					auto sizeArgType = evalExprType(sizeArg);

					auto u32Type = std::make_shared<U32TypeNameNode>(SIZE_MAX);

					if (!sizeArgType) {
						throw FatalCompilationError(
							Message(
								sizeArg->sourceLocation,
								MessageType::Error,
								"Cannot deduce type of the argument"));
					}

					uint32_t sizeArgRegIndex = allocReg(1);

					if (!isSameType(sizeArgType, u32Type)) {
						if (!isTypeNamesConvertible(sizeArgType, u32Type))
							throw FatalCompilationError(
								{ sizeArg->sourceLocation,
									MessageType::Error,
									"Incompatible argument type" });
						compileExpr(
							std::make_shared<CastExprNode>(u32Type, sizeArg),
							EvalPurpose::RValue,
							std::make_shared<RegRefNode>(sizeArgRegIndex));
					} else {
						compileExpr(
							sizeArg,
							EvalPurpose::RValue,
							std::make_shared<RegRefNode>(sizeArgRegIndex));
					}

					_insertIns(
						Opcode::ARRNEW,
						{ curMajorContext.curMinorContext.evalDest,
							t->elementType,
							std::make_shared<RegRefNode>(sizeArgRegIndex, true) });

					break;
				}
				case TypeId::Custom: {
					//
					// Instantiate a custom type.
					//
					auto node = resolveCustomTypeName((CustomTypeNameNode *)e->type.get());

					switch (node->getNodeType()) {
						case NodeType::Class:
						case NodeType::Interface: {
							std::shared_ptr<MemberNode> n = std::static_pointer_cast<MemberNode>(node);

							if (auto it = n->scope->members.find("new"); it != n->scope->members.end()) {
								std::deque<std::shared_ptr<TypeNameNode>> argTypes;

								for (auto &i : e->args) {
									auto t = evalExprType(i);
									if (!t)
										throw FatalCompilationError(
											Message(
												i->sourceLocation,
												MessageType::Error,
												"Error deducing type of the argument"));
									argTypes.push_back(t);
								}

								assert(it->second->getNodeType() == NodeType::Fn);

								std::shared_ptr<FnOverloadingNode> overloading;

								{
									auto overloadings = argDependentLookup((FnNode *)it->second.get(), argTypes, {});
									if (!overloadings.size()) {
										throw FatalCompilationError(
											Message(
												expr->sourceLocation,
												MessageType::Error,
												"No matching function was found"));
									} else if (overloadings.size() > 1) {
										for (auto i : overloadings) {
											messages.push_back(
												Message(
													i->sourceLocation,
													MessageType::Note,
													"Matched here"));
										}
										throw FatalCompilationError(
											Message(
												expr->sourceLocation,
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
										if (isSameType(argTypes[i], paramTypes[i])) {
											if (auto ce = evalConstExpr(e->args[i]); ce)
												_insertIns(Opcode::PUSHARG, { ce, paramTypes[i] });
											else {
												uint32_t tmpRegIndex = allocReg();

												compileExpr(e->args[i], EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
												_insertIns(Opcode::PUSHARG, { std::make_shared<RegRefNode>(tmpRegIndex, true), paramTypes[i] });
											}
										} else {
											uint32_t tmpRegIndex = allocReg();

											compileExpr(
												std::make_shared<CastExprNode>(argTypes[i], e->args[i]),
												EvalPurpose::RValue,
												std::make_shared<RegRefNode>(tmpRegIndex));
											_insertIns(Opcode::PUSHARG, { std::make_shared<RegRefNode>(tmpRegIndex, true), paramTypes[i] });
										}
									} else {
										if (auto ce = evalConstExpr(e->args[i]); ce)
											_insertIns(Opcode::PUSHARG, { ce, {} });
										else {
											uint32_t tmpRegIndex = allocReg();

											compileExpr(e->args[i], EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
											_insertIns(Opcode::PUSHARG, { std::make_shared<RegRefNode>(tmpRegIndex, true), {} });
										}
									}
								}

								IdRef fullName = getFullName(overloading.get());

								_insertIns(
									Opcode::NEW,
									{ curMajorContext.curMinorContext.evalDest,
										e->type,
										std::make_shared<IdRefExprNode>(fullName) });
							} else {
								_insertIns(
									Opcode::NEW,
									{ curMajorContext.curMinorContext.evalDest,
										e->type,
										{} });
							}

							break;
						}
						case NodeType::GenericParam:
							throw FatalCompilationError(
								Message(
									e->type->sourceLocation,
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
							e->type->sourceLocation,
							MessageType::Error,
							"Specified type is not constructible"));
			}

			curMajorContext.curMinorContext.evaluatedType = e->type;

			break;
		}
		case ExprType::Typeof: {
			auto e = std::static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				_insertIns(
					Opcode::TYPEOF,
					{ curMajorContext.curMinorContext.evalDest,
						ce });
			} else {
				uint32_t tmpRegIndex = allocReg();

				compileExpr(e->target, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
				_insertIns(
					Opcode::TYPEOF,
					{ curMajorContext.curMinorContext.evalDest,
						std::make_shared<RegRefNode>(tmpRegIndex, true) });
			}

			// TODO: Set evaluated type.

			break;
		}
		case ExprType::Cast: {
			auto e = std::static_pointer_cast<CastExprNode>(expr);

			curMajorContext.curMinorContext.expectedType = e->targetType;

			if (auto ce = evalConstExpr(e->target); ce) {
				if (isTypeNamesConvertible(evalExprType(ce), e->targetType)) {
					_insertIns(Opcode::CAST, { curMajorContext.curMinorContext.evalDest, e->targetType, ce });
				} else {
					throw FatalCompilationError({ e->sourceLocation, MessageType::Error, "Invalid type conversion" });
				}
			} else {
				auto originalType = evalExprType(e->target);

				if (!originalType)
					break;

				if (isTypeNamesConvertible(originalType, e->targetType)) {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(e->target, EvalPurpose::RValue, std::make_shared<RegRefNode>(tmpRegIndex));
					_insertIns(Opcode::CAST, { curMajorContext.curMinorContext.evalDest, e->targetType, std::make_shared<RegRefNode>(tmpRegIndex, true) });
				} else {
					throw FatalCompilationError({ e->sourceLocation, MessageType::Error, "Invalid type conversion" });
				}
			}

			curMajorContext.curMinorContext.evaluatedType = e->targetType;

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

			std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> resolvedParts;
			if (!resolveIdRef(e->ref, resolvedParts))
				throw FatalCompilationError(
					{ e->sourceLocation,
						MessageType::Error,
						"Identifier not found: `" + std::to_string(e->ref, this) + "'" });

			uint32_t tmpRegIndex = allocReg();

			auto determineOverloadingRegistry = [this, expr, &resolvedParts](std::shared_ptr<FnNode> x, const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs) -> std::shared_ptr<FnOverloadingNode> {
				if ((resolvedParts.size() > 2) || (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Call)) {
					//
					// Reference to a overloaded function is always ambiguous,
					// because we cannot determine which overloading is the user wanted.
					//
					if (x->overloadingRegistries.size() > 1) {
						throw FatalCompilationError(
							Message(
								expr->sourceLocation,
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
						x.get(),
						curMajorContext.curMinorContext.argTypes,
						genericArgs,
						curMajorContext.curMinorContext.isLastResolvedTargetStatic);

					if (!overloadings.size()) {
						throw FatalCompilationError(
							Message(
								expr->sourceLocation,
								MessageType::Error,
								"No matching function was found"));
					} else if (overloadings.size() > 1) {
						for (auto i : overloadings) {
							messages.push_back(
								Message(
									i->sourceLocation,
									MessageType::Note,
									"Matched here"));
						}
						throw FatalCompilationError(
							Message(
								expr->sourceLocation,
								MessageType::Error,
								"Ambiguous function call"));
					}

					curMajorContext.curMinorContext.lastCallTargetParams = overloadings[0]->params;
					// stub, TODO: Set a flag to hint that the last call target has varidic parameters.
					if (overloadings[0]->isVaridic())
						curMajorContext.curMinorContext.lastCallTargetParams.pop_back();

					curMajorContext.curMinorContext.lastCallTargetReturnType =
						overloadings[0]->returnType
							? overloadings[0]->returnType
							: std::make_shared<AnyTypeNameNode>(SIZE_MAX);
					curMajorContext.curMinorContext.isLastCallTargetStatic = overloadings[0]->access & ACCESS_STATIC;
					return overloadings[0];
				}
			};

			auto loadRest = [this, &determineOverloadingRegistry, &expr, &tmpRegIndex, &resolvedParts]() {
				for (size_t i = 1; i < resolvedParts.size(); ++i) {
					switch (resolvedParts[i].second->getNodeType()) {
						case NodeType::Var: {
							auto varNode = std::static_pointer_cast<VarNode>(resolvedParts[i].second);

							_insertIns(
								Opcode::RLOAD,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<RegRefNode>(tmpRegIndex, true),
									std::make_shared<IdRefExprNode>(resolvedParts[i].first) });

							if (varNode->type ? isLValueType(varNode->type) : false) {
								// Load once more for reference types.
								_insertIns(
									Opcode::LVALUE,
									{ std::make_shared<RegRefNode>(tmpRegIndex),
										std::make_shared<RegRefNode>(tmpRegIndex, true) });
							}

							// Intermediate scopes should always be loaded as rvalue.
							if ((i + 1 < resolvedParts.size()))
								_insertIns(
									Opcode::LVALUE,
									{ std::make_shared<RegRefNode>(tmpRegIndex),
										std::make_shared<RegRefNode>(tmpRegIndex, true) });
							else {
								curMajorContext.curMinorContext.evaluatedType = varNode->type->duplicate<TypeNameNode>();
								curMajorContext.curMinorContext.evaluatedType->isRef = true;
							}
							break;
						}
						case NodeType::Fn: {
							std::shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(
								std::static_pointer_cast<FnNode>(resolvedParts[i].second),
								resolvedParts[i].first.back().genericArgs);

							if (!curMajorContext.curMinorContext.isLastCallTargetStatic) {
								_insertIns(
									Opcode::STORE,
									{ curMajorContext.curMinorContext.thisDest,
										std::make_shared<RegRefNode>(tmpRegIndex, true) });
							}

#if SLKC_WITH_LANGUAGE_SERVER
							updateTokenInfo(resolvedParts[i].first.back().idxToken, [&overloading](TokenInfo &tokenInfo) {
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

								curMajorContext.curMinorContext.evaluatedType =
									std::make_shared<FnTypeNameNode>(
										overloading->returnType,
										paramTypes);
							}

							if (overloading->isVirtual)
								_insertIns(
									Opcode::RLOAD,
									{ std::make_shared<RegRefNode>(tmpRegIndex),
										std::make_shared<RegRefNode>(tmpRegIndex, true),
										std::make_shared<IdRefExprNode>(resolvedParts[i].first) });
							else {
								IdRef fullRef = getFullName(overloading.get());
								_insertIns(
									Opcode::LOAD,
									{ std::make_shared<RegRefNode>(tmpRegIndex),
										std::make_shared<IdRefExprNode>(fullRef) });
							}
							break;
						}
						default:
							assert(false);
					}
				}
			};

			auto &x = resolvedParts.front().second;
			switch (x->getNodeType()) {
				case NodeType::LocalVar: {
					auto localVarNode = std::static_pointer_cast<LocalVarNode>(x);

					_insertIns(
						Opcode::STORE,
						{ std::make_shared<RegRefNode>(tmpRegIndex),
							std::make_shared<LocalVarRefNode>(
								localVarNode->index,
								resolvedParts.size() > 1
									? true
									: curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue) });

					if (resolvedParts.size() == 1) {
						curMajorContext.curMinorContext.evaluatedType = localVarNode->type->duplicate<TypeNameNode>();
						curMajorContext.curMinorContext.evaluatedType->isRef = true;
					}

					loadRest();
					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							_insertIns(
								Opcode::LVALUE,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<RegRefNode>(tmpRegIndex, true) });
					}

					if (curMajorContext.curMinorContext.evalDest)
						_insertIns(
							Opcode::STORE,
							{ curMajorContext.curMinorContext.evalDest,
								std::make_shared<RegRefNode>(tmpRegIndex, true) });
					break;
				}
				case NodeType::ArgRef: {
					auto argRef = std::static_pointer_cast<ArgRefNode>(x);

					argRef->unwrapData = (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue);
					_insertIns(
						Opcode::STORE,
						{ std::make_shared<RegRefNode>(tmpRegIndex), x });

					auto paramNode = curFn->params[argRef->index];

					if (paramNode->type ? isLValueType(paramNode->type) : false) {
						// Load once more for reference types.
						_insertIns(
							Opcode::LVALUE,
							{ std::make_shared<RegRefNode>(tmpRegIndex),
								std::make_shared<RegRefNode>(tmpRegIndex, true) });
					}

					if (resolvedParts.size() == 1) {
						curMajorContext.curMinorContext.evaluatedType = paramNode->type->duplicate<TypeNameNode>();
						curMajorContext.curMinorContext.evaluatedType->isRef = true;
					}

					loadRest();
					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							_insertIns(
								Opcode::LVALUE,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<RegRefNode>(tmpRegIndex, true) });
					}

					if (curMajorContext.curMinorContext.evalDest)
						_insertIns(
							Opcode::STORE,
							{ curMajorContext.curMinorContext.evalDest, std::make_shared<RegRefNode>(tmpRegIndex, true) });
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
							IdRef ref = getFullName((MemberNode *)x.get());
							_insertIns(
								Opcode::LOAD,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<IdRefExprNode>(ref) });

							if (resolvedParts.size() == 1) {
								throw FatalCompilationError(
									Message(
										e->sourceLocation,
										MessageType::Error,
										"`" + std::to_string(e->ref, this) + "' is a type"));
							}
							break;
						}
						case NodeType::Var: {
							auto varNode = (VarNode *)x.get();

							IdRef ref = getFullName(varNode);
							_insertIns(
								Opcode::LOAD,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<IdRefExprNode>(ref) });

							if (resolvedParts.size() == 1) {
								curMajorContext.curMinorContext.evaluatedType = varNode->type->duplicate<TypeNameNode>();
								curMajorContext.curMinorContext.evaluatedType->isRef = true;
							}
							break;
						}
						case NodeType::Fn: {
							IdRef ref;

							FnNode *fn = (FnNode *)x.get();

							std::shared_ptr<FnOverloadingNode> overloading = determineOverloadingRegistry(std::static_pointer_cast<FnNode>(x), resolvedParts.front().first.back().genericArgs);
							_getFullName(overloading.get(), ref);

#if SLKC_WITH_LANGUAGE_SERVER
							updateTokenInfo(resolvedParts.front().first.back().idxToken, [&overloading](TokenInfo &tokenInfo) {
								tokenInfo.semanticInfo.correspondingMember = overloading;
							});
#endif

							std::deque<std::shared_ptr<TypeNameNode>> paramTypes;
							for (auto i : overloading->params) {
								paramTypes.push_back(i->type);
							}

							if (overloading->isVaridic())
								paramTypes.pop_back();

							if (resolvedParts.size() == 1) {
								curMajorContext.curMinorContext.evaluatedType =
									std::make_shared<FnTypeNameNode>(
										overloading->returnType,
										paramTypes);
							}

							_insertIns(
								Opcode::LOAD,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<IdRefExprNode>(ref) });
							break;
						}
						case NodeType::ThisRef: {
							auto owner = curMajorContext.curMinorContext.curScope->owner;

							switch (owner->getNodeType()) {
								case NodeType::Class:
								case NodeType::Interface: {
									_insertIns(Opcode::LTHIS, { std::make_shared<RegRefNode>(tmpRegIndex) });
									curMajorContext.curMinorContext.evaluatedType = curMajorContext.thisType;
									break;
								}
								default:
									throw FatalCompilationError(
										Message(
											e->sourceLocation,
											MessageType::Error,
											"Cannot use this reference in this context"));
							}

							break;
						}
						case NodeType::BaseRef:
							_insertIns(
								Opcode::LOAD,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<IdRefExprNode>(IdRef{ IdRefEntry(e->sourceLocation, SIZE_MAX, "base") }) });
							break;
						default:
							assert(false);
					}

					// Check if the target is static.
					loadRest();

					if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							_insertIns(
								Opcode::LVALUE,
								{ std::make_shared<RegRefNode>(tmpRegIndex),
									std::make_shared<RegRefNode>(tmpRegIndex, true) });
					}

					if (curMajorContext.curMinorContext.evalDest)
						_insertIns(
							Opcode::STORE,
							{ curMajorContext.curMinorContext.evalDest,
								std::make_shared<RegRefNode>(tmpRegIndex, true) });
					break;
				}
				default:
					assert(false);
			}

			break;
		}
		case ExprType::Array: {
			auto e = std::static_pointer_cast<ArrayExprNode>(expr);

			if ((!curMajorContext.curMinorContext.expectedType) ||
				curMajorContext.curMinorContext.expectedType->getTypeId() != TypeId::Array)
				throw FatalCompilationError(
					Message(
						e->sourceLocation,
						MessageType::Error,
						"Error deducing type of the expression"));

			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->sourceLocation,
						MessageType::Error,
						"Expecting a lvalue expression"));

			auto type = std::static_pointer_cast<ArrayTypeNameNode>(curMajorContext.curMinorContext.expectedType);

#if SLKC_WITH_LANGUAGE_SERVER
			updateTokenInfo(e->idxLBraceToken, [this](TokenInfo &tokenInfo) {
				tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
				tokenInfo.completionContext = CompletionContext::Expr;
			});

			for (size_t i = 0; i < e->idxCommaTokens.size(); ++i) {
				updateTokenInfo(e->idxCommaTokens[i], [this](TokenInfo &tokenInfo) {
					tokenInfo.tokenContext = TokenContext(curFn, curMajorContext);
					tokenInfo.completionContext = CompletionContext::Expr;
				});
			}
#endif

			// Check type of the members.
			for (auto &i : e->elements) {
				auto t = evalExprType(i);
				if (!t)
					throw FatalCompilationError(
						Message(
							i->sourceLocation,
							MessageType::Error,
							"Error deducing the element type"));

				if (!isSameType(t, type->elementType)) {
					if (!isTypeNamesConvertible(t, type->elementType))
						throw FatalCompilationError(
							Message(
								i->sourceLocation,
								MessageType::Error,
								"Incompatible element type"));
				}
			}

			curMajorContext.curMinorContext.evaluatedType = type;

			if (auto ce = evalConstExpr(e); ce) {
				_insertIns(Opcode::STORE, { curMajorContext.curMinorContext.evalDest, ce });
			} else {
				auto initArray = std::make_shared<ArrayExprNode>();
				initArray->sourceLocation = e->sourceLocation;
				initArray->elements.resize(e->elements.size());

				initArray->evaluatedElementType = type->elementType;

				// We use nullptr to represent non-constexpr expressions.
				for (size_t i = 0; i < e->elements.size(); ++i) {
					initArray->elements[i] = evalConstExpr(initArray->elements[i]);
				}

				uint32_t idxTmpArrayRegIndex = allocReg(),
						 idxTmpElementRegIndex = allocReg();

				_insertIns(
					Opcode::STORE,
					{ std::make_shared<RegRefNode>(idxTmpArrayRegIndex),
						initArray });

				// Assign non-constexpr expressions to corresponding elements.
				for (size_t i = 0; i < initArray->elements.size(); ++i) {
					if (!initArray->elements[i]) {
						_insertIns(
							Opcode::AT,
							{ std::make_shared<RegRefNode>(idxTmpElementRegIndex),
								std::make_shared<RegRefNode>(idxTmpArrayRegIndex, true),
								std::make_shared<U32LiteralExprNode>(idxTmpElementRegIndex, SIZE_MAX) });
						compileExpr(
							e->elements[i],
							EvalPurpose::RValue,
							std::make_shared<RegRefNode>(idxTmpElementRegIndex, true));
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
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->sourceLocation,
						MessageType::Error,
						"Expecting a lvalue expression"));

			_insertIns(Opcode::STORE, { curMajorContext.curMinorContext.evalDest, expr });

			switch (expr->getExprType()) {
				case ExprType::I8:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I8TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::I16:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I16TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::I32:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I32TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::I64:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<I64TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::U8:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U8TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::U16:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U16TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::U32:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U32TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::U64:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<U64TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::F32:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<F32TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::F64:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<F64TypeNameNode>(SIZE_MAX);
					break;
				case ExprType::String:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<StringTypeNameNode>(SIZE_MAX);
					break;
				case ExprType::Bool:
					curMajorContext.curMinorContext.evaluatedType = std::make_shared<BoolTypeNameNode>(SIZE_MAX);
					break;
			}
			break;
		}
		case ExprType::Null:
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::LValue)
				throw FatalCompilationError(
					Message(
						expr->sourceLocation,
						MessageType::Error,
						"Expecting a lvalue expression"));

			_insertIns(Opcode::STORE, { curMajorContext.curMinorContext.evalDest, expr });

			curMajorContext.curMinorContext.evaluatedType = std::make_shared<AnyTypeNameNode>(SIZE_MAX);
			break;
		case ExprType::Bad:
			break;
		default:
			assert(false);
	}

	if (!curMajorContext.curMinorContext.dryRun) {
		sld.nIns = curFn->body.size() - sld.offIns;
		curFn->srcLocDescs.push_back(sld);
	}
}
