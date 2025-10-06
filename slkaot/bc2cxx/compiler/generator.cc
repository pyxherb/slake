#include "../../compiler.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

void BC2CXX::recompileGeneratorFnOverloading(CompileContext &compileContext, std::shared_ptr<cxxast::Fn> fnOverloading) {
	RegularFnOverloadingObject *fo = (RegularFnOverloadingObject *)fnOverloading->rtOverloading.get();

	std::vector<std::shared_ptr<cxxast::Stmt>> *curStmtContainer;
	std::list<std::vector<std::shared_ptr<cxxast::Stmt>> *> curStmtContainerStack;
	auto pushCurStmtContainer = [&curStmtContainer, &curStmtContainerStack]() {
		curStmtContainerStack.push_back(curStmtContainer);
	};
	auto popCurStmtContainer = [&curStmtContainer, &curStmtContainerStack]() {
		curStmtContainer = curStmtContainerStack.back();
		curStmtContainerStack.pop_back();
	};

	std::shared_ptr<cxxast::Class> stateClass = std::make_shared<cxxast::Class>(mangleGeneratorStateClassName(fo));

	compileContext.rootNamespace->addPublicMember(stateClass);

	opti::ProgramAnalyzedInfo programInfo(compileContext.runtime);
	HostRefHolder hostRefHolder(peff::getDefaultAlloc());
	InternalExceptionPointer e = opti::analyzeProgramInfo(compileContext.runtime, fo, programInfo, hostRefHolder);
	if (e) {
		fprintf(stderr, "Error analyzing the program: %s\n", e->what());
		e.reset();
		return;
	}

	compileContext.isGenerator = true;

	for (size_t i = 0; i < fo->paramTypes.size(); ++i) {
		std::shared_ptr<cxxast::Var> regVar = std::make_shared<cxxast::Var>(
			mangleParamName(i),
			cxxast::StorageClass::Unspecified,
			compileType(compileContext, fo->paramTypes.at(i)),
			std::shared_ptr<cxxast::Expr>{});

		stateClass->addPublicMember(regVar);

		fnOverloading->name = mangleTypeName(fo->paramTypes.at(i)) + fnOverloading->name;
	}

	{
		std::shared_ptr<cxxast::Var> currentStateVar = std::make_shared<cxxast::Var>(
			"currentState",
			cxxast::StorageClass::Unspecified,
			std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unspecified, cxxast::IntModifierKind::Unspecified),
			std::shared_ptr<cxxast::Expr>{});

		stateClass->addPublicMember(currentStateVar);
	}

	{
		std::shared_ptr<cxxast::Var> stackFrameVersionVar = std::make_shared<cxxast::Var>(
			"stackFrameVersion",
			cxxast::StorageClass::Unspecified,
			std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unspecified, cxxast::IntModifierKind::Unspecified),
			std::shared_ptr<cxxast::IntLiteralExpr>(0));

		stateClass->addPublicMember(stackFrameVersionVar);
	}

	{
		std::shared_ptr<cxxast::Var> stackVarNumVar = std::make_shared<cxxast::Var>(
			"nLocalVars",
			cxxast::StorageClass::Unspecified,
			std::make_shared<cxxast::IntTypeName>(cxxast::SignKind::Unspecified, cxxast::IntModifierKind::Unspecified),
			std::shared_ptr<cxxast::IntLiteralExpr>(0));

		stateClass->addPublicMember(stackVarNumVar);
	}

	{
		fnOverloading->signature.paramTypes.push_back(
			std::make_shared<cxxast::PointerTypeName>(
				std::make_shared<cxxast::CustomTypeName>(
					false,
					cxxast::getFullRefOf(stateClass))));
	}

	{
		cxxast::VarDefPair varDefPair = { std::string("load_tmp"), {} };
		std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(genObjectRefTypeName(), std::vector<cxxast::VarDefPair>{ varDefPair });
		compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Ref, nullptr)));
		fnOverloading->body.push_back(stmt);
	}

	{
		compileContext.addStackSize(getLocalVarSizeAndAlignmentInfoOfType(Type(TypeId::Any)));
		fnOverloading->body.push_back(std::make_shared<cxxast::LocalVarDefStmt>(
			genAnyTypeName(),
			std::vector<cxxast::VarDefPair>{
				cxxast::VarDefPair{
					"read_var_tmp",
					{} } }));
	}

	std::shared_ptr<cxxast::SwitchStmt> rootSwitchStmt = std::make_shared<cxxast::SwitchStmt>(
		std::make_shared<cxxast::BinaryExpr>(
			cxxast::BinaryOp::PtrAccess,
			std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
			std::make_shared<cxxast::IdExpr>("currentState")));

	fnOverloading->body.push_back(rootSwitchStmt);

	int idxCurState = 0, stackFrameVersion = 0;

	cxxast::SwitchCase switchCase;

	auto clearCurSwitchCase = [&switchCase, &idxCurState]() {
		switchCase.expr = std::make_shared<cxxast::IntLiteralExpr>(idxCurState++);
		switchCase.body.clear();
	};

	switchCase.expr = std::make_shared<cxxast::IntLiteralExpr>(idxCurState++);

	curStmtContainer = &switchCase.body;

	for (size_t i = 0; i < fo->instructions.size(); ++i) {
		Instruction &ins = fo->instructions.at(i);

		if (ins.output != UINT32_MAX) {
			opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);

			compileContext.defineVirtualReg(ins.output, mangleRegLocalVarName(ins.output));

			compileContext.recyclableRegs[programInfo.analyzedRegInfo.at(ins.output).lifetime.offEndIns].insert(ins.output);

			if (compileContext.allocRecycledReg(*this, programInfo, ins.output, outputRegInfo.type)) {
			} else {
				std::shared_ptr<cxxast::Var> regVar = std::make_shared<cxxast::Var>(
					mangleRegLocalVarName(ins.output),
					cxxast::StorageClass::Unspecified,
					compileType(compileContext, outputRegInfo.type),
					std::shared_ptr<cxxast::Expr>{});

				stateClass->addPublicMember(regVar);
			}
		}

		curStmtContainer->push_back(
			std::make_shared<cxxast::LabelStmt>(mangleJumpDestLabelName(i)));

		switch (ins.opcode) {
			case Opcode::NOP:
				break;
			case Opcode::LOAD: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);

				HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[0].getEntityRef().asObject;

				compileContext.mappedObjects.insert((Object *)id.get());

				std::shared_ptr<cxxast::Expr> rhs;

				switch (outputRegInfo.expectedValue.valueType) {
					case ValueType::EntityRef: {
						EntityRef &entityRef = outputRegInfo.expectedValue.getEntityRef();

						switch (entityRef.kind) {
							case ObjectRefKind::ObjectRef: {
								Object *object = entityRef.asObject;

								if (auto astNode = getMappedAstNode(object);
									astNode) {
									rhs = std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
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
								} else {
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
												std::make_shared<cxxast::IdExpr>("load_tmp") })));

									rhs = std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>("load_tmp"),
											std::make_shared<cxxast::IdExpr>("asObject")),
										std::make_shared<cxxast::IdExpr>("instanceObject"));
								}
								break;
							}
							case ObjectRefKind::FieldRef: {
								FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

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
											std::make_shared<cxxast::IdExpr>("load_tmp") })));

								rhs = std::make_shared<cxxast::IdExpr>("load_tmp");
								break;
							}
							default:
								std::terminate();
						}
						break;
					}
				}

				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output).vregVarName))),
						rhs)));

				break;
			}
			case Opcode::RLOAD: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
				uint32_t idxBaseReg = ins.operands[0].getRegIndex();
				HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[1].getEntityRef().asObject;

				compileContext.mappedObjects.insert((Object *)id.get());

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
							std::make_shared<cxxast::IdExpr>("load_tmp"),
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::PtrAccess,
								std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
								std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(idxBaseReg).vregVarName))) })));

				std::shared_ptr<cxxast::Expr> rhs;

				switch (outputRegInfo.expectedValue.valueType) {
					case ValueType::EntityRef: {
						EntityRef &entityRef = outputRegInfo.expectedValue.getEntityRef();

						switch (entityRef.kind) {
							case ObjectRefKind::ObjectRef: {
								Object *object = entityRef.asObject;

								if (auto astNode = getMappedAstNode(object);
									astNode) {
									rhs = std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
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
								} else {
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
												std::make_shared<cxxast::IdExpr>("load_tmp") })));

									rhs = std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::MemberAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>("load_tmp"),
											std::make_shared<cxxast::IdExpr>("asObject")),
										std::make_shared<cxxast::IdExpr>("instanceObject"));
								}
								break;
							}
							case ObjectRefKind::FieldRef: {
								FieldRecord &fieldRecord = entityRef.asField.moduleObject->fieldRecords.at(entityRef.asField.index);

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
											std::make_shared<cxxast::IdExpr>("load_tmp") })));

								rhs = std::make_shared<cxxast::IdExpr>("load_tmp");
								break;
							}
							default:
								std::terminate();
						}
						break;
					}
				}

				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output).vregVarName))),
						rhs)));
				break;
			}
			case Opcode::LVAR: {
				std::shared_ptr<cxxast::Var> lvarVar = std::make_shared<cxxast::Var>(
					mangleLocalVarName(ins.output),
					cxxast::StorageClass::Unspecified,
					compileType(compileContext, ins.operands[0].getTypeName()),
					std::shared_ptr<cxxast::Expr>{});

				stateClass->addPublicMember(lvarVar);

				GeneratorLocalVarInfo info;

				compileContext.generatorLocalVarInfo[ins.output] = std::move(info);

				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>("nLocalVars")),
						std::make_shared<cxxast::IntLiteralExpr>(compileContext.generatorLocalVarInfo.size()))));

				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>(mangleLocalVarName(ins.output))),
						genDefaultValue(ins.operands[0].getTypeName()))));
				break;
			}
			case Opcode::LVALUE: {
				if (ins.output != UINT32_MAX) {
					opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output),
										  &sourceRegInfo = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex());
					std::shared_ptr<cxxast::Expr> lhs = std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::PtrAccess,
						std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
						std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output).vregVarName)));

					switch (sourceRegInfo.storageType) {
						case opti::RegStorageType::None: {
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
											compileValue(compileContext, ins.operands[0]),
											std::make_shared<cxxast::IdExpr>(std::string("read_var_tmp")) })));

							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										lhs,
										genGetValueDataExpr(
											outputRegInfo.type,
											std::make_shared<cxxast::IdExpr>(std::string("read_var_tmp"))))));

							break;
						}
						case opti::RegStorageType::FieldVar: {
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
											compileValue(compileContext, ins.operands[0]),
											std::make_shared<cxxast::IdExpr>(std::string("read_var_tmp")) })));

							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										lhs,
										genGetValueDataExpr(
											outputRegInfo.type,
											std::make_shared<cxxast::IdExpr>(std::string("read_var_tmp"))))));
							break;
						}
						case opti::RegStorageType::InstanceFieldVar: {
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
											compileValue(compileContext, ins.operands[0]),
											std::make_shared<cxxast::IdExpr>(std::string("read_var_tmp")) })));

							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										lhs,
										genGetValueDataExpr(
											outputRegInfo.type,
											std::make_shared<cxxast::IdExpr>(std::string("read_var_tmp"))))));

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
											compileValue(compileContext, ins.operands[0]),
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
													compileValue(compileContext, ins.operands[0]),
													std::make_shared<cxxast::IdExpr>("asArray")),
												std::make_shared<cxxast::IdExpr>("arrayObject")),
											std::make_shared<cxxast::IdExpr>("data")));
								std::shared_ptr<cxxast::Expr> elementExpr = std::make_shared<cxxast::BinaryExpr>(
									cxxast::BinaryOp::Subscript,
									dataPtrExpr,
									indexRefExpr);

								curStmtContainer->push_back(
									std::make_shared<cxxast::ExprStmt>(
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Assign,
											lhs,
											elementExpr)));
							}
							break;
						}
						case opti::RegStorageType::LocalVar: {
							uint32_t idxReg = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asLocalVar.definitionReg;
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										lhs,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::PtrAccess,
											std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
											std::make_shared<cxxast::IdExpr>(
												mangleLocalVarName(idxReg))))));
							break;
						}
						case opti::RegStorageType::ArgRef: {
							curStmtContainer->push_back(
								std::make_shared<cxxast::ExprStmt>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Assign,
										lhs,
										std::make_shared<cxxast::IdExpr>(
											mangleParamName(programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex()).storageInfo.asArgRef.idxArg)))));
							break;
						}
					}
					break;
				}
				break;
			}
			case Opcode::ENTER: {
				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>("stackFrameVersion")),
						std::make_shared<cxxast::IntLiteralExpr>(+stackFrameVersion))));
				break;
			}
			case Opcode::LEAVE: {
				++stackFrameVersion;
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
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
				std::shared_ptr<cxxast::Expr> expr =
					std::make_shared<cxxast::CastExpr>(
						compileType(compileContext, outputRegInfo.type),
						std::make_shared<cxxast::BinaryExpr>(
							op,
							compileValue(compileContext, ins.operands[0]),
							compileValue(compileContext, ins.operands[1])));

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
				break;
			}
			case Opcode::MOD: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
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

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
				break;
			}
			case Opcode::LSH: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
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

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
				break;
			}
			case Opcode::RSH: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
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

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
				break;
			}
			case Opcode::CMP: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
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

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
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
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);
				std::shared_ptr<cxxast::Expr> expr =
					std::make_shared<cxxast::CastExpr>(
						compileType(compileContext, outputRegInfo.type),
						std::make_shared<cxxast::UnaryExpr>(
							op,
							compileValue(compileContext, ins.operands[0])));

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
				break;
			}
			case Opcode::AT: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output);

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
								compileValue(compileContext, ins.operands[0]),
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

				curStmtContainer->push_back(
					std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							expr)));
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
								compileValue(compileContext, ins.operands[0]),
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

				std::shared_ptr<cxxast::BlockStmt> blockStmt = std::make_shared<cxxast::BlockStmt>();

				pushCurStmtContainer();

				curStmtContainer = &blockStmt->body;

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

				popCurStmtContainer();

				curStmtContainer->push_back(blockStmt);

				if (ins.output != UINT32_MAX) {
					curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							genGetValueDataExpr(
								programInfo.analyzedRegInfo.at(ins.output).type,
								std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::PtrAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>("aotContext"),
											std::make_shared<cxxast::IdExpr>("hostContext")),
										std::make_shared<cxxast::IdExpr>("getResult")),
									std::vector<std::shared_ptr<cxxast::Expr>>{})))));
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
								compileValue(compileContext, ins.operands[0]),
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

				std::shared_ptr<cxxast::BlockStmt> blockStmt = std::make_shared<cxxast::BlockStmt>();

				pushCurStmtContainer();

				curStmtContainer = &blockStmt->body;

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

				popCurStmtContainer();

				curStmtContainer->push_back(blockStmt);

				if (ins.output != UINT32_MAX) {
					curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Assign,
							compileValue(compileContext, ins.output),
							genGetValueDataExpr(
								programInfo.analyzedRegInfo.at(ins.output).type,
								std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::PtrAccess,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::MemberAccess,
											std::make_shared<cxxast::IdExpr>("aotContext"),
											std::make_shared<cxxast::IdExpr>("hostContext")),
										std::make_shared<cxxast::IdExpr>("getResult")),
									std::vector<std::shared_ptr<cxxast::Expr>>{})))));
				}

				break;
			}
			case Opcode::JMP: {
				uint32_t offDest = ins.operands[0].getU32();
				curStmtContainer->push_back(
					std::make_shared<cxxast::GotoStmt>(mangleJumpDestLabelName(offDest)));
				break;
			}
			case Opcode::JT: {
				uint32_t offDest = ins.operands[0].getU32();
				curStmtContainer->push_back(
					std::make_shared<cxxast::IfStmt>(
						compileValue(compileContext, ins.operands[1]),
						std::make_shared<cxxast::GotoStmt>(mangleJumpDestLabelName(offDest)),
						std::shared_ptr<cxxast::Stmt>{}));
				break;
			}
			case Opcode::JF: {
				uint32_t offDest = ins.operands[0].getU32();
				curStmtContainer->push_back(
					std::make_shared<cxxast::IfStmt>(
						compileValue(compileContext, ins.operands[1]),
						std::shared_ptr<cxxast::Stmt>{},
						std::make_shared<cxxast::GotoStmt>(mangleJumpDestLabelName(offDest))));
				break;
			}
			case Opcode::RET: {
				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>("currentState")),
						std::make_shared<cxxast::IntLiteralExpr>(-1))));

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
			case Opcode::YIELD: {
				curStmtContainer->push_back(std::make_shared<cxxast::ExprStmt>(
					std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Assign,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::PtrAccess,
							std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
							std::make_shared<cxxast::IdExpr>("currentState")),
						std::make_shared<cxxast::IntLiteralExpr>(+idxCurState))));

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

				rootSwitchStmt->switchCases.push_back(std::move(switchCase));
				clearCurSwitchCase();
				break;
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

	rootSwitchStmt->switchCases.push_back(std::move(switchCase));
	clearCurSwitchCase();
}
