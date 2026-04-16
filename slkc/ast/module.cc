#include "module.h"
#include "attribute.h"
#include "import.h"
#include "parser.h"
#include "document.h"

using namespace slkc;

SLKC_API Scope::Scope(peff::Alloc *allocator) noexcept
	: allocator(allocator),
	  _members(allocator),
	  _member_indices(allocator),
	  anonymous_imports(allocator),
	  impl_types(allocator),
	  generic_params(allocator),
	  generic_param_indices(allocator) {
}

SLKC_API Scope::~Scope() {
}

SLKC_API size_t Scope::push_member(AstNodePtr<MemberNode> member_node) noexcept {
	size_t n = _members.size();

	if (!_members.shrink_to_fit())
		return SIZE_MAX;

	if (!_members.push_back(std::move(member_node))) {
		return SIZE_MAX;
	}

	return n;
}

SLKC_API bool Scope::add_member(AstNodePtr<MemberNode> member_node) noexcept {
	size_t index;

	if ((index = push_member(member_node)) == SIZE_MAX) {
		return false;
	}

	return index_member(index);
}

SLKC_API bool Scope::index_member(size_t index_in_member_array) noexcept {
	AstNodePtr<MemberNode> m = _members.at(index_in_member_array);

	if (!_member_indices.insert(m->name, +index_in_member_array)) {
		return false;
	}

	m->set_parent(owner);

	return true;
}

SLKC_API void Scope::remove_member(const std::string_view &name) noexcept {
	size_t index = _member_indices.at(name);
	_members.erase_range(index, index + 1);
	_member_indices.remove(name);
	for (auto i : _member_indices) {
		if (i.second > index) {
			--i.second;
		}
	}
}

SLKC_API size_t Scope::push_generic_param(AstNodePtr<GenericParamNode> generic_param_node) noexcept {
	size_t n = generic_params.size();

	if (!generic_params.shrink_to_fit())
		return SIZE_MAX;

	if (!generic_params.push_back(std::move(generic_param_node))) {
		return SIZE_MAX;
	}

	return n;
}

SLKC_API bool Scope::add_generic_param(AstNodePtr<GenericParamNode> generic_param_node) noexcept {
	size_t index;

	if ((index = push_generic_param(generic_param_node)) == SIZE_MAX) {
		return false;
	}

	return index_generic_param(index);
}

SLKC_API bool Scope::index_generic_param(size_t index_in_generic_param_array) noexcept {
	AstNodePtr<GenericParamNode> m = generic_params.at(index_in_generic_param_array);

	if (!generic_param_indices.insert(m->name, +index_in_generic_param_array)) {
		return false;
	}

	m->set_parent(owner);

	return true;
}

SLKC_API void Scope::remove_generic_param(const std::string_view &name) noexcept {
	size_t index = generic_param_indices.at(name);
	generic_params.erase_range(index, index + 1);
	generic_param_indices.remove(name);
	for (auto i : generic_param_indices) {
		if (i.second > index) {
			--i.second;
		}
	}
}

SLKC_API Scope *Scope::duplicate(peff::Alloc *allocator, DuplicationContext &context) {
	peff::UniquePtr<Scope, peff::DeallocableDeleter<Scope>> dest;

	if (!(dest = Scope::alloc(allocator)))
		return nullptr;

	if (!dest->_members.resize(_members.size()))
		return nullptr;

	for (size_t i = 0; i < this->_members.size(); ++i) {
		if (!context.push_task([this, i, dest = dest.get(), allocator, &context]() -> bool {
				if (!(dest->_members.at(i) = this->_members.at(i)->do_duplicate(allocator, context).cast_to<MemberNode>()))
					return false;
				if (!dest->index_member(i))
					return false;
				return true;
			}))
			return nullptr;
	}

	if (!dest->anonymous_imports.build(this->anonymous_imports))
		return nullptr;

	dest->base_type = this->base_type;
	if (!context.push_task([this, dest = dest.get(), allocator, &context]() -> bool {
			if (this->base_type && !(dest->base_type = this->base_type->do_duplicate(allocator, context).cast_to<TypeNameNode>())) {
				return false;
			}
			return true;
		})) {
		return nullptr;
	}

	if (!dest->impl_types.resize(this->impl_types.size()))
		return nullptr;
	for (size_t i = 0; i < this->impl_types.size(); ++i) {
		if (!context.push_task([this, i, dest = dest.get(), allocator, &context]() -> bool {
				if (!(dest->impl_types.at(i) = this->impl_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			}))
			return nullptr;
	}

	if (!dest->generic_params.resize(generic_params.size()))
		return nullptr;

	for (size_t i = 0; i < this->generic_params.size(); ++i) {
		if (!context.push_task([this, i, dest = dest.get(), allocator, &context]() -> bool {
				if (!(dest->generic_params.at(i) = this->generic_params.at(i)->do_duplicate(allocator, context).cast_to<GenericParamNode>()))
					return false;
				if (!dest->index_generic_param(i))
					return false;
				return true;
			}))
			return nullptr;
	}

	return dest.release();
}

SLKC_API MemberNode::MemberNode(
	AstNodeType ast_node_type,
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: AstNode(ast_node_type, self_allocator, document),
	  name(self_allocator),
	  generic_args(self_allocator),
	  attributes(self_allocator) {
}

SLKC_API MemberNode::MemberNode(const MemberNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : AstNode(rhs, allocator, context), name(allocator), generic_args(allocator), attributes(allocator) {
	if (!name.build(rhs.name)) {
		succeeded_out = false;
		return;
	}

	access_modifier = rhs.access_modifier;

	if (!generic_args.resize(rhs.generic_args.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < generic_args.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(generic_args.at(i) = rhs.generic_args.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!attributes.resize(rhs.attributes.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < attributes.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(attributes.at(i) = rhs.attributes.at(i)->do_duplicate(allocator, context).cast_to<AttributeNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (rhs.scope) {
		if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
				if (!(this->scope = rhs.get_scope()->duplicate(allocator, context))) {
					return false;
				}
				this->scope->owner = this;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}

SLKC_API MemberNode::~MemberNode() {
}

SLKC_API AstNodePtr<AstNode> ModuleNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ModuleNode> duplicated_node(make_ast_node<ModuleNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ModuleNode::ModuleNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document,
	AstNodeType ast_node_type)
	: MemberNode(ast_node_type, self_allocator, document),
	  var_def_stmts(self_allocator) {
}

SLKC_API ModuleNode::ModuleNode(
	const ModuleNode &rhs,
	peff::Alloc *allocator,
	DuplicationContext &context,
	bool &succeeded_out)
	: MemberNode(rhs, allocator, context, succeeded_out),
	  var_def_stmts(allocator) {
	if (!succeeded_out) {
		return;
	}

	parser = rhs.parser;

	if (!var_def_stmts.resize(rhs.var_def_stmts.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < var_def_stmts.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(var_def_stmts.at(i) = rhs.var_def_stmts.at(i)->do_duplicate(allocator, context).cast_to<VarDefStmtNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	succeeded_out = true;
}

SLKC_API ModuleNode::~ModuleNode() {
}

SLKC_API void ModuleNode::set_parser(const peff::SharedPtr<Parser> &parser) {
	parser->document = {};
	parser->cur_parent = {};
	this->parser = parser;
}
