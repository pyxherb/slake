#ifndef _SLKC_COMPILER_COMPILER_H_
#define _SLKC_COMPILER_COMPILER_H_

#include "ast/ast.h"
#include <slake/runtime.h>

namespace slake {
	namespace slkc {
		enum MessageType {
			MSG_INFO = 0,
			MSG_NOTE,
			MSG_WARN,
			MSG_ERROR
		};

		struct Message {
			MessageType type;
			string msg;
			Location loc;

			Message(Location loc, const MessageType &type, const string &msg)
				: loc(loc), type(type), msg(msg) {
			}
		};

		struct FatalCompilationError : public std::runtime_error {
		public:
			Message message;

			inline FatalCompilationError(Message message)
				: message(message),
				  runtime_error("Error at " + std::to_string(message.loc) + ": " + message.msg) {
			}
			virtual ~FatalCompilationError() = default;
		};

		enum OptimizationLevel : uint8_t {
			OPTM_DISABLED = 0,	// Disable optimizations
			OPTM_SAFE,			// Safe
			OPTM_SPEED,			// Optimize for speed
			OPTM_MEM			// Optimize for memory usage
		};

		struct CompilerOptions {
			// Optimization level
			OptimizationLevel optimizationLevel = OPTM_DISABLED;
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
				STMT,	 // As a statement
				LVALUE,	 // As a lvalue
				RVALUE,	 // As a rvalue
				CALL,	 // As target of the call
			};

			struct Context {
				shared_ptr<TypeNameNode> expectedType;
				shared_ptr<Scope> curScope;
				deque<shared_ptr<TypeNameNode>> genericArgs;
				unordered_map<string, size_t> genericArgIndices;
				shared_ptr<CompiledFnNode> curFn;
				EvalPurpose evalPurpose;
				unordered_map<string, shared_ptr<LocalVarNode>> localVars;
				string breakLabel;
				string continueLabel;
				uint32_t curScopeLevel = 0;
				uint32_t breakScopeLevel = 0;
				uint32_t continueScopeLevel = 0;
				bool isLastCallTargetStatic = true;

				shared_ptr<AstNode> evalDest;
				set<RegId> preservedRegisters;
			};

			shared_ptr<Scope> _rootScope;
			shared_ptr<ModuleNode> _targetModule;
			unique_ptr<Runtime> _rt;
			Context context;
			deque<Context> _contextStack;
			InsMap curInsMap;

			static InsMap defaultInsMap;

			inline void pushContext() {
				_contextStack.push_back(context);
			}
			inline void popContext() {
				_contextStack.back().isLastCallTargetStatic = context.isLastCallTargetStatic;
				context = _contextStack.back();
				_contextStack.pop_back();
			}

			inline void setRegisterPreserved(RegId reg) {
				if (context.preservedRegisters.count(reg))
					throw std::logic_error("The register has already been set as preserved");
				context.preservedRegisters.insert(reg);
			}

			inline void unsetRegisterPreserved(RegId reg) {
				if (!context.preservedRegisters.count(reg))
					throw std::logic_error("The register is not preserved");
				context.preservedRegisters.erase(reg);
			}

			/// @brief Insert instructions to preserve the register if needed.
			/// @param reg Register to be preserved.
			/// @return True if the register needs to be preserved and then restored later.
			inline bool preserveRegister(RegId reg) {
				if (context.preservedRegisters.count(reg)) {
					context.curFn->insertIns(Opcode::PUSH, make_shared<RegRefNode>(reg));
					context.preservedRegisters.erase(reg);
					return true;
				}
				return false;
			}

			/// @brief Insert instructions to restore the register.
			/// @param reg Register to be restored.
			inline void restoreRegister(RegId reg) {
				context.curFn->insertIns(Opcode::POP, make_shared<RegRefNode>(reg));
				context.preservedRegisters.insert(reg);
			}

			shared_ptr<ExprNode> evalConstExpr(shared_ptr<ExprNode> expr);

			shared_ptr<TypeNameNode> evalExprType(shared_ptr<ExprNode> expr);

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

			/// @brief Resolve a reference with current context.
			/// @param ref Reference to be resolved.
			/// @param refParts Divided minimum parts of the reference that can be loaded in a single time.
			/// @param resolvedPartsOut Nodes referred by reference entries respectively.
			bool resolveRef(Ref ref, deque<pair<Ref, shared_ptr<AstNode>>> &partsOut);

			shared_ptr<Scope> scopeOf(shared_ptr<AstNode> node);
			shared_ptr<AstNode> resolveCustomType(shared_ptr<CustomTypeNameNode> typeName);

			bool isSameType(shared_ptr<TypeNameNode> x, shared_ptr<TypeNameNode> y);

			void _getFullName(MemberNode *member, Ref &ref);
			Ref getFullName(shared_ptr<MemberNode> member);

			void compileExpr(shared_ptr<ExprNode> expr);
			inline void compileExpr(shared_ptr<ExprNode> expr, EvalPurpose evalPurpose, shared_ptr<AstNode> evalDest) {
				pushContext();
				context.evalPurpose = evalPurpose;
				context.evalDest = evalDest;
				compileExpr(expr);
				popContext();
			}
			void compileStmt(shared_ptr<StmtNode> stmt);

			void compileScope(std::istream &is, std::ostream &os, shared_ptr<Scope> scope);
			void compileRef(std::ostream &fs, const Ref &ref);
			void compileTypeName(std::ostream &fs, shared_ptr<TypeNameNode> typeName);
			void compileValue(std::ostream &fs, shared_ptr<AstNode> expr);

			bool isDynamicMember(shared_ptr<AstNode> member);

			friend class AstVisitor;

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
