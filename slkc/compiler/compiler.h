#ifndef _SLKC_COMPILER_COMPILER_H_
#define _SLKC_COMPILER_COMPILER_H_

#include "ast/ast.h"
#include <slake/runtime.h>
#include <iostream>
#include <fstream>

namespace slake {
	namespace slkc {
		enum class MessageType {
			Info = 0,
			Note,
			Warn,
			Error
		};

		struct Message {
			MessageType type;
			string msg;
			Location loc;

			Message(Location loc, const MessageType &type, const string &msg)
				: loc(loc), type(type), msg(msg) {
			}
		};

		class FatalCompilationError : public std::runtime_error {
		public:
			Message message;

			inline FatalCompilationError(Message message)
				: message(message),
				  runtime_error("Error at " + std::to_string(message.loc) + ": " + message.msg) {
			}
			virtual ~FatalCompilationError() = default;
		};

		enum class OptimizationLevel : uint8_t {
			Disabled = 0,  // Disable optimizations
			Safe,		   // Safe
			Speed,		   // Optimize for speed
			MemoryUsage	   // Optimize for memory usage
		};

		struct CompilerOptions {
			// Optimization level
			OptimizationLevel optimizationLevel = OptimizationLevel::Disabled;
		};

		using CompilerFlags = uint32_t;

		constexpr static CompilerFlags
			COMP_FAILED = 0x00000001,
			COMP_DELETING = 0x80000000;

		const size_t OUTPUT_SIZE_MAX = 0x20000000;

		class Compiler {
		private:
			enum class EvalPurpose {
				Stmt,	 // As a statement
				LValue,	 // As a lvalue
				RValue,	 // As a rvalue
				Call,	 // As the target of a calling expression
			};

			/// @brief Statement level context
			struct MinorContext {
				shared_ptr<TypeNameNode> expectedType;
				shared_ptr<Scope> curScope;

				uint32_t breakScopeLevel = 0;
				uint32_t continueScopeLevel = 0;
				string breakLabel, continueLabel;

				EvalPurpose evalPurpose;

				bool isLastCallTargetStatic = true;
				std::set<AstNode *> resolvedOwners;

				deque<shared_ptr<TypeNameNode>> argTypes;
				bool isArgTypesSet = false;

				shared_ptr<AstNode> evalDest, thisDest;
			};

			/// @brief Block level context
			struct MajorContext {
				deque<MinorContext> savedMinorContexts;
				MinorContext curMinorContext;

				unordered_map<string, shared_ptr<LocalVarNode>> localVars;
				uint32_t curRegCount;
				uint32_t curScopeLevel = 0;

				GenericParamNodeList genericParams;
				unordered_map<string, size_t> genericParamIndices;

				shared_ptr<TypeNameNode> thisType;

				inline void pushMinorContext() {
					savedMinorContexts.push_back(curMinorContext);
				}

				inline void popMinorContext() {
					savedMinorContexts.back().isLastCallTargetStatic = curMinorContext.isLastCallTargetStatic;
					curMinorContext = savedMinorContexts.back();
					savedMinorContexts.pop_back();
				}

				inline void mergeGenericParams(const GenericParamNodeList &genericParams) {
					this->genericParams.insert(
						this->genericParams.end(),
						genericParams.begin(),
						genericParams.end());
					genericParamIndices = genGenericParamIndicies(this->genericParams);
				}
			};

			MajorContext curMajorContext;
			shared_ptr<CompiledFnNode> curFn;

			struct ResolvedOwnersSaver {
				std::set<AstNode *> resolvedOwners;
				MinorContext &context;
				bool discarded = false;

				inline ResolvedOwnersSaver(MinorContext &context) : context(context), resolvedOwners(context.resolvedOwners) {}
				inline ~ResolvedOwnersSaver() {
					if (!discarded)
						context.resolvedOwners = resolvedOwners;
				}

				inline void discard() {
					discarded = true;
				}
			};

			shared_ptr<Scope> _rootScope = make_shared<Scope>();
			shared_ptr<ModuleNode> _targetModule;
			unique_ptr<Runtime> _rt;
			deque<MajorContext> _savedMajorContexts;

			void pushMajorContext();
			void popMajorContext();

			void pushMinorContext();
			void popMinorContext();

