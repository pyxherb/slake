#include "../compiler.h"

using namespace slkc;

SLKC_API peff::Option<CompilationError> slkc::get_full_id_ref(peff::Alloc *allocator, AstNodePtr<MemberNode> m, IdRefPtr &id_ref_out) {
	IdRefPtr p(peff::alloc_and_construct<IdRef>(allocator, ASTNODE_ALIGNMENT, allocator));

	for (;;) {
		IdRefEntry entry(allocator);

		if (!m->name.size()) {
			break;
		}

		if (!entry.name.build(m->name)) {
			return gen_out_of_memory_comp_error();
		}

		if (!entry.generic_args.resize(m->generic_args.size())) {
			return gen_out_of_memory_comp_error();
		}

		for (size_t i = 0; i < entry.generic_args.size(); ++i) {
			if (!(entry.generic_args.at(i) = m->generic_args.at(i)->duplicate<AstNode>(allocator))) {
				return gen_out_of_memory_comp_error();
			}
		}

		if (!p->entries.push_front(std::move(entry))) {
			return gen_out_of_memory_comp_error();
		}

		switch (m->get_ast_node_type()) {
			case AstNodeType::FnOverloading:
				if (!m->parent->parent)
					goto end;
				m = m->parent->parent->shared_from_this().cast_to<MemberNode>();
				break;
			default:
				if (!m->parent)
					goto end;
				m = m->parent->shared_from_this().cast_to<MemberNode>();
		}
	}

end:
	id_ref_out = std::move(p);

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::resolve_static_member(
	CompileEnv *compile_env,
	peff::SharedPtr<Document> document,
	const AstNodePtr<MemberNode> &member_node,
	const IdRefEntry &name,
	AstNodePtr<MemberNode> &member_out) {
	AstNodePtr<MemberNode> result;

	switch (member_node->get_ast_node_type()) {
		case AstNodeType::Module: {
			AstNodePtr<ModuleNode> mod = member_node.cast_to<ModuleNode>();

			if (auto it = mod->member_indices.find(name.name); it != mod->member_indices.end()) {
				result = mod->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Class: {
			AstNodePtr<ClassNode> cls = member_node.cast_to<ClassNode>();

			if (auto it = cls->member_indices.find(name.name); it != cls->member_indices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Interface: {
			AstNodePtr<InterfaceNode> cls = member_node.cast_to<InterfaceNode>();

			if (auto it = cls->member_indices.find(name.name); it != cls->member_indices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Struct: {
			AstNodePtr<StructNode> cls = member_node.cast_to<StructNode>();

			if (auto it = cls->member_indices.find(name.name); it != cls->member_indices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		case AstNodeType::UnionEnum: {
			AstNodePtr<UnionEnumNode> cls = member_node.cast_to<UnionEnumNode>();

			if (auto it = cls->member_indices.find(name.name); it != cls->member_indices.end()) {
				result = cls->members.at(it.value());
			}

			break;
		}
		default:
			result = {};
	}

	if (result) {
		if (name.generic_args.size()) {
			SLKC_RETURN_IF_COMP_ERROR(document->instantiate_generic_object(result, name.generic_args, result));
		}

		switch (result->get_ast_node_type()) {
			case AstNodeType::Var: {
				AstNodePtr<VarNode> m = result.cast_to<VarNode>();

				// Check if the variable member is static.
				if (!(m->access_modifier & slake::ACCESS_STATIC)) {
					member_out = {};
					return {};
				}
				break;
			}
			case AstNodeType::Fn: {
				AstNodePtr<FnNode> m = result.cast_to<FnNode>();

				// Check if the slot contains any static method.
				for (auto i : m->overloadings) {
					if (i->access_modifier & slake::ACCESS_STATIC)
						goto pass;
				}

				member_out = {};
				return {};
			pass:
				break;
			}
			default:
				break;
		}
		member_out = result;
		return {};
	}

	member_out = {};
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::resolve_instance_member(
	CompileEnv *compile_env,
	peff::SharedPtr<Document> document,
	AstNodePtr<MemberNode> member_node,
	const IdRefEntry &name,
	AstNodePtr<MemberNode> &member_out) {
	AstNodePtr<MemberNode> result;

	switch (member_node->get_ast_node_type()) {
		case AstNodeType::Module: {
			AstNodePtr<ModuleNode> mod = member_node.cast_to<ModuleNode>();

			if (auto it = mod->member_indices.find(name.name); it != mod->member_indices.end()) {
				result = mod->members.at(it.value());
			}

			break;
		}
		case AstNodeType::Class: {
			AstNodePtr<ClassNode> m = member_node.cast_to<ClassNode>();

			if (auto it = m->member_indices.find(name.name); it != m->member_indices.end()) {
				result = m->members.at(it.value());
			} else {
				{
					AstNodePtr<ClassNode> base_type;
					SLKC_RETURN_IF_COMP_ERROR(visit_base_class(m->base_type, base_type, nullptr));
					if (base_type) {
						SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, base_type.cast_to<MemberNode>(), name, result));

						if (result) {
							goto class_resolution_succeeded;
						}
					}
				}

				for (auto &i : m->impl_types) {
					{
						AstNodePtr<InterfaceNode> base_type;
						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, nullptr));
						if (base_type) {
							SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, base_type.cast_to<MemberNode>(), name, result));

							if (result) {
								goto class_resolution_succeeded;
							}
						}
					}
				}
			}

		class_resolution_succeeded:
			break;
		}
		case AstNodeType::Interface: {
			AstNodePtr<InterfaceNode> m = member_node.cast_to<InterfaceNode>();

			if (auto it = m->member_indices.find(name.name); it != m->member_indices.end()) {
				result = m->members.at(it.value());
			} else {
				for (auto &i : m->impl_types) {
					{
						AstNodePtr<InterfaceNode> base_type;
						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, nullptr));
						if (base_type) {
							SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, base_type.cast_to<MemberNode>(), name, result));

							if (result) {
								goto interface_resolution_succeeded;
							}
						}
					}
				}
			}

		interface_resolution_succeeded:
			break;
		}
		case AstNodeType::Struct: {
			AstNodePtr<StructNode> m = member_node.cast_to<StructNode>();

			if (auto it = m->member_indices.find(name.name); it != m->member_indices.end()) {
				result = m->members.at(it.value());
			}

			break;
		}
		case AstNodeType::UnionEnumItem: {
			AstNodePtr<UnionEnumItemNode> m = member_node.cast_to<UnionEnumItemNode>();

			if (auto it = m->member_indices.find(name.name); it != m->member_indices.end()) {
				result = m->members.at(it.value());
			}

			break;
		}
		case AstNodeType::UnionEnum: {
			AstNodePtr<UnionEnumNode> m = member_node.cast_to<UnionEnumNode>();

			if (auto it = m->member_indices.find(name.name); it != m->member_indices.end()) {
				result = m->members.at(it.value());
			}

			break;
		}
		case AstNodeType::GenericParam: {
			AstNodePtr<GenericParamNode> m = member_node.cast_to<GenericParamNode>();

			{
				AstNodePtr<ClassNode> base_type;
				SLKC_RETURN_IF_COMP_ERROR(visit_base_class(m->generic_constraint->base_type, base_type, nullptr));
				if (base_type) {
					SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, base_type.cast_to<MemberNode>(), name, result));

					if (result) {
						goto generic_param_resolution_succeeded;
					}
				}
			}

			for (auto &i : m->generic_constraint->impl_types) {
				{
					AstNodePtr<InterfaceNode> base_type;
					SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, nullptr));
					if (base_type) {
						SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, base_type.cast_to<MemberNode>(), name, result));

						if (result) {
							goto generic_param_resolution_succeeded;
						}
					}
				}
			}

		generic_param_resolution_succeeded:
			break;
		}
		case AstNodeType::This: {
			AstNodePtr<ThisNode> cls = member_node.cast_to<ThisNode>();

			SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, cls->document->shared_from_this(), cls->this_type, name, result));

			break;
		}
		case AstNodeType::Var: {
			AstNodePtr<VarNode> m = member_node.cast_to<VarNode>();

			if (m->type->type_name_kind != TypeNameKind::Custom) {
				result = {};
				break;
			}

			AstNodePtr<TypeNameNode> type;
			SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(m->type, type));

			AstNodePtr<MemberNode> tm;
			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(compile_env, document, type.cast_to<CustomTypeNameNode>(), tm));

			if (!tm) {
				result = {};
				break;
			}

			SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, tm, name, result));

			break;
		}
		default:
			result = {};
	}

	if (result) {
		if (name.generic_args.size()) {
			SLKC_RETURN_IF_COMP_ERROR(document->instantiate_generic_object(result, name.generic_args, result));
		}

		switch (result->get_ast_node_type()) {
			case AstNodeType::Var: {
				AstNodePtr<VarNode> m = result.cast_to<VarNode>();

				// Check if the variable member is static or not.
				if (m->access_modifier & slake::ACCESS_STATIC) {
					return {};
				}
				break;
			}
			case AstNodeType::Fn: {
				AstNodePtr<FnNode> m = result.cast_to<FnNode>();

				// Check if the slot contains any instance method.
				for (auto i : m->overloadings) {
					if (!(i->access_modifier & slake::ACCESS_STATIC))
						goto pass;
				}

				member_out = {};
				return {};
			pass:
				break;
			}
			default:
				break;
		}
		member_out = result;
		return {};
	}

	member_out = {};
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::is_member_accessible(
	CompileEnv *compile_env,
	AstNodePtr<MemberNode> parent,
	AstNodePtr<MemberNode> member,
	bool &result_out) {
	if (member->is_public())
		goto access_check_passed;
	if (compile_env->cur_parent_access_node) {
		auto p = member->parent->shared_from_this().cast_to<MemberNode>();

		if (p->get_ast_node_type() == AstNodeType::Fn)
			p = p->parent->shared_from_this().cast_to<MemberNode>();

		auto access_node = compile_env->cur_parent_access_node.cast_to<MemberNode>();

		switch (p->get_ast_node_type()) {
			case AstNodeType::Class:
				if (access_node->get_ast_node_type() == AstNodeType::Class) {
					bool result;

					SLKC_RETURN_IF_COMP_ERROR(is_base_of(compile_env->document, access_node.cast_to<ClassNode>(), p->shared_from_this().cast_to<ClassNode>(), result));

					if (result) {
						// Child classes can always access parent's members.
						goto access_check_passed;
					}
				}
				for (AstNodePtr<MemberNode> j = p; j; j = j->parent ? j->parent->shared_from_this().cast_to<MemberNode>() : AstNodePtr<MemberNode>{}) {
					if (access_node->get_ast_node_type() == AstNodeType::Module) {
						switch (j->get_ast_node_type()) {
							case AstNodeType::Class:
							case AstNodeType::Interface:
							case AstNodeType::Struct:
								break;
							default:
								goto not_internal;
						}
					}
					if (j == access_node)
						// Outer class is always accessible to internal classes' members.
						goto access_check_passed;
				}
				break;
			case AstNodeType::Module:
				// Members in the same module can access each other.
				if (p == compile_env->cur_parent_access_node.cast_to<MemberNode>())
					goto access_check_passed;
				break;
		}

	not_internal:;
	}
	// TODO: Check if this is a friend.
	result_out = false;
	return {};
access_check_passed:
	result_out = true;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::resolve_id_ref(
	CompileEnv *compile_env,
	peff::SharedPtr<Document> document,
	const AstNodePtr<MemberNode> &resolve_root,
	IdRefEntry *id_ref,
	size_t num_entries,
	AstNodePtr<MemberNode> &member_out,
	ResolvedIdRefPartList *resolved_part_list_out,
	bool is_static) {
	AstNodePtr<MemberNode> cur_member = resolve_root;

	bool is_post_static_parts = !is_static;

	auto update_static_status = [&cur_member, &is_static]() {
		switch (cur_member->get_ast_node_type()) {
			case AstNodeType::Var:
				is_static = false;
				break;
			default:
				break;
		}
	};

	update_static_status();

	for (size_t i = 0; i < num_entries; ++i) {
		const IdRefEntry &cur_entry = id_ref[i];

		AstNodePtr<MemberNode> parent = cur_member;
		if (is_static) {
			SLKC_RETURN_IF_COMP_ERROR(resolve_static_member(compile_env, document, parent, cur_entry, cur_member));
		} else {
			SLKC_RETURN_IF_COMP_ERROR(resolve_instance_member(compile_env, document, parent, cur_entry, cur_member));
		}

		if (!cur_member) {
			member_out = {};
			if (resolved_part_list_out) {
				resolved_part_list_out->clear_and_shrink();
			}
			return {};
		}

		if (compile_env) {
			if (cur_member->get_ast_node_type() == AstNodeType::Module)
				goto access_check_passed;
			if (cur_member->get_ast_node_type() == AstNodeType::Fn)
				goto access_check_passed;
			bool accessible;
			SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, parent, cur_member, accessible));
			if (!accessible)
				return CompilationError(
					TokenRange{ document->main_module, cur_entry.name_token_index },
					CompilationErrorKind::MemberIsNotAccessible);
		}
	access_check_passed:
		update_static_status();

		// We assume that all parts after the static parts are in instance.
		if (resolved_part_list_out) {
			if (!is_static) {
				if (!is_post_static_parts) {
					ResolvedIdRefPart part = { is_static, i, cur_member };

					assert(part.num_entries);

					if (!resolved_part_list_out->push_back(std::move(part)))
						return gen_out_of_memory_comp_error();

					is_post_static_parts = true;
				} else {
					ResolvedIdRefPart part = { is_static, 1, cur_member };

					if (!resolved_part_list_out->push_back(std::move(part)))
						return gen_out_of_memory_comp_error();
				}
			}
		}
	}

	if (resolved_part_list_out) {
		if (is_static) {
			ResolvedIdRefPart part = { is_static, num_entries, cur_member };

			if (!resolved_part_list_out->push_back(std::move(part)))
				return gen_out_of_memory_comp_error();
		}
	}

	member_out = cur_member;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::resolve_id_ref_with_scope_node(
	CompileEnv *compile_env,
	peff::SharedPtr<Document> document,
	peff::Set<AstNodePtr<MemberNode>> &walked_nodes,
	const AstNodePtr<MemberNode> &resolve_scope,
	IdRefEntry *id_ref,
	size_t num_entries,
	AstNodePtr<MemberNode> &member_out,
	ResolvedIdRefPartList *resolved_part_list_out,
	bool is_static,
	bool is_sealed) {
	if (walked_nodes.contains(resolve_scope)) {
		member_out = {};
		return {};
	}

	if ((num_entries == 1) && (!is_sealed)) {
		const IdRefEntry &initial_entry = id_ref[0];

		if (!initial_entry.generic_args.size()) {
			AstNodePtr<MemberNode> cur_scope = resolve_scope;

		reresolve_with_new_scope:
			switch (cur_scope->get_ast_node_type()) {
				case AstNodeType::Class: {
					AstNodePtr<ClassNode> m = cur_scope.cast_to<ClassNode>();

					if (auto it = m->generic_param_indices.find(initial_entry.name); it != m->generic_param_indices.end()) {
						member_out = m->generic_params.at(it.value()).cast_to<MemberNode>();
						return {};
					}
					if (m->parent) {
						cur_scope = m->parent->shared_from_this().cast_to<MemberNode>();
						goto reresolve_with_new_scope;
					}
					break;
				}
				case AstNodeType::Interface: {
					AstNodePtr<InterfaceNode> m = cur_scope.cast_to<InterfaceNode>();

					if (auto it = m->generic_param_indices.find(initial_entry.name); it != m->generic_param_indices.end()) {
						member_out = m->generic_params.at(it.value()).cast_to<MemberNode>();
						return {};
					}
					if (m->parent) {
						cur_scope = m->parent->shared_from_this().cast_to<MemberNode>();
						goto reresolve_with_new_scope;
					}
					break;
				}
				case AstNodeType::Struct: {
					AstNodePtr<StructNode> m = cur_scope.cast_to<StructNode>();

					if (auto it = m->generic_param_indices.find(initial_entry.name); it != m->generic_param_indices.end()) {
						member_out = m->generic_params.at(it.value()).cast_to<MemberNode>();
						return {};
					}
					if (m->parent) {
						cur_scope = m->parent->shared_from_this().cast_to<MemberNode>();
						goto reresolve_with_new_scope;
					}
					break;
				}
				case AstNodeType::UnionEnum: {
					AstNodePtr<UnionEnumNode> m = cur_scope.cast_to<UnionEnumNode>();

					if (auto it = m->generic_param_indices.find(initial_entry.name); it != m->generic_param_indices.end()) {
						member_out = m->generic_params.at(it.value()).cast_to<MemberNode>();
						return {};
					}
					if (m->parent) {
						cur_scope = m->parent->shared_from_this().cast_to<MemberNode>();
						goto reresolve_with_new_scope;
					}
					break;
				}
				case AstNodeType::FnOverloading: {
					AstNodePtr<FnOverloadingNode> m = cur_scope.cast_to<FnOverloadingNode>();

					if (auto it = m->generic_param_indices.find(initial_entry.name); it != m->generic_param_indices.end()) {
						member_out = m->generic_params.at(it.value()).cast_to<MemberNode>();
						return {};
					}

					cur_scope = m->parent->parent->shared_from_this().cast_to<MemberNode>();
					goto reresolve_with_new_scope;
				}
				default:;
			}
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref(compile_env, document, resolve_scope, id_ref, num_entries, member_out, resolved_part_list_out, is_static));

	if (!member_out) {
		switch (resolve_scope->get_ast_node_type()) {
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> m = resolve_scope.cast_to<ClassNode>();

				if (!walked_nodes.insert(m.cast_to<MemberNode>())) {
					return gen_out_of_memory_comp_error();
				}

				{
					AstNodePtr<ClassNode> base_type;
					SLKC_RETURN_IF_COMP_ERROR(visit_base_class(m->base_type, base_type, &walked_nodes));
					if (base_type) {
						SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, base_type.cast_to<MemberNode>(), id_ref, num_entries, member_out, resolved_part_list_out, is_static, true));

						if (member_out) {
							return {};
						}
					}
				}
				walked_nodes.clear();

				for (auto &i : m->impl_types) {
					if (!walked_nodes.insert(m.cast_to<MemberNode>())) {
						return gen_out_of_memory_comp_error();
					}
					{
						AstNodePtr<InterfaceNode> base_type;
						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, &walked_nodes));
						if (base_type) {
							SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, base_type.cast_to<MemberNode>(), id_ref, num_entries, member_out, resolved_part_list_out, is_static, true));

							if (member_out) {
								return {};
							}
						}
					}
					walked_nodes.clear();
				}

				if (m->parent && (!is_sealed)) {
					AstNodePtr<MemberNode> p = m->parent->shared_from_this().cast_to<MemberNode>();

					switch (p->get_ast_node_type()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
						case AstNodeType::Module:
							SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, p.cast_to<MemberNode>(), id_ref, num_entries, member_out, resolved_part_list_out, is_static));

							if (member_out) {
								return {};
							}
							break;
					}
				}
				break;
			}
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> m = resolve_scope.cast_to<InterfaceNode>();

				if (!walked_nodes.insert(m.cast_to<MemberNode>())) {
					return gen_out_of_memory_comp_error();
				}

				for (auto &i : m->impl_types) {
					if (!walked_nodes.insert(m.cast_to<MemberNode>())) {
						return gen_out_of_memory_comp_error();
					}
					{
						AstNodePtr<InterfaceNode> base_type;
						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, &walked_nodes));
						if (base_type) {
							SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, base_type.cast_to<MemberNode>(), id_ref, num_entries, member_out, resolved_part_list_out, is_static, true));

							if (member_out) {
								return {};
							}
						}
					}
					walked_nodes.clear();
				}

				if (m->parent && (!is_sealed)) {
					AstNodePtr<MemberNode> p = m->parent->shared_from_this().cast_to<MemberNode>();

					switch (p->get_ast_node_type()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
						case AstNodeType::Module:
							SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, p, id_ref, num_entries, member_out, resolved_part_list_out, is_static));

							if (member_out) {
								return {};
							}
							break;
					}
				}
				break;
			}
			case AstNodeType::Struct:
			case AstNodeType::UnionEnum:
			case AstNodeType::UnionEnumItem:
			case AstNodeType::Module: {
				AstNodePtr<ModuleNode> m = resolve_scope.cast_to<ModuleNode>();

				if (!walked_nodes.insert(m.cast_to<MemberNode>())) {
					return gen_out_of_memory_comp_error();
				}

				if (m->parent && (!is_sealed)) {
					AstNodePtr<MemberNode> p = m->parent->shared_from_this().cast_to<MemberNode>();

					switch (p->get_ast_node_type()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
						case AstNodeType::Module:
							SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, p, id_ref, num_entries, member_out, resolved_part_list_out, is_static));

							if (member_out) {
								return {};
							}
							break;
					}
				}
				break;
			}
			case AstNodeType::FnOverloading: {
				AstNodePtr<FnOverloadingNode> m = resolve_scope.cast_to<FnOverloadingNode>();

				if (!walked_nodes.insert(m.cast_to<MemberNode>())) {
					return gen_out_of_memory_comp_error();
				}

				if (!m->parent)
					std::terminate();

				AstNodePtr<FnNode> slot;
				{
					AstNodePtr<MemberNode> p = m->parent->shared_from_this().cast_to<MemberNode>();

					if (p->get_ast_node_type() != AstNodeType::Fn)
						std::terminate();
					slot = p.cast_to<FnNode>();
				}

				if (slot->parent) {
					SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(compile_env, document, walked_nodes, slot->parent->shared_from_this().cast_to<MemberNode>(), id_ref, num_entries, member_out, resolved_part_list_out, is_static, false));
				}

				if (member_out) {
					return {};
				}
				break;
			}
			default:
				break;
		}
	} else {
		return {};
	}

	member_out = {};
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::resolve_custom_type_name(
	CompileEnv *compile_env,
	peff::SharedPtr<Document> document,
	const AstNodePtr<CustomTypeNameNode> &type_name,
	AstNodePtr<MemberNode> &member_node_out,
	peff::Set<AstNodePtr<MemberNode>> *walked_nodes) {
	AstNodePtr<MemberNode> member;

	if (type_name->cached_resolve_result.is_valid()) {
		member = type_name->cached_resolve_result.lock();
		goto resolved;
	}

	if (walked_nodes) {
		SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(nullptr, document, *walked_nodes, type_name->context_node.lock(), type_name->id_ref_ptr->entries.data(), type_name->id_ref_ptr->entries.size(), member, nullptr));
	} else {
		peff::Set<AstNodePtr<MemberNode>> my_walked_nodes(document->allocator.get());
		SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref_with_scope_node(nullptr, document, my_walked_nodes, type_name->context_node.lock(), type_name->id_ref_ptr->entries.data(), type_name->id_ref_ptr->entries.size(), member, nullptr));
	}

	if (member) {
		goto resolved;
	}

