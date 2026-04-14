#include "module.h"
#include "attribute.h"
#include "import.h"
#include "parser.h"
#include "document.h"

using namespace slkc;

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
				if (!(generic_args.at(i) = rhs.generic_args.at(i)->duplicate<TypeNameNode>(allocator)))
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
				if (!(attributes.at(i) = rhs.attributes.at(i)->duplicate<AttributeNode>(allocator)))
					return false;
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
	  members(self_allocator),
	  member_indices(self_allocator),
	  anonymous_imports(self_allocator),
	  var_def_stmts(self_allocator) {
}

SLKC_API ModuleNode::ModuleNode(const ModuleNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out), members(allocator), member_indices(allocator), anonymous_imports(allocator), var_def_stmts(allocator) {
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
				if (!(var_def_stmts.at(i) = rhs.var_def_stmts.at(i)->duplicate<VarDefStmtNode>(allocator)))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!anonymous_imports.resize(rhs.anonymous_imports.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < anonymous_imports.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(anonymous_imports.at(i) = rhs.anonymous_imports.at(i)->duplicate<ImportNode>(allocator)))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!members.resize(rhs.members.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < members.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				AstNodePtr<MemberNode> &m = members.at(i);
				const AstNodePtr<MemberNode> &rm = rhs.members.at(i);
				if (!(m = rm->duplicate<MemberNode>(allocator)))
					return false;

				if (!index_member(i))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	is_var_def_stmts_normalized = rhs.is_var_def_stmts_normalized;

	succeeded_out = true;
}

SLKC_API ModuleNode::~ModuleNode() {
	if(parser)
	parser.reset();
}

SLKC_API size_t ModuleNode::push_member(AstNodePtr<MemberNode> member_node) noexcept {
	size_t n = members.size();

	if (!members.shrink_to_fit())
		return false;

	if (!members.push_back(std::move(member_node))) {
		return SIZE_MAX;
	}

	return n;
}

SLKC_API bool ModuleNode::add_member(AstNodePtr<MemberNode> member_node) noexcept {
	size_t index;

	if ((index = push_member(member_node)) == SIZE_MAX) {
		return false;
	}

	return index_member(index);
}

SLKC_API bool ModuleNode::index_member(size_t index_in_member_array) noexcept {
	AstNodePtr<MemberNode> m = members.at(index_in_member_array);

	if (!member_indices.insert(m->name, +index_in_member_array)) {
		return false;
	}

	m->set_parent(this);

	return true;
}

SLKC_API void ModuleNode::remove_member(const std::string_view &name) noexcept {
	size_t index = member_indices.at(name);
	members.erase_range(index, index + 1);
	member_indices.remove(name);
	for (auto i : member_indices) {
		if (i.second > index) {
			--i.second;
		}
	}
}

SLKC_API void ModuleNode::set_parser(const peff::SharedPtr<Parser> &parser) {
	parser->document = {};
	parser->cur_parent = {};
	this->parser = parser;
}
