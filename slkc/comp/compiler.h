#ifndef _SLKC_COMP_COMPILER_H_
#define _SLKC_COMP_COMPILER_H_

#include "../ast/parser.h"

namespace slkc {
	SLKC_API peff::Option<slkc::CompilationError> typeNameCmp(AstNodePtr<TypeNameNode> lhs, AstNodePtr<TypeNameNode> rhs, int &out) noexcept;
	SLKC_API peff::Option<slkc::CompilationError> typeNameListCmp(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs, int &out) noexcept;

	enum class ExprEvalPurpose {
		EvalType,	// None
		Stmt,		// As a statement
		LValue,		// As a lvalue
		RValue,		// As a rvalue
		Call,		// As target of a calling expression
		Unpacking,	// For unpacking, note that it is used for notifying the expression compiler to prepare the expression to be unpacked, not actually to unpack it.
	};

	struct StmtCompileEnvironment {
	};

	struct LocalVarRegistry {
		AstNodePtr<VarNode> varNode;
		uint32_t idxReg;
	};

	struct Label {
		peff::String name;
		uint32_t offset = 0;

		SLAKE_FORCEINLINE Label(peff::String &&name) : name(std::move(name)) {}
	};

	class CompilationContext {
	public:
		CompilationContext *parent = nullptr;

		SLKC_API CompilationContext(CompilationContext *parent);
		SLKC_API virtual ~CompilationContext();

		[[nodiscard]] virtual peff::Option<CompilationError> allocLabel(uint32_t &labelIdOut) = 0;
		virtual void setLabelOffset(uint32_t labelId, uint32_t offset) const = 0;
		[[nodiscard]] virtual peff::Option<CompilationError> setLabelName(uint32_t labelId, const std::string_view &name) = 0;
		virtual uint32_t getLabelOffset(uint32_t labelId) const = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> allocReg(uint32_t &regOut) = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> emitIns(uint32_t idxSld, slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) = 0;
		[[nodiscard]] virtual peff::Option<CompilationError> emitIns(uint32_t idxSld, slake::Opcode opcode, uint32_t outputRegIndex, slake::Value *operands, size_t nOperands) = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> allocLocalVar(const TokenRange &tokenRange, const std::string_view &name, uint32_t reg, AstNodePtr<TypeNameNode> type, AstNodePtr<VarNode> &localVarOut) = 0;
		[[nodiscard]] virtual AstNodePtr<VarNode> getLocalVarInCurLevel(const std::string_view &name) const = 0;
		virtual AstNodePtr<VarNode> getLocalVar(const std::string_view &name) const = 0;

		virtual void setBreakLabel(uint32_t labelId, uint32_t blockLevel) = 0;
		virtual void setContinueLabel(uint32_t labelId, uint32_t blockLevel) = 0;

		virtual uint32_t getBreakLabel() const = 0;
		virtual uint32_t getContinueLabel() const = 0;

		virtual uint32_t getBreakLabelBlockLevel() const = 0;
		virtual uint32_t getContinueLabelBlockLevel() const = 0;

		virtual uint32_t getCurInsOff() const = 0;

		[[nodiscard]] virtual peff::Option<CompilationError> enterBlock() = 0;
		virtual void leaveBlock() = 0;

		virtual uint32_t getBlockLevel() = 0;

		virtual peff::Option<CompilationError> registerSourceLocDesc(slake::slxfmt::SourceLocDesc sld, uint32_t &indexOut) = 0;

		SLKC_API AstNodePtr<VarNode> lookupLocalVar(const std::string_view &name) const;
	};

	struct CompileEnvironment;

	class NormalCompilationContext : public CompilationContext {
	public:
		struct BlockLayer {
			peff::HashMap<std::string_view, AstNodePtr<VarNode>> localVars;

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

		peff::DynArray<slake::slxfmt::SourceLocDesc> sourceLocDescs;
		peff::Map<slake::slxfmt::SourceLocDesc, size_t> sourceLocDescsMap;

		peff::DynArray<peff::SharedPtr<Label>> labels;
		peff::HashMap<std::string_view, size_t> labelNameIndices;

		uint32_t breakStmtJumpDestLabel = UINT32_MAX, continueStmtJumpDestLabel = UINT32_MAX;
		uint32_t breakStmtBlockLevel = 0, continueStmtBlockLevel = 0;