resolved:
	if (member) {
		type_name->cached_resolve_result = to_weak_ptr(member);

		switch (member->get_ast_node_type()) {
			case AstNodeType::Class:
			case AstNodeType::Interface:
			case AstNodeType::Struct:
			case AstNodeType::GenericParam:
			case AstNodeType::ScopedEnum:
			case AstNodeType::UnionEnum:
			case AstNodeType::UnionEnumItem:
				member_node_out = member;
				return {};
			default:;
		}

		member_node_out = {};
		return {};
	}

	member_node_out = {};
	return {};
}

[[nodiscard]] SLKC_API
	peff::Option<CompilationError>
	slkc::resolve_base_overriden_custom_type_name(
		peff::SharedPtr<Document> document,
		const AstNodePtr<CustomTypeNameNode> &type_name,
		AstNodePtr<TypeNameNode> &type_name_out) {
	AstNodePtr<MemberNode> member;

	SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, document, type_name, member, nullptr));

	if (!member) {
		type_name_out = {};
		return {};
	}

	switch (member->get_ast_node_type()) {
		case AstNodeType::GenericParam: {
			auto gp = member.cast_to<GenericParamNode>();

			if (gp->generic_constraint) {
				auto &c = gp->generic_constraint;

				if (c->base_type) {
					auto bt = c->base_type;

					switch (bt->type_name_kind) {
						case TypeNameKind::I8:
						case TypeNameKind::I16:
						case TypeNameKind::I32:
						case TypeNameKind::I64:
						case TypeNameKind::ISize:
						case TypeNameKind::U8:
						case TypeNameKind::U16:
						case TypeNameKind::U32:
						case TypeNameKind::U64:
						case TypeNameKind::USize:
						case TypeNameKind::F32:
						case TypeNameKind::F64:
						case TypeNameKind::String:
						case TypeNameKind::Bool:
						case TypeNameKind::Any:
						case TypeNameKind::Unpacking:
						case TypeNameKind::Fn:
						case TypeNameKind::Array:
							type_name_out = bt;
							break;
					}
				}
			}
		}
	}

	type_name_out = type_name.cast_to<TypeNameNode>();

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::visit_base_class(AstNodePtr<TypeNameNode> cls, AstNodePtr<ClassNode> &class_out, peff::Set<AstNodePtr<MemberNode>> *walked_nodes) {
	if (cls && (cls->type_name_kind == TypeNameKind::Custom)) {
		AstNodePtr<MemberNode> base_type;

		SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, cls->document->shared_from_this(), cls.cast_to<CustomTypeNameNode>(), base_type, walked_nodes));

		if (base_type && (base_type->get_ast_node_type() == AstNodeType::Class)) {
			AstNodePtr<ClassNode> b = base_type.cast_to<ClassNode>();
			bool is_cyclic_inherited;

			SLKC_RETURN_IF_COMP_ERROR(b->is_cyclic_inherited(is_cyclic_inherited));

			if (((!walked_nodes) || (!walked_nodes->contains(base_type))) && (!is_cyclic_inherited)) {
				class_out = b;
			}
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::visit_base_interface(AstNodePtr<TypeNameNode> cls, AstNodePtr<InterfaceNode> &class_out, peff::Set<AstNodePtr<MemberNode>> *walked_nodes) {
	if (cls && (cls->type_name_kind == TypeNameKind::Custom)) {
		AstNodePtr<MemberNode> base_type;

		SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, cls->document->shared_from_this(), cls.cast_to<CustomTypeNameNode>(), base_type, walked_nodes));

		if (base_type && (base_type->get_ast_node_type() == AstNodeType::Interface)) {
			AstNodePtr<InterfaceNode> b = base_type.cast_to<InterfaceNode>();
			bool is_cyclic_inherited;

			SLKC_RETURN_IF_COMP_ERROR(b->is_cyclic_inherited(is_cyclic_inherited));

			if (((!walked_nodes) || (!walked_nodes->contains(base_type))) && (!is_cyclic_inherited)) {
				class_out = b;
			}
		}
	}

	return {};
}
