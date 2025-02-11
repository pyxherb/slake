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
					Fn
				};

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

				SLAKE_FORCEINLINE std::shared_ptr<cxxast::TypeName> genSizeTypeName() {
					return std::make_shared<cxxast::CustomTypeName>(
						false,
						std::make_shared<cxxast::IdExpr>("size_t"));
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

				std::string mangleFnName(const std::string &fnName);
				std::string mangleOperatorName(const std::string &operatorName);
				std::string mangleFieldName(const std::string &fieldName);
				std::shared_ptr<cxxast::Namespace> completeModuleNamespace(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries);
				std::shared_ptr<cxxast::Expr> compileRef(CompileContext &compileContext, const peff::DynArray<IdRefEntry> &entries);
				std::shared_ptr<cxxast::Expr> compileValue(CompileContext &compileContext, const Value &value);
				std::shared_ptr<cxxast::TypeName> compileType(CompileContext &compileContext, const Type &type);
				std::shared_ptr<cxxast::FnOverloading> compileFnOverloading(CompileContext &compileContext, FnOverloadingObject *fnOverloadingObject);
				std::shared_ptr<cxxast::Fn> compileFn(CompileContext &compileContext, FnObject *fnObject);
				std::pair<std::shared_ptr<cxxast::Fn>, std::shared_ptr<cxxast::Fn>> separatePublicAndPrivateFn(CompileContext &compileContext, std::shared_ptr<cxxast::Fn> fn);
				std::shared_ptr<cxxast::Class> compileClass(CompileContext &compileContext, ClassObject *moduleObject);
				void compileModule(CompileContext &compileContext, ModuleObject *moduleObject);
				std::pair<std::shared_ptr<cxxast::IfndefDirective>, std::shared_ptr<cxxast::Namespace>> compile(ModuleObject *moduleObject);

				void dumpAstNode(std::ostream &os, std::shared_ptr<cxxast::ASTNode> astNode, ASTDumpMode dumpMode);
			};
		}
	}
}

#endif
