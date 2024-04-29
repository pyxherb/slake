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

		enum class EvalPurpose {
			Stmt,	 // As a statement
			LValue,	 // As a lvalue
			RValue,	 // As a rvalue
			Call,	 // As the target of a calling expression
		};

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
			Trait,		// User is entering in a trait.
			Stmt,		// User is entering a statement.

			Import,		   // User is entering an import item.
			Type,		   // User is entering a type name.
			Name,		   // User is entering name of an identifier.
			Expr,		   // User is entering an expression.
			MemberAccess,  // User is accessing an member.
		};

		/// @brief Statement level context
		struct MinorContext {
			shared_ptr<TypeNameNode> expectedType;
			shared_ptr<Scope> curScope;

			unordered_map<string, shared_ptr<LocalVarNode>> localVars;

			uint32_t breakScopeLevel = 0;
			uint32_t continueScopeLevel = 0;
			string breakLabel, continueLabel;

			EvalPurpose evalPurpose;

			bool isLastCallTargetStatic = true;

			deque<shared_ptr<TypeNameNode>> argTypes;
			bool isArgTypesSet = false;

			shared_ptr<AstNode> evalDest, thisDest;
		};

		/// @brief Block level context
		struct MajorContext {
			deque<MinorContext> savedMinorContexts;
			MinorContext curMinorContext;

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

		struct TokenContext {
			unordered_map<string, shared_ptr<LocalVarNode>> localVars;

			shared_ptr<Scope> curScope;

			GenericParamNodeList genericParams;
			unordered_map<string, size_t> genericParamIndices;

			deque<shared_ptr<ParamNode>> params;
			unordered_map<string, size_t> paramIndices;

			TokenContext() = default;
			TokenContext(const TokenContext &) = default;
			TokenContext(TokenContext &&) = default;
			inline TokenContext(shared_ptr<CompiledFnNode> curFn, const MajorContext &majorContext) {
				localVars = majorContext.curMinorContext.localVars;
				curScope = majorContext.curMinorContext.curScope;
				genericParams = majorContext.genericParams;
				genericParamIndices = majorContext.genericParamIndices;
				if (curFn) {
					params = curFn->params;
					paramIndices = curFn->paramIndices;
				}
			}

			TokenContext &operator=(const TokenContext &) = default;
			TokenContext &operator=(TokenContext &&) = default;

			inline TokenContext toMemberAccessContext() {
				TokenContext context;

				context.curScope = this->curScope;

				return context;
			}
		};

		struct TokenInfo {
			string hoverInfo = "";
			SemanticType semanticType = SemanticType::None;
			std::set<SemanticTokenModifier> semanticModifiers;
			CompletionContext completionContext = CompletionContext::None;

			struct {
				std::string name;

				Ref ref;

				shared_ptr<AstNode> correspondingMember;
			} semanticInfo;

			// Corresponding token context, for completion.
			TokenContext tokenContext;
		};

		class Compiler {
		private:
			MajorContext curMajorContext;
			shared_ptr<CompiledFnNode> curFn;

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

			struct RefResolveContext {
				bool isTopLevel = true;
				bool keepTokenScope = false;
			};

			bool _resolveRef(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut, const RefResolveContext &resolveContext);
			bool _resolveRefWithOwner(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut, const RefResolveContext &resolveContext);

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

			shared_ptr<FnOverloadingNode> argDependentLookup(
				Location loc,
				FnNode *fn,
				const deque<shared_ptr<TypeNameNode>> &argTypes,
				const deque<shared_ptr<TypeNameNode>> &genericArgs);

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

			//
			// Generic begin
			//

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
					shared_ptr<MemberNode>,			  // Cached instantiated value.
					GenericNodeArgListComparator>;

			using GenericNodeCacheDirectory = std::map<
				MemberNode *,  // Original uninstantiated generic value.
				GenericNodeCacheTable>;

			/// @brief Cached instances of generic values.
			mutable GenericNodeCacheDirectory _genericCacheDir;

			struct GenericNodeInstantiationContext {
				const deque<shared_ptr<TypeNameNode>> *genericArgs;
				unordered_map<string, shared_ptr<TypeNameNode>> mappedGenericArgs;
				shared_ptr<MemberNode> mappedNode = {};
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

			//
			// Generic end
			//

			//
			// Semantic begin
			//

			void updateCompletionContext(size_t idxToken, CompletionContext completionContext);
			void updateCompletionContext(shared_ptr<TypeNameNode> targetTypeName, CompletionContext completionContext);
			void updateCompletionContext(const Ref &ref, CompletionContext completionContext);

			//
			// Semantic end
			//

			//
			// Class begin
			//

			void verifyInheritanceChain(ClassNode *node, std::set<AstNode *> &walkedNodes);
			void verifyInheritanceChain(InterfaceNode *node, std::set<AstNode *> &walkedNodes);
			void verifyInheritanceChain(TraitNode *node, std::set<AstNode *> &walkedNodes);

			void getMemberNodes(Scope *scope, std::unordered_map<std::string, MemberNode *> &membersOut);
			void getMemberNodes(ClassNode *node, std::unordered_map<std::string, MemberNode *> &membersOut);
			void getMemberNodes(InterfaceNode *node, std::unordered_map<std::string, MemberNode *> &membersOut);
			void getMemberNodes(TraitNode *node, std::unordered_map<std::string, MemberNode *> &membersOut);

			//
			// Class end
			//

			friend class AstVisitor;
			friend class MemberNode;
			friend string std::to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName);
			friend class Parser;
			friend struct Document;

		public:
			std::deque<TokenInfo> tokenInfos;
			deque<Message> messages;
			deque<string> modulePaths;
			CompilerOptions options;
			CompilerFlags flags = 0;
			Lexer lexer;

			inline Compiler(CompilerOptions options = {})
				: options(options), _rt(make_unique<Runtime>(RT_NOJIT)) {}
			~Compiler();

			void compile(std::istream &is, std::ostream &os, bool isImport = false);

			inline void reset() {
				curMajorContext = MajorContext();
				curFn.reset();

				_rootScope = make_shared<Scope>();
				_targetModule.reset();
				_rt = make_unique<Runtime>(RT_NOJIT);
				_savedMajorContexts.clear();

				importedDefinitions.clear();
				importedModules.clear();

				tokenInfos.clear();
				messages.clear();
				modulePaths.clear();
				options = CompilerOptions();
				flags = 0;
				lexer.reset();

				_genericCacheDir.clear();
			}

			Ref getFullName(MemberNode *member);
		};
	}
}

#endif
