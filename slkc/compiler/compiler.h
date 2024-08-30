#ifndef _SLKC_COMPILER_COMPILER_H_
#define _SLKC_COMPILER_COMPILER_H_

#include "ast/ast.h"
#include <slake/runtime.h>
#include <iostream>
#include <fstream>

#include <config.h>

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
			std::string msg;
			SourceLocation loc;

			Message(SourceLocation loc, const MessageType &type, const std::string &msg)
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

		enum class EvalPurpose {
			None,	 // None
			Stmt,	 // As a statement
			LValue,	 // As a lvalue
			RValue,	 // As a rvalue
			Call,	 // As the target of a calling expression
		};

		/// @brief Statement level context
		struct MinorContext {
			bool dryRun = false;  // Dry run: Update semantic information but do not compile

			std::shared_ptr<TypeNameNode> expectedType;
			std::shared_ptr<TypeNameNode> evaluatedType;
			std::shared_ptr<Scope> curScope;

			std::unordered_map<std::string, std::shared_ptr<LocalVarNode>> localVars;

			uint32_t breakScopeLevel = 0;
			uint32_t continueScopeLevel = 0;
			std::string breakLabel, continueLabel;

			EvalPurpose evalPurpose = EvalPurpose::None;

			bool isLastResolvedTargetStatic = true;

			bool isLastCallTargetStatic = true;
			std::deque<std::shared_ptr<ParamNode>> lastCallTargetParams;
			std::shared_ptr<TypeNameNode> lastCallTargetReturnType;

			std::deque<std::shared_ptr<TypeNameNode>> argTypes;
			bool isArgTypesSet = false;

			std::shared_ptr<AstNode> evalDest, thisDest;
		};

		/// @brief Block level context
		struct MajorContext {
			std::deque<MinorContext> savedMinorContexts;
			MinorContext curMinorContext;

			uint32_t curRegCount = 0;
			uint32_t curScopeLevel = 0;

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			std::shared_ptr<TypeNameNode> thisType;

			bool isImport = false;

			inline void pushMinorContext() {
				savedMinorContexts.push_back(curMinorContext);
			}

			inline void popMinorContext() {
				auto &lastMinorContext = savedMinorContexts.back();
				lastMinorContext.isLastCallTargetStatic = curMinorContext.isLastCallTargetStatic;
				lastMinorContext.lastCallTargetParams = curMinorContext.lastCallTargetParams;
				lastMinorContext.lastCallTargetReturnType = curMinorContext.lastCallTargetReturnType;

				curMinorContext = savedMinorContexts.back();
				savedMinorContexts.pop_back();
			}
		};

