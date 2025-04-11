#ifndef _SLKC_COMP_COMPILER_H_
#define _SLKC_COMP_COMPILER_H_

#include "../ast/parser.h"

namespace slkc {
	SLKC_API std::optional<slkc::CompilationError> typeNameCmp(peff::SharedPtr<TypeNameNode> lhs, peff::SharedPtr<TypeNameNode> rhs, int &out) noexcept;
	SLKC_API std::optional<slkc::CompilationError> typeNameListCmp(const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs, int &out) noexcept;

	enum class ExprEvalPurpose {
		None,	 // None
		Stmt,	 // As a statement
		LValue,	 // As a lvalue
		RValue,	 // As a rvalue
		Call,	 // As target of a calling expression
	};

	struct StmtCompileContext {
	};

	struct LocalVarRegistry {
		peff::SharedPtr<VarNode> varNode;
		uint32_t idxReg;
	};

	struct BlockCompileContext {
		peff::HashMap<std::string_view, peff::SharedPtr<VarNode>> localVars;

		SLAKE_FORCEINLINE BlockCompileContext(peff::Alloc *allocator) : localVars(allocator) {
		}
	};

	struct FnCompileContext {
		peff::SharedPtr<FnNode> currentFn;
		peff::DynArray<slake::Instruction> instructionsOut;
		peff::HashMap<peff::String, uint32_t> labels;
		peff::List<peff::SharedPtr<BlockCompileContext>> blockCompileContexts;

		SLAKE_FORCEINLINE FnCompileContext(peff::Alloc *allocator) : instructionsOut(allocator), labels(allocator), blockCompileContexts(allocator) {}

		SLAKE_FORCEINLINE void reset() {
			instructionsOut.clear();
			labels.clear();
			blockCompileContexts.clear();
		}
	};

	struct CompileContext : public peff::RcObject {
		peff::RcObjectPtr<peff::Alloc> selfAllocator, allocator;
		peff::SharedPtr<Document> document;
		peff::DynArray<CompilationError> errors;
		peff::DynArray<CompilationWarning> warnings;
		FnCompileContext fnCompileContext;
		uint32_t nTotalRegs = 0;

		SLAKE_FORCEINLINE CompileContext(
			peff::Alloc *selfAllocator,
			peff::Alloc *allocator)
			: selfAllocator(selfAllocator),
			  allocator(allocator),
			  errors(allocator),
			  warnings(allocator),
			  fnCompileContext(allocator) {}

		SLKC_API virtual ~CompileContext();

		SLKC_API virtual void onRefZero() noexcept override;

		SLAKE_FORCEINLINE std::optional<CompilationError> pushIns(slake::Instruction &&ins) {
			if (!fnCompileContext.instructionsOut.pushBack(std::move(ins)))
				return genOutOfMemoryCompError();

			return {};
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> pushError(CompilationError &&error) {
			if (!errors.pushBack(std::move(error)))
				return genOutOfMemoryCompError();

			return {};
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> pushWarning(CompilationWarning &&warning) {
			if (!warnings.pushBack(std::move(warning)))
				return genOutOfMemoryCompError();

			return {};
		}

		SLAKE_FORCEINLINE uint32_t allocReg() {
			return nTotalRegs++;
		}

		SLAKE_API std::optional<CompilationError> emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands);
	};

	struct GenericInstantiationContext {
		peff::RcObjectPtr<peff::Alloc> allocator;
		const peff::DynArray<peff::SharedPtr<TypeNameNode>> *genericArgs;
		peff::HashMap<std::string_view, peff::SharedPtr<TypeNameNode>> mappedGenericArgs;
		peff::SharedPtr<MemberNode> mappedNode;

		SLAKE_FORCEINLINE GenericInstantiationContext(
			peff::Alloc *allocator,
			const peff::DynArray<peff::SharedPtr<TypeNameNode>> *genericArgs)
			: allocator(allocator),
			  genericArgs(genericArgs),
			  mappedGenericArgs(allocator) {
		}
	};

