#include "../bc2cxx.h"

using namespace slake;
using namespace slake::slkaot;
using namespace slake::slkaot::bc2cxx;

void BC2CXX::recompileFnOverloading(CompileContext &compileContext, std::shared_ptr<cxxast::Fn> fnOverloading) {
	FnOverloadingObject *fnOverloadingObject = fnOverloading->rtOverloading.get();

	switch (fnOverloadingObject->overloadingKind) {
	case FnOverloadingKind::Regular: {
		RegularFnOverloadingObject *fo = (RegularFnOverloadingObject *)fnOverloadingObject;

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

			switch (ins.opcode) {
			case Opcode::NOP:
				break;
			case Opcode::LOAD: {
				opti::RegAnalyzedInfo &outputRegInfo = programInfo.analyzedRegInfo.at(ins.output.getRegIndex());

				switch (outputRegInfo.expectedValue.valueType) {
				case ValueType::ObjectRef: {
					ObjectRef &objectRef = outputRegInfo.expectedValue.getObjectRef();

					switch (objectRef.kind) {
					case ObjectRefKind::InstanceRef: {
						Object *object = objectRef.asInstance.instanceObject;

						compileContext.pushDynamicContents();
						compileContext.dynamicContents.compilationTarget = CompilationTarget::VarDef;

						std::shared_ptr<cxxast::TypeName> t = genObjectRefTypeName();

						std::string varName = mangleRegLocalVarName(ins.output.getRegIndex());
						cxxast::VarDefPair varDefPair;

						if (auto astNode = getMappedAstNode(object);
							astNode) {
							varDefPair = {
								varName,
								std::make_shared<cxxast::CallExpr>(
									std::make_shared<cxxast::BinaryExpr>(
										cxxast::BinaryOp::Scope,
										std::make_shared<cxxast::BinaryExpr>(
											cxxast::BinaryOp::Scope,
											std::make_shared<cxxast::IdExpr>("slake"),
											std::make_shared<cxxast::IdExpr>("ObjectRef")),
										std::make_shared<cxxast::IdExpr>("makeAotPtrRef")),
									std::vector<std::shared_ptr<cxxast::Expr>>{ _getAbsRef(astNode) })
							};
						} else {
							varDefPair = { varName, {} };
						}

						std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(t, std::vector<cxxast::VarDefPair>{ varDefPair });

						fnOverloading->body.push_back(stmt);

						compileContext.popDynamicContents();
						break;
					}
					case ObjectRefKind::FieldRef: {
						FieldRecord &fieldRecord = objectRef.asField.moduleObject->fieldRecords.at(objectRef.asField.index);

						compileContext.pushDynamicContents();
						compileContext.dynamicContents.compilationTarget = CompilationTarget::VarDef;

						std::shared_ptr<cxxast::TypeName> t = compileType(compileContext, fieldRecord.type);

						cxxast::VarDefPair varDefPair = { mangleRegLocalVarName(ins.output.getRegIndex()), {} };

						std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(t, std::vector<cxxast::VarDefPair>{ varDefPair });

						fnOverloading->body.push_back(stmt);

						compileContext.popDynamicContents();
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
				HostObjectRef<IdRefObject> id = (IdRefObject *)ins.operands[1].getObjectRef().asInstance.instanceObject;

				compileContext.constantObjects.insert((Object *)id.get());

				std::shared_ptr<cxxast::TypeName> t = genObjectRefTypeName();

				cxxast::VarDefPair varDefPair = { mangleRegLocalVarName(ins.output.getRegIndex()), {} };

				std::shared_ptr<cxxast::LocalVarDefStmt> stmt = std::make_shared<cxxast::LocalVarDefStmt>(t, std::vector<cxxast::VarDefPair>{ varDefPair });

				fnOverloading->body.push_back(stmt);

				fnOverloading->body.push_back(genReturnIfExceptStmt(
					std::make_shared<cxxast::CallExpr>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::MemberAccess,
							genAotContextRef(),
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::PtrAccess,
								std::make_shared<cxxast::IdExpr>("runtime"),
								std::make_shared<cxxast::IdExpr>("resolveIdRef"))),
						std::vector<std::shared_ptr<cxxast::Expr>>{
							std::make_shared<cxxast::BinaryExpr>(cxxast::BinaryOp::PtrAccess,
								std::make_shared<cxxast::CastExpr>(
									std::make_shared<cxxast::PointerTypeName>(
										std::make_shared<cxxast::CustomTypeName>(
											false,
											std::make_shared<cxxast::BinaryExpr>(
												cxxast::BinaryOp::Scope,
												_getAbsRef(compileContext.rootNamespace),
												std::make_shared<cxxast::IdExpr>("ConstantObjects")))),
												genConstantObjectsRef()),
								std::make_shared<cxxast::IdExpr>(mangleConstantObjectName(id.get()))),
							std::make_shared<cxxast::IdExpr>(mangleRegLocalVarName(ins.output.getRegIndex())),
							std::make_shared<cxxast::IdExpr>(mangleRegLocalVarName(idxBaseReg)) })));
				break;
			}
			}
		}
		break;
	}
	}
}