		const uint32_t baseInsOff;
		peff::DynArray<slake::Instruction> generatedInstructions;

		SLKC_API NormalCompilationContext(CompileEnvironment *compileEnv, CompilationContext *parent);
		SLKC_API virtual ~NormalCompilationContext();

		SLKC_API virtual peff::Option<CompilationError> allocLabel(uint32_t &labelIdOut) override;
		SLKC_API virtual void setLabelOffset(uint32_t labelId, uint32_t offset) const override;
		SLKC_API virtual peff::Option<CompilationError> setLabelName(uint32_t labelId, const std::string_view &name) override;
		SLKC_API virtual uint32_t getLabelOffset(uint32_t labelId) const override;

		SLKC_API virtual peff::Option<CompilationError> allocReg(uint32_t &regOut) override;

		SLKC_API virtual peff::Option<CompilationError> emitIns(uint32_t idxSld, slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) override;
		SLKC_API virtual peff::Option<CompilationError> emitIns(uint32_t idxSld, slake::Opcode opcode, uint32_t outputRegIndex, slake::Value *operands, size_t nOperands) override;

		SLKC_API virtual peff::Option<CompilationError> allocLocalVar(const TokenRange &tokenRange, const std::string_view &name, uint32_t reg, AstNodePtr<TypeNameNode> type, AstNodePtr<VarNode> &localVarOut) override;
		SLKC_API virtual AstNodePtr<VarNode> getLocalVarInCurLevel(const std::string_view &name) const override;
		SLKC_API virtual AstNodePtr<VarNode> getLocalVar(const std::string_view &name) const override;

		SLKC_API virtual void setBreakLabel(uint32_t labelId, uint32_t blockLevel) override;
		SLKC_API virtual void setContinueLabel(uint32_t labelId, uint32_t blockLevel) override;

		SLKC_API virtual uint32_t getBreakLabel() const override;
		SLKC_API virtual uint32_t getContinueLabel() const override;

		SLKC_API virtual uint32_t getBreakLabelBlockLevel() const override;
		SLKC_API virtual uint32_t getContinueLabelBlockLevel() const override;

		SLKC_API virtual uint32_t getCurInsOff() const override;

		SLKC_API virtual peff::Option<CompilationError> enterBlock() override;
		SLKC_API virtual void leaveBlock() override;

		SLKC_API virtual uint32_t getBlockLevel() override;

		SLKC_API virtual peff::Option<CompilationError> registerSourceLocDesc(slake::slxfmt::SourceLocDesc sld, uint32_t &indexOut) override;
	};

	struct CompileEnvironment {
		std::atomic_size_t refCount;
		slake::Runtime *runtime;
		slake::HostRefHolder hostRefHolder;
		peff::RcObjectPtr<peff::Alloc> selfAllocator, allocator;
		peff::SharedPtr<Document> document;
		peff::DynArray<CompilationError> errors;
		peff::DynArray<CompilationWarning> warnings;
		AstNodePtr<FnOverloadingNode> curOverloading;
		AstNodePtr<ThisNode> thisNode;
		uint32_t flags;

