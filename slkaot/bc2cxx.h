#ifndef _SLKAOT_BC2CXX_H_
#define _SLKAOT_BC2CXX_H_

#include "cxxast.h"
#include <iostream>
#include <slake/opti/proganal.h>

namespace slake {
	namespace slkaot {
		namespace bc2cxx {
			class BC2CXX {
			public:
				enum class ASTDumpMode {
					Header = 0,
					Source
				};

				std::map<EntityRef, std::shared_ptr<cxxast::AbstractMember>> runtimeEntityToAstNodeMap;
				std::map<std::shared_ptr<cxxast::AbstractMember>, EntityRef> astNodeToRuntimeEntityMap;
				std::set<std::shared_ptr<cxxast::Fn>> recompilableFns;

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::AbstractMember> getMappedAstNode(Object *object) {
					if (auto it = runtimeEntityToAstNodeMap.find(EntityRef::makeObjectRef(object));
						it != runtimeEntityToAstNodeMap.end()) {
						return it->second;
					}
					return nullptr;
				}

				SLAKE_FORCEINLINE void registerRuntimeEntityToAstNodeRegistry(Object *object, std::shared_ptr<cxxast::AbstractMember> m) {
					runtimeEntityToAstNodeMap[EntityRef::makeObjectRef(object)] = m;
					astNodeToRuntimeEntityMap[m] = EntityRef::makeObjectRef(object);
				}

				SLAKE_FORCEINLINE void registerRuntimeEntityToAstNodeRegistry(EntityRef entityRef, std::shared_ptr<cxxast::AbstractMember> m) {
					runtimeEntityToAstNodeMap[entityRef] = m;
					astNodeToRuntimeEntityMap[m] = entityRef;
				}

			private:
				void _dumpStmt(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode, size_t indentLevel);
				void _dumpAstNode(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode, size_t indentLevel);
				bool _isSimpleIdExpr(std::shared_ptr<cxxast::Expr> expr);
				std::shared_ptr<cxxast::Expr> _getAbsRef(std::shared_ptr<cxxast::AbstractMember> m);
				std::shared_ptr<cxxast::Expr> _getLastExpr(std::shared_ptr<cxxast::Expr> e);
				void _applyGenericArgs(std::shared_ptr<cxxast::Expr> expr, cxxast::GenericArgList &&args);

			public:
				struct VirtualRegInfo {
					std::string vregVarName;
					uint32_t nameBorrowedReg = UINT32_MAX;
					bool isLoadInsResult = false;

					SLAKE_FORCEINLINE VirtualRegInfo(std::string &&vregVarName) : vregVarName(vregVarName) {}
				};

				struct GeneratorLocalVarInfo {
					int version;
				};

				struct CompileContext {
					Runtime *runtime;
					std::shared_ptr<cxxast::Namespace> rootNamespace;
					std::map<uint32_t, VirtualRegInfo> vregInfo;
					// Each entry represents a boundary of register lifetime, each element in the entry represents a recyclable register.
					std::map<uint32_t, std::set<uint32_t>> recyclableRegs;
					std::set<uint32_t> recycledRegs;
					std::set<HostObjectRef<>> mappedObjects;
					std::map<uint32_t, GeneratorLocalVarInfo> generatorLocalVarInfo;
					size_t estimatedStackSize = 0;
					bool isGenerator = false;

					SLAKE_FORCEINLINE CompileContext(Runtime *runtime, std::shared_ptr<cxxast::Namespace> rootNamespace) : runtime(runtime), rootNamespace(rootNamespace) {}

					SLAKE_FORCEINLINE void alignStackSize(size_t alignment) {
						if (size_t i = estimatedStackSize % alignment; i) {
							estimatedStackSize += alignment - i;
						}
					}

					SLAKE_FORCEINLINE void addStackSize(size_t size, size_t alignment) {
						alignStackSize(alignment);
						estimatedStackSize += size;
					}

