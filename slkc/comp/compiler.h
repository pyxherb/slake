#ifndef _SLKC_COMP_COMPILER_H_
#define _SLKC_COMP_COMPILER_H_

#include "../ast/parser.h"

namespace slkc {
	SLKC_API std::optional<slkc::CompilationError> typeNameCmp(peff::SharedPtr<TypeNameNode> lhs, peff::SharedPtr<TypeNameNode> rhs, int &out) noexcept;
	SLKC_API std::optional<slkc::CompilationError> typeNameListCmp(const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs, int &out) noexcept;

	enum class ExprEvalPurpose {
		EvalType,	// None
		Stmt,		// As a statement
		LValue,		// As a lvalue
		RValue,		// As a rvalue
		Call,		// As target of a calling expression
		Unpacking,	// For unpacking, note that it is used for notifying the expression compiler to prepare the expression to be unpacked, not actually to unpack it.
	};

	struct StmtCompileContext {
	};

	struct LocalVarRegistry {
		peff::SharedPtr<VarNode> varNode;
		uint32_t idxReg;
	};

	struct Label {
		peff::String name;
		uint32_t offset = UINT32_MAX;

		SLAKE_FORCEINLINE Label(peff::String &&name) : name(std::move(name)) {}
	};

	class CompilationContext {
	public:
		CompilationContext *parent = nullptr;

		SLKC_API CompilationContext(CompilationContext *parent);
		SLKC_API virtual ~CompilationContext();

		[[nodiscard]] virtual std::optional<CompilationError> allocLabel(uint32_t &labelIdOut) = 0;
		virtual void setLabelOffset(uint32_t labelId, uint32_t offset) const = 0;
		[[nodiscard]] virtual std::optional<CompilationError> setLabelName(uint32_t labelId, const std::string_view &name) = 0;
		virtual uint32_t getLabelOffset(uint32_t labelId) = 0;

		[[nodiscard]] virtual std::optional<CompilationError> allocReg(uint32_t &regOut) = 0;

		[[nodiscard]] virtual std::optional<CompilationError> emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) = 0;
		[[nodiscard]] virtual std::optional<CompilationError> emitIns(slake::Opcode opcode, uint32_t outputRegIndex, slake::Value *operands, size_t nOperands) = 0;

		[[nodiscard]] virtual std::optional<CompilationError> allocLocalVar(const TokenRange &tokenRange, const std::string_view &name, uint32_t reg, peff::SharedPtr<TypeNameNode> type, peff::SharedPtr<VarNode> &localVarOut) = 0;
		[[nodiscard]] virtual peff::SharedPtr<VarNode> getLocalVarInCurLevel(const std::string_view &name) = 0;
		virtual peff::SharedPtr<VarNode> getLocalVar(const std::string_view &name) = 0;

		virtual void setBreakLabel(uint32_t labelId, uint32_t blockLevel) = 0;
		virtual void setContinueLabel(uint32_t labelId, uint32_t blockLevel) = 0;

		virtual uint32_t getBreakLabel() = 0;
		virtual uint32_t getContinueLabel() = 0;

		virtual uint32_t getBreakLabelBlockLevel() = 0;
		virtual uint32_t getContinueLabelBlockLevel() = 0;

		virtual uint32_t getCurInsOff() const = 0;

		[[nodiscard]] virtual std::optional<CompilationError> enterBlock() = 0;
		virtual void leaveBlock() = 0;

		virtual uint32_t getBlockLevel() = 0;

