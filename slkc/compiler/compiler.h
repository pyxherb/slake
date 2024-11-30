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
			SourceLocation sourceLocation;

			Message(SourceLocation sourceLocation, const MessageType &type, const std::string &msg)
				: sourceLocation(sourceLocation), type(type), msg(msg) {
			}
		};

		class FatalCompilationError : public std::runtime_error {
		public:
			Message message;

			inline FatalCompilationError(Message message)
				: message(message),
				  runtime_error("Error at " + std::to_string(message.sourceLocation) + ": " + message.msg) {
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
			bool hasVarArgs = false;

			std::deque<std::shared_ptr<TypeNameNode>> argTypes;
			bool isArgTypesSet = false;

			std::shared_ptr<AstNode> evalDest, thisDest;

#if SLKC_WITH_LANGUAGE_SERVER
			// For argument expression snippets.
			size_t curCorrespondingArgIndex;
			std::shared_ptr<ParamNode> curCorrespondingParam;
#endif
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
			ArgExpr,	   // User is entering an expression as an argument.
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

				std::shared_ptr<ParamNode> correspondingParam;

				std::shared_ptr<IdRefNode> importedPath;
			} semanticInfo;

			// Corresponding token context, for completion.
			TokenContext tokenContext;
		};
#endif

		struct SourceDocument {
			Compiler *compiler;
			std::shared_ptr<ModuleNode> targetModule;
			std::unique_ptr<Lexer> lexer;
			std::deque<Message> messages;
			std::mutex mutex;
#if SLKC_WITH_LANGUAGE_SERVER
			std::deque<TokenInfo> tokenInfos;
#endif
		};

		class RootNode : public MemberNode {
		public:
			RootNode() = default;
			inline RootNode(Compiler *compiler)
				: MemberNode(compiler, 0) {
				setScope(std::make_shared<Scope>());
			}
			virtual ~RootNode() = default;

			virtual inline NodeType getNodeType() const override { return NodeType::Root; }

			virtual IdRefEntry getName() const override {
				return IdRefEntry(tokenRange, SIZE_MAX, "::", {});
			}
		};

		struct CompileContext {
			Compiler *compiler;
			MajorContext curMajorContext;
			std::shared_ptr<FnOverloadingNode> curFnOverloading;
			std::shared_ptr<CompiledFnNode> curFn;
			std::deque<MajorContext> savedMajorContexts;

			void pushMajorContext();
			void popMajorContext();

			void pushMinorContext();
			void popMinorContext();

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

			uint32_t allocLocalVar(std::string name, std::shared_ptr<TypeNameNode> type);
			uint32_t allocReg();
		};

		class Compiler {
		private:

			std::unique_ptr<Runtime> associatedRuntime;

			std::shared_ptr<ClassNode> _i32Class;

			void registerBuiltinTypedefs();

			std::shared_ptr<ExprNode> evalConstExpr(CompileContext *compileContext, std::shared_ptr<ExprNode> expr);

			std::shared_ptr<TypeNameNode> evalExprType(CompileContext *compileContext, std::shared_ptr<ExprNode> expr);

			std::shared_ptr<ExprNode> castLiteralExpr(std::shared_ptr<ExprNode> expr, TypeId targetType);

			bool isLiteralTypeName(std::shared_ptr<TypeNameNode> typeName);
			bool isNumericTypeName(std::shared_ptr<TypeNameNode> typeName);
			bool isDecimalType(std::shared_ptr<TypeNameNode> typeName);
			bool isCompoundTypeName(std::shared_ptr<TypeNameNode> typeName);
			bool isLValueType(std::shared_ptr<TypeNameNode> typeName);
			std::shared_ptr<TypeNameNode> toRValueTypeName(std::shared_ptr<TypeNameNode> typeName);
			std::shared_ptr<TypeNameNode> toLValueTypeName(std::shared_ptr<TypeNameNode> typeName);

			bool _isTypeNamesConvertible(CompileContext *compileContext, std::shared_ptr<InterfaceNode> st, std::shared_ptr<ClassNode> dt);
			bool _isTypeNamesConvertible(CompileContext *compileContext, std::shared_ptr<ClassNode> st, std::shared_ptr<InterfaceNode> dt);
			bool _isTypeNamesConvertible(CompileContext *compileContext, std::shared_ptr<InterfaceNode> st, std::shared_ptr<InterfaceNode> dt);
			bool _isTypeNamesConvertible(CompileContext *compileContext, std::shared_ptr<ClassNode> st, std::shared_ptr<ClassNode> dt);

			bool isTypeNamesConvertible(CompileContext *compileContext, std::shared_ptr<TypeNameNode> src, std::shared_ptr<TypeNameNode> dest);

			int getTypeNameWeight(std::shared_ptr<TypeNameNode> t);
			std::shared_ptr<TypeNameNode> getStrongerTypeName(std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y);

			struct IdRefResolveContext {
				size_t curIndex = 0;
				bool isTopLevel = true;
				bool keepTokenScope = false;
				bool isStatic = true;
				std::set<Scope *> resolvingScopes;	// Current resolving scopes, used for breaking looping resolving.
			};

			using IdRefResolvedPart = std::pair<std::shared_ptr<IdRefNode>, std::shared_ptr<AstNode>>;
			using IdRefResolvedParts = std::deque<IdRefResolvedPart>;

			bool _resolveIdRef(CompileContext *compileContext, Scope *scope, std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut, bool &isStaticOut, IdRefResolveContext resolveContext);
			bool _resolveIdRefWithOwner(CompileContext *compileContext, Scope *scope, std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut, bool &isStaticOut, IdRefResolveContext resolveContext);

			/// @brief Resolve a reference with current context.
			/// @note This method also updates resolved generic arguments.
			/// @param ref Reference to be resolved.
			/// @param refParts Divided minimum parts of the reference that can be loaded in a single time.
			/// @param resolvedPartsOut Where to store nodes referred by reference entries respectively.
			bool resolveIdRef(CompileContext *compileContext, std::shared_ptr<IdRefNode> ref, IdRefResolvedParts &partsOut, bool &isStaticOut, bool ignoreDynamicPrecedings = false);

			/// @brief Resolve a reference with specified scope.
			/// @note This method also updates resolved generic arguments.
			/// @param scope Scope to be used for resolving.
			/// @param ref Reference to be resolved.
			/// @param resolvedPartsOut Where to store nodes referred by reference entries respectively.
			bool resolveIdRefWithScope(CompileContext *compileContext, Scope *scope, std::shared_ptr<IdRefNode> ref, bool &isStaticOut, IdRefResolvedParts &partsOut);

			void _argDependentLookup(
				CompileContext *compileContext,
				FnNode *fn,
				const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
				const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
				std::deque<std::shared_ptr<FnOverloadingNode>> &overloadingsOut,
				bool isStatic);
			std::deque<std::shared_ptr<FnOverloadingNode>> argDependentLookup(
				CompileContext *compileContext,
				FnNode *fn,
				const std::deque<std::shared_ptr<TypeNameNode>> &argTypes,
				const std::deque<std::shared_ptr<TypeNameNode>> &genericArgs,
				bool isStatic = false);

			std::shared_ptr<Scope> scopeOf(CompileContext *compileContext, AstNode *node);

			std::shared_ptr<AstNode> _resolveCustomTypeName(CompileContext *compileContext, CustomTypeNameNode *typeName, const std::set<Scope *> &resolvingScopes);

			bool isSameType(CompileContext *compileContext, std::shared_ptr<TypeNameNode> x, std::shared_ptr<TypeNameNode> y);

			void _getFullName(MemberNode *member, IdRefEntries &ref);

			void compileUnaryOpExpr(CompileContext *compileContext, std::shared_ptr<UnaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType);
			void compileBinaryOpExpr(CompileContext *compileContext, std::shared_ptr<BinaryOpExprNode> e, std::shared_ptr<TypeNameNode> lhsType, std::shared_ptr<TypeNameNode> rhsType);
			void compileExpr(CompileContext *compileContext, std::shared_ptr<ExprNode> expr);
			inline void compileExpr(CompileContext *compileContext, std::shared_ptr<ExprNode> expr, EvalPurpose evalPurpose, std::shared_ptr<AstNode> evalDest, std::shared_ptr<AstNode> thisDest = {}) {
				compileContext->pushMinorContext();

				compileContext->curMajorContext.curMinorContext.evalPurpose = evalPurpose;
				compileContext->curMajorContext.curMinorContext.evalDest = evalDest;
				compileContext->curMajorContext.curMinorContext.thisDest = thisDest;
				compileExpr(compileContext, expr);

				compileContext->popMinorContext();
			}
			void compileStmt(CompileContext *compileContext, std::shared_ptr<StmtNode> stmt);

			void compileScope(CompileContext *compileContext, std::istream &is, std::ostream &os, std::shared_ptr<Scope> scope);
			void compileIdRef(CompileContext *compileContext, std::ostream &fs, std::shared_ptr<IdRefNode> ref);
			void compileTypeName(CompileContext *compileContext, std::ostream &fs, std::shared_ptr<TypeNameNode> typeName);
			void compileValue(CompileContext *compileContext, std::ostream &fs, std::shared_ptr<AstNode> expr);
			void compileGenericParam(CompileContext *compileContext, std::ostream &fs, std::shared_ptr<GenericParamNode> genericParam);

			bool isDynamicMember(std::shared_ptr<AstNode> member);

			//
			// Import begin
			//

			std::set<Object *> importedDefinitions;

			struct ModuleRefComparator {
				inline bool operator()(const IdRefEntries &lhs, const IdRefEntries &rhs) const {
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

			struct ModuleImportInfo {
				std::string docName;
			};
			std::map<IdRefEntries, ModuleImportInfo, ModuleRefComparator> importedModules;

			static std::unique_ptr<std::ifstream> moduleLocator(Runtime *rt, HostObjectRef<IdRefObject> ref);
			std::shared_ptr<Scope> completeModuleNamespaces(std::shared_ptr<IdRefNode> ref);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, FnObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ModuleObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, ClassObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, InterfaceObject *value);
			void importDefinitions(std::shared_ptr<Scope> scope, std::shared_ptr<MemberNode> parent, Object *value);
			void importModule(std::shared_ptr<IdRefNode> ref);
			std::shared_ptr<TypeNameNode> toTypeName(slake::Type runtimeType);
			std::shared_ptr<IdRefNode> toAstIdRef(std::pmr::deque<slake::IdRefEntry> runtimeRefEntries);
			std::shared_ptr<IdRefNode> toAstIdRef(std::deque<slake::IdRefEntry> runtimeRefEntries);

			//
			// Import end
			//

			//
			// Generic begin
			//

			struct GenericNodeArgListComparator {
				bool operator()(
					const std::deque<std::shared_ptr<TypeNameNode>> &lhs,
					const std::deque<std::shared_ptr<TypeNameNode>> &rhs) const noexcept;
			};

			bool isFnOverloadingDuplicated(CompileContext *compileContext, std::shared_ptr<FnOverloadingNode> lhs, std::shared_ptr<FnOverloadingNode> rhs) {
				if (lhs->params.size() != rhs->params.size())
					return false;

				for (size_t i = 0; i < lhs->params.size(); ++i) {
					if (!isSameType(compileContext, lhs->params[i]->type, rhs->params[i]->type))
						return false;
				}

				if (lhs->genericParams.size() != rhs->genericParams.size())
					return false;

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
			void updateCompletionContext(std::shared_ptr<IdRefNode> ref, CompletionContext completionContext);

			void updateSemanticType(size_t idxToken, SemanticType type);
			void updateSemanticType(std::shared_ptr<TypeNameNode> targetTypeName, SemanticType type);
			void updateSemanticType(std::shared_ptr<IdRefNode> ref, SemanticType type);

			void updateTokenInfo(size_t idxToken, std::function<void(TokenInfo &info)> updater);
#endif

			//
			// Semantic end
			//

			//
			// Verify begin
			//

			void verifyInheritanceChain(CompileContext *compileContext, ClassNode *node, std::set<AstNode *> &walkedNodes);
			void verifyInheritanceChain(CompileContext *compileContext, InterfaceNode *node, std::set<AstNode *> &walkedNodes);
			void verifyInheritanceChain(CompileContext *compileContext, GenericParamNode *node, std::set<AstNode *> &walkedNodes);

			inline void verifyInheritanceChain(CompileContext *compileContext, ClassNode *node) {
				std::set<AstNode *> walkedNodes;
				verifyInheritanceChain(compileContext, node, walkedNodes);
			}
			inline void verifyInheritanceChain(CompileContext *compileContext, InterfaceNode *node) {
				std::set<AstNode *> walkedNodes;
				verifyInheritanceChain(compileContext, node, walkedNodes);
			}
			inline void verifyInheritanceChain(CompileContext *compileContext, GenericParamNode *node) {
				std::set<AstNode *> walkedNodes;
				verifyInheritanceChain(compileContext, node, walkedNodes);
			}

			void verifyGenericParams(CompileContext *compileContext, const GenericParamNodeList &params);

			void collectMethodsForFulfillmentVerification(CompileContext *compileContext, std::shared_ptr<Scope> scope, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut);
			void collectMethodsForFulfillmentVerification(CompileContext *compileContext, InterfaceNode *node, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut);
			void collectMethodsForFulfillmentVerification(CompileContext *compileContext, ClassNode *node, std::unordered_map<std::string, std::set<std::shared_ptr<FnOverloadingNode>>> &unfilledMethodsOut);
			void verifyIfImplementationFulfilled(CompileContext *compileContext, std::shared_ptr<ClassNode> node);

			//
			// Verify end
			//

			void validateScope(CompileContext *compileContext, Scope *scope);
			void scanAndLinkParentFns(Scope *scope, FnNode *fn, const std::string &name);

			std::shared_ptr<Scope> mergeScope(CompileContext *compileContext, Scope *a, Scope *b, bool keepStaticMembers = false);

			friend class MemberNode;
			friend std::string std::to_string(std::shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool forMangling);
			friend class Parser;

		public:
			std::shared_ptr<RootNode> _rootNode = std::make_shared<RootNode>(this);
			std::unordered_map<std::string, std::unique_ptr<SourceDocument>> sourceDocs;

			std::string curDocName;
			std::string mainDocName;

			std::deque<std::string> modulePaths;
			CompilerOptions options;
			CompilerFlags flags = 0;

			inline Compiler(CompilerOptions options = {})
				: options(options), associatedRuntime(std::make_unique<Runtime>(std::pmr::get_default_resource(), RT_NOJIT)) {}
			~Compiler();

			/// @brief Resolve a custom type.
			/// @note This method also updates resolved generic arguments.
			/// @param typeName Type name to be resolved.
			/// @return Resolved node, nullptr otherwise.
			std::shared_ptr<AstNode> resolveCustomTypeName(CompileContext *compileContext, CustomTypeNameNode *typeName);

			void pushMessage(const std::string &docName, const Message &message);

			inline SourceDocument *addDoc(const std::string &docName) {
				auto doc = std::make_unique<SourceDocument>();

				SourceDocument *pDoc = doc.get();

				doc->compiler = this;
				doc->lexer = std::make_unique<Lexer>();
				doc->targetModule = std::make_shared<ModuleNode>(this);

				sourceDocs[docName] = std::move(doc);

				return pDoc;
			}
			void reloadDoc(const std::string &docName);
			inline void removeDoc(const std::string &docName) {
				reloadDoc(docName);

				sourceDocs.erase(docName);
			}

			void compile(std::istream &is, std::ostream &os);

			void reload();

			std::shared_ptr<IdRefNode> getFullName(MemberNode *member);

			inline SourceLocation tokenRangeToSourceLocation(const TokenRange &tokenRange) {
				return SourceLocation{
					tokenRange.document->lexer->tokens[tokenRange.beginIndex]->location.beginPosition,
					tokenRange.document->lexer->tokens[tokenRange.endIndex]->location.endPosition
				};
			}

			SourceDocument *getCurDoc() {
				return sourceDocs.at(curDocName).get();
			}
		};
	}
}

#endif