	struct ImplementationDetectionContext {
		peff::Set<peff::SharedPtr<InterfaceNode>> walkedInterfaces;

		SLAKE_FORCEINLINE ImplementationDetectionContext(peff::Alloc *allocator) : walkedInterfaces(allocator) {}
	};

	struct CompileExprResult {
		peff::SharedPtr<TypeNameNode> evaluatedType;
	};

	struct ResolvedIdRefPart {
		size_t nEntries;
		peff::SharedPtr<MemberNode> member;
	};

	using ResolvedIdRefPartList = peff::DynArray<ResolvedIdRefPart>;

	class Compiler {
	private:
		static SLKC_API std::optional<CompilationError> _compileOrCastOperand(
			CompileContext *compileContext,
			uint32_t regOut,
			ExprEvalPurpose evalPurpose,
			peff::SharedPtr<TypeNameNode> desiredType,
			peff::SharedPtr<ExprNode> operand,
			peff::SharedPtr<TypeNameNode> operandType);
		static SLKC_API std::optional<CompilationError> _compileSimpleBinaryExpr(
			CompileContext *compileContext,
			peff::SharedPtr<BinaryExprNode> expr,
			ExprEvalPurpose evalPurpose,
			peff::SharedPtr<TypeNameNode> lhsType,
			peff::SharedPtr<TypeNameNode> desiredLhsType,
			ExprEvalPurpose lhsEvalPurpose,
			peff::SharedPtr<TypeNameNode> rhsType,
			peff::SharedPtr<TypeNameNode> desiredRhsType,
			ExprEvalPurpose rhsEvalPurpose,
			uint32_t resultRegOut,
			CompileExprResult &resultOut,
			slake::Opcode opcode);
		static SLKC_API std::optional<CompilationError> _compileSimpleBinaryAssignOpExpr(
			CompileContext *compileContext,
			peff::SharedPtr<BinaryExprNode> expr,
			ExprEvalPurpose evalPurpose,
			peff::SharedPtr<TypeNameNode> lhsType,
			peff::SharedPtr<TypeNameNode> rhsType,
			peff::SharedPtr<TypeNameNode> desiredRhsType,
			ExprEvalPurpose rhsEvalPurpose,
			uint32_t resultRegOut,
			CompileExprResult &resultOut,
			slake::Opcode opcode);

	public:
		static SLKC_API std::optional<CompilationError> resolveStaticMember(
			peff::SharedPtr<Document> document,
			const peff::SharedPtr<MemberNode> &memberNode,
			const IdRefEntry &name,
			peff::SharedPtr<MemberNode> &memberOut);
		[[nodiscard]] static SLKC_API
			std::optional<CompilationError>
			resolveInstanceMember(
				peff::SharedPtr<Document> document,
				peff::SharedPtr<MemberNode> memberNode,
				const IdRefEntry &name,
				peff::SharedPtr<MemberNode> &memberOut);
		[[nodiscard]] static SLKC_API
			std::optional<CompilationError>
			resolveIdRef(
				peff::SharedPtr<Document> document,
				const peff::SharedPtr<MemberNode> &resolveRoot,
				IdRefEntry *idRef,
				size_t nEntries,
				peff::SharedPtr<MemberNode> &memberOut,
				ResolvedIdRefPartList *resolvedPartListOut,
				bool isStatic = true);
		/// @brief Resolve an identifier reference with a scope object and its parents.
		/// @param document Document for resolution.
		/// @param walkedNodes Reference to the container to store the walked nodes, should be empty on the top level.
		/// @param resolveScope Scope object for resolution.
		/// @param idRef Identifier entry array for resolution.
		/// @param nEntries Number of identifier entries.
		/// @param memberOut Where will be used for output member storage, `nullptr` if not found.
		/// @param isStatic Controls if the initial resolution is static or instance.
		/// @param isSealed Controls if not go into the parent of the current scope object.
		/// @return The fatal error encountered during the resolution.
		[[nodiscard]] static SLKC_API
			std::optional<CompilationError>
			resolveIdRefWithScopeNode(
				peff::SharedPtr<Document> document,
				peff::Set<peff::SharedPtr<MemberNode>> &walkedNodes,
				const peff::SharedPtr<MemberNode> &resolveScope,
				IdRefEntry *idRef,
				size_t nEntries,
				peff::SharedPtr<MemberNode> &memberOut,
				ResolvedIdRefPartList *resolvedPartListOut,
				bool isStatic = true,
				bool isSealed = false);
		/// @brief Resolve a custom type name.
		/// @param compileContext The compile context.
		/// @param resolveContext Previous resolve context.
		/// @param typeName Type name to be resolved.
		/// @param memberNodeOut Where the resolved member node will be stored.
		/// @return Critical error encountered that forced the resolution to interrupt.
		[[nodiscard]] static SLKC_API
			std::optional<CompilationError>
			resolveCustomTypeName(
				peff::SharedPtr<Document> document,
				const peff::SharedPtr<CustomTypeNameNode> &typeName,
				peff::SharedPtr<MemberNode> &memberNodeOut);

