#include "../comp/compiler.h"

using namespace slkc;

SLKC_API Document::Document(peff::Alloc *allocator): allocator(allocator), externalModuleProviders(allocator), genericCacheDir(allocator) {
}

SLKC_API Document::~Document() {
	clearDeferredDestructibleAstNodes();
}

SLKC_API void Document::clearDeferredDestructibleAstNodes() {
	AstNode *i, *next;

	while ((i = destructibleAstNodeList)) {
		destructibleAstNodeList = nullptr;

		while (i) {
			next = i->nextDestructible;
			i->destructor(i);
			i = next;
		};
	}
}

SLAKE_API bool TypeNameListCmp::operator()(const peff::DynArray<peff::SharedPtr<TypeNameNode>> &lhs, const peff::DynArray<peff::SharedPtr<TypeNameNode>> &rhs) const noexcept {
	int result;
	// Note that we just need one critical error to notify the compiler
	// that we have encountered errors that will force the compilation
	// to be interrupted.
	if ((storedError = typeNameListCmp(lhs, rhs, result))) {
		return false;
	}
	return result < 0;
}
