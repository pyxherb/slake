#ifndef _SLKC_DOCUMENT_H_
#define _SLKC_DOCUMENT_H_

#include <slkc/basedefs.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/map.h>
#include <peff/advutils/shared_ptr.h>

namespace slkc {
#define SLKC_RETURN_IF_COMP_ERROR(...)                              \
	if (peff::Option<slkc::CompilationError> _ = (__VA_ARGS__); _) \
		return _;                                                   \
	else                                                            \
		;
#define SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(lvar, ...)                             \
	if (lvar = (__VA_ARGS__)) \
		return lvar;                                                  \
	else                                                           \
		;

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
		mutable peff::Option<slkc::CompilationError> storedError;

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
			const peff::DynArray<AstNodePtr<TypeNameNode>> &genericArgs,
			AstNodePtr<MemberNode> &memberOut) const;

		SLKC_API peff::Option<CompilationError> instantiateGenericObject(
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
