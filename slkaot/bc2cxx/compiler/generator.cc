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

		if (ins.output.valueType != ValueType::Undefined) {
			opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());

			compileContext.defineVirtualReg(ins.output.getRegIndex(), mangleRegLocalVarName(ins.output.getRegIndex()));

			compileContext.recyclableRegs[programInfo.analyzedRegInfo.at(ins.output.getRegIndex()).lifetime.offEndIns].insert(ins.output.getRegIndex());

			if (compileContext.allocRecycledReg(*this, programInfo, ins.output.getRegIndex(), outputRegInfo.type)) {
			} else {
				std::shared_ptr<cxxast::Var> regVar = std::make_shared<cxxast::Var>(
					mangleRegLocalVarName(ins.output.getRegIndex()),
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
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());

				HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[0].getEntityRef().asObject.instanceObject;

				compileContext.mappedObjects.insert((Object *)id.get());

				std::shared_ptr<cxxast::Expr> rhs;

				switch (outputRegInfo.expectedValue.valueType) {
					case ValueType::EntityRef: {
						EntityRef &entityRef = outputRegInfo.expectedValue.getEntityRef();

						switch (entityRef.kind) {
							case ObjectRefKind::ObjectRef: {
								Object *object = entityRef.asObject.instanceObject;

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
							std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName))),
						rhs)));

				break;
			}
			case Opcode::RLOAD: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());
				uint32_t idxBaseReg = ins.operands[0].getRegIndex();
				HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[1].getEntityRef().asObject.instanceObject;

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
								Object *object = entityRef.asObject.instanceObject;

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
							std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName))),
						rhs)));
				break;
			}
			case Opcode::LVAR: {
				std::shared_ptr<cxxast::Var> lvarVar = std::make_shared<cxxast::Var>(
					mangleLocalVarName(ins.output.getRegIndex()),
					cxxast::StorageClass::Unspecified,
					compileType(compileContext, ins.operands[0].getTypeName()),
					std::shared_ptr<cxxast::Expr>{});

				stateClass->addPublicMember(lvarVar);

				GeneratorLocalVarInfo info;

				compileContext.generatorLocalVarInfo[ins.output.getRegIndex()] = std::move(info);

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
							std::make_shared<cxxast::IdExpr>(mangleLocalVarName(ins.output.getRegIndex()))),
						genDefaultValue(ins.operands[0].getTypeName()))));
				break;
			}
			case Opcode::LVALUE: {
				if (ins.output.valueType != ValueType::Undefined) {
					opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex()),
										  &sourceRegInfo = programInfo.analyzedRegInfo.at(ins.operands[0].getRegIndex());
					std::shared_ptr<cxxast::Expr> lhs = std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::PtrAccess,
						std::make_shared<cxxast::IdExpr>(mangleParamName(0)),
						std::make_shared<cxxast::IdExpr>(std::string(compileContext.getVirtualRegInfo(ins.output.getRegIndex()).vregVarName)));

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