		static SLKC_API std::optional<CompilationError> collectInvolvedInterfaces(
			peff::SharedPtr<Document> document,
			const peff::SharedPtr<InterfaceNode> &derived,
			peff::Set<peff::SharedPtr<InterfaceNode>> &walkedInterfaces,
			bool insertSelf);
		static SLKC_API std::optional<CompilationError> isImplementedByInterface(
			peff::SharedPtr<Document> document,
			const peff::SharedPtr<InterfaceNode> &base,
			const peff::SharedPtr<InterfaceNode> &derived,
			bool &whetherOut);
		static SLKC_API std::optional<CompilationError> isImplementedByClass(
			peff::SharedPtr<Document> document,
			const peff::SharedPtr<InterfaceNode> &base,
			const peff::SharedPtr<ClassNode> &derived,
			bool &whetherOut);
		static SLKC_API std::optional<CompilationError> isBaseOf(
			peff::SharedPtr<Document> document,
			const peff::SharedPtr<ClassNode> &base,
			const peff::SharedPtr<ClassNode> &derived,
			bool &whetherOut);

		static SLKC_API std::optional<CompilationError> removeRefOfType(
			peff::SharedPtr<TypeNameNode> src,
			peff::SharedPtr<TypeNameNode> &typeNameOut);
		static SLKC_API std::optional<CompilationError> isSameType(
			const peff::SharedPtr<TypeNameNode> &lhs,
			const peff::SharedPtr<TypeNameNode> &rhs,
			bool &whetherOut);
		static SLKC_API std::optional<CompilationError> isTypeConvertible(
			const peff::SharedPtr<TypeNameNode> &src,
			const peff::SharedPtr<TypeNameNode> &dest,
			bool &whetherOut);
		static SLKC_API std::optional<CompilationError> compileUnaryExpr(
			CompileContext *compileContext,
			peff::SharedPtr<UnaryExprNode> expr,
			ExprEvalPurpose evalPurpose,
			uint32_t resultRegOut,
			CompileExprResult &resultOut);
		static SLKC_API std::optional<CompilationError> compileBinaryExpr(
			CompileContext *compileContext,
			peff::SharedPtr<BinaryExprNode> expr,
			ExprEvalPurpose evalPurpose,
			uint32_t resultRegOut,
			CompileExprResult &resultOut);
		static SLKC_API std::optional<CompilationError> compileExpr(
			CompileContext *compileContext,
			const peff::SharedPtr<ExprNode> &expr,
			ExprEvalPurpose evalPurpose,
			uint32_t resultRegOut,
			CompileExprResult &resultOut);
		SLAKE_FORCEINLINE static std::optional<CompilationError> evalExprType(
			CompileContext *compileContext,
			const peff::SharedPtr<ExprNode> &expr,
			peff::SharedPtr<TypeNameNode> &typeOut) {
			CompileExprResult result;
			SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, expr, ExprEvalPurpose::None, UINT32_MAX, result));
			typeOut = result.evaluatedType;
			return {};
		}
	};
}

#endif
