#ifndef _SLKC_COMPILER_COMPILER_H_
#define _SLKC_COMPILER_COMPILER_H_

#include "ast/ast.h"
#include <slake/runtime.h>

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
			COMP_FAILED = 0x00000001;

		const size_t OUTPUT_SIZE_MAX = 0x20000000;

		struct InsMapEntry {
			string name;
			deque<TypeId> operandTypes;

			inline InsMapEntry(string name, deque<TypeId> operandTypes)
				: name(name), operandTypes(operandTypes) {}
		};

		using InsMap = map<Opcode, InsMapEntry>;

		class Compiler {
		private:
			enum class EvalPurpose {
				Stmt,	 // As a statement
				LValue,	 // As a lvalue
				RValue,	 // As a rvalue
				Call,	 // As the target of a call
			};

			/// @brief Statement level context
			struct MinorContext {
				shared_ptr<TypeNameNode> expectedType;
				shared_ptr<Scope> curScope;
				deque<shared_ptr<TypeNameNode>> genericArgs;
				unordered_map<string, size_t> genericArgIndices;
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

				inline void pushMinorContext() {
					savedMinorContexts.push_back(curMinorContext);
				}

				inline void popMinorContext() {
					savedMinorContexts.back().isLastCallTargetStatic = curMinorContext.isLastCallTargetStatic;
					curMinorContext = savedMinorContexts.back();
					savedMinorContexts.pop_back();
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
			InsMap curInsMap;

			static InsMap defaultInsMap;

			void pushMajorContext();
			void popMajorContext();

			void pushMinorContext();
			void popMinorContext();

			shared_ptr<ExprNode> evalConstExpr(shared_ptr<ExprNode> expr);

			shared_ptr<TypeNameNode> evalExprType(shared_ptr<ExprNode> expr);

			shared_ptr<ExprNode> castLiteralExpr(shared_ptr<ExprNode> expr, Type targetType);

			bool isLiteralType(shared_ptr<TypeNameNode> typeName);
			bool isNumericType(shared_ptr<TypeNameNode> typeName);
			bool isDecimalType(shared_ptr<TypeNameNode> typeName);
			bool isCompoundType(shared_ptr<TypeNameNode> typeName);

			bool _areTypesConvertible(shared_ptr<InterfaceNode> st, shared_ptr<ClassNode> dt);

			bool _areTypesConvertible(shared_ptr<ClassNode> st, shared_ptr<InterfaceNode> dt);
			bool _areTypesConvertible(shared_ptr<InterfaceNode> st, shared_ptr<InterfaceNode> dt);
			bool _areTypesConvertible(shared_ptr<MemberNode> st, shared_ptr<TraitNode> dt);

			bool areTypesConvertible(shared_ptr<TypeNameNode> src, shared_ptr<TypeNameNode> dest);

			bool _resolveRef(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);
			bool _resolveRefWithOwner(Scope *scope, const Ref &ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);

			/// @brief Resolve a reference with current context.
			/// @param ref Reference to be resolved.
			/// @param refParts Divided minimum parts of the reference that can be loaded in a single time.
			/// @param resolvedPartsOut Nodes referred by reference entries respectively.
			bool resolveRef(Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);

			FnOverloadingRegistry *argDependentLookup(Location loc, FnNode *fn, const deque<shared_ptr<TypeNameNode>> &argTypes);

			shared_ptr<Scope> scopeOf(AstNode *node);
			shared_ptr<AstNode> resolveCustomType(CustomTypeNameNode *typeName);

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

			bool isDynamicMember(shared_ptr<AstNode> member);

			uint32_t allocLocalVar(string name, shared_ptr<TypeNameNode> type);
			uint32_t allocReg(uint32_t nRegs = 1);

			set<Value *> importedDefinitions;
			set<ModuleRef> importedModules;

			static unique_ptr<ifstream> moduleLocator(Runtime *rt, ValueRef<RefValue> ref);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, BasicFnValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, ModuleValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, ClassValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, InterfaceValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, TraitValue *value);
			void importDefinitions(shared_ptr<Scope> scope, shared_ptr<MemberNode> parent, Value *value);
			void importModule(string name, const ModuleRef &ref, shared_ptr<Scope> scope);
			shared_ptr<TypeNameNode> toTypeName(slake::Type runtimeType);
			Ref toAstRef(deque<slake::RefEntry> runtimeRefEntries);
			slkc::GenericQualifier toAstGenericQualifier(slake::GenericQualifier qualifier);

			friend class AstVisitor;
			friend string std::to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName);

		public:
			deque<Message> messages;
			deque<string> modulePaths;
			CompilerOptions options;
			CompilerFlags flags = 0;

			inline Compiler(CompilerOptions options = {})
				: options(options), _rt(make_unique<Runtime>(RT_NOJIT)), curInsMap(defaultInsMap) {}
			~Compiler() = default;

			void compile(std::istream &is, std::ostream &os);
		};
	}
}

#endif
