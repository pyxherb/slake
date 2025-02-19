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

				enum class CompilationTarget {
					None = 0,
					Module,
					Class,
					Fn,
					VarDef,
					Param
				};

				std::map<ObjectRef, std::shared_ptr<cxxast::AbstractMember>> runtimeObjectToAstNodeMap;
				std::map<std::shared_ptr<cxxast::AbstractMember>, ObjectRef> astNodeToRuntimeObjectMap;
				std::set<std::shared_ptr<cxxast::Fn>> recompilableFns;

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::AbstractMember> getMappedAstNode(Object *object) {
					if (auto it = runtimeObjectToAstNodeMap.find(ObjectRef::makeInstanceRef(object));
						it != runtimeObjectToAstNodeMap.end()) {
						return it->second;
					}
					return nullptr;
				}

				SLAKE_FORCEINLINE void registerRuntimeObjectToAstNodeRegistry(Object *object, std::shared_ptr<cxxast::AbstractMember> m) {
					runtimeObjectToAstNodeMap[ObjectRef::makeInstanceRef(object)] = m;
					astNodeToRuntimeObjectMap[m] = ObjectRef::makeInstanceRef(object);
				}

				SLAKE_FORCEINLINE void registerRuntimeObjectToAstNodeRegistry(ObjectRef objectRef, std::shared_ptr<cxxast::AbstractMember> m) {
					runtimeObjectToAstNodeMap[objectRef] = m;
					astNodeToRuntimeObjectMap[m] = objectRef;
				}

			private:
				void _dumpStmt(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode, size_t indentLevel);
				void _dumpAstNode(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode, size_t indentLevel);
				bool _isSimpleIdExpr(std::shared_ptr<cxxast::Expr> expr);
				std::shared_ptr<cxxast::Expr> _getAbsRef(std::shared_ptr<cxxast::AbstractMember> m);
				std::shared_ptr<cxxast::Expr> _getLastExpr(std::shared_ptr<cxxast::Expr> e);
				void _applyGenericArgs(std::shared_ptr<cxxast::Expr> expr, cxxast::GenericArgList &&args);

			public:
				struct DynamicCompileContextContents {
					CompilationTarget compilationTarget = CompilationTarget::None;
					std::map<uint32_t, std::string> vregNames;
				};

				struct CompileContext {
					Runtime *runtime;
					std::shared_ptr<cxxast::Namespace> rootNamespace;
					std::list<DynamicCompileContextContents> savedDynamicContents;
					DynamicCompileContextContents dynamicContents;
					std::set<HostObjectRef<>> constantObjects;

					SLAKE_FORCEINLINE CompileContext(Runtime *runtime, std::shared_ptr<cxxast::Namespace> rootNamespace) : runtime(runtime), rootNamespace(rootNamespace) {}

					SLAKE_FORCEINLINE void pushDynamicContents() {
						savedDynamicContents.push_back(dynamicContents);
					}

					SLAKE_FORCEINLINE void popDynamicContents() {
						dynamicContents = savedDynamicContents.back();
						savedDynamicContents.pop_back();
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

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genAnyTypeName() {
					return std::make_shared<cxxast::PointerTypeName>(
						std::make_shared<cxxast::CustomTypeName>(
							false,
							std::make_shared<cxxast::BinaryExpr>(
								cxxast::BinaryOp::Scope,
								std::make_shared<cxxast::IdExpr>("slake"),
								std::make_shared<cxxast::IdExpr>("Value"))));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genObjectRefTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::BinaryExpr>(
							cxxast::BinaryOp::Scope,
							std::make_shared<cxxast::IdExpr>("slake"),
							std::make_shared<cxxast::IdExpr>("ObjectRef")));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genSizeTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::IdExpr>("size_t"));
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

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::Expr> genConstantObjectsRef() {
					return std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::MemberAccess,
						genAotContextRef(),
						std::make_shared<cxxast::IdExpr>("constantObjects", cxxast::GenericArgList{}));
				}

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genAotContextTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::BinaryExpr>(
						cxxast::BinaryOp::Scope,
						std::make_shared<cxxast::IdExpr>("slake", cxxast::GenericArgList{}),
							std::make_shared<cxxast::IdExpr>("AOTFnExecContext", cxxast::GenericArgList{})));
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

				std::string mangleConstantObjectName(Object *object);
				std::string mangleRegLocalVarName(uint32_t idxReg);
				std::string mangleParamName(uint32_t idxArg);
				std::string mangleRefForTypeName(const peff::DynArray<IdRefEntry> &entries);
				std::string mangleTypeName(const Type &type);
				std::string mangleClassName(const std::string &className, const GenericArgList &genericArgs);
				std::string mangleFnName(const std::string_view &fnName);
				std::string mangleFieldName(const std::string &fieldName);
				std::shared_ptr<cxxast::Namespace> completeModuleNamespace(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries);
				std::shared_ptr<cxxast::Expr> compileRef(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries);
				std::shared_ptr<cxxast::Expr> compileValue(CompileContext &compileContext, const Value &value);
				std::shared_ptr<cxxast::TypeName> compileType(CompileContext &compileContext, const Type &type);
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