			shared_ptr<ExprNode> evalConstExpr(shared_ptr<ExprNode> expr);

			shared_ptr<TypeNameNode> evalExprType(shared_ptr<ExprNode> expr);

			shared_ptr<ExprNode> castLiteralExpr(shared_ptr<ExprNode> expr, Type targetType);

			bool isLiteralTypeName(shared_ptr<TypeNameNode> typeName);
			bool isNumericTypeName(shared_ptr<TypeNameNode> typeName);
			bool isDecimalType(shared_ptr<TypeNameNode> typeName);
			bool isCompoundTypeName(shared_ptr<TypeNameNode> typeName);
			bool isLValueType(shared_ptr<TypeNameNode> typeName);

			bool _isTypeNamesConvertible(shared_ptr<InterfaceNode> st, shared_ptr<ClassNode> dt);

			bool _isTypeNamesConvertible(shared_ptr<ClassNode> st, shared_ptr<InterfaceNode> dt);
			bool _isTypeNamesConvertible(shared_ptr<InterfaceNode> st, shared_ptr<InterfaceNode> dt);
			bool _isTypeNamesConvertible(shared_ptr<MemberNode> st, shared_ptr<TraitNode> dt);

			bool isTypeNamesConvertible(shared_ptr<TypeNameNode> src, shared_ptr<TypeNameNode> dest);

			bool _resolveRef(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);
			bool _resolveRefWithOwner(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);

			/// @brief Resolve a reference with current context.
			/// @note This method also updates resolved generic arguments.
			/// @param ref Reference to be resolved.
			/// @param refParts Divided minimum parts of the reference that can be loaded in a single time.
			/// @param resolvedPartsOut Where to store nodes referred by reference entries respectively.
			bool resolveRef(Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);

			/// @brief Resolve a reference with specified scope.
			/// @note This method also updates resolved generic arguments.
			/// @param scope Scope to be used for resolving.
			/// @param ref Reference to be resolved.
			/// @param resolvedPartsOut Where to store nodes referred by reference entries respectively.
			bool resolveRefWithScope(Scope *scope, Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);

			shared_ptr<FnOverloadingNode> argDependentLookup(Location loc, FnNode *fn, const deque<shared_ptr<TypeNameNode>> &argTypes, const deque<shared_ptr<TypeNameNode>> &genericArgs);

			shared_ptr<Scope> scopeOf(AstNode *node);

			/// @brief Resolve a custom type.
			/// @note This method also updates resolved generic arguments.
			/// @param typeName Type name to be resolved.
			/// @return Resolved node, nullptr otherwise.
			shared_ptr<AstNode> resolveCustomTypeName(CustomTypeNameNode *typeName);

			bool isSameType(shared_ptr<TypeNameNode> x, shared_ptr<TypeNameNode> y);

			string mangleName(
				string name,
				const deque<shared_ptr<TypeNameNode>> &argTypes,
				bool isConst);

			void _getFullName(MemberNode *member, Ref &ref);
			Ref getFullName(MemberNode *member);

			void compileExpr(shared_ptr<ExprNode> expr);
			inline void compileExpr(shared_ptr<ExprNode> expr, EvalPurpose evalPurpose, shared_ptr<AstNode> evalDest, shared_ptr<AstNode> thisDest = {}) {
				pushMinorContext();

				curMajorContext.curMinorContext.evalPurpose = evalPurpose;
				curMajorContext.curMinorContext.evalDest = evalDest;
				curMajorContext.curMinorContext.thisDest = thisDest;
				compileExpr(expr);

				popMinorContext();
			}
			void compileStmt(shared_ptr<StmtNode> stmt);

			void compileScope(std::istream &is, std::ostream &os, shared_ptr<Scope> scope);
			void compileRef(std::ostream &fs, const Ref &ref);
			void compileTypeName(std::ostream &fs, shared_ptr<TypeNameNode> typeName);
			void compileValue(std::ostream &fs, shared_ptr<AstNode> expr);
			void compileGenericParam(std::ostream &fs, shared_ptr<GenericParamNode> genericParam);

			bool isDynamicMember(shared_ptr<AstNode> member);

			uint32_t allocLocalVar(string name, shared_ptr<TypeNameNode> type);
			uint32_t allocReg(uint32_t nRegs = 1);

			set<Value *> importedDefinitions;