		SLKC_API peff::SharedPtr<VarNode> lookupLocalVar(const std::string_view &name);
	};

	struct CompileContext;

	class NormalCompilationContext : public CompilationContext {
	public:
		struct BlockLayer {
			peff::HashMap<std::string_view, peff::SharedPtr<VarNode>> localVars;

			SLAKE_FORCEINLINE BlockLayer(peff::Alloc *allocator) : localVars(allocator) {
			}
			SLAKE_FORCEINLINE BlockLayer(BlockLayer &&rhs) : localVars(std::move(rhs.localVars)) {
			}
			SLKC_API ~BlockLayer();

			SLAKE_FORCEINLINE BlockLayer &operator=(BlockLayer &&rhs) {
				localVars = std::move(rhs.localVars);
				return *this;
			}
		};

		peff::RcObjectPtr<peff::Alloc> allocator;

		CompilationContext *const parent = nullptr;

		peff::SharedPtr<Document> document;

		const uint32_t baseBlockLevel;
		peff::List<BlockLayer> savedBlockLayers;
		BlockLayer curBlockLayer;

		uint32_t nTotalRegs = 0;

		peff::DynArray<peff::SharedPtr<Label>> labels;
		peff::HashMap<std::string_view, size_t> labelNameIndices;

		uint32_t breakStmtJumpDestLabel = UINT32_MAX, continueStmtJumpDestLabel = UINT32_MAX;
		uint32_t breakStmtBlockLevel = 0, continueStmtBlockLevel = 0;

		const uint32_t baseInsOff;
		peff::DynArray<slake::Instruction> generatedInstructions;

		SLKC_API NormalCompilationContext(CompileContext *compileContext, CompilationContext *parent);
		SLKC_API virtual ~NormalCompilationContext();

		SLKC_API virtual std::optional<CompilationError> allocLabel(uint32_t &labelIdOut) override;
		SLKC_API virtual void setLabelOffset(uint32_t labelId, uint32_t offset) const override;
		SLKC_API virtual std::optional<CompilationError> setLabelName(uint32_t labelId, const std::string_view &name) override;
		SLKC_API virtual uint32_t getLabelOffset(uint32_t labelId) override;

		SLKC_API virtual std::optional<CompilationError> allocReg(uint32_t &regOut) override;

		SLKC_API virtual std::optional<CompilationError> emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) override;
		SLKC_API virtual std::optional<CompilationError> emitIns(slake::Opcode opcode, uint32_t outputRegIndex, slake::Value *operands, size_t nOperands) override;

		SLKC_API virtual std::optional<CompilationError> allocLocalVar(const TokenRange &tokenRange, const std::string_view &name, uint32_t reg, peff::SharedPtr<TypeNameNode> type, peff::SharedPtr<VarNode> &localVarOut) override;
		SLKC_API virtual peff::SharedPtr<VarNode> getLocalVarInCurLevel(const std::string_view &name) override;
		SLKC_API virtual peff::SharedPtr<VarNode> getLocalVar(const std::string_view &name) override;

		SLKC_API virtual void setBreakLabel(uint32_t labelId, uint32_t blockLevel) override;
		SLKC_API virtual void setContinueLabel(uint32_t labelId, uint32_t blockLevel) override;

		SLKC_API virtual uint32_t getBreakLabel() override;
		SLKC_API virtual uint32_t getContinueLabel() override;

		SLKC_API virtual uint32_t getBreakLabelBlockLevel() override;
		SLKC_API virtual uint32_t getContinueLabelBlockLevel() override;

		SLKC_API virtual uint32_t getCurInsOff() const override;

		SLKC_API virtual std::optional<CompilationError> enterBlock() override;
		SLKC_API virtual void leaveBlock() override;

		SLKC_API virtual uint32_t getBlockLevel() override;
	};

	struct CompileContext : public peff::RcObject {
		slake::Runtime *runtime;
		slake::HostRefHolder hostRefHolder;
		peff::RcObjectPtr<peff::Alloc> selfAllocator, allocator;
		peff::SharedPtr<Document> document;
		peff::DynArray<CompilationError> errors;
		peff::DynArray<CompilationWarning> warnings;
		peff::SharedPtr<FnOverloadingNode> curOverloading;
		peff::SharedPtr<ThisNode> thisNode;
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
			  flags(0) {}

		SLKC_API virtual ~CompileContext();

		SLKC_API virtual void onRefZero() noexcept override;

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

		SLAKE_FORCEINLINE void reset() {
			curOverloading = {};
			thisNode = {};
		}
	};

	struct PrevBreakPointHolder {
		CompilationContext *compileContext;
		uint32_t lastBreakStmtJumpDestLabel,
			lastBreakStmtBlockLevel;

		SLAKE_FORCEINLINE PrevBreakPointHolder(CompilationContext *compileContext)
			: compileContext(compileContext),
			  lastBreakStmtJumpDestLabel(compileContext->getBreakLabel()),
			  lastBreakStmtBlockLevel(compileContext->getBreakLabelBlockLevel()) {}
		SLAKE_FORCEINLINE ~PrevBreakPointHolder() {
			compileContext->setBreakLabel(lastBreakStmtJumpDestLabel, lastBreakStmtBlockLevel);
		}
	};

	struct PrevContinuePointHolder {
		CompilationContext *compileContext;
		uint32_t lastContinueStmtJumpDestLabel,
			lastContinueStmtBlockLevel;

		SLAKE_FORCEINLINE PrevContinuePointHolder(CompilationContext *compileContext)
			: compileContext(compileContext),
			  lastContinueStmtJumpDestLabel(compileContext->getContinueLabel()),
			  lastContinueStmtBlockLevel(compileContext->getContinueLabelBlockLevel()) {}
		SLAKE_FORCEINLINE ~PrevContinuePointHolder() {
			compileContext->setContinueLabel(lastContinueStmtJumpDestLabel, lastContinueStmtBlockLevel);
		}
	};

	struct CompileExprResult {
		peff::SharedPtr<TypeNameNode> evaluatedType;

		// For parameter name query, etc, if exists.
		peff::SharedPtr<FnNode> callTargetFnSlot;
		peff::DynArray<peff::SharedPtr<FnOverloadingNode>> callTargetMatchedOverloadings;
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
		CompilationContext *compilationContext,
		uint32_t regOut,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<TypeNameNode> desiredType,
		peff::SharedPtr<ExprNode> operand,
		peff::SharedPtr<TypeNameNode> operandType);
	[[nodiscard]] SLKC_API std::optional<CompilationError> _compileSimpleBinaryExpr(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
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
		CompilationContext *compilationContext,
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
		CompilationContext *compilationContext,
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
		CompilationContext *compilationContext,
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
		CompilationContext *compilationContext,
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

	[[nodiscard]] SLKC_API std::optional<CompilationError> collectInvolvedInterfaces(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<InterfaceNode> &derived,
		peff::Set<peff::SharedPtr<InterfaceNode>> &walkedInterfaces,
		bool insertSelf);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isImplementedByInterface(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<InterfaceNode> &base,
		const peff::SharedPtr<InterfaceNode> &derived,
		bool &whetherOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isImplementedByClass(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<InterfaceNode> &base,
		const peff::SharedPtr<ClassNode> &derived,
		bool &whetherOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isBaseOf(
		peff::SharedPtr<Document> document,
		const peff::SharedPtr<ClassNode> &base,
		const peff::SharedPtr<ClassNode> &derived,
		bool &whetherOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> removeRefOfType(
		peff::SharedPtr<TypeNameNode> src,
		peff::SharedPtr<TypeNameNode> &typeNameOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isLValueType(
		peff::SharedPtr<TypeNameNode> src,
		bool &whetherOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isSameType(
		const peff::SharedPtr<TypeNameNode> &lhs,
		const peff::SharedPtr<TypeNameNode> &rhs,
		bool &whetherOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> getTypePromotionLevel(
		const peff::SharedPtr<TypeNameNode> &typeName,
		int &levelOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> determinePromotionalType(
		peff::SharedPtr<TypeNameNode> lhs,
		peff::SharedPtr<TypeNameNode> rhs,
		peff::SharedPtr<TypeNameNode> &typeNameOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isSameTypeInSignature(
		const peff::SharedPtr<TypeNameNode> &lhs,
		const peff::SharedPtr<TypeNameNode> &rhs,
		bool &whetherOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isTypeConvertible(
		const peff::SharedPtr<TypeNameNode> &src,
		const peff::SharedPtr<TypeNameNode> &dest,
		bool isSealed,
		bool &whetherOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> _doExpandParamListTypeNameTree(
		peff::SharedPtr<TypeNameNode> &type);
	[[nodiscard]] SLKC_API std::optional<CompilationError> simplifyParamListTypeNameTree(
		peff::SharedPtr<TypeNameNode> type,
		peff::Alloc *allocator,
		peff::SharedPtr<TypeNameNode> &typeNameOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> getUnpackedTypeOf(
		peff::SharedPtr<TypeNameNode> type,
		peff::SharedPtr<TypeNameNode> &typeNameOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileUnaryExpr(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
		peff::SharedPtr<UnaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileBinaryExpr(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
		peff::SharedPtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileExpr(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
		const peff::SharedPtr<ExprNode> &expr,
		ExprEvalPurpose evalPurpose,
		peff::SharedPtr<TypeNameNode> desiredType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileStmt(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
		const peff::SharedPtr<StmtNode> &stmt);
	[[nodiscard]] SLKC_API std::optional<CompilationError> evalExprType(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
		const peff::SharedPtr<ExprNode> &expr,
		peff::SharedPtr<TypeNameNode> &typeOut,
		peff::SharedPtr<TypeNameNode> desiredType = {});

	[[nodiscard]] SLKC_API std::optional<CompilationError> evalConstExpr(
		CompileContext *compileContext,
		CompilationContext *compilationContext,
		peff::SharedPtr<ExprNode> expr,
		peff::SharedPtr<ExprNode> &exprOut);

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
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileGenericParams(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod,
		peff::SharedPtr<GenericParamNode> *genericParams,
		size_t nGenericParams,
		slake::GenericParamList &genericParamListOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> compileModule(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod,
		slake::ModuleObject *modOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> reindexFnParams(
		CompileContext *compileContext,
		peff::SharedPtr<FnOverloadingNode> fn);
	[[nodiscard]] SLKC_API std::optional<CompilationError> indexFnParams(
		CompileContext *compileContext,
		peff::SharedPtr<FnOverloadingNode> fn);

	[[nodiscard]] SLKC_API std::optional<CompilationError> reindexClassGenericParams(
		CompileContext *compileContext,
		peff::SharedPtr<ClassNode> cls);
	[[nodiscard]] SLKC_API std::optional<CompilationError> indexClassGenericParams(
		CompileContext *compileContext,
		peff::SharedPtr<ClassNode> cls);

	[[nodiscard]] SLKC_API std::optional<CompilationError> reindexInterfaceGenericParams(
		CompileContext *compileContext,
		peff::SharedPtr<InterfaceNode> interfaceNode);
	[[nodiscard]] SLKC_API std::optional<CompilationError> indexInterfaceGenericParams(
		CompileContext *compileContext,
		peff::SharedPtr<InterfaceNode> interfaceNode);

	[[nodiscard]] SLKC_API std::optional<CompilationError> indexModuleMembers(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> moduleNode);

	[[nodiscard]] SLKC_API std::optional<CompilationError> determineFnOverloading(
		CompileContext *compileContext,
		peff::SharedPtr<FnNode> fnSlot,
		const peff::SharedPtr<TypeNameNode> *argTypes,
		size_t nArgTypes,
		bool isStatic,
		peff::DynArray<peff::SharedPtr<FnOverloadingNode>> &matchedOverloadings,
		peff::Set<peff::SharedPtr<MemberNode>> *walkedParents = nullptr);
	[[nodiscard]] SLKC_API std::optional<CompilationError> fnToTypeName(
		CompileContext *compileContext,
		peff::SharedPtr<FnOverloadingNode> fn,
		peff::SharedPtr<FnTypeNameNode> &evaluatedTypeOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> cleanupUnusedModuleTree(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> leaf);
	[[nodiscard]] SLKC_API std::optional<CompilationError> completeParentModules(
		CompileContext *compileContext,
		IdRef *modulePath,
		peff::SharedPtr<ModuleNode> leaf);

	[[nodiscard]] SLKC_API std::optional<CompilationError> renormalizeModuleVarDefStmts(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod);
	[[nodiscard]] SLKC_API std::optional<CompilationError> normalizeModuleVarDefStmts(
		CompileContext *compileContext,
		peff::SharedPtr<ModuleNode> mod);

	[[nodiscard]] SLKC_API std::optional<CompilationError> isFnSignatureSame(peff::SharedPtr<VarNode> *lParams, peff::SharedPtr<VarNode> *rParams, size_t nParams, bool &whetherOut);
	[[nodiscard]] SLKC_API std::optional<CompilationError> isFnSignatureDuplicated(peff::SharedPtr<FnOverloadingNode> lhs, peff::SharedPtr<FnOverloadingNode> rhs, bool &whetherOut);

	[[nodiscard]] SLKC_API std::optional<CompilationError> visitBaseClass(peff::SharedPtr<TypeNameNode> cls, peff::SharedPtr<ClassNode> &classOut, peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes);
	[[nodiscard]] SLKC_API std::optional<CompilationError> visitBaseInterface(peff::SharedPtr<TypeNameNode> cls, peff::SharedPtr<InterfaceNode> &classOut, peff::Set<peff::SharedPtr<MemberNode>> *walkedNodes);

	class Writer {
	public:
		SLKC_API virtual ~Writer();

		virtual std::optional<CompilationError> write(const char *src, size_t size) = 0;

		SLAKE_FORCEINLINE std::optional<CompilationError> writeI8(int8_t data) noexcept {
			return write((char *)&data, sizeof(int8_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeI16(int16_t data) noexcept {
			return write((char *)&data, sizeof(int16_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeI32(int32_t data) noexcept {
			return write((char *)&data, sizeof(int32_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeI64(int64_t data) noexcept {
			return write((char *)&data, sizeof(int64_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeU8(uint8_t data) noexcept {
			return write((char *)&data, sizeof(uint8_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeU16(uint16_t data) noexcept {
			return write((char *)&data, sizeof(uint16_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeU32(uint32_t data) noexcept {
			return write((char *)&data, sizeof(uint32_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeU64(uint64_t data) noexcept {
			return write((char *)&data, sizeof(uint64_t));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeF32(float data) noexcept {
			return write((char *)&data, sizeof(float));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeF64(double data) noexcept {
			return write((char *)&data, sizeof(double));
		}

		SLAKE_FORCEINLINE std::optional<CompilationError> writeBool(bool data) noexcept {
			return write((char *)&data, sizeof(bool));
		}
	};

	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpGenericParam(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::GenericParam &genericParams);
	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpIdRefEntries(
		peff::Alloc *allocator,
		Writer *writer,
		const peff::DynArray<slake::IdRefEntry> &entries);
	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpIdRef(
		peff::Alloc *allocator,
		Writer *writer,
		slake::IdRefObject *ref);
	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpValue(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::Value &value);
	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpTypeName(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::Type &type);
	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpModuleMembers(
		peff::Alloc *allocator,
		Writer *writer,
		slake::ModuleObject *mod);
	[[nodiscard]] SLKC_API std::optional<CompilationError> dumpModule(
		peff::Alloc *allocator,
		Writer *writer,
		slake::ModuleObject *mod);

	class ExternalModuleProvider {
	public:
		const char *providerName;

		SLKC_API ExternalModuleProvider(const char *providerName);
		SLKC_API virtual ~ExternalModuleProvider();

		virtual std::optional<CompilationError> loadModule(CompileContext *compileContext, IdRef *moduleName) = 0;
	};

	class FileSystemExternalModuleProvider : public ExternalModuleProvider {
	public:
		peff::DynArray<peff::String> importPaths;

		SLKC_API FileSystemExternalModuleProvider(peff::Alloc *allocator);
		SLKC_API virtual ~FileSystemExternalModuleProvider();

		SLKC_API virtual std::optional<CompilationError> loadModule(CompileContext *compileContext, IdRef *moduleName) override;
		SLKC_API bool registerImportPath(peff::String &&path);
	};
}

#endif