					SLAKE_FORCEINLINE void addStackSize(const std::pair<size_t, size_t> &sizeAlignmentPair) {
						addStackSize(sizeAlignmentPair.first, sizeAlignmentPair.second);
						;
					}

					SLAKE_FORCEINLINE VirtualRegInfo &defineVirtualReg(uint32_t reg, std::string &&vregVarName) {
						return vregInfo.insert({ reg, VirtualRegInfo(std::move(vregVarName)) }).first->second;
					}

					SLAKE_FORCEINLINE VirtualRegInfo &getVirtualRegInfo(uint32_t reg) {
						return vregInfo.at(reg);
					}

					SLAKE_FORCEINLINE void markVirtualRegAsLoadInsResult(uint32_t reg) {
						vregInfo.at(reg).isLoadInsResult = true;
					}

					SLAKE_API bool allocRecycledReg(BC2CXX &bc2cxx, const opti::ProgramAnalyzedInfo &analyzedInfo, uint32_t reg, const Type &type);

					SLAKE_FORCEINLINE void resetForCompilation() {
						vregInfo.clear();
						recyclableRegs.clear();
						recycledRegs.clear();
						estimatedStackSize = 0;
						isGenerator = false;
						generatorLocalVarInfo.clear();
					}
				};

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genInstanceObjectTypeName() {
					return std::make_shared<cxxast::PointerTypeName>(
						std::make_shared<cxxast::CustomTypeName>(
							false,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::IdExpr>("slake"),
								std::make_shared<cxxast::IdExpr>("Object"))));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genArrayObjectTypeName() {
					return std::make_shared<cxxast::PointerTypeName>(
						std::make_shared<cxxast::CustomTypeName>(
							false,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::IdExpr>("slake"),
								std::make_shared<cxxast::IdExpr>("ArrayObject"))));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genIdRefObjectTypeName() {
					return std::make_shared<cxxast::PointerTypeName>(
						std::make_shared<cxxast::CustomTypeName>(
							false,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::IdExpr>("slake"),
								std::make_shared<cxxast::IdExpr>("IdRefObject"))));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genNativeAotPtrTypeName() {
					return std::make_shared<cxxast::PointerTypeName>(
						std::make_shared<cxxast::VoidTypeName>());
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genValueRefExpr() {
					return std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Scope,
						std::make_shared<cxxast::IdExpr>("slake"),
						std::make_shared<cxxast::IdExpr>("Value"));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genAnyTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						genValueRefExpr());
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genObjectRefTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Scope,
							std::make_shared<cxxast::IdExpr>("slake"),
							std::make_shared<cxxast::IdExpr>("EntityRef")));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genSizeTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::IdExpr>("size_t"));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genInternalExceptionPtrTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Scope,
							std::make_shared<cxxast::IdExpr>("slake"),
							std::make_shared<cxxast::IdExpr>("InternalExceptionPointer")));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Stmt> genReturnIfExceptStmt(std::shared_ptr<cxxast::Expr> expr) {
					return std::make_shared<cxxast::ExprStmt>(
						std::make_shared<cxxast::CallExpr>(
							std::make_shared<cxxast::IdExpr>("SLAKE_RETURN_IF_EXCEPT"),
							std::vector<std::shared_ptr<cxxast::Expr>>{ expr }));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genRegLocalVarRefExpr(uint32_t index) {
					return std::make_shared<cxxast::IdExpr>(mangleRegLocalVarName(index));
				}

				SLAKE_FORCEINLINE std::string genAotContextParamName() {
					return "aotContext";
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genAotContextRef() {
					return std::make_shared<cxxast::IdExpr>(genAotContextParamName(), cxxast::GenericArgList{});
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genUnwrappedHostContextRef() {
					return std::make_shared<cxxast::CallExpr>(
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::MemberAccess,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::MemberAccess,
								std::make_shared<cxxast::IdExpr>("aotContext"),
								std::make_shared<cxxast::IdExpr>("hostContext")),
							std::make_shared<cxxast::IdExpr>("get")),
						std::vector<std::shared_ptr<cxxast::Expr>>{});
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genThisRef() {
					return std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::MemberAccess,
						std::make_shared<cxxast::IdExpr>("aotContext"),
						std::make_shared<cxxast::IdExpr>("thisObject"));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genMappedObjectsRef() {
					return std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::MemberAccess,
						genAotContextRef(),
						std::make_shared<cxxast::IdExpr>("mappedObjects", cxxast::GenericArgList{}));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genAotContextTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Scope,
							std::make_shared<cxxast::IdExpr>("slake", cxxast::GenericArgList{}),
							std::make_shared<cxxast::IdExpr>("AOTFnExecContext", cxxast::GenericArgList{})));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genFnOverloadingPtrTypeName() {
					return std::make_shared<cxxast::PointerTypeName>(
						std::make_shared<cxxast::CustomTypeName>(
							false,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::IdExpr>("slake", cxxast::GenericArgList{}),
								std::make_shared<cxxast::IdExpr>("FnOverloadingObject", cxxast::GenericArgList{}))));
				}

				SLAKE_FORCEINLINE cxxast::GenericArgList genSelfGenericArgs(const cxxast::GenericParamList &genericParams) {
					cxxast::GenericArgList args;

					for (size_t i = 0; i < genericParams.size(); ++i) {
						args.push_back(std::make_shared<cxxast::CustomTypeName>(
							false,
							std::make_shared<cxxast::IdExpr>(
								std::string(genericParams[i].name))));
					}

					return args;
				}

				std::shared_ptr<cxxast::Expr> genGetValueDataExpr(const Type &type, std::shared_ptr<cxxast::Expr> expr);
				std::shared_ptr<cxxast::Expr> genDefaultValue(const Type &type);

				std::pair<size_t, size_t> getLocalVarSizeAndAlignmentInfoOfType(const Type &type);

				std::string mangleGeneratorStateClassName(const std::string_view &fnName);
				std::string mangleJumpDestLabelName(uint32_t offIns);
				std::string mangleConstantObjectName(Object *object);
				std::string mangleRegLocalVarName(uint32_t idxReg);
				std::string mangleArgListLocalVarName(uint32_t idxReg);
				std::string mangleLocalVarName(uint32_t idxReg);
				std::string mangleParamName(uint32_t idxArg);
				std::string mangleRefForTypeName(const peff::DynArray<IdRefEntry> &entries);
				std::string mangleTypeName(const Type &type);
				std::string mangleClassName(const std::string &className, const GenericArgList &genericArgs);
				std::string mangleFnName(const std::string_view &fnName);
				std::string mangleFieldName(const std::string &fieldName);
				std::shared_ptr<cxxast::Namespace> completeModuleNamespace(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries);
				std::shared_ptr<cxxast::Expr> compileRef(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries);
				std::shared_ptr<cxxast::Expr> compileValue(CompileContext &compileContext, const Value &value);
				std::shared_ptr<cxxast::Expr> compileValueAsAny(CompileContext &compileContext, const Value &value);
				std::shared_ptr<cxxast::TypeName> compileType(CompileContext &compileContext, const Type &type);
				std::shared_ptr<cxxast::TypeName> compileParamType(CompileContext &compileContext, const Type &type);
				std::shared_ptr<cxxast::Fn> compileFnOverloading(CompileContext &compileContext, FnOverloadingObject *fnOverloadingObject);
				void recompileFnOverloading(CompileContext &compileContext, std::shared_ptr<cxxast::Fn> fnOverloading);
				std::shared_ptr<cxxast::Class> compileClass(CompileContext &compileContext, ClassObject *moduleObject);
				void compileModule(CompileContext &compileContext, ModuleObject *moduleObject);
				std::pair<std::shared_ptr<cxxast::IfndefDirective>, std::shared_ptr<cxxast::Namespace>> compile(ModuleObject *moduleObject);

				void dumpAstNode(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode);
			};
		}
	}
}

#endif
