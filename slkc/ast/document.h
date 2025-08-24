#ifndef _SLKC_DOCUMENT_H_
#define _SLKC_DOCUMENT_H_

#include "basedefs.h"
#include <peff/containers/dynarray.h>
#include <peff/containers/map.h>
#include <peff/advutils/shared_ptr.h>

namespace slkc {
	class Document;

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
		ExpectingId,
		IdNotFound,
		ParamAlreadyDefined,
		GenericParamAlreadyDefined,
		InvalidInitializerListUsage,
		ErrorDeducingInitializerListType,
		ErrorDeducingSwitchConditionType,
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

#define SLKC_RETURN_IF_COMP_ERROR(...) \
	if (std::optional<slkc::CompilationError> _ = (__VA_ARGS__); _) return _

	enum class CompilationWarningKind : int {
		UnusedExprResult = 0,
	};

	SLAKE_FORCEINLINE CompilationError genOutOfMemoryCompError() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfMemory);
	}

	SLAKE_FORCEINLINE CompilationError genOutOfRuntimeMemoryCompError() {
		return CompilationError(TokenRange{ 0, 0 }, CompilationErrorKind::OutOfRuntimeMemory);
	}

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

	struct TypeNameListCmp {
		Document *document;
		mutable std::optional<slkc::CompilationError> storedError;

		SLAKE_FORCEINLINE TypeNameListCmp(Document *document) : document(document) {}

		SLAKE_API bool operator()(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs) const noexcept;
	};

	class MemberNode;

	using GenericCacheTable =
		peff::Map<
			peff::DynArray<AstNodePtr<TypeNameNode>>,
			AstNodePtr<MemberNode>,
			TypeNameListCmp>;

	struct GenericInstantiationContext {
		peff::RcObjectPtr<peff::Alloc> allocator;
		const peff::DynArray<AstNodePtr<TypeNameNode>> *genericArgs;
		peff::HashMap<std::string_view, AstNodePtr<TypeNameNode>> mappedGenericArgs;
		AstNodePtr<MemberNode> mappedNode;

		SLAKE_FORCEINLINE GenericInstantiationContext(
			peff::Alloc *allocator,
			const peff::DynArray<AstNodePtr<TypeNameNode>> *genericArgs)
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
		peff::DynArray<peff::SharedPtr<ExternalModuleProvider>> externalModuleProviders;
		peff::Map<
			MemberNode *,
			GenericCacheTable>
			genericCacheDir;

		AstNode *destructibleAstNodeList = nullptr;

		SLKC_API Document(peff::Alloc *allocator);
		SLKC_API virtual ~Document();

		SLKC_API std::optional<CompilationError> lookupGenericCacheTable(AstNodePtr<MemberNode> originalObject, GenericCacheTable *&tableOut);

		SLKC_API std::optional<CompilationError> lookupGenericCacheTable(
			AstNodePtr<MemberNode> originalObject,
			const GenericCacheTable *&tableOut) const {
			return const_cast<Document *>(this)->lookupGenericCacheTable(originalObject, const_cast<GenericCacheTable *&>(tableOut));
		}

		SLKC_API std::optional<CompilationError> lookupGenericCache(
			AstNodePtr<MemberNode> originalObject,
			const peff::DynArray<AstNodePtr<TypeNameNode>> &genericArgs,
			AstNodePtr<MemberNode> &memberOut) const;

		SLKC_API std::optional<CompilationError> instantiateGenericObject(
			AstNodePtr<MemberNode> originalObject,
			const peff::DynArray<AstNodePtr<TypeNameNode>> &genericArgs,
			AstNodePtr<MemberNode> &memberOut);

		SLAKE_FORCEINLINE void clearDeferredDestructibleAstNodes() {
			if (destructibleAstNodeList) {
				_doClearDeferredDestructibleAstNodes();
			}
		}
	};
}

#endif
