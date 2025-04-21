#ifndef _SLKC_COMP_COMPILER_H_
#define _SLKC_COMP_COMPILER_H_

#include "../ast/parser.h"

namespace slkc {
	SLKC_API std::optional<slkc::CompilationError> typeNameCmp(peff::SharedPtr<TypeNameNode> lhs, peff::SharedPtr<TypeNameNode> rhs, int &out) noexcept;
	SLKC_API std::optional<slkc::CompilationError> typeNameListCmp(const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs, int &out) noexcept;

	enum class ExprEvalPurpose {
		EvalType,  // None
		Stmt,	   // As a statement
		LValue,	   // As a lvalue
		RValue,	   // As a rvalue
		Call,	   // As target of a calling expression
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

	struct Label {
		peff::String name;
		uint32_t offset = UINT32_MAX;

		SLAKE_FORCEINLINE Label(peff::String &&name) : name(std::move(name)) {}
	};

	struct FnCompileContext {
		peff::SharedPtr<FnNode> currentFn;
		peff::SharedPtr<ThisNode> thisNode;
		peff::DynArray<slake::Instruction> instructionsOut;
		peff::DynArray<peff::SharedPtr<Label>> labels;
		peff::HashMap<std::string_view, size_t> labelNameIndices;
		peff::List<peff::SharedPtr<BlockCompileContext>> blockCompileContexts;
		uint32_t nTotalRegs = 0;

		SLAKE_FORCEINLINE FnCompileContext(peff::Alloc *allocator) : instructionsOut(allocator), labels(allocator), blockCompileContexts(allocator), labelNameIndices(allocator) {}

		SLAKE_FORCEINLINE void reset() {
			currentFn = {};
			thisNode = {};
			instructionsOut.clear();
			labels.clear();
			blockCompileContexts.clear();
			labelNameIndices.clear();
			labels.clear();
		}
	};

	constexpr uint32_t
		// Do not compile and thus we don't need extraneous memory allocation for byte code generation.
		COMPCTXT_NOCOMPILE = 0x01;

	struct CompileContext : public peff::RcObject {
		slake::Runtime *runtime;
		slake::HostRefHolder hostRefHolder;
		peff::RcObjectPtr<peff::Alloc> selfAllocator, allocator;
		peff::SharedPtr<Document> document;
		peff::DynArray<CompilationError> errors;
		peff::DynArray<CompilationWarning> warnings;
		FnCompileContext fnCompileContext;
		uint32_t flags;

		SLAKE_FORCEINLINE CompileContext(
			slake::Runtime *runtime,
			peff::SharedPtr<Document> document,
			peff::Alloc *selfAllocator,
			peff::Alloc *allocator)
			: runtime(runtime),
			  document(document),
			  hostRefHolder(allocator),
			  selfAllocator(selfAllocator),
			  allocator(allocator),
			  errors(allocator),
			  warnings(allocator),
			  fnCompileContext(allocator),
			  flags(0) {}

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

		SLAKE_FORCEINLINE std::optional<CompilationError> allocLabel(uint32_t &labelIdOut) {
			peff::SharedPtr<Label> label = peff::makeShared<Label>(document->allocator.get(), peff::String(document->allocator.get()));

			if (!label) {
				return genOutOfMemoryCompError();
			}

			labelIdOut = fnCompileContext.labels.size();

			if (!fnCompileContext.labels.pushBack(peff::SharedPtr<Label>(label))) {
				return genOutOfMemoryCompError();
			}

			peff::ScopeGuard removeLabelGuard = [this]() noexcept {
				fnCompileContext.labels.popBack();
			};

			removeLabelGuard.release();

			return {};
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> allocLabel(peff::String &&name, uint32_t &labelIdOut) {
			peff::SharedPtr<Label> label = peff::makeShared<Label>(document->allocator.get(), std::move(name));

			if (!label) {
				return genOutOfMemoryCompError();
			}

			labelIdOut = fnCompileContext.labels.size();

			if (!fnCompileContext.labels.pushBack(peff::SharedPtr<Label>(label))) {
				return genOutOfMemoryCompError();
			}

			peff::ScopeGuard removeLabelGuard = [this]() noexcept {
				fnCompileContext.labels.popBack();
			};

			if (!fnCompileContext.labelNameIndices.insert(label->name, labelIdOut)) {
				return genOutOfMemoryCompError();
			}

			removeLabelGuard.release();

			return {};
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> allocLabel(const std::string_view &name, uint32_t &labelIdOut) {
			peff::String builtName(document->allocator.get());

			if (!builtName.build(name)) {
				return genOutOfMemoryCompError();
			}

			return allocLabel(std::move(builtName), labelIdOut);
		}

		SLAKE_FORCEINLINE peff::SharedPtr<Label> getLabel(uint32_t labelId) const {
			return fnCompileContext.labels.at(labelId);
		}

		SLAKE_FORCEINLINE peff::SharedPtr<Label> getLabelByName(const std::string_view &name) const {
			return fnCompileContext.labels.at(fnCompileContext.labelNameIndices.at(name));
		}

		SLAKE_FORCEINLINE uint32_t getCurInsOff() const {
			return (uint32_t)fnCompileContext.instructionsOut.size();
		}

		SLAKE_FORCEINLINE uint32_t allocReg() {
			return fnCompileContext.nTotalRegs++;
		}

		SLAKE_API std::optional<CompilationError> emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands);
	};