		SLAKE_FORCEINLINE CompileEnvironment(
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

		SLKC_API virtual ~CompileEnvironment();

		SLKC_API virtual void onRefZero() noexcept;
		SLAKE_FORCEINLINE size_t incRef() noexcept {
			return ++refCount;
		}
		SLAKE_FORCEINLINE size_t decRef() noexcept {
			if (!--refCount) {
				onRefZero();
				return 0;
			}

			return refCount;
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> pushError(CompilationError &&error) {
			if (!errors.pushBack(std::move(error)))
				return genOutOfMemoryCompError();

			return {};
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> pushWarning(CompilationWarning &&warning) {
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
		CompilationContext *compileEnv;
		uint32_t lastBreakStmtJumpDestLabel,
			lastBreakStmtBlockLevel;

		SLAKE_FORCEINLINE PrevBreakPointHolder(CompilationContext *compileEnv)
			: compileEnv(compileEnv),
			  lastBreakStmtJumpDestLabel(compileEnv->getBreakLabel()),
			  lastBreakStmtBlockLevel(compileEnv->getBreakLabelBlockLevel()) {}
		SLAKE_FORCEINLINE ~PrevBreakPointHolder() {
			compileEnv->setBreakLabel(lastBreakStmtJumpDestLabel, lastBreakStmtBlockLevel);
		}
	};

	struct PrevContinuePointHolder {
		CompilationContext *compileEnv;
		uint32_t lastContinueStmtJumpDestLabel,
			lastContinueStmtBlockLevel;

		SLAKE_FORCEINLINE PrevContinuePointHolder(CompilationContext *compileEnv)
			: compileEnv(compileEnv),
			  lastContinueStmtJumpDestLabel(compileEnv->getContinueLabel()),
			  lastContinueStmtBlockLevel(compileEnv->getContinueLabelBlockLevel()) {}
		SLAKE_FORCEINLINE ~PrevContinuePointHolder() {
			compileEnv->setContinueLabel(lastContinueStmtJumpDestLabel, lastContinueStmtBlockLevel);
		}
	};

	struct CompileExprResult {
		AstNodePtr<TypeNameNode> evaluatedType;

		// For parameter name query, etc, if exists.
		AstNodePtr<FnNode> callTargetFnSlot;
		peff::DynArray<AstNodePtr<FnOverloadingNode>> callTargetMatchedOverloadings;
		uint32_t idxThisRegOut = UINT32_MAX;

		SLAKE_FORCEINLINE CompileExprResult(peff::Alloc *allocator) : callTargetMatchedOverloadings(allocator) {}
	};

	struct ResolvedIdRefPart {
		bool isStatic;
		size_t nEntries;
		AstNodePtr<MemberNode> member;
	};

	using ResolvedIdRefPartList = peff::DynArray<ResolvedIdRefPart>;

	[[nodiscard]] SLKC_API peff::Option<CompilationError> _compileOrCastOperand(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		uint32_t regOut,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<TypeNameNode> desiredType,
		AstNodePtr<ExprNode> operand,
		AstNodePtr<TypeNameNode> operandType);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _compileSimpleBinaryExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<TypeNameNode> lhsType,
		AstNodePtr<TypeNameNode> desiredLhsType,
		ExprEvalPurpose lhsEvalPurpose,
		AstNodePtr<TypeNameNode> rhsType,
		AstNodePtr<TypeNameNode> desiredRhsType,
		ExprEvalPurpose rhsEvalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		slake::Opcode opcode,
		uint32_t idxSld);
	peff::Option<CompilationError> _compileSimpleAssignBinaryExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<TypeNameNode> lhsType,
		AstNodePtr<TypeNameNode> desiredLhsType,
		AstNodePtr<TypeNameNode> rhsType,
		AstNodePtr<TypeNameNode> desiredRhsType,
		ExprEvalPurpose rhsEvalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		uint32_t idxSld);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _compileSimpleLAndBinaryExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<BoolTypeNameNode> boolType,
		AstNodePtr<TypeNameNode> lhsType,
		AstNodePtr<TypeNameNode> rhsType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		slake::Opcode opcode,
		uint32_t idxSld);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _compileSimpleLOrBinaryExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<BoolTypeNameNode> boolType,
		AstNodePtr<TypeNameNode> lhsType,
		AstNodePtr<TypeNameNode> rhsType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		slake::Opcode opcode,
		uint32_t idxSld);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _compileSimpleBinaryAssignOpExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<TypeNameNode> lhsType,
		AstNodePtr<TypeNameNode> rhsType,
		AstNodePtr<TypeNameNode> desiredRhsType,
		ExprEvalPurpose rhsEvalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut,
		slake::Opcode opcode,
		uint32_t idxSld);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> resolveStaticMember(
		CompileEnvironment *compileEnv,
		peff::SharedPtr<Document> document,
		const AstNodePtr<MemberNode> &memberNode,
		const IdRefEntry &name,
		AstNodePtr<MemberNode> &memberOut);
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolveInstanceMember(
			CompileEnvironment *compileEnv,
			peff::SharedPtr<Document> document,
			AstNodePtr<MemberNode> memberNode,
			const IdRefEntry &name,
			AstNodePtr<MemberNode> &memberOut);
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolveIdRef(
			CompileEnvironment *compileEnv,
			peff::SharedPtr<Document> document,
			const AstNodePtr<MemberNode> &resolveRoot,
			IdRefEntry *idRef,
			size_t nEntries,
			AstNodePtr<MemberNode> &memberOut,
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
		peff::Option<CompilationError>
		resolveIdRefWithScopeNode(
			CompileEnvironment *compileEnv,
			peff::SharedPtr<Document> document,
			peff::Set<AstNodePtr<MemberNode>> &walkedNodes,
			const AstNodePtr<MemberNode> &resolveScope,
			IdRefEntry *idRef,
			size_t nEntries,
			AstNodePtr<MemberNode> &memberOut,
			ResolvedIdRefPartList *resolvedPartListOut,
			bool isStatic = true,
			bool isSealed = false);
	/// @brief Resolve a custom type name.
	/// @param compileEnv The compile context.
	/// @param resolveContext Previous resolve context.
	/// @param typeName Type name to be resolved.
	/// @param memberNodeOut Where the resolved member node will be stored.
	/// @return Critical error encountered that forced the resolution to interrupt.
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolveCustomTypeName(
			peff::SharedPtr<Document> document,
			const AstNodePtr<CustomTypeNameNode> &typeName,
			AstNodePtr<MemberNode> &memberNodeOut,
			peff::Set<AstNodePtr<MemberNode>> *walkedNodes = nullptr);
	[[nodiscard]] SLKC_API
		peff::Option<CompilationError>
		resolveBaseOverridenCustomTypeName(
			peff::SharedPtr<Document> document,
			const AstNodePtr<CustomTypeNameNode> &typeName,
			AstNodePtr<TypeNameNode> &typeNameOut);

	/// @brief Collect interfaces involved in the whole inheritance chain.
	/// @note Note that this function does not clear current set.
	/// @param document Document to be operated.
	/// @param derived Leaf interface node.
	/// @param walkedInterfaces Where the involved interfaces are stored.
	/// @param insertSelf Controls whether to insert the leaf interface itself into the involved interface set.
	/// @return std::nullopt No error.
	/// @return CompilationErrorKind::CyclicInheritedInterface Cyclic inherited interface was detected.
	[[nodiscard]] SLKC_API peff::Option<CompilationError> collectInvolvedInterfaces(
		peff::SharedPtr<Document> document,
		const AstNodePtr<InterfaceNode> &derived,
		peff::Set<AstNodePtr<InterfaceNode>> &walkedInterfaces,
		bool insertSelf);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isImplementedByInterface(
		peff::SharedPtr<Document> document,
		const AstNodePtr<InterfaceNode> &base,
		const AstNodePtr<InterfaceNode> &derived,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isImplementedByClass(
		peff::SharedPtr<Document> document,
		const AstNodePtr<InterfaceNode> &base,
		const AstNodePtr<ClassNode> &derived,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isStructRecursed(
		peff::SharedPtr<Document> document,
		const AstNodePtr<StructNode> &derived);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isBaseOf(
		peff::SharedPtr<Document> document,
		const AstNodePtr<ClassNode> &base,
		const AstNodePtr<ClassNode> &derived,
		bool &whetherOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> removeRefOfType(
		AstNodePtr<TypeNameNode> src,
		AstNodePtr<TypeNameNode> &typeNameOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isLValueType(
		AstNodePtr<TypeNameNode> src,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isSameType(
		const AstNodePtr<TypeNameNode> &lhs,
		const AstNodePtr<TypeNameNode> &rhs,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> getTypePromotionLevel(
		const AstNodePtr<TypeNameNode> &typeName,
		int &levelOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> determinePromotionalType(
		AstNodePtr<TypeNameNode> lhs,
		AstNodePtr<TypeNameNode> rhs,
		AstNodePtr<TypeNameNode> &typeNameOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isSameTypeInSignature(
		const AstNodePtr<TypeNameNode> &lhs,
		const AstNodePtr<TypeNameNode> &rhs,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isTypeConvertible(
		const AstNodePtr<TypeNameNode> &src,
		const AstNodePtr<TypeNameNode> &dest,
		bool isSealed,
		bool &whetherOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> _isTypeNameParamListTypeNameTree(
		AstNodePtr<TypeNameNode> type,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _doExpandParamListTypeNameTree(
		AstNodePtr<TypeNameNode> &type);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> simplifyParamListTypeNameTree(
		AstNodePtr<TypeNameNode> type,
		peff::Alloc *allocator,
		AstNodePtr<TypeNameNode> &typeNameOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> _isTypeNameGenericParamFacade(
		AstNodePtr<TypeNameNode> type,
		bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> _doExpandGenericParamFacadeTypeNameTree(
		AstNodePtr<TypeNameNode> &type);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> simplifyGenericParamFacadeTypeNameTree(
		AstNodePtr<TypeNameNode> type,
		peff::Alloc *allocator,
		AstNodePtr<TypeNameNode> &typeNameOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> getUnpackedTypeOf(
		AstNodePtr<TypeNameNode> type,
		AstNodePtr<TypeNameNode> &typeNameOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileUnaryExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<UnaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileBinaryExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<BinaryExprNode> expr,
		ExprEvalPurpose evalPurpose,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		const AstNodePtr<ExprNode> &expr,
		ExprEvalPurpose evalPurpose,
		AstNodePtr<TypeNameNode> desiredType,
		uint32_t resultRegOut,
		CompileExprResult &resultOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileStmt(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		const AstNodePtr<StmtNode> &stmt);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> evalExprType(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		const AstNodePtr<ExprNode> &expr,
		AstNodePtr<TypeNameNode> &typeOut,
		AstNodePtr<TypeNameNode> desiredType = {});

	[[nodiscard]] SLKC_API peff::Option<CompilationError> evalConstExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<ExprNode> expr,
		AstNodePtr<ExprNode> &exprOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> getFullIdRef(peff::Alloc *allocator, AstNodePtr<MemberNode> m, IdRefPtr &idRefOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileTypeName(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<TypeNameNode> typeName,
		slake::TypeRef &typeOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileIdRef(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		const IdRefEntry *entries,
		size_t nEntries,
		AstNodePtr<TypeNameNode> *paramTypes,
		size_t nParams,
		bool hasVarArgs,
		AstNodePtr<TypeNameNode> overridenType,
		slake::HostObjectRef<slake::IdRefObject> &idRefOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileValueExpr(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<ExprNode> expr,
		slake::Value &valueOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileGenericParams(
		CompileEnvironment *compileEnv,
		CompilationContext *compilationContext,
		AstNodePtr<ModuleNode> mod,
		AstNodePtr<GenericParamNode> *genericParams,
		size_t nGenericParams,
		slake::GenericParamList &genericParamListOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> compileModule(
		CompileEnvironment *compileEnv,
		AstNodePtr<ModuleNode> mod,
		slake::ModuleObject *modOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> reindexFnParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<FnOverloadingNode> fn);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> indexFnParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<FnOverloadingNode> fn);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> reindexClassGenericParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<ClassNode> cls);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> indexClassGenericParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<ClassNode> cls);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> reindexInterfaceGenericParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<InterfaceNode> interfaceNode);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> indexInterfaceGenericParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<InterfaceNode> interfaceNode);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> reindexStructGenericParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<StructNode> structNode);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> indexStructGenericParams(
		CompileEnvironment *compileEnv,
		AstNodePtr<StructNode> structNode);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> indexModuleMembers(
		CompileEnvironment *compileEnv,
		AstNodePtr<ModuleNode> moduleNode);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> determineFnOverloading(
		CompileEnvironment *compileEnv,
		AstNodePtr<FnNode> fnSlot,
		const AstNodePtr<TypeNameNode> *argTypes,
		size_t nArgTypes,
		bool isStatic,
		peff::DynArray<AstNodePtr<FnOverloadingNode>> &matchedOverloadings,
		peff::Set<AstNodePtr<MemberNode>> *walkedParents = nullptr);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> fnToTypeName(
		CompileEnvironment *compileEnv,
		AstNodePtr<FnOverloadingNode> fn,
		AstNodePtr<FnTypeNameNode> &evaluatedTypeOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> cleanupUnusedModuleTree(
		CompileEnvironment *compileEnv,
		AstNodePtr<ModuleNode> leaf);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> completeParentModules(
		CompileEnvironment *compileEnv,
		IdRef *modulePath,
		AstNodePtr<ModuleNode> leaf);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> renormalizeModuleVarDefStmts(
		CompileEnvironment *compileEnv,
		AstNodePtr<ModuleNode> mod);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> normalizeModuleVarDefStmts(
		CompileEnvironment *compileEnv,
		AstNodePtr<ModuleNode> mod);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> isFnSignatureSame(AstNodePtr<VarNode> *lParams, AstNodePtr<VarNode> *rParams, size_t nParams, AstNodePtr<TypeNameNode> lOverridenType, AstNodePtr<TypeNameNode> rOverridenType, bool &whetherOut);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> isFnSignatureDuplicated(AstNodePtr<FnOverloadingNode> lhs, AstNodePtr<FnOverloadingNode> rhs, bool &whetherOut);

	[[nodiscard]] SLKC_API peff::Option<CompilationError> visitBaseClass(AstNodePtr<TypeNameNode> cls, AstNodePtr<ClassNode> &classOut, peff::Set<AstNodePtr<MemberNode>> *walkedNodes);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> visitBaseInterface(AstNodePtr<TypeNameNode> cls, AstNodePtr<InterfaceNode> &classOut, peff::Set<AstNodePtr<MemberNode>> *walkedNodes);

	SLAKE_FORCEINLINE slake::slxfmt::SourceLocDesc tokenRangeToSld(const TokenRange &tokenRange) {
		slake::slxfmt::SourceLocDesc sld;

		SourceLocation srcLoc = tokenRange.moduleNode->parser->tokenList.at(tokenRange.beginIndex)->sourceLocation;

		sld.line = srcLoc.beginPosition.line;
		sld.column = srcLoc.beginPosition.column;

		return sld;
	}

	class Writer {
	public:
		SLKC_API virtual ~Writer();

		virtual peff::Option<CompilationError> write(const char *src, size_t size) = 0;

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeI8(int8_t data) noexcept {
			return write((char *)&data, sizeof(int8_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeI16(int16_t data) noexcept {
			return write((char *)&data, sizeof(int16_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeI32(int32_t data) noexcept {
			return write((char *)&data, sizeof(int32_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeI64(int64_t data) noexcept {
			return write((char *)&data, sizeof(int64_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeU8(uint8_t data) noexcept {
			return write((char *)&data, sizeof(uint8_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeU16(uint16_t data) noexcept {
			return write((char *)&data, sizeof(uint16_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeU32(uint32_t data) noexcept {
			return write((char *)&data, sizeof(uint32_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeU64(uint64_t data) noexcept {
			return write((char *)&data, sizeof(uint64_t));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeF32(float data) noexcept {
			return write((char *)&data, sizeof(float));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeF64(double data) noexcept {
			return write((char *)&data, sizeof(double));
		}

		SLAKE_FORCEINLINE peff::Option<CompilationError> writeBool(bool data) noexcept {
			return write((char *)&data, sizeof(bool));
		}
	};

	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpGenericParam(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::GenericParam &genericParams);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpIdRefEntries(
		peff::Alloc *allocator,
		Writer *writer,
		const peff::DynArray<slake::IdRefEntry> &entries);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpIdRef(
		peff::Alloc *allocator,
		Writer *writer,
		slake::IdRefObject *ref);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpValue(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::Value &value);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpTypeName(
		peff::Alloc *allocator,
		Writer *writer,
		const slake::TypeRef &type);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpModuleMembers(
		peff::Alloc *allocator,
		Writer *writer,
		slake::ModuleObject *mod);
	[[nodiscard]] SLKC_API peff::Option<CompilationError> dumpModule(
		peff::Alloc *allocator,
		Writer *writer,
		slake::ModuleObject *mod);

	class ExternalModuleProvider {
	public:
		const char *providerName;

		SLKC_API ExternalModuleProvider(const char *providerName);
		SLKC_API virtual ~ExternalModuleProvider();

		virtual peff::Option<CompilationError> loadModule(CompileEnvironment *compileEnv, IdRef *moduleName) = 0;
	};

	class FileSystemExternalModuleProvider : public ExternalModuleProvider {
	public:
		peff::DynArray<peff::String> importPaths;

		SLKC_API FileSystemExternalModuleProvider(peff::Alloc *allocator);
		SLKC_API virtual ~FileSystemExternalModuleProvider();

		SLKC_API virtual peff::Option<CompilationError> loadModule(CompileEnvironment *compileEnv, IdRef *moduleName) override;
		SLKC_API bool registerImportPath(peff::String &&path);
	};

	extern size_t szDefaultParseThreadStack;
	extern size_t szDefaultCompileThreadStack;
}

#endif
