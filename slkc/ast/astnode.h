#ifndef _SLKC_AST_ASTNODE_H_
#define _SLKC_AST_ASTNODE_H_

#include "lexer.h"
#include <peff/advutils/shared_ptr.h>

namespace slkc {
	enum class AstNodeType : uint8_t {
		Struct = 0,
		Enum,
		EnumItem,
		AttributeDef,
		Attribute,
		FnSlot,
		Fn,
		Stmt,
		Expr,
		TypeName,
		Using,
		Var,
		GenericParam,
		Module,
		Class,
		Interface,
		Import,

		Root,
		This,

		Bad
	};

	struct TokenRange {
		size_t beginIndex = SIZE_MAX, endIndex = SIZE_MAX;

		inline TokenRange() = default;
		inline TokenRange(size_t index) : beginIndex(index), endIndex(index) {}
		inline TokenRange(size_t beginIndex, size_t endIndex) : beginIndex(beginIndex), endIndex(endIndex) {}

		SLAKE_FORCEINLINE operator bool() const {
			return beginIndex != SIZE_MAX;
		}

		SLAKE_FORCEINLINE bool operator<(const TokenRange &rhs) const {
			return beginIndex < rhs.beginIndex;
		}

		SLAKE_FORCEINLINE bool operator>(const TokenRange &rhs) const {
			return beginIndex < rhs.beginIndex;
		}
	};

	constexpr static size_t ASTNODE_ALIGNMENT = sizeof(std::max_align_t);

	class AstNode;

	typedef void (*AstNodeDestructor)(AstNode *astNode);

	template<typename T>
	using AstNodePtr = peff::SharedPtr<T>;

	class AstNode : public peff::SharedFromThis<AstNode> {
	private:
		AstNodeType _astNodeType;

	protected:
		SLKC_API virtual AstNodePtr<AstNode> doDuplicate(peff::Alloc *newAllocator) const;

	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		Document *document;
		TokenRange tokenRange;

		AstNode *_nextDestructible = nullptr;
		AstNodeDestructor _destructor = nullptr;

		SLKC_API AstNode(AstNodeType astNodeType, peff::Alloc *selfAllocator, const peff::SharedPtr<Document> &document);
		SLKC_API AstNode(const AstNode &other, peff::Alloc *newAllocator);
		SLKC_API virtual ~AstNode();

		template <typename T>
		SLAKE_FORCEINLINE peff::SharedPtr<T> duplicate(peff::Alloc *newAllocator) const noexcept {
			return doDuplicate(newAllocator).template castTo<T>();
		}

		SLAKE_FORCEINLINE AstNodeType getAstNodeType() const noexcept {
			return _astNodeType;
		}
	};

	SLKC_API void addAstNodeToDestructibleList(AstNode *astNode, AstNodeDestructor _destructor);

	template <typename T>
	struct AstNodeControlBlock : public peff::SharedPtr<T>::DefaultSharedPtrControlBlock {
		PEFF_FORCEINLINE AstNodeControlBlock(peff::Alloc *allocator, T *ptr) noexcept : peff::SharedPtr<T>::DefaultSharedPtrControlBlock(allocator, ptr) {}
		inline virtual ~AstNodeControlBlock() {}

		inline virtual void onStrongRefZero() noexcept override {
			addAstNodeToDestructibleList(this->ptr, [](AstNode *astNode) {
				peff::destroyAndRelease<T>(astNode->selfAllocator.get(), static_cast<T *>(astNode), alignof(T));
			});
		}

		inline virtual void onRefZero() noexcept override {
			peff::destroyAndRelease<AstNodeControlBlock<T>>(this->allocator.get(), this, alignof(AstNodeControlBlock<T>));
		}
	};

	template <typename T, typename... Args>
	SLAKE_FORCEINLINE AstNodePtr<T> makeAstNode(peff::Alloc *allocator, Args &&...args) {
		return peff::makeSharedWithControlBlock<T, AstNodeControlBlock<T>>(allocator, std::forward<Args>(args)...);
	}

