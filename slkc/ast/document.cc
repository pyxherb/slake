#include "../comp/compiler.h"

using namespace slkc;

SLKC_API Document::Document(peff::Alloc *allocator) : allocator(allocator), external_module_providers(allocator), generic_cache_dir(allocator) {
}

SLKC_API Document::~Document() {
	_do_clear_deferred_destructible_ast_nodes();
}

SLKC_API void Document::_do_clear_deferred_destructible_ast_nodes() {
	AstNode *i, *next;

	while ((i = destructible_ast_node_list)) {
		destructible_ast_node_list = nullptr;

		while (i) {
			next = i->_next_destructible;
			i->_destructor(i);
			i = next;
		};
	}
}

SLAKE_API GenericArgListCmp::GenericArgListCmp(Document *document, slake::Runtime *runtime) : document(document), runtime(runtime) {
}

SLAKE_API GenericArgListCmp::GenericArgListCmp(const GenericArgListCmp &r) : document(r.document), runtime(r.runtime) {
}

SLAKE_API GenericArgListCmp::GenericArgListCmp::~GenericArgListCmp() {
}

SLAKE_API peff::Option<int> GenericArgListCmp::operator()(const peff::DynArray<AstNodePtr<TypeNameNode>> &lhs, const peff::DynArray<AstNodePtr<TypeNameNode>> &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;
	for (size_t i = 0; i < lhs.size(); ++i) {
		AstNodePtr<TypeNameNode> l = lhs.at(i), r = rhs.at(i);

		if (l->get_ast_node_type() < r->get_ast_node_type())
			return -1;
		if (l->get_ast_node_type() > r->get_ast_node_type())
			return 1;
		int result;
		if (auto e = type_name_cmp(l.cast_to<TypeNameNode>(), r.cast_to<TypeNameNode>(), result); e) {
			stored_error = std::move(e);
			return {};
		}
		if (result)
			return result;
		return 0;
	}

	std::terminate();
}