			struct ModuleRefComparator {
				inline bool operator()(const Ref &lhs, const Ref &rhs) const {
					if (lhs.size() < rhs.size())
						return true;
					if (lhs.size() > rhs.size())
						return false;

					for (size_t i = 0; i < lhs.size(); ++i) {
						if (!(lhs[i].name < rhs[i].name))
							return false;
					}

					return true;
				}
			};
			set<Ref, ModuleRefComparator> importedModules;

			static unique_ptr<ifstream> moduleLocator(Runtime *rt, ValueRef<RefValue> ref);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, BasicFnValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, ModuleValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, ClassValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, InterfaceValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, TraitValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, Value *value);
			void importModule(string name, const Ref &ref, shared_ptr<Scope> scope);
			shared_ptr<TypeNameNode> toTypeName(slake::Type runtimeType);
			Ref toAstRef(deque<slake::RefEntry> runtimeRefEntries);

			struct GenericNodeArgListComparator {
				inline bool operator()(
					const deque<shared_ptr<TypeNameNode>> &lhs,
					const deque<shared_ptr<TypeNameNode>> &rhs) const noexcept {
					if (lhs.size() < rhs.size())
						return true;
					if (lhs.size() > rhs.size())
						return false;

					for (size_t i = 0; i < lhs.size(); ++i) {
						auto l = lhs[i], r = rhs[i];
						auto lhsTypeId = l->getTypeId(), rhsTypeId = r->getTypeId();

						if (lhsTypeId < rhsTypeId)
							return true;
						else if (lhsTypeId > rhsTypeId)
							return false;
						else {
							//
							// Do some special checks for some kinds of type name - such as custom.
							//
							switch (lhsTypeId) {
								case Type::Custom: {
									auto lhsTypeName = static_pointer_cast<CustomTypeNameNode>(lhs[i]),
										 rhsTypeName = static_pointer_cast<CustomTypeNameNode>(rhs[i]);

									if (lhsTypeName->compiler < rhsTypeName->compiler)
										return true;
									else if (lhsTypeName->compiler > rhsTypeName->compiler)
										return false;
									else {
										auto lhsNode = lhsTypeName->compiler->resolveCustomTypeName(lhsTypeName.get()),
											 rhsNode = rhsTypeName->compiler->resolveCustomTypeName(rhsTypeName.get());

										if (lhsNode < rhsNode)
											return true;
										else if (lhsNode > rhsNode)
											return false;
									}

									break;
								}
							}
						}
					}

					return false;
				}
			};

			using GenericNodeCacheTable =
				std::map<
					deque<shared_ptr<TypeNameNode>>,  // Generic arguments.
					MemberNode *,					  // Cached instantiated value.
					GenericNodeArgListComparator>;

			using GenericNodeCacheDirectory = std::map<
				MemberNode *,  // Original uninstantiated generic value.
				GenericNodeCacheTable>;

			/// @brief Cached instances of generic values.
			mutable GenericNodeCacheDirectory _genericCacheDir;

			struct GenericNodeInstantiationContext {
				const deque<shared_ptr<TypeNameNode>> *genericArgs;
				unordered_map<string, shared_ptr<TypeNameNode>> mappedGenericArgs;
			};

			void walkTypeNameNodeForGenericInstantiation(
				shared_ptr<TypeNameNode> &type,
				GenericNodeInstantiationContext &instantiationContext);
			void walkNodeForGenericInstantiation(
				shared_ptr<AstNode> node,
				GenericNodeInstantiationContext &instantiationContext);
			void mapGenericParams(shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext);
			shared_ptr<MemberNode> instantiateGenericNode(shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext);

			shared_ptr<FnOverloadingNode> instantiateGenericFnOverloading(shared_ptr<FnOverloadingNode> overloading, GenericNodeInstantiationContext &instantiationContext);

			friend class AstVisitor;
			friend class MemberNode;
			friend string std::to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName);
			friend class Parser;

		public:
			deque<Message> messages;
			deque<string> modulePaths;
			CompilerOptions options;
			CompilerFlags flags = 0;

			inline Compiler(CompilerOptions options = {})
				: options(options), _rt(make_unique<Runtime>(RT_NOJIT)) {}
			~Compiler();

			void compile(std::istream &is, std::ostream &os);
		};
	}
}

#endif
