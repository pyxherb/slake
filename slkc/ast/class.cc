#include "../comp/compiler.h"
#include "class.h"

using namespace slkc;

SLKC_API AstNodePtr<AstNode> ClassNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ClassNode> duplicated_node(make_ast_node<ClassNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ClassNode::ClassNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::Class),
	  idx_generic_param_comma_tokens(self_allocator) {
}

SLKC_API ClassNode::ClassNode(const ClassNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out), idx_generic_param_comma_tokens(allocator) {
	if (!succeeded_out) {
		return;
	}

	if (!idx_generic_param_comma_tokens.resize(rhs.idx_generic_param_comma_tokens.size())) {
		succeeded_out = false;
		return;
	}

	memcpy(idx_generic_param_comma_tokens.data(), rhs.idx_generic_param_comma_tokens.data(), sizeof(size_t) * idx_generic_param_comma_tokens.size());

	idx_langle_bracket_token = rhs.idx_langle_bracket_token;
	idx_rangle_bracket_token = rhs.idx_rangle_bracket_token;

	is_generic_params_indexed = rhs.is_generic_params_indexed;

	succeeded_out = true;
}

SLKC_API ClassNode::~ClassNode() {
}

SLKC_API peff::Option<CompilationError> ClassNode::is_cyclic_inherited(bool &whether_out) {
	if (is_cyclic_inheritance_checked) {
		whether_out = is_cyclic_inherited_flag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(update_cyclic_inherited_status());

	whether_out = is_cyclic_inherited_flag;
	return {};
}

struct HigherRankedCyclicInheritanceWalkFrame {
	AstNodePtr<CustomTypeNameNode> type_name;
	size_t idx_cur_type_arg = 0;
};

SLKC_API peff::Option<CompilationError> slkc::is_higher_ranked_cyclic_inherited(
	AstNodePtr<MemberNode> cls,
	peff::Alloc *allocator,
	bool &result_out,
	bool forced_updates) noexcept {
	assert(cls->scope);

	if (cls->scope->cached_is_higher_ranked_cyclic_inherited) {
		result_out = cls->scope->cached_is_higher_ranked_cyclic_inherited.value();
		return {};
	}

	peff::List<HigherRankedCyclicInheritanceWalkFrame> frames(allocator);

	if (cls->scope->base_type && (cls->scope->base_type->tn_kind == TypeNameKind::Custom)) {
		if (!frames.push_back({ cls->scope->base_type.cast_to<CustomTypeNameNode>(), 0 }))
			return gen_oom_comp_error();
	}

	for (auto i : cls->scope->impl_types) {
		if (cls->scope->base_type->tn_kind == TypeNameKind::Custom) {
			if (!frames.push_back({ i.cast_to<CustomTypeNameNode>(), 0 }))
				return gen_oom_comp_error();
		}
	}

	while (frames.size()) {
		AstNodePtr<TypeNameNode> cur_type_arg;
		{
			auto &cur_frame = frames.back();
			auto &type_name = cur_frame.type_name;

			if (cur_frame.idx_cur_type_arg >= type_name->id_ref_ptr->entries.back().generic_args.size()) {
				frames.pop_back();
				continue;
			}

			cur_type_arg = type_name->id_ref_ptr->entries.back().generic_args.at(cur_frame.idx_cur_type_arg);

			++cur_frame.idx_cur_type_arg;
		}

		if (cur_type_arg->tn_kind == TypeNameKind::Custom) {
			auto ctn = cur_type_arg.cast_to<CustomTypeNameNode>();
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(
				resolve_custom_type_name(
					nullptr,
					cls->document->shared_from_this(),
					ctn,
					m,
					false));

			if (m) {
				if (m == cls) {
					cls->scope->cached_is_higher_ranked_cyclic_inherited = (result_out = true);
					return {};
				}
			}

			if (!frames.push_back(HigherRankedCyclicInheritanceWalkFrame{ ctn, 0 }))
				return gen_oom_comp_error();
		}
	}

	cls->scope->cached_is_higher_ranked_cyclic_inherited = (result_out = false);
	return {};
}

struct HigherRankedRecursedWalkFrame {
	AstNodePtr<CustomTypeNameNode> type_name;
	size_t idx_cur_type_arg = 0;
};

SLKC_API peff::Option<CompilationError> slkc::is_higher_ranked_recursed(
	AstNodePtr<MemberNode> member,
	AstNodePtr<CustomTypeNameNode> ctn,
	peff::Alloc *allocator,
	bool &result_out) noexcept {
	assert(member);

	if (member->scope->cached_is_higher_ranked_recursed) {
		result_out = member->scope->cached_is_higher_ranked_recursed.value();
		return {};
	}

	peff::List<HigherRankedRecursedWalkFrame> frames(allocator);

	if (!frames.push_back({ ctn, 0 }))
		return gen_oom_comp_error();

	while (frames.size()) {
		AstNodePtr<TypeNameNode> cur_type_arg;
		{
			auto &cur_frame = frames.back();
			auto &type_name = cur_frame.type_name;

			if (cur_frame.idx_cur_type_arg >= type_name->id_ref_ptr->entries.back().generic_args.size()) {
				frames.pop_back();
				continue;
			}

			cur_type_arg = type_name->id_ref_ptr->entries.back().generic_args.at(cur_frame.idx_cur_type_arg);

			++cur_frame.idx_cur_type_arg;
		}

		if (cur_type_arg->tn_kind == TypeNameKind::Custom) {
			auto converted_tn = cur_type_arg.cast_to<CustomTypeNameNode>();
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(
				resolve_custom_type_name(
					nullptr,
					converted_tn->document->shared_from_this(),
					converted_tn,
					m,
					false));

			if (m) {
				if (m == member) {
					member->scope->cached_is_higher_ranked_recursed = (result_out = true);
					return {};
				}
			}

			if (!frames.push_back(HigherRankedRecursedWalkFrame{ converted_tn, 0 }))
				return gen_oom_comp_error();
		}
	}

	member->scope->cached_is_higher_ranked_recursed = (result_out = false);
	return {};
}

SLKC_API peff::Option<CompilationError> ClassNode::update_cyclic_inherited_status() {
	if (is_cyclic_inherited_flag)
		return {};

	if (scope->base_type && scope->base_type->tn_kind == TypeNameKind::Custom) {
		AstNodePtr<CustomTypeNameNode> ctn;
		SLKC_RETURN_IF_COMP_ERROR(
			is_higher_ranked_cyclic_inherited(
				shared_from_this().cast_to<MemberNode>(),
				document->allocator.get(),
				is_cyclic_inherited_flag,
				ctn));
	}

	if (!is_cyclic_inherited_flag) {
		SLKC_RETURN_IF_COMP_ERROR(is_base_of(document->shared_from_this(), shared_from_this().cast_to<ClassNode>(), shared_from_this().cast_to<ClassNode>(), is_cyclic_inherited_flag));
	}

	is_cyclic_inheritance_checked = true;
	return {};
}

SLKC_API AstNodePtr<AstNode> InterfaceNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<InterfaceNode> duplicated_node(make_ast_node<InterfaceNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API InterfaceNode::InterfaceNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::Interface),
	  idx_generic_param_comma_tokens(self_allocator) {
}

SLKC_API InterfaceNode::InterfaceNode(const InterfaceNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out), idx_generic_param_comma_tokens(allocator) {
	if (!succeeded_out) {
		return;
	}

	if (!idx_generic_param_comma_tokens.resize(rhs.idx_generic_param_comma_tokens.size())) {
		succeeded_out = false;
		return;
	}

	memcpy(idx_generic_param_comma_tokens.data(), rhs.idx_generic_param_comma_tokens.data(), sizeof(size_t) * idx_generic_param_comma_tokens.size());

	idx_langle_bracket_token = rhs.idx_langle_bracket_token;
	idx_rangle_bracket_token = rhs.idx_rangle_bracket_token;

	is_generic_params_indexed = rhs.is_generic_params_indexed;

	succeeded_out = true;
}

SLKC_API InterfaceNode::~InterfaceNode() {
}

SLKC_API peff::Option<CompilationError> InterfaceNode::is_cyclic_inherited(bool &whether_out) {
	if (is_cyclic_inheritance_checked) {
		whether_out = is_cyclic_inherited_flag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(update_cyclic_inherited_status());

	whether_out = is_cyclic_inherited_flag;
	return {};
}

SLKC_API peff::Option<CompilationError> InterfaceNode::update_cyclic_inherited_status() {
	peff::Set<AstNodePtr<InterfaceNode>> involved_interfaces(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(is_higher_ranked_cyclic_inherited(shared_from_this().cast_to<MemberNode>(), document->allocator.get(), is_cyclic_inherited_flag, true));
	if (is_cyclic_inherited_flag) {
		is_cyclic_inheritance_checked = true;
		return {};
	}

	if (auto e = collect_involved_interfaces(document->shared_from_this(), shared_from_this().cast_to<InterfaceNode>(), involved_interfaces, true); e) {
		if (e->error_kind == CompilationErrorKind::CyclicInheritedInterface) {
			is_cyclic_inherited_flag = true;
			is_cyclic_inheritance_checked = true;
			if (!cyclic_inheritance_error.has_value()) {
				cyclic_inheritance_error = std::move(*e);
			}
			e.reset();

			return {};
		}
		return e;
	}

	is_cyclic_inherited_flag = false;
	is_cyclic_inheritance_checked = true;

	return {};
}

SLKC_API AstNodePtr<AstNode> StructNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<StructNode> duplicated_node(make_ast_node<StructNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API StructNode::StructNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::Struct),
	  impl_types(self_allocator),
	  generic_params(self_allocator),
	  generic_param_indices(self_allocator),
	  idx_generic_param_comma_tokens(self_allocator) {
}

SLKC_API StructNode::StructNode(const StructNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out), impl_types(allocator), generic_params(allocator), generic_param_indices(allocator), idx_generic_param_comma_tokens(allocator) {
	if (!succeeded_out) {
		return;
	}

	if (!impl_types.resize(rhs.impl_types.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < impl_types.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(impl_types.at(i) = rhs.impl_types.at(i)->do_duplicate(allocator, context).cast_to<TypeNameNode>()))
					return false;
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!generic_params.resize(rhs.generic_params.size())) {
		succeeded_out = false;
		return;
	}

	for (size_t i = 0; i < generic_params.size(); ++i) {
		if (!context.push_task([this, i, &rhs, allocator, &context]() -> bool {
				if (!(generic_params.at(i) = rhs.generic_params.at(i)->do_duplicate(allocator, context).cast_to<GenericParamNode>()))
					return false;

				generic_params.at(i)->set_parent(this);
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	for (const auto &[k, v] : rhs.generic_param_indices) {
		auto captured_v = v;
		if (!context.push_task([this, v = captured_v, &rhs, allocator, &context]() -> bool {
				if (!generic_param_indices.insert(generic_params.at(v)->name, +v)) {
					return false;
				}
				return true;
			})) {
			succeeded_out = false;
			return;
		}
	}

	if (!idx_generic_param_comma_tokens.resize(rhs.idx_generic_param_comma_tokens.size())) {
		succeeded_out = false;
		return;
	}

	memcpy(idx_generic_param_comma_tokens.data(), rhs.idx_generic_param_comma_tokens.data(), sizeof(size_t) * idx_generic_param_comma_tokens.size());

	idx_langle_bracket_token = rhs.idx_langle_bracket_token;
	idx_rangle_bracket_token = rhs.idx_rangle_bracket_token;

	is_generic_params_indexed = rhs.is_generic_params_indexed;

	succeeded_out = true;
}

SLKC_API StructNode::~StructNode() {
}

SLKC_API peff::Option<CompilationError> StructNode::is_recursed_type(bool &whether_out) {
	if (is_recursed_type_checked) {
		whether_out = is_recursed_type_flag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(update_recursed_type_status());

	whether_out = is_recursed_type_flag;
	return {};
}

SLKC_API peff::Option<CompilationError> StructNode::update_recursed_type_status() {
	is_recursed_type_flag = false;

	SLKC_RETURN_IF_COMP_ERROR(is_struct_recursed(document->shared_from_this(), shared_from_this().cast_to<StructNode>(), is_recursed_type_flag));

	is_recursed_type_checked = true;
	return {};
}

SLKC_API AstNodePtr<AstNode> ThisNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ThisNode> duplicated_node(make_ast_node<ThisNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ThisNode::ThisNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::This, self_allocator, document) {
}

SLKC_API ThisNode::ThisNode(const ThisNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	this_type = rhs.this_type;

	succeeded_out = true;
}

SLKC_API ThisNode::~ThisNode() {
}

struct CollectInvolvedInterfacesFrame {
	AstNodePtr<InterfaceNode> interface_node;
	size_t index;
};

struct CollectInvolvedInterfacesContext {
	peff::List<CollectInvolvedInterfacesFrame> frames;

	SLAKE_FORCEINLINE CollectInvolvedInterfacesContext(peff::Alloc *allocator) : frames(allocator) {}
};

static peff::Option<CompilationError> _collect_involved_interfaces(
	peff::SharedPtr<Document> document,
	CollectInvolvedInterfacesContext &context,
	AstNodePtr<InterfaceNode> interface_node,
	peff::Set<AstNodePtr<InterfaceNode>> &walked_interfaces) {
	if (!context.frames.push_back({ interface_node, 0 }))
		return gen_oom_comp_error();

	while (context.frames.size()) {
		CollectInvolvedInterfacesFrame &cur_frame = context.frames.back();

		const AstNodePtr<InterfaceNode> &cur_interface = cur_frame.interface_node;

		// Check if the interface has cyclic inheritance.
		if (!cur_frame.index) {
			for (auto &i : context.frames) {
				if ((&i != &cur_frame) && (i.interface_node == cur_frame.interface_node)) {
					auto source = context.frames.front();
					return CompilationError(source.interface_node->scope->impl_types.at(source.index - 1)->token_range, CompilationErrorKind::CyclicInheritedInterface);
				}
			}
		}
		if (cur_frame.index >= cur_interface->scope->impl_types.size()) {
			if (!walked_interfaces.insert(AstNodePtr<InterfaceNode>(cur_interface)))
				return gen_oom_comp_error();
			context.frames.pop_back();
			continue;
		}

		AstNodePtr<TypeNameNode> t = cur_interface->scope->impl_types.at(cur_frame.index);

		AstNodePtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, t.cast_to<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->get_ast_node_type() != AstNodeType::Interface) {
			goto malformed;
		}

		if (!context.frames.push_back({ m.cast_to<InterfaceNode>(), 0 }))
			return gen_oom_comp_error();

		++cur_frame.index;
	}

	return {};

malformed:
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::collect_involved_interfaces(
	peff::SharedPtr<Document> document,
	const AstNodePtr<InterfaceNode> &bottom,
	peff::Set<AstNodePtr<InterfaceNode>> &walked_interfaces,
	bool insert_self) {
	if (walked_interfaces.contains(bottom)) {
		return {};
	}
	if (insert_self) {
		if (!walked_interfaces.insert(AstNodePtr<InterfaceNode>(bottom))) {
			return gen_oom_comp_error();
		}
	}

	CollectInvolvedInterfacesContext context(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(_collect_involved_interfaces(document, context, bottom, walked_interfaces));

	return {};

malformed:
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::collect_involved_interfaces_phased_bfs(
	const peff::Set<AstNodePtr<InterfaceNode>> &interfaces_in,
	peff::Set<AstNodePtr<InterfaceNode>> &new_interfaces_out) {
	assert(&interfaces_in != const_cast<const peff::Set<AstNodePtr<InterfaceNode>> *>(&new_interfaces_out));
	for (auto i : interfaces_in) {
		if (!i->scope)
			continue;
		for (auto j : i->scope->impl_types) {
			AstNodePtr<InterfaceNode> m;
			SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(j, m, nullptr));
			if (!m)
				continue;
			if (!new_interfaces_out.insert(std::move(m)))
				return gen_oom_comp_error();
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::collect_inherited_members(
	peff::SharedPtr<Document> document,
	const AstNodePtr<ClassNode> &bottom,
	peff::Set<AstNodePtr<MemberNode>> &walked_members,
	bool insert_self) {
	auto i = bottom;

	while (i) {
		// Collect current class.
		if ((i != bottom) || (insert_self)) {
			if (!walked_members.insert(i.cast_to<MemberNode>()))
				return {};
		}

		// Collect each involved interface.
		for (auto j : i->scope->impl_types) {
			peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());
			AstNodePtr<InterfaceNode> interface_node;
			SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(j, interface_node, nullptr));
			SLKC_RETURN_IF_COMP_ERROR(collect_involved_interfaces(document, interface_node, interfaces, true));

			for (auto k : interfaces) {
				if (!walked_members.insert(k.cast_to<MemberNode>()))
					return {};
			}
		}

		SLKC_RETURN_IF_COMP_ERROR(visit_base_class(i->scope->base_type, i, nullptr));
	}

	return {};
}

struct IndexedStructRecursionCheckFrameExData {
	size_t index;
};

struct StructRecursionCheckFrame {
	AstNodePtr<MemberNode> struct_node;
	std::variant<IndexedStructRecursionCheckFrameExData> ex_data;
};

struct StructRecursionCheckContext {
	peff::List<StructRecursionCheckFrame> frames;

	SLAKE_FORCEINLINE StructRecursionCheckContext(peff::Alloc *allocator) : frames(allocator) {}
};

static peff::Option<CompilationError> _is_struct_recursed(
	peff::SharedPtr<Document> document,
	StructRecursionCheckContext &context,
	peff::Set<AstNodePtr<MemberNode>> &walked_structs,
	bool &whether_out) {
	whether_out = false;
	while (context.frames.size()) {
		StructRecursionCheckFrame &cur_frame = context.frames.back();

		if (!cur_frame.struct_node->scope) {
			walked_structs.remove(cur_frame.struct_node);
			context.frames.pop_back();
			continue;
		}

		switch (cur_frame.struct_node->get_ast_node_type()) {
			case AstNodeType::UnionEnum: {
				const AstNodePtr<UnionEnumNode> &cur_struct = cur_frame.struct_node.cast_to<UnionEnumNode>();
				IndexedStructRecursionCheckFrameExData &ex_data = std::get<IndexedStructRecursionCheckFrameExData>(cur_frame.ex_data);

				if (!ex_data.index) {
					if (walked_structs.contains(cur_struct.cast_to<MemberNode>())) {
						whether_out = true;
						return {};
					}
					if (!walked_structs.insert(cur_struct.cast_to<MemberNode>()))
						return gen_oom_comp_error();
				}
				if (ex_data.index >= cur_struct->scope->get_member_num()) {
					walked_structs.remove(cur_struct.cast_to<MemberNode>());
					context.frames.pop_back();
					continue;
				}

				AstNodePtr<MemberNode> v = cur_struct->scope->get_member(ex_data.index);

				if (v->get_ast_node_type() == AstNodeType::UnionEnumItem) {
					if (!context.frames.push_back(StructRecursionCheckFrame{ v.cast_to<MemberNode>(), IndexedStructRecursionCheckFrameExData{ 0 } }))
						return gen_oom_comp_error();
				}

				++ex_data.index;
				break;
			}
			case AstNodeType::Class:
			case AstNodeType::Interface:
				walked_structs.remove(cur_frame.struct_node);
				context.frames.pop_back();
				continue;
			default: {
				const AstNodePtr<MemberNode> &cur_struct = cur_frame.struct_node;
				IndexedStructRecursionCheckFrameExData &ex_data = std::get<IndexedStructRecursionCheckFrameExData>(cur_frame.ex_data);

				if (!ex_data.index) {
					if (walked_structs.contains(cur_struct)) {
						whether_out = true;
						return {};
					}
					if (!walked_structs.insert(AstNodePtr<MemberNode>(cur_struct)))
						return gen_oom_comp_error();
				}
				if (ex_data.index >= cur_struct->scope->get_member_num()) {
					walked_structs.remove(cur_struct);
					context.frames.pop_back();
					continue;
				}

				AstNodePtr<MemberNode> v = cur_struct->scope->get_member(ex_data.index);

				if (v->get_ast_node_type() == AstNodeType::Var) {
					AstNodePtr<VarNode> var_member = v.cast_to<VarNode>();

					AstNodePtr<MemberNode> m;

					if (auto t = var_member->type; t->tn_kind == TypeNameKind::Custom) {
						SLKC_RETURN_IF_COMP_ERROR(is_higher_ranked_recursed(cur_struct, t.cast_to<CustomTypeNameNode>(), document->allocator.get(), whether_out));
						if (whether_out)
							return {};
						SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, t.cast_to<CustomTypeNameNode>(), m));

						if (m) {
							if (!context.frames.push_back(StructRecursionCheckFrame{ m.cast_to<MemberNode>(), IndexedStructRecursionCheckFrameExData{ 0 } }))
								return gen_oom_comp_error();
						}
					}
				}

				++ex_data.index;
				break;
			}
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_struct_recursed(
	peff::SharedPtr<Document> document,
	const AstNodePtr<StructNode> &derived,
	bool &whether_out) {
	StructRecursionCheckContext context(document->allocator.get());
	peff::Set<AstNodePtr<MemberNode>> walked_structs(document->allocator.get()), walked_higher_ranked_structs(document->allocator.get());

	if (!context.frames.push_back(StructRecursionCheckFrame{ derived.cast_to<MemberNode>(), IndexedStructRecursionCheckFrameExData{ 0 } }))
		return gen_oom_comp_error();

	return _is_struct_recursed(document, context, walked_structs, whether_out);
}

SLKC_API peff::Option<CompilationError> slkc::is_union_enum_recursed(
	peff::SharedPtr<Document> document,
	const AstNodePtr<UnionEnumNode> &derived,
	bool &whether_out) {
	StructRecursionCheckContext context(document->allocator.get());
	peff::Set<AstNodePtr<MemberNode>> walked_structs(document->allocator.get());

	if (!context.frames.push_back(StructRecursionCheckFrame{ derived.cast_to<MemberNode>(), IndexedStructRecursionCheckFrameExData{ 0 } }))
		return gen_oom_comp_error();

	return _is_struct_recursed(document, context, walked_structs, whether_out);
}

SLKC_API peff::Option<CompilationError> slkc::is_implemented_by_interface(
	peff::SharedPtr<Document> document,
	const AstNodePtr<InterfaceNode> &base,
	const AstNodePtr<InterfaceNode> &derived,
	bool &whether_out) {
	peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(collect_involved_interfaces(document, derived, interfaces, true));

	whether_out = interfaces.contains(base);
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_implemented_by_class(
	peff::SharedPtr<Document> document,
	const AstNodePtr<InterfaceNode> &base,
	const AstNodePtr<ClassNode> &derived,
	bool &whether_out) {
	peff::Set<AstNodePtr<ClassNode>> walked_classes(document->allocator.get());

	if (!walked_classes.insert(AstNodePtr<ClassNode>(derived))) {
		return gen_oom_comp_error();
	}

	AstNodePtr<ClassNode> current_class = derived;
	AstNodePtr<TypeNameNode> current_type = derived->scope->base_type;

	while (current_type) {
		if (current_type->tn_kind != TypeNameKind::Custom) {
			goto malformed;
		}

		AstNodePtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, current_type.cast_to<CustomTypeNameNode>(), m));

		if (m->get_ast_node_type() != AstNodeType::Class) {
			goto malformed;
		}

		current_class = m.cast_to<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walked_classes.contains(current_class)) {
			whether_out = true;
			return {};
		}

		for (size_t i = 0; i < current_class->scope->impl_types.size(); ++i) {
			AstNodePtr<TypeNameNode> t = derived->scope->impl_types.at(i);

			AstNodePtr<MemberNode> m;
			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, t.cast_to<CustomTypeNameNode>(), m));

			if (!m) {
				goto malformed;
			}

			if (m->get_ast_node_type() != AstNodeType::Interface) {
				goto malformed;
			}

			AstNodePtr<InterfaceNode> interface_node = m.cast_to<InterfaceNode>();

			if (interface_node == base) {
				whether_out = true;
				return {};
			}

			SLKC_RETURN_IF_COMP_ERROR(is_implemented_by_interface(document, base, interface_node, whether_out));

			if (whether_out) {
				whether_out = true;
				return {};
			}
		}

		if (!walked_classes.insert(AstNodePtr<ClassNode>(current_class))) {
			return gen_oom_comp_error();
		}
	}

	whether_out = false;
	return {};

malformed:
	whether_out = false;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_base_of(
	peff::SharedPtr<Document> document,
	const AstNodePtr<ClassNode> &base,
	const AstNodePtr<ClassNode> &derived,
	bool &whether_out) {
	peff::Set<AstNodePtr<ClassNode>> walked_classes(document->allocator.get());

	if (!walked_classes.insert(AstNodePtr<ClassNode>(derived))) {
		return gen_oom_comp_error();
	}

	AstNodePtr<ClassNode> current_class = derived;
	AstNodePtr<TypeNameNode> current_type;

	while ((current_type = current_class->scope->base_type)) {
		if (current_type->tn_kind != TypeNameKind::Custom) {
			goto malformed;
		}

		AstNodePtr<MemberNode> m;
		SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, current_type.cast_to<CustomTypeNameNode>(), m));

		if (!m) {
			goto malformed;
		}

		if (m->get_ast_node_type() != AstNodeType::Class) {
			goto malformed;
		}

		current_class = m.cast_to<ClassNode>();

		// Make sure that the function will work properly when the class has cyclic inheritance.
		if (walked_classes.contains(current_class)) {
			whether_out = true;
			return {};
		}

		if (current_class == base) {
			whether_out = true;
			return {};
		}

		if (!walked_classes.insert(AstNodePtr<ClassNode>(current_class))) {
			return gen_oom_comp_error();
		}
	}

	whether_out = false;
	return {};

malformed:
	whether_out = false;
	return {};
}

SLKC_API AstNodePtr<AstNode> EnumItemNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<EnumItemNode> duplicated_node(make_ast_node<EnumItemNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API EnumItemNode::EnumItemNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: MemberNode(AstNodeType::EnumItem, self_allocator, document) {
}

SLKC_API EnumItemNode::EnumItemNode(const EnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : MemberNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.enum_value && !(enum_value = rhs.enum_value->do_duplicate(allocator, context).cast_to<ExprNode>())) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	// No need to copy filled value.

	succeeded_out = true;
}

SLKC_API EnumItemNode::~EnumItemNode() {
}

SLKC_API AstNodePtr<AstNode> ConstEnumNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ConstEnumNode> duplicated_node(make_ast_node<ConstEnumNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ConstEnumNode::ConstEnumNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::ConstEnum) {
}

SLKC_API ConstEnumNode::ConstEnumNode(const ConstEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.underlying_type && !(underlying_type = rhs.underlying_type->do_duplicate(allocator, context).cast_to<TypeNameNode>())) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ConstEnumNode::~ConstEnumNode() {
}

SLKC_API AstNodePtr<AstNode> ScopedEnumNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<ScopedEnumNode> duplicated_node(make_ast_node<ScopedEnumNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API ScopedEnumNode::ScopedEnumNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::ScopedEnum) {
}

SLKC_API ScopedEnumNode::ScopedEnumNode(const ScopedEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}

	if (!context.push_task([this, &rhs, allocator, &context]() -> bool {
			if (rhs.underlying_type && !(underlying_type = rhs.underlying_type->do_duplicate(allocator, context).cast_to<TypeNameNode>())) {
				return false;
			}
			return true;
		})) {
		succeeded_out = false;
		return;
	}

	succeeded_out = true;
}

SLKC_API ScopedEnumNode::~ScopedEnumNode() {
}

SLKC_API AstNodePtr<AstNode> UnionEnumItemNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnionEnumItemNode> duplicated_node(make_ast_node<UnionEnumItemNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API UnionEnumItemNode::UnionEnumItemNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::UnionEnumItem) {
}

SLKC_API UnionEnumItemNode::UnionEnumItemNode(const UnionEnumItemNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out) {
	if (!succeeded_out) {
		return;
	}
}

SLKC_API UnionEnumItemNode::~UnionEnumItemNode() {
}

SLKC_API AstNodePtr<AstNode> UnionEnumNode::do_duplicate(peff::Alloc *new_allocator, DuplicationContext &context) const {
	bool succeeded = false;
	AstNodePtr<UnionEnumNode> duplicated_node(make_ast_node<UnionEnumNode>(new_allocator, *this, new_allocator, context, succeeded));
	if ((!duplicated_node) || (!succeeded)) {
		return {};
	}

	return duplicated_node.cast_to<AstNode>();
}

SLKC_API UnionEnumNode::UnionEnumNode(
	peff::Alloc *self_allocator,
	const peff::SharedPtr<Document> &document)
	: ModuleNode(self_allocator, document, AstNodeType::UnionEnum),
	  idx_generic_param_comma_tokens(self_allocator) {
}

SLKC_API UnionEnumNode::UnionEnumNode(const UnionEnumNode &rhs, peff::Alloc *allocator, DuplicationContext &context, bool &succeeded_out) : ModuleNode(rhs, allocator, context, succeeded_out), idx_generic_param_comma_tokens(allocator) {
	if (!succeeded_out) {
		return;
	}

	if (!idx_generic_param_comma_tokens.resize(rhs.idx_generic_param_comma_tokens.size())) {
		succeeded_out = false;
		return;
	}

	memcpy(idx_generic_param_comma_tokens.data(), rhs.idx_generic_param_comma_tokens.data(), sizeof(size_t) * idx_generic_param_comma_tokens.size());

	idx_langle_bracket_token = rhs.idx_langle_bracket_token;
	idx_rangle_bracket_token = rhs.idx_rangle_bracket_token;

	is_generic_params_indexed = rhs.is_generic_params_indexed;

	succeeded_out = true;
}

SLKC_API UnionEnumNode::~UnionEnumNode() {
}

SLKC_API peff::Option<CompilationError> UnionEnumNode::is_recursed_type(bool &whether_out) {
	if (is_recursed_type_checked) {
		whether_out = is_recursed_type_flag;
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(update_recursed_type_status());

	whether_out = is_recursed_type_flag;

	return {};
}

SLKC_API peff::Option<CompilationError> UnionEnumNode::update_recursed_type_status() {
	return {};
}