#if SLKC_WITH_LANGUAGE_SERVER
		enum class SemanticType {
			None = 0,	 // None
			Type,		 // Type
			Class,		 // Class
			Enum,		 // Enumeration
			Interface,	 // Interface
			Struct,		 // Structure
			TypeParam,	 // Type parameter
			Param,		 // Parameter
			Var,		 // Variable
			Property,	 // Property
			EnumMember,	 // Enumeration Member
			Fn,			 // Function
			Method,		 // Method
			Keyword,	 // Keyword
			Modifier,	 // Modifier
			Comment,	 // Comment
			String,		 // String
			Number,		 // Number
			Operator	 // Operator
		};

		enum class SemanticTokenModifier {
			Decl = 0,
			Def,
			Readonly,
			Static,
			Deprecated,
			Abstract,
			Async
		};

		enum class CompletionContext {
			None = 0,	// No context
			TopLevel,	// User is entering in the top level.
			Class,		// User is entering in a class.
			Interface,	// User is entering in an interface.
			Stmt,		// User is entering a statement.

			Import,		   // User is entering an import item.
			Type,		   // User is entering a type name.
			Name,		   // User is entering name of an identifier.
			ModuleName,	   // User is entering name of a module.
			Expr,		   // User is entering an expression.
			MemberAccess,  // User is accessing an member.
		};

		struct TokenContext {
			std::unordered_map<std::string, std::shared_ptr<LocalVarNode>> localVars;

			std::shared_ptr<Scope> curScope;

			GenericParamNodeList genericParams;
			std::unordered_map<std::string, size_t> genericParamIndices;

			std::deque<std::shared_ptr<ParamNode>> params;
			std::unordered_map<std::string, size_t> paramIndices;

			TokenContext() = default;
			TokenContext(const TokenContext &) = default;
			TokenContext(TokenContext &&) = default;
			inline TokenContext(std::shared_ptr<CompiledFnNode> curFn, const MajorContext &majorContext) {
				localVars = majorContext.curMinorContext.localVars;
				curScope = majorContext.curMinorContext.curScope;
				genericParams = majorContext.genericParams;
				genericParamIndices = majorContext.genericParamIndices;
				if (curFn) {
					params = curFn->params;
					paramIndices = curFn->paramIndices;
				}
			}

			inline TokenContext(
				const std::unordered_map<std::string, std::shared_ptr<LocalVarNode>> localVars,
				std::shared_ptr<Scope> curScope,
				const GenericParamNodeList &genericParams,
				const std::unordered_map<std::string, size_t> &genericParamIndices,
				const std::deque<std::shared_ptr<ParamNode>> &params,
				const std::unordered_map<std::string, size_t> &paramIndices)
				: localVars(localVars),
				  curScope(curScope),
				  genericParams(genericParams),
				  genericParamIndices(genericParamIndices),
				  params(params),
				  paramIndices(paramIndices) {
			}

			TokenContext &operator=(const TokenContext &) = default;
			TokenContext &operator=(TokenContext &&) = default;
		};

		struct TokenInfo {
			std::string hoverInfo = "";
			SemanticType semanticType = SemanticType::None;
			std::set<SemanticTokenModifier> semanticModifiers;
			CompletionContext completionContext = CompletionContext::None;

			struct {
				bool isTopLevelRef = true;
				bool isStatic = true;

				std::shared_ptr<AstNode> correspondingMember;

				IdRef importedPath;
			} semanticInfo;

			// Corresponding token context, for completion.
			TokenContext tokenContext;
		};