	enum class CompilationErrorKind : int {
		OutOfMemory = 0,
		OutOfRuntimeMemory,
		ExpectingLValueExpr,
		ExpectingRValueExpr,
		TargetIsNotCallable,
		TargetIsNotUnpackable,
		NoSuchFnOverloading,
		IncompatibleOperand,
		OperatorNotFound,
		AmbiguousOperatorCall,
		MismatchedGenericArgNumber,
		DoesNotReferToATypeName,
		ExpectingTypeName,
		ExpectingClassName,
		ExpectingInterfaceName,
		AbstractMethodNotImplemented,
		CyclicInheritedClass,
		CyclicInheritedInterface,
		RecursedStruct,
		ExpectingId,
		IdNotFound,
		ParamAlreadyDefined,
		GenericParamAlreadyDefined,
		InvalidInitializerListUsage,
		ErrorDeducingInitializerListType,
		ErrorDeducingSwitchConditionType,
		ErrorDeducingArgType,
		ErrorEvaluatingConstSwitchCaseCondition,
		MismatchedSwitchCaseConditionType,
		ErrorDeducingMatchConditionType,
		DuplicatedSwitchCaseBranch,
		ErrorDeducingMatchResultType,
		ErrorEvaluatingConstMatchCaseCondition,
		MismatchedMatchCaseConditionType,
		DuplicatedMatchCaseBranch,
		MissingDefaultMatchCaseBranch,
		InvalidThisUsage,
		NoMatchingFnOverloading,
		UnableToDetermineOverloading,
		ArgsMismatched,
		MemberAlreadyDefined,
		MissingBindingObject,
		RedundantWithObject,
		LocalVarAlreadyExists,
		InvalidBreakUsage,
		InvalidContinueUsage,
		InvalidCaseLabelUsage,
		TypeIsNotConstructible,
		InvalidCast,
		FunctionOverloadingDuplicated,
		RequiresInitialValue,
		ErrorDeducingVarType,
		TypeIsNotUnpackable,
		InvalidVarArgHintDuringInstantiation,
		CannotBeUnpackedInThisContext,
		TypeIsNotSubstitutable,
		RequiresCompTimeExpr,
		TypeArgTypeMismatched,

		ImportLimitExceeded,
		MalformedModuleName,
		ErrorParsingImportedModule,
		ModuleNotFound,
		RegLimitExceeded,

		ErrorWritingCompiledModule
	};

	class TypeNameNode;
	class ModuleNode;
	class FnOverloadingNode;

	struct IncompatibleOperandErrorExData {
		AstNodePtr<TypeNameNode> desiredType;
	};

	struct ErrorParsingImportedModuleErrorExData {
		std::optional<LexicalError> lexicalError;
		AstNodePtr<ModuleNode> mod;

		SLAKE_FORCEINLINE ErrorParsingImportedModuleErrorExData(LexicalError &&lexicalError) : lexicalError(std::move(lexicalError)) {}
		SLAKE_FORCEINLINE ErrorParsingImportedModuleErrorExData(AstNodePtr<ModuleNode> mod) : mod(mod) {}
	};

	struct AbstractMethodNotImplementedErrorExData {
		AstNodePtr<FnOverloadingNode> overloading;
	};

	struct CompilationError {
		TokenRange tokenRange;
		CompilationErrorKind errorKind;
		std::variant<std::monostate, IncompatibleOperandErrorExData, ErrorParsingImportedModuleErrorExData, AbstractMethodNotImplementedErrorExData> exData;

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			CompilationErrorKind errorKind)
			: tokenRange(tokenRange),
			  errorKind(errorKind) {
			assert(tokenRange);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			IncompatibleOperandErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(CompilationErrorKind::IncompatibleOperand),
			  exData(exData) {
			assert(tokenRange);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			ErrorParsingImportedModuleErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(CompilationErrorKind::ErrorParsingImportedModule),
			  exData(exData) {
			assert(tokenRange);
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			AbstractMethodNotImplementedErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(CompilationErrorKind::AbstractMethodNotImplemented),
			  exData(exData) {
			assert(tokenRange);
		}

		SLAKE_FORCEINLINE bool operator<(const CompilationError &rhs) const noexcept {
			return tokenRange < rhs.tokenRange;
		}

		SLAKE_FORCEINLINE bool operator>(const CompilationError &rhs) const noexcept {
			return tokenRange > rhs.tokenRange;
		}
	};
}

#endif
