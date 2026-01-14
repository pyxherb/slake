#ifndef _SLKC_DOCUMENT_H_
#define _SLKC_DOCUMENT_H_

#include <slkc/basedefs.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/map.h>
#include <peff/advutils/shared_ptr.h>
#include "astnode.h"

namespace slkc {
#define SLKC_RETURN_IF_COMP_ERROR(...)                             \
	if (peff::Option<slkc::CompilationError> _ = (__VA_ARGS__); _) \
		return _;                                                  \
	else
#define SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(lvar, ...) \
	if (lvar = (__VA_ARGS__))                          \
		return lvar;                                   \
	else

	enum class CompilationErrorKind : int {
		OutOfMemory = 0,
		StackOverflow,
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
		RecursedValueType,
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
		InterfaceMethodsConflicted,
		TypeIsNotInitializable,
		MemberIsNotAccessible,
		InvalidEnumBaseType,
		EnumItemIsNotAssignable,
		IncompatibleInitialValueType,

		ImportLimitExceeded,
		MalformedModuleName,
		ErrorParsingImportedModule,
		ModuleNotFound,
		RegLimitExceeded,

		InvalidMnemonic,

		ErrorWritingCompiledModule
	};

	class TypeNameNode;
	class ModuleNode;
	class FnOverloadingNode;

	struct IncompatibleOperandErrorExData {
		AstNodePtr<TypeNameNode> desiredType;
	};

	struct ErrorParsingImportedModuleErrorExData {
		peff::Option<LexicalError> lexicalError;
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
			  exData(std::move(exData)) {
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

	SLAKE_FORCEINLINE CompilationError genOutOfMemoryCompError() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfMemory);
	}

	SLAKE_FORCEINLINE CompilationError genStackOverflow() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::StackOverflow);
	}

	SLAKE_FORCEINLINE CompilationError genOutOfRuntimeMemoryCompError() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfRuntimeMemory);
	}

	enum class CompilationWarningKind : int {
		UnusedExprResult = 0,
	};

	struct CompilationWarning {
		TokenRange tokenRange;
		CompilationWarningKind warningKind;
		std::variant<std::monostate> exData;

		SLAKE_FORCEINLINE CompilationWarning(
			const TokenRange &tokenRange,
			CompilationWarningKind warningKind)
			: tokenRange(tokenRange),
			  warningKind(warningKind) {
		}
	};

	struct CompileEnvironment;
	struct GenericArgListCmp {
		Document *document;
		peff::RcObjectPtr<CompileEnvironment> compileEnv;
		mutable peff::Option<slkc::CompilationError> storedError;

		SLAKE_API GenericArgListCmp(Document *document, CompileEnvironment *compileEnv);
		SLAKE_API GenericArgListCmp(const GenericArgListCmp &r);
		SLAKE_API ~GenericArgListCmp();

		SLAKE_API peff::Option<int> operator()(const peff::DynArray<AstNodePtr<AstNode>> &lhs, const peff::DynArray<AstNodePtr<AstNode>> &rhs) const noexcept;
	};

	class MemberNode;

	using GenericCacheTable =
		peff::FallibleMap<
			peff::DynArray<AstNodePtr<AstNode>>,
			AstNodePtr<MemberNode>,
			GenericArgListCmp,
			true>;

	struct GenericInstantiationContext {
		peff::RcObjectPtr<peff::Alloc> allocator;
		const peff::DynArray<AstNodePtr<AstNode>> *genericArgs;
		peff::HashMap<std::string_view, AstNodePtr<AstNode>> mappedGenericArgs;
		AstNodePtr<MemberNode> mappedNode;

		SLAKE_FORCEINLINE GenericInstantiationContext(
			peff::Alloc *allocator,
			const peff::DynArray<AstNodePtr<AstNode>> *genericArgs)
			: allocator(allocator),
			  genericArgs(genericArgs),
			  mappedGenericArgs(allocator) {
		}
	};

	class ExternalModuleProvider;

	class Document : public peff::SharedFromThis<Document> {
	private:
		SLKC_API void _doClearDeferredDestructibleAstNodes();

	public:
		peff::RcObjectPtr<peff::Alloc> allocator;
		AstNodePtr<ModuleNode> rootModule;
		ModuleNode *mainModule = nullptr;
		peff::DynArray<peff::SharedPtr<ExternalModuleProvider>> externalModuleProviders;
		peff::Map<
			MemberNode *,
			GenericCacheTable>
			genericCacheDir;

		AstNode *destructibleAstNodeList = nullptr;

		SLKC_API Document(peff::Alloc *allocator);
		SLKC_API virtual ~Document();

		SLKC_API peff::Option<CompilationError> lookupGenericCacheTable(AstNodePtr<MemberNode> originalObject, GenericCacheTable *&tableOut);

		SLKC_API peff::Option<CompilationError> lookupGenericCacheTable(
			AstNodePtr<MemberNode> originalObject,
			const GenericCacheTable *&tableOut) const {
			return const_cast<Document *>(this)->lookupGenericCacheTable(originalObject, const_cast<GenericCacheTable *&>(tableOut));
		}

		SLKC_API peff::Option<CompilationError> lookupGenericCache(
			AstNodePtr<MemberNode> originalObject,
			const peff::DynArray<AstNodePtr<AstNode>> &genericArgs,
			AstNodePtr<MemberNode> &memberOut) const;

		SLKC_API peff::Option<CompilationError> instantiateGenericObject(
			AstNodePtr<MemberNode> originalObject,
			const peff::DynArray<AstNodePtr<AstNode>> &genericArgs,
			AstNodePtr<MemberNode> &memberOut);

		SLAKE_FORCEINLINE void clearDeferredDestructibleAstNodes() {
			if (destructibleAstNodeList) {
				_doClearDeferredDestructibleAstNodes();
			}
		}
	};
}

#endif
