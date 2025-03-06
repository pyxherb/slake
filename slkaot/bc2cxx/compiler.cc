#include "../bc2cxx.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

SLAKE_FORCEINLINE bool BC2CXX::CompileContext::allocRecycledReg(BC2CXX &bc2cxx, const opti::ProgramAnalyzedInfo &analyzedInfo, uint32_t reg, const Type &type) {
	VirtualRegInfo &targetVregInfo = vregInfo.at(reg);
	for (auto i : recycledRegs) {
		const opti::RegAnalyzedInfo &regInfo = analyzedInfo.analyzedRegInfo.at(i);

		switch (regInfo.type.typeId) {
			case TypeId::I8:
			case TypeId::I16:
			case TypeId::I32:
			case TypeId::I64:
			case TypeId::U8:
			case TypeId::U16:
			case TypeId::U32:
			case TypeId::U64:
			case TypeId::F32:
			case TypeId::F64:
			case TypeId::Bool:
				if (regInfo.type == type)
					goto succeeded;
				break;
			case TypeId::String:
			case TypeId::Instance:
			case TypeId::Array:
			case TypeId::FnDelegate:
				switch (type.typeId) {
					case TypeId::String:
					case TypeId::Instance:
					case TypeId::Array:
					case TypeId::FnDelegate:
						goto succeeded;
					default:;
				}
				break;
			case TypeId::Ref: {
				if (type.typeId == TypeId::Ref) {
					goto succeeded;
				}
				break;
			}
			default:
				std::terminate();
		}

		continue;
	succeeded:
		printf("Reused register #%u for #%u\n", i, reg);
		targetVregInfo.vregVarName = bc2cxx.mangleRegLocalVarName(i);
		targetVregInfo.nameBorrowedReg = i;
		recycledRegs.erase(i);
		return true;
	}
	return false;
}