#endif

		class Compiler {
		private:
			MajorContext curMajorContext;
			std::shared_ptr<CompiledFnNode> curFn;

			std::shared_ptr<Scope> _rootScope = std::make_shared<Scope>();
			std::shared_ptr<ModuleNode> _targetModule;
			std::unique_ptr<Runtime> _rt;
			std::deque<MajorContext> _savedMajorContexts;

			std::shared_ptr<ClassNode> _i32Class;

			void registerBuiltinTypedefs();

			void pushMajorContext();
			void popMajorContext();

			void pushMinorContext();
			void popMinorContext();

			std::shared_ptr<ExprNode> evalConstExpr(std::shared_ptr<ExprNode> expr);

			std::shared_ptr<TypeNameNode> evalExprType(std::shared_ptr<ExprNode> expr);

			std::shared_ptr<ExprNode> castLiteralExpr(std::shared_ptr<ExprNode> expr, TypeId targetType);

			bool isLiteralTypeName(std::shared_ptr<TypeNameNode> typeName);
			bool isNumericTypeName(std::shared_ptr<TypeNameNode> typeName);
			bool isDecimalType(std::shared_ptr<TypeNameNode> typeName);
			bool isCompoundTypeName(std::shared_ptr<TypeNameNode> typeName);
			bool isLValueType(std::shared_ptr<TypeNameNode> typeName);

			bool _isTypeNamesConvertible(std::shared_ptr<InterfaceNode> st, std::shared_ptr<ClassNode> dt);
			bool _isTypeNamesConvertible(std::shared_ptr<ClassNode> st, std::shared_ptr<InterfaceNode> dt);
			bool _isTypeNamesConvertible(std::shared_ptr<InterfaceNode> st, std::shared_ptr<InterfaceNode> dt);
			bool _isTypeNamesConvertible(std::shared_ptr<ClassNode> st, std::shared_ptr<ClassNode> dt);

			bool isTypeNamesConvertible(std::shared_ptr<TypeNameNode> src, std::shared_ptr<TypeNameNode> dest);

			int getTypeNameWeight(std::shared_ptr<TypeNameNode> t);
			std::shared_ptr<TypeNameNode> getStrongerTypeName(std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y);

			struct IdRefResolveContext {
				bool isTopLevel = true;
				bool keepTokenScope = false;
				bool isStatic = true;
				std::set<Scope *> resolvingScopes;	// Current resolving scopes, used for breaking looping resolving.
			};

			bool _resolveIdRef(Scope *scope, const IdRef &ref, std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> &partsOut, IdRefResolveContext resolveContext);
			bool _resolveIdRefWithOwner(Scope *scope, const IdRef &ref, std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> &partsOut, IdRefResolveContext resolveContext);

			/// @brief Resolve a reference with current context.
			/// @note This method also updates resolved generic arguments.
			/// @param ref Reference to be resolved.
			/// @param refParts Divided minimum parts of the reference that can be loaded in a single time.
			/// @param resolvedPartsOut Where to store nodes referred by reference entries respectively.
			bool resolveIdRef(IdRef ref, std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> &partsOut, bool ignoreDynamicPrecedings = false);

			/// @brief Resolve a reference with specified scope.
			/// @note This method also updates resolved generic arguments.
			/// @param scope Scope to be used for resolving.
			/// @param ref Reference to be resolved.
			/// @param resolvedPartsOut Where to store nodes referred by reference entries respectively.
			bool resolveIdRefWithScope(Scope *scope, IdRef ref, std::deque<std::pair<IdRef, std::shared_ptr<AstNode>>> &partsOut);

			void _argDependentLookup(
				FnNode *fn,
				const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
				const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
				std::deque<std::shared_ptr<FnOverloadingNode>> &overloadingsOut,
				bool isStatic);
			std::deque<std::shared_ptr<FnOverloadingNode>> argDependentLookup(
				FnNode *fn,
				const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
				const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
				bool isStatic = false);

			std::shared_ptr<Scope> scopeOf(AstNode *node);

			std::shared_ptr<AstNode> _resolveCustomTypeName(CustomTypeNameNode *typeName, const std::set<Scope *> &resolvingScopes);
			/// @brief Resolve a custom type.
			/// @note This method also updates resolved generic arguments.
			/// @param typeName Type name to be resolved.
			/// @return Resolved node, nullptr otherwise.
			std::shared_ptr<AstNode> resolveCustomTypeName(CustomTypeNameNode *typeName);

			bool isSameType(std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y);

			void _getFullName(MemberNode *member, IdRef &ref);

			struct UnaryOpRegistry {
				slake::Opcode opcode;
				bool lvalueOperand;
				bool lvalueResult;
			};
			static std::map<UnaryOp, UnaryOpRegistry> _unaryOpRegs;

			struct BinaryOpRegistry {
				slake::Opcode opcode;
				bool isLhsLvalue;
				bool isRhsLvalue;
				bool isResultLvalue;
			};
			static std::map<BinaryOp, BinaryOpRegistry> _binaryOpRegs;

			void compileUnaryOpExpr(std::shared_ptr<UnaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType);
			void compileBinaryOpExpr(std::shared_ptr<BinaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType, std::shared_ptr<TypeNameNode> rhsType);
			void compileExpr(std::shared_ptr<ExprNode> expr);
			inline void compileExpr(std::shared_ptr<ExprNode> expr, EvalPurpose evalPurpose, std::shared_ptr<AstNode> evalDest, std::shared_ptr<AstNode> thisDest = {}) {
				pushMinorContext();

				curMajorContext.curMinorContext.evalPurpose = evalPurpose;
				curMajorContext.curMinorContext.evalDest = evalDest;
				curMajorContext.curMinorContext.thisDest = thisDest;
				compileExpr(expr);

				popMinorContext();
			}
			void compileStmt(std::shared_ptr<StmtNode> stmt);

			void compileScope(std::istream &is, std::ostream &os, std::shared_ptr<Scope> scope);
			void compileIdRef(std::ostream &fs, const IdRef &ref);
			void compileTypeName(std::ostream &fs, std::shared_ptr<TypeNameNode> typeName);
			void compileValue(std::ostream &fs, std::shared_ptr<AstNode> expr);
			void compileGenericParam(std::ostream &fs, std::shared_ptr<GenericParamNode> genericParam);

			bool isDynamicMember(std::shared_ptr<AstNode> member);

			uint32_t allocLocalVar(std::string name, std::shared_ptr<TypeNameNode> type);
			uint32_t allocReg();

			inline void _insertIns(Ins ins) {
				if (curMajorContext.curMinorContext.dryRun)
					return;
				curFn->insertIns(ins);
			}
			inline void _insertIns(
				Opcode opcode,
				std::shared_ptr<AstNode> output,
				std::deque<std::shared_ptr<AstNode>> operands) {
				_insertIns(Ins{ opcode, output, operands });
			}
			inline void _insertLabel(std::string name) {
				if (curMajorContext.curMinorContext.dryRun)
					return;
				curFn->insertLabel(name);
			}

			//
			// Import begin
			//

			std::set<Object *> importedDefinitions;

			struct ModuleRefComparator {
				inline bool operator()(const IdRef &lhs, const IdRef &rhs) const {
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
			std::set<IdRef, ModuleRefComparator> importedModules;

			static std::unique_ptr<std::ifstream> moduleLocator(Runtime *rt, HostObjectRef<IdRefObject> ref);
			std::shared_ptr<Scope> completeModuleNamespaces(const IdRef &ref);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, FnObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ModuleObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ClassObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, InterfaceObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, Object *value);
			void importModule(const IdRef &ref);
			std::shared_ptr<TypeNameNode> toTypeName(slake::Type runtimeType);
			IdRef toAstIdRef(std::deque<slake::IdRefEntry> runtimeRefEntries);

			//
			// Import end
			//

			//
			// Generic begin
			//

			struct GenericNodeArgListComparator {
				inline bool operator()(
					const std::deque<std::shared_ptr<TypeNameNode>> &lhs,
					const std::deque<std::shared_ptr<TypeNameNode>> &rhs) const noexcept {
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
								case TypeId::Custom: {
									auto lhsTypeName = std::static_pointer_cast<CustomTypeNameNode>(lhs[i]),
										 rhsTypeName = std::static_pointer_cast<CustomTypeNameNode>(rhs[i]);

									if (lhsTypeName->compiler < rhsTypeName->compiler)
										return true;
									else if (lhsTypeName->compiler > rhsTypeName->compiler)
										return false;
									else {
										std::shared_ptr<AstNode> lhsNode, rhsNode;
										try {
											lhsNode = lhsTypeName->compiler->resolveCustomTypeName(lhsTypeName.get());
											rhsNode = rhsTypeName->compiler->resolveCustomTypeName(rhsTypeName.get());
										} catch (FatalCompilationError e) {
											// Supress the exceptions - the function should be noexcept, we have to raise compilation errors out of the comparator.
										}

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

			bool isParamTypesSame(const std::deque<std::shared_ptr<ParamNode>> &lhs, const std::deque<std::shared_ptr<ParamNode>> &rhs) {
				if (lhs.size() == rhs.size()) {
					for (size_t l = 0; l < rhs.size(); ++l) {
						if (!isSameType(lhs[l]->type, rhs[l]->type))
							return true;
					}

					return false;
				}

				return true;
			}

			using GenericNodeCacheTable =
				std::map<
					std::deque<std::shared_ptr<TypeNameNode>>,	// Generic arguments.
					std::shared_ptr<MemberNode>,				// Cached instantiated value.
					GenericNodeArgListComparator>;

			using GenericNodeCacheDirectory = std::map<
				MemberNode *,  // Original uninstantiated generic value.
				GenericNodeCacheTable>;

			/// @brief Cached instances of generic values.
			mutable GenericNodeCacheDirectory _genericCacheDir;

			struct GenericNodeInstantiationContext {
				const std::deque<std::shared_ptr<TypeNameNode>> *genericArgs;
				std::unordered_map<std::string, std::shared_ptr<TypeNameNode>> mappedGenericArgs;
				std::shared_ptr<MemberNode> mappedNode = {};
			};

			void walkTypeNameNodeForGenericInstantiation(
				std::shared_ptr<TypeNameNode> &type,
				GenericNodeInstantiationContext &instantiationContext);
			void walkNodeForGenericInstantiation(
				std::shared_ptr<AstNode> node,
				GenericNodeInstantiationContext &instantiationContext);
			void mapGenericParams(std::shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext);
			std::shared_ptr<MemberNode> instantiateGenericNode(std::shared_ptr<MemberNode> node, GenericNodeInstantiationContext &instantiationContext);

			std::shared_ptr<FnOverloadingNode> instantiateGenericFnOverloading(std::shared_ptr<FnOverloadingNode> overloading, GenericNodeInstantiationContext &instantiationContext);

			//
			// Generic end
			//

			//
			// Semantic begin
			//
#if SLKC_WITH_LANGUAGE_SERVER
			void updateCompletionContext(size_t idxToken, CompletionContext completionContext);
			void updateCompletionContext(std::shared_ptr<TypeNameNode> targetTypeName, CompletionContext completionContext);
			void updateCompletionContext(const IdRef &ref, CompletionContext completionContext);

			void updateSemanticType(size_t idxToken, SemanticType type);
			void updateSemanticType(std::shared_ptr<TypeNameNode> targetTypeName, SemanticType type);
			void updateSemanticType(const IdRef &ref, SemanticType type);

			void updateTokenInfo(size_t idxToken, std::function<void(TokenInfo &info)> updater);
#endif

			//
			// Semantic end
			//

			//
			// Verify begin
			//

			void verifyInheritanceChain(ClassNode *node, std::set<AstNode *> &walkedNodes);
			void verifyInheritanceChain(InterfaceNode *node, std::set<AstNode *> &walkedNodes);
			void verifyInheritanceChain(GenericParamNode *node, std::set<AstNode *> &walkedNodes);

			inline void verifyInheritanceChain(ClassNode *node) {
				std::set<AstNode *> walkedNodes;
				verifyInheritanceChain(node, walkedNodes);
			}
			inline void verifyInheritanceChain(InterfaceNode *node) {
				std::set<AstNode *> walkedNodes;
				verifyInheritanceChain(node, walkedNodes);
			}
			inline void verifyInheritanceChain(GenericParamNode *node) {
				std::set<AstNode *> walkedNodes;
				verifyInheritanceChain(node, walkedNodes);
			}

			void verifyGenericParams(const GenericParamNodeList &params);

			void collectMethodsForFulfillmentVerification(std::shared_ptr<Scope> scope, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut);
			void collectMethodsForFulfillmentVerification(InterfaceNode *node, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut);
			void collectMethodsForFulfillmentVerification(ClassNode *node, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut);
			void verifyIfImplementationFulfilled(std::shared_ptr<ClassNode> node);

			//
			// Verify end
			//

			void validateScope(Scope *scope);
			void scanAndLinkParentFns(Scope *scope, FnNode *fn, const std::string &name);

			std::shared_ptr<Scope> mergeScope(Scope *a, Scope *b, bool keepStaticMembers = false);

			friend class AstVisitor;
			friend class MemberNode;
			friend std::string std::to_string(std::shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool forMangling);
			friend class Parser;
			friend struct Document;

		public:
#if SLKC_WITH_LANGUAGE_SERVER
			std::deque<TokenInfo> tokenInfos;
#endif
			std::deque<Message> messages;
			std::deque<std::string> modulePaths;
			CompilerOptions options;
			CompilerFlags flags = 0;
			std::unique_ptr<Lexer> lexer;

			inline Compiler(CompilerOptions options = {})
				: options(options), _rt(std::make_unique<Runtime>(RT_NOJIT)) {}
			~Compiler();

			void compile(std::istream &is, std::ostream &os, std::shared_ptr<ModuleNode> targetModule = {});

			void reset();

			IdRef getFullName(MemberNode *member);

			void optimizeFn(std::shared_ptr<FnNode> fnNode);
		};
	}
}

#endif
