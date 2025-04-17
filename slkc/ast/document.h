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
		TargetIsNotCallable,
		NoSuchFnOverloading,
		IncompatibleOperand,
		OperatorNotFound,
		MismatchedGenericArgNumber,
		DoesNotReferToATypeName,
		ExpectingClassName,
		ExpectingInterfaceName,
		CyclicInheritedClass,
		ExpectingId,
		IdNotFound,

		ImportLimitExceeded,
	};

	class TypeNameNode;

	struct IncompatibleOperandErrorExData {
		peff::SharedPtr<TypeNameNode> desiredType;
	};

	struct CompilationError {
		TokenRange tokenRange;
		CompilationErrorKind errorKind;
		std::variant<std::monostate, IncompatibleOperandErrorExData> exData;

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			CompilationErrorKind errorKind)
			: tokenRange(tokenRange),
			  errorKind(errorKind) {
		}

		SLAKE_FORCEINLINE CompilationError(
			const TokenRange &tokenRange,
			IncompatibleOperandErrorExData &&exData)
			: tokenRange(tokenRange),
			  errorKind(CompilationErrorKind::IncompatibleOperand),
			  exData(exData) {
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

		SLAKE_FORCEINLINE TypeNameListCmp(Document *compileContext) : document(document) {}

		SLAKE_API bool operator()(const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs) const noexcept;
	};

	using GenericCacheTable =
		peff::Map<
			peff::DynArray<peff::SharedPtr<TypeNameNode>>,
			peff::SharedPtr<MemberNode>,
			TypeNameListCmp>;

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

	class Document : public peff::SharedFromThis<Document> {
	public:
		peff::RcObjectPtr<peff::Alloc> allocator;
		peff::Map<
			peff::SharedPtr<MemberNode>,
			GenericCacheTable>
			genericCacheDir;

		SLKC_API Document(peff::Alloc *allocator);
		SLKC_API virtual ~Document();

		SLKC_API std::optional<CompilationError> lookupGenericCacheTable(peff::SharedPtr<MemberNode> originalObject, GenericCacheTable *&tableOut);

		SLKC_API std::optional<CompilationError> lookupGenericCacheTable(
			peff::SharedPtr<MemberNode> originalObject,
			const GenericCacheTable *&tableOut) const {
			return const_cast<Document *>(this)->lookupGenericCacheTable(originalObject, const_cast<GenericCacheTable *&>(tableOut));
		}

		SLKC_API std::optional<CompilationError> lookupGenericCache(
			peff::SharedPtr<MemberNode> originalObject,
			const peff::DynArray<peff::SharedPtr<TypeNameNode>> &genericArgs,
			peff::SharedPtr<MemberNode> &memberOut) const;

		SLKC_API std::optional<CompilationError> instantiateGenericObject(
			peff::SharedPtr<MemberNode> originalObject,
			const peff::DynArray<peff::SharedPtr<TypeNameNode>> &genericArgs,
			peff::SharedPtr<MemberNode> &memberOut);
	};
}

#endif