void BC2CXX::recompileFnOverloading(CompileContext &compileContext, std::shared_ptr<cxxast::Fn> fnOverloading) {
	compileContext.resetForCompilation();

	FnOverloadingObject *fnOverloadingObject = fnOverloading->rtOverloading.get();

	switch (fnOverloadingObject->overloadingKind) {
		case FnOverloadingKind::Regular: {
			RegularFnOverloadingObject *fo = (RegularFnOverloadingObject *)fnOverloadingObject;
			std::vector<std::shared_ptr<cxxast::Stmt>> *curStmtContainer = &fnOverloading->body;
			std::list<std::vector<std::shared_ptr<cxxast::Stmt>> *> curStmtContainerStack;
			std::list<size_t> branchBlockLeavingBoundaries;
			auto pushCurStmtContainer = [&curStmtContainer, &curStmtContainerStack]() {
				curStmtContainerStack.push_back(curStmtContainer);
			};
			auto popCurStmtContainer = [&curStmtContainer, &curStmtContainerStack]() {
				curStmtContainer = curStmtContainerStack.back();
				curStmtContainerStack.pop_back();
			};

			opti::ProgramAnalyzedInfo programInfo(compileContext.runtime);
			HostRefHolder hostRefHolder(peff::getDefaultAlloc());
			InternalExceptionPointer e = opti::analyzeProgramInfo(compileContext.runtime, fo, programInfo, hostRefHolder);
			if (e) {
				fprintf(stderr, "Error analyzing the program: %s\n", e->what());
				e.reset();
				return;
			}

			for (size_t i = 0; i < fo->instructions.size(); ++i) {
				Instruction &ins = fo->instructions.at(i);

				if (branchBlockLeavingBoundaries.size() && i == branchBlockLeavingBoundaries.back()) {
					popCurStmtContainer();
					branchBlockLeavingBoundaries.pop_back();
				}

				curStmtContainer->push_back(
					std::make_shared<cxxast::LabelStmt>(mangleJumpDestLabelName(i)));

				if (ins.output.valueType != ValueType::Undefined) {
					opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());

					compileContext.defineVirtualReg(ins.output.getRegIndex(), mangleRegLocalVarName(ins.output.getRegIndex()));

					compileContext.recyclableRegs[programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).lifetime.offEndIns].insert(ins.output.getRegIndex());
				}

				switch (ins.opcode) {
					case Opcode::NOP:
						break;
					case Opcode::LOAD: {
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[0].getEntityRef().asObject.instanceObject;

						compileContext.mappedObjects.insert(id.get());

						switch (outputRegInfo.expectedValue.valueType) {
							case ValueType::EntityRef: {
								EntityRef &entityRef = outputRegInfo.expectedValue.getEntityRef();

								switch (entityRef.kind) {
									case ObjectRefKind::ObjectRef: {
										Object *object = entityRef.asObject.instanceObject;

										std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

										if (auto astNode = getMappedAstNode(object);
											astNode) {
											std::shared_ptr<cxxast::Expr> rhs = std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
												std::make_shared<cxxast::CastExpr>(
													std::make_shared<cxxast::PointerTypeName>(
														std::make_shared<cxxast::CustomTypeName>(
															false,
															std::make_shared<cxxast::BinaryExpr>(
																cxxast::BinaryOp::Scope,
																_getAbsRef(compileContext.rootNamespace),
																std::make_shared<cxxast::IdExpr>("MappedObjects")))),
													genMappedObjectsRef()),
												std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(object)));

											if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
												curStmtContainer->push_back(
													std::make_shared<cxxast::ExprStmt>(
														std::make_shared<cxxast::BinaryExpr>(
															cxxast::BinaryOp::Assign,
															std::make_shared<cxxast::IdExpr>(std::string(varName)),
															rhs)));
											} else {
												cxxast::VarDefPair varDefPair;

												// Check if the object is already mapped in the module.
												// If so, we can just simply use the reference to the native member.
												varDefPair = {
													varName,
													rhs
												};

												std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(genInstanceObjectTypeName(), std::vector<cxxast::VarDefPair>{ varDefPair });

												compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));

												curStmtContainer->push_back(stmt);
											}
										} else {
											std::shared_ptr<cxxast::Expr> rhs;

											if (!compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), Type(TypeId::Ref, nullptr))) {
												cxxast::VarDefPair varDefPair;

												varDefPair = { varName, {} };

												std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(genObjectRefTypeName(), std::vector<cxxast::VarDefPair>{ varDefPair });

												compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Ref, nullptr)));

												curStmtContainer->push_back(stmt);
											}

											rhs = std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName));

											curStmtContainer->push_back(genReturnIfExceptStmt(
												std::make_shared<cxxast::CallExpr>(
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::MemberAccess,
														genAotContextRef(),
														std::make_shared<cxxast::BinaryExpr>(
															cxxast::BinaryOp::PtrAccess,
															std::make_shared<cxxast::IdExpr>("runtime"),
															std::make_shared<cxxast::IdExpr>("resolveIdRef"))),
													std::vector<std::shared_ptr<cxxast::Expr>>{
														std::make_shared<cxxast::CastExpr>(
															genIdRefObjectTypeName(),
															std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
																std::make_shared<cxxast::CastExpr>(
																	std::make_shared<cxxast::PointerTypeName>(
																		std::make_shared<cxxast::CustomTypeName>(
																			false,
																			std::make_shared<cxxast::BinaryExpr>(
																				cxxast::BinaryOp::Scope,
																				_getAbsRef(compileContext.rootNamespace),
																				std::make_shared<cxxast::IdExpr>("MappedObjects")))),
																	genMappedObjectsRef()),
																std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(id.get())))),
														rhs })));

											compileContext.markVirtualRegAsLoadInsResult(ins.output.getRegIndex());
										}
										break;
									}
									case ObjectRefKind::FieldRef: {
										FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

										std::shared_ptr<cxxast::TypeName> t = compileType(compileContext, fieldRecord.type);

										if (!compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
											cxxast::VarDefPair varDefPair = { std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName), {} };

											std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(t, std::vector<cxxast::VarDefPair>{ varDefPair });

											compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));

											curStmtContainer->push_back(stmt);
										}

										curStmtContainer->push_back(genReturnIfExceptStmt(
											std::make_shared<cxxast::CallExpr>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::MemberAccess,
													genAotContextRef(),
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::PtrAccess,
														std::make_shared<cxxast::IdExpr>("runtime"),
														std::make_shared<cxxast::IdExpr>("resolveIdRef"))),
												std::vector<std::shared_ptr<cxxast::Expr>>{
													std::make_shared<cxxast::CastExpr>(
														genIdRefObjectTypeName(),
														std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
															std::make_shared<cxxast::CastExpr>(
																std::make_shared<cxxast::PointerTypeName>(
																	std::make_shared<cxxast::CustomTypeName>(
																		false,
																		std::make_shared<cxxast::BinaryExpr>(
																			cxxast::BinaryOp::Scope,
																			_getAbsRef(compileContext.rootNamespace),
																			std::make_shared<cxxast::IdExpr>("MappedObjects")))),
																genMappedObjectsRef()),
															std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(id.get())))),
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)) })));
										break;
									}
									default:
										std::terminate();
								}
								break;
							}
						}
						break;
					}
					case Opcode::RLOAD: {
						uint32_t idxBaseReg = ins.operands[0].getRegIndex();
						HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[1].getEntityRef().asObject.instanceObject;

						compileContext.mappedObjects.insert((Object *)id.get());

						if (!compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), Type(TypeId::Ref, nullptr))) {
							cxxast::VarDefPair varDefPair = { std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName), {} };

							std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(genObjectRefTypeName(), std::vector<cxxast::VarDefPair>{ varDefPair });

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Ref, nullptr)));

							curStmtContainer->push_back(stmt);
						}

						curStmtContainer->push_back(genReturnIfExceptStmt(
							std::make_shared<cxxast::CallExpr>(
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::MemberAccess,
									genAotContextRef(),
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::PtrAccess,
										std::make_shared<cxxast::IdExpr>("runtime"),
										std::make_shared<cxxast::IdExpr>("resolveIdRef"))),
								std::vector<std::shared_ptr<cxxast::Expr>>{
									std::make_shared<cxxast::CastExpr>(
										genIdRefObjectTypeName(),
										std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
											std::make_shared<cxxast::CastExpr>(
												std::make_shared<cxxast::PointerTypeName>(
													std::make_shared<cxxast::CustomTypeName>(
														false,
														std::make_shared<cxxast::BinaryExpr>(
															cxxast::BinaryOp::Scope,
															_getAbsRef(compileContext.rootNamespace),
															std::make_shared<cxxast::IdExpr>("MappedObjects")))),
												genMappedObjectsRef()),
											std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(id.get())))),
									std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
									std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(idxBaseReg).vregVarName)) })));

						compileContext.markVirtualRegAsLoadInsResult(ins.output.getRegIndex());
						break;
					}
					case Opcode::STORE: {
						opti::RegAnalyzedInfo &regInfo = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex());

						std::shared_ptr<cxxast::Expr> lhs, rhs;

						switch (regInfo.storageType) {
							case opti::RegStorageType::None:
								lhs =
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
											std::make_shared<cxxast::IdExpr>("asAotPtr")),
										std::make_shared<cxxast::IdExpr>("ptr"));
								switch (ins.operands[1].valueType) {
									case ValueType::I8:
									case ValueType::I16:
									case ValueType::I32:
									case ValueType::I64:
									case ValueType::U8:
									case ValueType::U16:
									case ValueType::U32:
									case ValueType::U64:
									case ValueType::F32:
									case ValueType::F64:
									case ValueType::Bool:
									case ValueType::EntityRef:
										lhs =
											std::make_shared<cxxast::UnaryExpr>(
												cxxast::UnaryOp::Dereference,
												std::make_shared<cxxast::CastExpr>(
													std::make_shared<cxxast::PointerTypeName>(compileType(compileContext, valueTypeToTypeId(ins.operands[1].valueType))),
													lhs));
										rhs = compileValue(compileContext, ins.operands[1]);
										break;
									case ValueType::RegRef:
										lhs =
											std::make_shared<cxxast::UnaryExpr>(
												cxxast::UnaryOp::Dereference,
												std::make_shared<cxxast::CastExpr>(
													std::make_shared<cxxast::PointerTypeName>(compileType(compileContext, programInfo.analyzedRegInfo.at(ins.operands[1].getRegIndex()).type)),
													lhs));
										rhs = std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[1].getRegIndex()).vregVarName));
										break;
									default:
										std::terminate();
								}
								break;
							case opti::RegStorageType::FieldVar:
								lhs =
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
											std::make_shared<cxxast::IdExpr>("asAotPtr")),
										std::make_shared<cxxast::IdExpr>("ptr"));
								// TODO: Implement it.
								break;
							case opti::RegStorageType::LocalVar:
								lhs = std::make_shared<cxxast::IdExpr>(mangleLocalVarName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asLocalVar.definitionReg));
								break;
							case opti::RegStorageType::ArgRef:
								lhs = std::make_shared<cxxast::IdExpr>(mangleParamName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).expectedValue.getEntityRef().asArg.argIndex));
								break;
						}

						switch (regInfo.storageType) {
							case opti::RegStorageType::None:
								lhs =
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
											std::make_shared<cxxast::IdExpr>("asAotPtr")),
										std::make_shared<cxxast::IdExpr>("ptr"));
								switch (ins.operands[1].valueType) {
									case ValueType::I8:
									case ValueType::I16:
									case ValueType::I32:
									case ValueType::I64:
									case ValueType::U8:
									case ValueType::U16:
									case ValueType::U32:
									case ValueType::U64:
									case ValueType::F32:
									case ValueType::F64:
									case ValueType::Bool:
									case ValueType::EntityRef:
										lhs =
											std::make_shared<cxxast::UnaryExpr>(
												cxxast::UnaryOp::Dereference,
												std::make_shared<cxxast::CastExpr>(
													std::make_shared<cxxast::PointerTypeName>(compileType(compileContext, valueTypeToTypeId(ins.operands[1].valueType))),
													lhs));
										break;
									case ValueType::RegRef:
										lhs =
											std::make_shared<cxxast::UnaryExpr>(
												cxxast::UnaryOp::Dereference,
												std::make_shared<cxxast::CastExpr>(
													std::make_shared<cxxast::PointerTypeName>(compileType(compileContext, programInfo.analyzedRegInfo.at(ins.operands[1].getRegIndex()).type)),
													lhs));
										break;
									default:
										std::terminate();
								}
								break;
							case opti::RegStorageType::FieldVar:
								lhs =
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
											std::make_shared<cxxast::IdExpr>("asAotPtr")),
										std::make_shared<cxxast::IdExpr>("ptr"));
								// TODO: Implement it.
								break;
							case opti::RegStorageType::LocalVar:
								lhs = std::make_shared<cxxast::IdExpr>(mangleLocalVarName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asLocalVar.definitionReg));
								break;
							case opti::RegStorageType::ArgRef:
								lhs = std::make_shared<cxxast::IdExpr>(mangleParamName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).expectedValue.getEntityRef().asArg.argIndex));
								break;
						}

						rhs = std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[1].getRegIndex()).vregVarName));

						curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
							std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::Assign,
								lhs,
								rhs)));
						break;
					}
					case Opcode::MOV: {
						std::shared_ptr<cxxast::Expr> rhs;
						std::shared_ptr<cxxast::TypeName> type;
						Type regType;

						std::vector<cxxast::VarDefPair> varDefPairs;

						switch (ins.operands[0].valueType) {
							case ValueType::I8:
							case ValueType::I16:
							case ValueType::I32:
							case ValueType::I64:
							case ValueType::U8:
							case ValueType::U16:
							case ValueType::U32:
							case ValueType::U64:
							case ValueType::F32:
							case ValueType::F64:
							case ValueType::Bool:
								regType = valueTypeToTypeId(ins.operands[0].valueType);
								type = compileType(compileContext, Type(valueTypeToTypeId(ins.operands[0].valueType)));
								rhs = compileValue(compileContext, ins.operands[0]);
								break;
							case ValueType::EntityRef: {
								EntityRef &entityRef = ins.operands[0].getEntityRef();

								switch (entityRef.kind) {
									case ObjectRefKind::ObjectRef:
										regType = Type(TypeId::Instance);
										compileContext.mappedObjects.insert(entityRef.asObject.instanceObject);
										type = genInstanceObjectTypeName();
										break;
									default:
										regType = Type(TypeId::Ref, nullptr);
										type = genObjectRefTypeName();
								}
								rhs = compileValue(compileContext, ins.operands[0]);
								break;
							}
							case ValueType::RegRef: {
								uint32_t regIndex = ins.operands[0].getRegIndex();

								regType = programInfo.analyzedRegInfo.at(regIndex).type;

								type = compileType(compileContext, regType);

								switch (regType.typeId) {
									case TypeId::String:
									case TypeId::Instance:
									case TypeId::Array:
									case TypeId::FnDelegate:
										if ((compileContext.getVirtualRegInfo(regIndex).isLoadInsResult)) {
											rhs = std::make_shared<cxxast::BinaryExpr>(
												cxxast::BinaryOp::MemberAccess,
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::MemberAccess,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(regIndex).vregVarName)),
													std::make_shared<cxxast::IdExpr>("asObject")),
												std::make_shared<cxxast::IdExpr>("instanceObject"));
										} else {
											rhs = std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(regIndex).vregVarName));
										}
										break;
									default:
										rhs = std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(regIndex).vregVarName));
								}
								break;
							}
							default:
								std::terminate();
						}

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), regType)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										rhs)));
						} else {
							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).type));

							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									type,
									std::vector<cxxast::VarDefPair>{
										{ compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName,
											rhs } }));
						}
						break;
					}
					case Opcode::LARG: {
						std::shared_ptr<cxxast::TypeName> t = genObjectRefTypeName();

						cxxast::VarDefPair varDefPair;

						varDefPair = { compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName,
							std::make_shared<cxxast::CallExpr>(
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Scope,
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::IdExpr>("slake"),
										std::make_shared<cxxast::IdExpr>("EntityRef")),
									std::make_shared<cxxast::IdExpr>("makeAotPtrRef")),
								std::vector<std::shared_ptr<cxxast::Expr>>{
									std::make_shared<cxxast::CastExpr>(
										std::make_shared<cxxast::PointerTypeName>(std::make_shared<cxxast::VoidTypeName>()),
										std::make_shared<cxxast::UnaryExpr>(
											cxxast::UnaryOp::AddressOf,
											std::make_shared<cxxast::IdExpr>(mangleParamName(ins.operands[0].getU32())))) }) };

						std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(t, std::vector<cxxast::VarDefPair>{ varDefPair });

						compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Ref, nullptr)));

						curStmtContainer->push_back(stmt);
						break;
					}
					case Opcode::LVAR: {
						std::shared_ptr<cxxast::Expr> rhs;
						std::shared_ptr<cxxast::TypeName> type;

						switch (ins.operands[0].getTypeName().typeId) {
							case TypeId::I8:
								rhs = compileValue(compileContext, Value((int8_t)0));
								break;
							case TypeId::I16:
								rhs = compileValue(compileContext, Value((int16_t)0));
								break;
							case TypeId::I32:
								rhs = compileValue(compileContext, Value((int32_t)0));
								break;
							case TypeId::I64:
								rhs = compileValue(compileContext, Value((int64_t)0));
								break;
							case TypeId::U8:
								rhs = compileValue(compileContext, Value((uint8_t)0));
								break;
							case TypeId::U16:
								rhs = compileValue(compileContext, Value((uint16_t)0));
								break;
							case TypeId::U32:
								rhs = compileValue(compileContext, Value((uint32_t)0));
								break;
							case TypeId::U64:
								rhs = compileValue(compileContext, Value((uint64_t)0));
								break;
							case TypeId::Bool:
								rhs = compileValue(compileContext, Value((bool)false));
								break;
							case TypeId::String:
							case TypeId::Instance:
							case TypeId::Array:
							case TypeId::FnDelegate:
								rhs = compileValue(compileContext, Value(EntityRef::makeObjectRef(nullptr)));
								break;
							case TypeId::Ref:
								// Should not be initialized
								break;
							case TypeId::Any:
								rhs = compileValue(compileContext, Value(EntityRef::makeObjectRef(nullptr)));
								break;
							default:
								std::terminate();
						}

						type = compileType(compileContext, ins.operands[0].getTypeName());

						std::string lvarName = mangleLocalVarName(ins.output.getRegIndex());

						compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(ins.operands[0].getTypeName()));

						curStmtContainer->push_back(
							std::make_shared<cxxast::LocalVarDefStmt>(
								type,
								std::vector<cxxast::VarDefPair>{
									{ lvarName,
										rhs } }));

						compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Ref, nullptr)));
						curStmtContainer->push_back(
							std::make_shared<cxxast::LocalVarDefStmt>(
								genObjectRefTypeName(),
								std::vector<cxxast::VarDefPair>{
									{ compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName,
										std::make_shared<cxxast::CallExpr>(
											std::make_shared<cxxast::BinaryExpr>(
												cxxast::BinaryOp::Scope,
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::Scope,
													std::make_shared<cxxast::IdExpr>("slake"),
													std::make_shared<cxxast::IdExpr>("EntityRef")),
												std::make_shared<cxxast::IdExpr>("makeAotPtrRef")),
											std::vector<std::shared_ptr<cxxast::Expr>>{
												std::make_shared<cxxast::CastExpr>(
													std::make_shared<cxxast::PointerTypeName>(std::make_shared<cxxast::VoidTypeName>()),
													std::make_shared<cxxast::UnaryExpr>(
														cxxast::UnaryOp::AddressOf,
														std::make_shared<cxxast::IdExpr>(std::string(lvarName)))) }) } }));
						break;
					}
					case Opcode::LVALUE: {
						if (ins.output.valueType != ValueType::Undefined) {
							opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex()),
												  &sourceRegInfo = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex());
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							switch (sourceRegInfo.storageType) {
								case opti::RegStorageType::None: {
									std::string tmpLocalVarName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName + "_tmp";
									compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Any)));
									curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(
										genAnyTypeName(),
										std::vector<cxxast::VarDefPair>{
											cxxast::VarDefPair{
												tmpLocalVarName,
												{} } }));

									curStmtContainer->push_back(
										genReturnIfExceptStmt(
											std::make_shared<cxxast::CallExpr>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::PtrAccess,
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::MemberAccess,
														genAotContextRef(),
														std::make_shared<cxxast::IdExpr>("runtime")),
													std::make_shared<cxxast::IdExpr>("readVar")),
												std::vector<std::shared_ptr<cxxast::Expr>>{
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
													std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName)) })));

									if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
										curStmtContainer->push_back(
											std::make_shared<cxxast::ExprStmt>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::Assign,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
													genGetValueDataExpr(
														outputRegInfo.type,
														std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName))))));
									} else {
										cxxast::VarDefPair varDefPair = {
											varName,
											genGetValueDataExpr(
												outputRegInfo.type,
												std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName)))
										};

										compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
										curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(compileType(compileContext, outputRegInfo.type), std::vector<cxxast::VarDefPair>{ varDefPair }));
									}

									break;
								}
								case opti::RegStorageType::FieldVar: {
									std::string tmpLocalVarName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName + "_tmp";
									compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Any)));
									curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(
										genAnyTypeName(),
										std::vector<cxxast::VarDefPair>{
											cxxast::VarDefPair{
												tmpLocalVarName,
												{} } }));

									curStmtContainer->push_back(
										genReturnIfExceptStmt(
											std::make_shared<cxxast::CallExpr>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::PtrAccess,
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::MemberAccess,
														genAotContextRef(),
														std::make_shared<cxxast::IdExpr>("runtime")),
													std::make_shared<cxxast::IdExpr>("readVar")),
												std::vector<std::shared_ptr<cxxast::Expr>>{
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
													std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName)) })));

									if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
										curStmtContainer->push_back(
											std::make_shared<cxxast::ExprStmt>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::Assign,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
													genGetValueDataExpr(
														outputRegInfo.type,
														std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName))))));
									} else {
										cxxast::VarDefPair varDefPair = {
											varName,
											genGetValueDataExpr(
												outputRegInfo.type,
												std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName)))
										};

										compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
										curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(compileType(compileContext, outputRegInfo.type), std::vector<cxxast::VarDefPair>{ varDefPair }));
									}
									break;
								}
								case opti::RegStorageType::InstanceFieldVar: {
									std::string tmpLocalVarName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName + "_tmp";
									compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Any)));
									curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(
										genAnyTypeName(),
										std::vector<cxxast::VarDefPair>{
											cxxast::VarDefPair{
												tmpLocalVarName,
												{} } }));

									curStmtContainer->push_back(
										genReturnIfExceptStmt(
											std::make_shared<cxxast::CallExpr>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::PtrAccess,
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::MemberAccess,
														genAotContextRef(),
														std::make_shared<cxxast::IdExpr>("runtime")),
													std::make_shared<cxxast::IdExpr>("readVar")),
												std::vector<std::shared_ptr<cxxast::Expr>>{
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
													std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName)) })));

									if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
										curStmtContainer->push_back(
											std::make_shared<cxxast::ExprStmt>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::Assign,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
													genGetValueDataExpr(
														outputRegInfo.type,
														std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName))))));
									} else {
										cxxast::VarDefPair varDefPair = {
											varName,
											genGetValueDataExpr(
												outputRegInfo.type,
												std::make_shared<cxxast::IdExpr>(std::string(tmpLocalVarName)))
										};

										compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
										curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(compileType(compileContext, outputRegInfo.type), std::vector<cxxast::VarDefPair>{ varDefPair }));
									}

									break;
								}
								case opti::RegStorageType::ArrayElement: {
									if (ins.operands[0].valueType != ValueType::RegRef) {
										// TODO: Implement it
									} else {
										opti::RegAnalyzedInfo &regInfo = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex());
										Type &elementType = regInfo.type.getArrayExData();

										std::shared_ptr<cxxast::Expr>
											indexRefExpr = std::make_shared<cxxast::BinaryExpr>(
												cxxast::BinaryOp::MemberAccess,
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::MemberAccess,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
													std::make_shared<cxxast::IdExpr>("asArray")),
												std::make_shared<cxxast::IdExpr>("index"));
										std::shared_ptr<cxxast::Expr> dataPtrExpr =
											std::make_shared<cxxast::CastExpr>(
												std::make_shared<cxxast::PointerTypeName>(
													compileType(compileContext, elementType)),
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::PtrAccess,
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::MemberAccess,
														std::make_shared<cxxast::BinaryExpr>(
															cxxast::BinaryOp::MemberAccess,
															std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
															std::make_shared<cxxast::IdExpr>("asArray")),
														std::make_shared<cxxast::IdExpr>("arrayObject")),
													std::make_shared<cxxast::IdExpr>("data")));
										std::shared_ptr<cxxast::Expr> elementExpr = std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Subscript,
											dataPtrExpr,
											indexRefExpr);

										if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
											curStmtContainer->push_back(
												std::make_shared<cxxast::ExprStmt>(
													std::make_shared<cxxast::BinaryExpr>(
														cxxast::BinaryOp::Assign,
														std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
														elementExpr)));
										} else {
											cxxast::VarDefPair varDefPair = {
												varName,
												elementExpr
											};

											compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
											curStmtContainer->push_back(
												std::make_shared<cxxast::LocalVarDefStmt>(
													compileType(compileContext, outputRegInfo.type),
													std::vector<cxxast::VarDefPair>{ varDefPair }));
										}
									}
									break;
								}
								case opti::RegStorageType::LocalVar: {
									if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
										curStmtContainer->push_back(
											std::make_shared<cxxast::ExprStmt>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::Assign,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
													std::make_shared<cxxast::IdExpr>(
														mangleLocalVarName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asLocalVar.definitionReg)))));
									} else {
										cxxast::VarDefPair varDefPair = {
											varName,
											std::make_shared<cxxast::IdExpr>(
												mangleLocalVarName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asLocalVar.definitionReg))
										};
										compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
										curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(compileType(compileContext, outputRegInfo.type), std::vector<cxxast::VarDefPair>{ varDefPair }));
									}
									break;
								}
								case opti::RegStorageType::ArgRef: {
									if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
										curStmtContainer->push_back(
											std::make_shared<cxxast::ExprStmt>(
												std::make_shared<cxxast::BinaryExpr>(
													cxxast::BinaryOp::Assign,
													std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
													std::make_shared<cxxast::IdExpr>(
														mangleParamName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asArgRef.idxArg)))));
									} else {
										cxxast::VarDefPair varDefPair = { varName,
											std::make_shared<cxxast::IdExpr>(
												mangleParamName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asArgRef.idxArg)) };

										compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
										curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(compileType(compileContext, outputRegInfo.type), std::vector<cxxast::VarDefPair>{ varDefPair }));
									}
									break;
								}
							}
							break;
						}
						break;
					}
					case Opcode::ENTER: {
						std::shared_ptr<cxxast::BlockStmt> blockStmt = std::make_shared<cxxast::BlockStmt>();

						curStmtContainer->push_back(blockStmt);

						pushCurStmtContainer();

						curStmtContainer = &blockStmt->body;

						break;
					}
					case Opcode::LEAVE: {
						popCurStmtContainer();
						break;
					}
					case Opcode::ADD:
					case Opcode::SUB:
					case Opcode::MUL:
					case Opcode::DIV:
					case Opcode::AND:
					case Opcode::OR:
					case Opcode::XOR:
					case Opcode::LAND:
					case Opcode::LOR:
					case Opcode::EQ:
					case Opcode::NEQ:
					case Opcode::LT:
					case Opcode::GT:
					case Opcode::LTEQ:
					case Opcode::GTEQ: {
						cxxast::BinaryOp op;

						switch (ins.opcode) {
							case Opcode::ADD:
								op = cxxast::BinaryOp::Add;
								break;
							case Opcode::SUB:
								op = cxxast::BinaryOp::Sub;
								break;
							case Opcode::MUL:
								op = cxxast::BinaryOp::Mul;
								break;
							case Opcode::DIV:
								op = cxxast::BinaryOp::Div;
								break;
							case Opcode::AND:
								op = cxxast::BinaryOp::And;
								break;
							case Opcode::OR:
								op = cxxast::BinaryOp::Or;
								break;
							case Opcode::XOR:
								op = cxxast::BinaryOp::Xor;
								break;
							case Opcode::LAND:
								op = cxxast::BinaryOp::LAnd;
								break;
							case Opcode::LOR:
								op = cxxast::BinaryOp::LOr;
								break;
							case Opcode::EQ:
								op = cxxast::BinaryOp::Eq;
								break;
							case Opcode::NEQ:
								op = cxxast::BinaryOp::Neq;
								break;
							case Opcode::LT:
								op = cxxast::BinaryOp::Lt;
								break;
							case Opcode::GT:
								op = cxxast::BinaryOp::Gt;
								break;
							case Opcode::LTEQ:
								op = cxxast::BinaryOp::LtEq;
								break;
							case Opcode::GTEQ:
								op = cxxast::BinaryOp::GtEq;
								break;
							default:
								std::terminate();
						}
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						std::shared_ptr<cxxast::Expr> expr =
							std::make_shared<cxxast::CastExpr>(
								compileType(compileContext, outputRegInfo.type),
								std::make_shared<cxxast::BinaryExpr>(
									op,
									compileValue(compileContext, ins.operands[0]),
									compileValue(compileContext, ins.operands[1])));

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::MOD: {
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						std::shared_ptr<cxxast::Expr> expr;

						Type type;
						if (ins.operands[0].valueType == ValueType::RegRef) {
							type = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).type;
						} else {
							type = valueTypeToTypeId(ins.operands[0].valueType);
						}

						switch (type.typeId) {
							case TypeId::I8:
							case TypeId::I16:
							case TypeId::I32:
							case TypeId::I64:
							case TypeId::U8:
							case TypeId::U16:
							case TypeId::U32:
							case TypeId::U64:
								expr =
									std::make_shared<cxxast::CastExpr>(
										compileType(compileContext, outputRegInfo.type),
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Mod,
											compileValue(compileContext, ins.operands[0]),
											compileValue(compileContext, ins.operands[1])));
								break;
							case TypeId::F32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("fmodf")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::F64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("fmod")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							default:
								std::terminate();
						}

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::LSH: {
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						std::shared_ptr<cxxast::Expr> expr;

						Type type;
						if (ins.operands[0].valueType == ValueType::RegRef) {
							type = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).type;
						} else {
							type = valueTypeToTypeId(ins.operands[0].valueType);
						}

						switch (type.typeId) {
							case TypeId::I8:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlSigned8")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I16:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlSigned16")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlSigned32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlSigned64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U8:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlUnsigned8")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U16:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlUnsigned16")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlUnsigned32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shlUnsigned64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							default:
								std::terminate();
						}

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::RSH: {
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						std::shared_ptr<cxxast::Expr> expr;

						Type type;
						if (ins.operands[0].valueType == ValueType::RegRef) {
							type = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).type;
						} else {
							type = valueTypeToTypeId(ins.operands[0].valueType);
						}

						switch (type.typeId) {
							case TypeId::I8:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrSigned8")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I16:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrSigned16")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrSigned32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrSigned64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U8:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrUnsigned8")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U16:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrUnsigned16")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrUnsigned32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("shrUnsigned64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							default:
								std::terminate();
						}

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::CMP: {
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						std::shared_ptr<cxxast::Expr> expr;

						Type type;
						if (ins.operands[0].valueType == ValueType::RegRef) {
							type = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).type;
						} else {
							type = valueTypeToTypeId(ins.operands[0].valueType);
						}

						switch (type.typeId) {
							case TypeId::I8:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareI8")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I16:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareI16")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareI32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::I64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareI64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U8:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareU8")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U16:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareU16")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareU32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::U64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareU64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::F32:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareF32")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							case TypeId::F64:
								expr = std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("flib")),
										std::make_shared<cxxast::IdExpr>("compareF64")),
									std::vector<std::shared_ptr<cxxast::Expr>>{
										compileValue(compileContext, ins.operands[0]),
										compileValue(compileContext, ins.operands[1]) });
								break;
							default:
								std::terminate();
						}

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::NOT:
					case Opcode::LNOT:
					case Opcode::NEG: {
						cxxast::UnaryOp op;

						switch (ins.opcode) {
							case Opcode::NOT:
								op = cxxast::UnaryOp::Not;
								break;
							case Opcode::LNOT:
								op = cxxast::UnaryOp::LNot;
								break;
							case Opcode::NEG:
								op = cxxast::UnaryOp::Negate;
								break;
							default:
								std::terminate();
						}
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
						std::shared_ptr<cxxast::Expr> expr =
							std::make_shared<cxxast::CastExpr>(
								compileType(compileContext, outputRegInfo.type),
								std::make_shared<cxxast::UnaryExpr>(
									op,
									compileValue(compileContext, ins.operands[0])));

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::AT: {
						opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());

						opti::RegAnalyzedInfo &regInfo = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex());
						Type &elementType = regInfo.type.getArrayExData();

						std::shared_ptr<cxxast::Expr>
							indexRefExpr = compileValue(compileContext, ins.operands[1]);
						std::shared_ptr<cxxast::Expr> arrayObjectPtrExpr;

						if (compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).isLoadInsResult) {
							arrayObjectPtrExpr =
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::MemberAccess,
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
										std::make_shared<cxxast::IdExpr>("asArray")),
									std::make_shared<cxxast::IdExpr>("arrayObject"));
						} else {
							arrayObjectPtrExpr = std::make_shared<cxxast::CastExpr>(
								genArrayObjectTypeName(),
								compileValue(compileContext, ins.operands[0]));
						}

						std::shared_ptr<cxxast::Expr> expr = std::make_shared<cxxast::CallExpr>(
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Scope,
									std::make_shared<cxxast::IdExpr>("slake"),
									std::make_shared<cxxast::IdExpr>("EntityRef")),
								std::make_shared<cxxast::IdExpr>("makeArrayElementRef")),
							std::vector<std::shared_ptr<cxxast::Expr>>{
								arrayObjectPtrExpr,
								indexRefExpr });

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										expr)));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								expr
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, outputRegInfo.type),
									std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
						break;
					}
					case Opcode::JMP: {
						uint32_t offDest = ins.operands[0].getU32();
						if (offDest > i) {
							branchBlockLeavingBoundaries.push_back(offDest);
							std::shared_ptr<cxxast::BlockStmt> blockStmt = std::make_shared<cxxast::BlockStmt>();

							curStmtContainer->push_back(blockStmt);

							pushCurStmtContainer();

							curStmtContainer = &blockStmt->body;
						}
						curStmtContainer->push_back(
							std::make_shared<cxxast::GotoStmt>(mangleJumpDestLabelName(offDest)));
						break;
					}
					case Opcode::JT: {
						uint32_t offDest = ins.operands[0].getU32();
						if (offDest > i) {
							branchBlockLeavingBoundaries.push_back(offDest);
							std::shared_ptr<cxxast::BlockStmt> blockStmt = std::make_shared<cxxast::BlockStmt>();

							curStmtContainer->push_back(blockStmt);

							pushCurStmtContainer();

							curStmtContainer = &blockStmt->body;
						}
						curStmtContainer->push_back(
							std::make_shared<cxxast::IfStmt>(
								compileValue(compileContext, ins.operands[1]),
								std::make_shared<cxxast::GotoStmt>(mangleJumpDestLabelName(offDest)),
								std::shared_ptr<cxxast::Stmt>{}));
						break;
					}
					case Opcode::JF: {
						uint32_t offDest = ins.operands[0].getU32();
						if (offDest > i) {
							branchBlockLeavingBoundaries.push_back(offDest);
							std::shared_ptr<cxxast::BlockStmt> blockStmt = std::make_shared<cxxast::BlockStmt>();

							curStmtContainer->push_back(blockStmt);

							pushCurStmtContainer();

							curStmtContainer = &blockStmt->body;
						}
						curStmtContainer->push_back(
							std::make_shared<cxxast::IfStmt>(
								compileValue(compileContext, ins.operands[1]),
								std::shared_ptr<cxxast::Stmt>{},
								std::make_shared<cxxast::GotoStmt>(mangleJumpDestLabelName(offDest))));
						break;
					}
					case Opcode::PUSHARG:
						break;
					case Opcode::CALL: {
						std::shared_ptr<cxxast::Expr> targetExpr;

						if (compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).isLoadInsResult) {
							targetExpr =
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::MemberAccess,
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
										std::make_shared<cxxast::IdExpr>("asObject")),
									std::make_shared<cxxast::IdExpr>("instanceObject"));
						} else {
							targetExpr = compileValue(compileContext, ins.operands[0]);
						}

						std::vector<std::shared_ptr<cxxast::Expr>> args;

						size_t nArgs = programInfo.analyzedFnCallInfo.at(i).argPushInsOffs.size();

						for (auto j : programInfo.analyzedFnCallInfo.at(i).argPushInsOffs) {
							Instruction &ins = fo->instructions.at(j);

							args.push_back(compileValue(compileContext, ins.operands[0]));
						}

						{
							auto info = getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Any));
							compileContext.addStackSize(info.first * nArgs, info.second);
						}

						curStmtContainer->push_back(
							std::make_shared<cxxast::LocalVarDefStmt>(
								genAnyTypeName(),
								std::vector<cxxast::VarDefPair>{
									{ mangleArgListLocalVarName(i),
										std::make_shared<cxxast::InitializerListExpr>(std::shared_ptr<cxxast::TypeName>{}, std::move(args)),
										0 } }));

						curStmtContainer->push_back(genReturnIfExceptStmt(
							std::make_shared<cxxast::CallExpr>(
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::PtrAccess,
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::IdExpr>("aotContext"),
										std::make_shared<cxxast::IdExpr>("runtime")),
									std::make_shared<cxxast::IdExpr>("execFnInAotFn")),
								std::vector<std::shared_ptr<cxxast::Expr>>{
									std::make_shared<cxxast::CastExpr>(
										genFnOverloadingPtrTypeName(),
										targetExpr),
									genUnwrappedHostContextRef(),
									std::make_shared<cxxast::NullptrLiteralExpr>(),
									std::make_shared<cxxast::IdExpr>(mangleArgListLocalVarName(i)),
									std::make_shared<cxxast::IntLiteralExpr>(nArgs)
									// TODO: Pass the stack information
								})));

						if (ins.output.valueType != ValueType::Undefined) {
							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).type),
									std::vector<cxxast::VarDefPair>{
										{ mangleArgListLocalVarName(i) } }));

							curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Assign,
									std::make_shared<cxxast::IdExpr>(mangleArgListLocalVarName(i)),
									std::make_shared<cxxast::CallExpr>(
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::PtrAccess,
											std::make_shared<cxxast::BinaryExpr>(
												cxxast::BinaryOp::MemberAccess,
												std::make_shared<cxxast::IdExpr>("aotContext"),
												std::make_shared<cxxast::IdExpr>("hostContext")),
											std::make_shared<cxxast::IdExpr>("getResult")),
										std::vector<std::shared_ptr<cxxast::Expr>>{}))));
						}

						break;
					}
					case Opcode::MCALL:
					case Opcode::CTORCALL: {
						std::shared_ptr<cxxast::Expr> targetExpr;

						if (compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).isLoadInsResult) {
							targetExpr =
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::MemberAccess,
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.operands[0].getRegIndex()).vregVarName)),
										std::make_shared<cxxast::IdExpr>("asObject")),
									std::make_shared<cxxast::IdExpr>("instanceObject"));
						} else {
							targetExpr = compileValue(compileContext, ins.operands[0]);
						}

						std::vector<std::shared_ptr<cxxast::Expr>> args;

						size_t nArgs = programInfo.analyzedFnCallInfo.at(i).argPushInsOffs.size();

						for (auto j : programInfo.analyzedFnCallInfo.at(i).argPushInsOffs) {
							Instruction &ins = fo->instructions.at(j);

							args.push_back(compileValue(compileContext, ins.operands[0]));
						}

						{
							auto info = getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Any));
							compileContext.addStackSize(info.first * nArgs, info.second);
						}

						curStmtContainer->push_back(
							std::make_shared<cxxast::LocalVarDefStmt>(
								genAnyTypeName(),
								std::vector<cxxast::VarDefPair>{
									{ mangleArgListLocalVarName(i),
										std::make_shared<cxxast::InitializerListExpr>(std::shared_ptr<cxxast::TypeName>{}, std::move(args)),
										0 } }));

						curStmtContainer->push_back(genReturnIfExceptStmt(
							std::make_shared<cxxast::CallExpr>(
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::PtrAccess,
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::IdExpr>("aotContext"),
										std::make_shared<cxxast::IdExpr>("runtime")),
									std::make_shared<cxxast::IdExpr>("execFnInAotFn")),
								std::vector<std::shared_ptr<cxxast::Expr>>{
									std::make_shared<cxxast::CastExpr>(
										genFnOverloadingPtrTypeName(),
										targetExpr),
									genUnwrappedHostContextRef(),
									compileValue(compileContext, ins.operands[1]),
									std::make_shared<cxxast::IdExpr>(mangleArgListLocalVarName(i)),
									std::make_shared<cxxast::IntLiteralExpr>(nArgs)
									// TODO: Pass the stack information
								})));

						if (ins.output.valueType != ValueType::Undefined) {
							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).type));
							curStmtContainer->push_back(
								std::make_shared<cxxast::LocalVarDefStmt>(
									compileType(compileContext, programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).type),
									std::vector<cxxast::VarDefPair>{
										{ mangleArgListLocalVarName(i) } }));

							curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Assign,
									std::make_shared<cxxast::IdExpr>(mangleArgListLocalVarName(i)),
									std::make_shared<cxxast::CallExpr>(
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::PtrAccess,
											std::make_shared<cxxast::BinaryExpr>(
												cxxast::BinaryOp::MemberAccess,
												std::make_shared<cxxast::IdExpr>("aotContext"),
												std::make_shared<cxxast::IdExpr>("hostContext")),
											std::make_shared<cxxast::IdExpr>("getResult")),
										std::vector<std::shared_ptr<cxxast::Expr>>{}))));
						}

						break;
					}
					case Opcode::RET: {
						curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Assign,
								std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::MemberAccess,
									std::make_shared<cxxast::IdExpr>("aotContext"),
									std::make_shared<cxxast::IdExpr>("returnValue")),
								compileValueAsAny(compileContext, ins.operands[0]))));

						curStmtContainer->push_back(
							std::make_shared<cxxast::ReturnStmt>(
								std::make_shared<cxxast::InitializerListExpr>(
									std::shared_ptr<cxxast::TypeName>{},
									std::vector<std::shared_ptr<cxxast::Expr>>{})));
						break;
					}
					case Opcode::LTHIS: {
						const opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());

						if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)),
										genGetValueDataExpr(
											outputRegInfo.type,
											genThisRef()))));
						} else {
							std::string &varName = compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName;

							cxxast::VarDefPair varDefPair = {
								varName,
								genGetValueDataExpr(
									outputRegInfo.type,
									genThisRef())
							};

							compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(outputRegInfo.type));
							curStmtContainer->push_back(std::make_shared<cxxast::LocalVarDefStmt>(compileType(compileContext, outputRegInfo.type), std::vector<cxxast::VarDefPair>{ varDefPair }));
						}
					}
				}

				if (auto it = compileContext.recyclableRegs.find(i); it != compileContext.recyclableRegs.end()) {
					for (auto j : it->second) {
						VirtualRegInfo &vregInfo = compileContext.getVirtualRegInfo(j);
						if (vregInfo.nameBorrowedReg != UINT32_MAX)
							compileContext.recycledRegs.insert(vregInfo.nameBorrowedReg);
						else
							compileContext.recycledRegs.insert(j);
					}
				}
			}

			if (branchBlockLeavingBoundaries.size()) {
				// TODO: Warn the user that the program is malformed.
				while (branchBlockLeavingBoundaries.size()) {
					branchBlockLeavingBoundaries.pop_back();
					popCurStmtContainer();
				}
			}
			break;
		}
	}
}