	struct CompileExprResult {
		peff::SharedPtr<TypeNameNode> evaluatedType;

		// For parameter name query, etc, if exists.
		peff::SharedPtr<FnSlotNode> callTargetFnSlot;
		peff::DynArray<peff::SharedPtr<FnNode>> callTargetMatchedOverloadings;
		uint32_t idxThisRegOut = UINT32_MAX;

		SLAKE_FORCEINLINE CompileExprResult(peff::Alloc *allocator) : callTargetMatchedOverloadings(allocator) {}
	};

	struct ResolvedIdRefPart {
		bool isStatic;
		size_t nEntries;
		peff::SharedPtr<MemberNode> member;
	};

	using ResolvedIdRefPartList = peff::DynArray<ResolvedIdRefPart>;

	[[nodiscard]] SLKC_API std::optional<CompilationError> _compileOrCastOperand(
		CompileContext *compileContext,
		uint32_t regOut,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<TypeNameNode> desiredType,
		peff::SharedPtr<ExprNode> operand,
		peff::SharedPtr<TypeNameNode> operandType);
	[[nodiscard]] SLKC_API std::optional<CompilationError> _compileSimpleBinaryExpr(
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
	std::optional<CompilationError> _compileSimpleAssignBinaryExpr(
		CompileContext *compileContext,
		peff::SharedPtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<TypeNameNode> lhsType,
		peff::SharedPtr<TypeNameNode> desiredLhsType,
		peff::SharedPtr<TypeNameNode> rhsType,
		peff::SharedPtr<TypeNameNode> desiredRhsType,
		ExprEvalPurpose rhsEvalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> _compileSimpleLAndBinaryExpr(
		CompileContext *compileContext,
		peff::SharedPtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<BoolTypeNameNode> boolType,
		peff::SharedPtr<TypeNameNode> lhsType,
		peff::SharedPtr<TypeNameNode> rhsType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		slake::Opcode opcode);
	[[nodiscard]] SLKC_API std::optional<CompilationError> _compileSimpleLOrBinaryExpr(
		CompileContext *compileContext,
		peff::SharedPtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<BoolTypeNameNode> boolType,
		peff::SharedPtr<TypeNameNode> lhsType,
		peff::SharedPtr<TypeNameNode> rhsType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		slake::Opcode opcode);
	[[nodiscard]] SLKC_API std::optional<CompilationError> _compileSimpleBinaryAssignOpExpr(
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

	[[nodiscard]] SLKC_API std::optional<CompilationError> resolveStaticMember(
		CompileContext *compileContext,
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<MemberNode> &memberNode,
		const IdRefEntry &name,
		peff::SharedPtr<MemberNode> &memberOut);
	[[nodiscard]] SLKC_API
		std::optional<CompilationError>
		resolveInstanceMember(
			CompileContext *compileContext,
			peff::SharedPtr<Document> document,
			peff::SharedPtr<MemberNode> memberNode,
			const IdRefEntry &name,
			peff::SharedPtr<MemberNode> &memberOut);
	[[nodiscard]] SLKC_API
		std::optional<CompilationError>
		resolveIdRef(
			CompileContext *compileContext,
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
	[[nodiscard]] SLKC_API
		std::optional<CompilationError>
		resolveIdRefWithScopeNode(
			CompileContext *compileContext,
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
	[[nodiscard]] SLKC_API
		std::optional<CompilationError>
		resolveCustomTypeName(
			peff::SharedPtr<Document> document,
			const peff::SharedPtr<CustomTypeNameNode> &typeName,
			peff::SharedPtr<MemberNode> &memberNodeOut,
			peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes = nullptr);

	SLKC_API std::optional<CompilationError> collectInvolvedInterfaces(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<InterfaceNode> &derived,
		peff::Set<peff::SharedPtr<InterfaceNode>> &walkedInterfaces,
		bool insertSelf);
	SLKC_API std::optional<CompilationError> isImplementedByInterface(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<InterfaceNode> &base,
		const peff::SharedPtr<InterfaceNode> &derived,
		bool &whetherOut);
	SLKC_API std::optional<CompilationError> isImplementedByClass(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<InterfaceNode> &base,
		const peff::SharedPtr<ClassNode> &derived,
		bool &whetherOut);
	SLKC_API std::optional<CompilationError> isBaseOf(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<ClassNode> &base,
		const peff::SharedPtr<ClassNode> &derived,
		bool &whetherOut);

	SLKC_API std::optional<CompilationError> removeRefOfType(
		peff::SharedPtr<TypeNameNode> src,
		peff::SharedPtr<TypeNameNode> &typeNameOut);
	SLKC_API std::optional<CompilationError> isLValueType(
		peff::SharedPtr<TypeNameNode> src,
		bool &whetherOut);
	SLKC_API std::optional<CompilationError> isSameType(
		const peff::SharedPtr<TypeNameNode> &lhs,
		const peff::SharedPtr<TypeNameNode> &rhs,
		bool &whetherOut);
	SLKC_API std::optional<CompilationError> isTypeConvertible(
		const peff::SharedPtr<TypeNameNode> &src,
		const peff::SharedPtr<TypeNameNode> &dest,
		bool &whetherOut);
	SLKC_API std::optional<CompilationError> compileUnaryExpr(
		CompileContext *compileContext,
		peff::SharedPtr<UnaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	SLKC_API std::optional<CompilationError> compileBinaryExpr(
		CompileContext *compileContext,
		peff::SharedPtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	SLKC_API std::optional<CompilationError> compileExpr(
		CompileContext *compileContext,
		const peff::SharedPtr<ExprNode> &expr,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<TypeNameNode> desiredType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	SLKC_API std::optional<CompilationError> compileStmt(
		CompileContext *compileContext,
		const peff::SharedPtr<StmtNode> &stmt);
	SLAKE_FORCEINLINE static std::optional<CompilationError> evalExprType(
		CompileContext *compileContext,
		const peff::SharedPtr<ExprNode> &expr,
		peff::SharedPtr<TypeNameNode> &typeOut,
		peff::SharedPtr<TypeNameNode> desiredType = {}) {
		CompileExprResult result(compileContext->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(compileExpr(compileContext, expr, ExprEvalPurpose::EvalType, desiredType, UINT32_MAX, result));
		typeOut = result.evaluatedType;
		return {};
	}

	[[nodiscard]] SLKC_API std::optional<CompilationError> getFullIdRef(peff::Alloc *allocator, peff::SharedPtr<MemberNode> m, IdRefPtr &idRefOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> compileTypeName(
		CompileContext *compileContext,
		peff::SharedPtr<TypeNameNode> typeName,
		slake::Type &typeOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileIdRef(
		CompileContext *compileContext,
		const IdRefEntry *entries,
		size_t nEntries,
		peff::SharedPtr<TypeNameNode> *paramTypes,
		size_t nParams,
		bool hasVarArgs,
		slake::HostObjectRef<slake::IdRefObject> &idRefOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileValueExpr(
		CompileContext *compileContext,
		peff::SharedPtr<ExprNode> expr,
		slake::Value &valueOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileModule(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod,
		slake::ModuleObject *modOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> reindexFnParams(
		CompileContext *compileContext,
		peff::SharedPtr<FnNode> fn);
	[[nodiscard]] SLKC_API std::optional<CompilationError> indexFnParams(
		CompileContext *compileContext,
		peff::SharedPtr<FnNode> fn);

	[[nodiscard]] SLKC_API std::optional<CompilationError> determineFnOverloading(
		CompileContext *compileContext,
		peff::SharedPtr<FnSlotNode> fnSlot,
		const peff::SharedPtr<TypeNameNode> *argTypes,
		size_t nArgTypes,
		bool isStatic,
		peff::DynArray<peff::SharedPtr<FnNode>> &matchedOverloadings,
		peff::Set<peff::SharedPtr<MemberNode>> *walkedParents = nullptr);
	[[nodiscard]] SLKC_API std::optional<CompilationError> fnToTypeName(
		CompileContext *compileContext,
		peff::SharedPtr<FnNode> fn,
		peff::SharedPtr<FnTypeNameNode> &evaluatedTypeOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> renormalizeModuleVarDefStmts(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod);
	[[nodiscard]] SLKC_API std::optional<CompilationError> normalizeModuleVarDefStmts(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod);

	[[nodiscard]] SLKC_API std::optional<CompilationError> visitBaseClass(peff::SharedPtr<TypeNameNode> cls, peff::SharedPtr<ClassNode> &classOut, peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes);
	[[nodiscard]] SLKC_API std::optional<CompilationError> visitBaseInterface(peff::SharedPtr<TypeNameNode> cls, peff::SharedPtr<InterfaceNode> &classOut, peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes);

	class Writer {
	public:
		SLKC_API virtual ~Writer();

		virtual std::optional<CompilationError> write(const char *src, size_t size) = 0;
	};

	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpModule(
		peff::Alloc *allocator,
		Writer *writer,
		slake::ModuleObject *mod);
}

#endif
