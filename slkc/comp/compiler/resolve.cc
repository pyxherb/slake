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
			return gen_oom_comp_error();
		}

		if (!entry.generic_args.resize(m->generic_args.size())) {
			return gen_oom_comp_error();
		}

		for (size_t i = 0; i < entry.generic_args.size(); ++i) {
			if (!(entry.generic_args.at(i) = m->generic_args.at(i)->duplicate<AstNode>(allocator))) {
				return gen_oom_comp_error();
			}
		}

		if (!p->entries.push_front(std::move(entry))) {
			return gen_oom_comp_error();
		}

		switch (m->get_ast_node_type()) {
			case AstNodeType::FnOverloading:
				if (!m->outer->outer)
					goto end;
				m = m->outer->outer->shared_from_this().cast_to<MemberNode>();
				break;
			default:
				if (!m->outer)
					goto end;
				m = m->outer->shared_from_this().cast_to<MemberNode>();
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

	if (member_node->scope) {
		result = member_node->scope->try_get_member(name.name);
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
reresolve:
	AstNodePtr<MemberNode> result;

	// Try to resolve with the current scope.
	if (member_node->scope)
		result = member_node->scope->try_get_member(name.name);

	if (!result) {
		switch (member_node->get_ast_node_type()) {
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> m = member_node.cast_to<InterfaceNode>();
				{
					peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());
					while (interfaces.size()) {
						for (auto i : interfaces) {
							if (i->scope) {
								// Get member from interfaces of current shadowing depth.
								// See above.
								if ((result = i->scope->try_get_member(name.name)))
									goto interface_resolution_succeeded;
							}
						}

						peff::Set<AstNodePtr<InterfaceNode>> new_interfaces(document->allocator.get());

						SLKC_RETURN_IF_COMP_ERROR(collect_involved_interfaces_phased_bfs(interfaces, new_interfaces));
						interfaces = std::move(new_interfaces);
					}
				}

			interface_resolution_succeeded:
				break;
			}
			case AstNodeType::GenericParam: {
				AstNodePtr<GenericParamNode> m = member_node.cast_to<GenericParamNode>();

				{
					AstNodePtr<ClassNode> class_node;

					SLKC_RETURN_IF_COMP_ERROR(visit_base_class(m->generic_constraint->base_type, class_node, nullptr));

					while (m) {
						if (class_node->scope && (result = class_node->scope->try_get_member(name.name)))
							goto generic_param_resolution_succeeded;

						peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());
						{
							peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());
							while (interfaces.size()) {
								for (auto i : interfaces) {
									if (i->scope) {
										// Get member from interfaces of current shadowing depth.
										// See above.
										if ((result = i->scope->try_get_member(name.name)))
											goto generic_param_resolution_succeeded;
									}
								}

								peff::Set<AstNodePtr<InterfaceNode>> new_interfaces(document->allocator.get());

								SLKC_RETURN_IF_COMP_ERROR(collect_involved_interfaces_phased_bfs(interfaces, new_interfaces));
								interfaces = std::move(new_interfaces);
							}
						}

						SLKC_RETURN_IF_COMP_ERROR(visit_base_class(class_node->scope->base_type, class_node, nullptr));
					}
				}

				{
					for (auto i : m->generic_constraint->impl_types) {
						AstNodePtr<InterfaceNode> interface_node;
						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, interface_node, nullptr));

						peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());

						SLKC_RETURN_IF_COMP_ERROR(collect_involved_interfaces(document, interface_node, interfaces, true));

						for (auto i : interfaces) {
							if (member_node->scope) {
								if ((result = i->scope->try_get_member(name.name)))
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

				if (m->type->tn_kind != TypeNameKind::Custom) {
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

				member_node = tm;

				goto reresolve;
			}
			default: {
				AstNodePtr<ClassNode> m = member_node.cast_to<ClassNode>();

				SLKC_RETURN_IF_COMP_ERROR(visit_base_class(member_node->scope->base_type, m, nullptr));

				while (m) {
					// Try getting member from base class.
					if (m->scope && (result = m->scope->try_get_member(name.name)))
						goto general_resolution_succeeded;

					{
						peff::Set<AstNodePtr<InterfaceNode>> interfaces(document->allocator.get());
						while (interfaces.size()) {
							for (auto i : interfaces) {
								if (i->scope) {
									// Try getting member from interfaces of current shadowing depth.
									// The conflicts will be handled during the compilation stage.
									if ((result = i->scope->try_get_member(name.name)))
										goto general_resolution_succeeded;
								}
							}

							peff::Set<AstNodePtr<InterfaceNode>> new_interfaces(document->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR(collect_involved_interfaces_phased_bfs(interfaces, new_interfaces));
							interfaces = std::move(new_interfaces);
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(visit_base_class(m->scope->base_type, m, nullptr));
				}
			general_resolution_succeeded:;
			}
		}
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
					member_out = {};
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
		auto p = member->outer->shared_from_this().cast_to<MemberNode>();

		if (p->get_ast_node_type() == AstNodeType::Fn)
			p = p->outer->shared_from_this().cast_to<MemberNode>();

		auto access_node = compile_env->cur_parent_access_node.cast_to<MemberNode>();

		switch (p->get_ast_node_type()) {
			case AstNodeType::Class:
				if (access_node->get_ast_node_type() == AstNodeType::Class) {
					bool result;

					SLKC_RETURN_IF_COMP_ERROR(is_base_of(compile_env->get_document(), p->shared_from_this().cast_to<ClassNode>(), access_node.cast_to<ClassNode>(), result));

					if (result) {
						// Child classes can always access parent's members.
						if (member->is_protected())
							goto access_check_passed;
					}
				}
				for (AstNodePtr<MemberNode> j = p; j; j = j->outer ? j->outer->shared_from_this().cast_to<MemberNode>() : AstNodePtr<MemberNode>{}) {
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
			default:
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
						return gen_oom_comp_error();

					is_post_static_parts = true;
				} else {
					ResolvedIdRefPart part = { is_static, 1, cur_member };

					if (!resolved_part_list_out->push_back(std::move(part)))
						return gen_oom_comp_error();
				}
			}
		}
	}

	if (resolved_part_list_out) {
		if (is_static) {
			ResolvedIdRefPart part = { is_static, num_entries, cur_member };

			if (!resolved_part_list_out->push_back(std::move(part)))
				return gen_oom_comp_error();
		}
	}

	member_out = cur_member;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::resolve_id_ref_with_scope_node(
	CompileEnv *compile_env,
	peff::SharedPtr<Document> document,
	peff::Set<AstNodePtr<MemberNode>> &walked_nodes,
	AstNodePtr<MemberNode> resolve_scope,
	IdRefEntry *id_ref,
	size_t num_entries,
	AstNodePtr<MemberNode> &member_out,
	ResolvedIdRefPartList *resolved_part_list_out,
	bool is_static,
	bool is_sealed) {
reresolve:
	if (walked_nodes.contains(resolve_scope)) {
		member_out = {};
		return {};
	}

	// Try resolving with generic parameters.
	if (num_entries == 1) {
		const IdRefEntry &initial_entry = id_ref[0];

		if (!initial_entry.generic_args.size()) {
			AstNodePtr<MemberNode> cur_scope = resolve_scope;

			auto i = resolve_scope.get();
			do {
				if (i->scope) {
					if (auto p = i->scope->try_get_generic_param(initial_entry.name); p) {
						member_out = p.cast_to<MemberNode>();
						return {};
					}
				}

				i = i->outer;
			} while (is_sealed && i->outer);
		}
	}

	SLKC_RETURN_IF_COMP_ERROR(resolve_id_ref(compile_env, document, resolve_scope, id_ref, num_entries, member_out, resolved_part_list_out, is_static));

	// Try resolving with outer scope.
	if ((!member_out) && (!is_sealed)) {
		if (resolve_scope->outer) {
			AstNodePtr<MemberNode> p = resolve_scope->outer->shared_from_this().cast_to<MemberNode>();

			switch (p->get_ast_node_type()) {
				case AstNodeType::Class:
				case AstNodeType::Interface:
				case AstNodeType::Struct:
				case AstNodeType::Module:
					resolve_scope = p.cast_to<MemberNode>();

					goto reresolve;
				case AstNodeType::Fn:
					resolve_scope = p.cast_to<MemberNode>()->outer->shared_from_this().cast_to<MemberNode>();

					goto reresolve;
				default:
					break;
			}
		}
	}

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

					switch (bt->tn_kind) {
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

SLKC_API peff::Option<CompilationError> slkc::visit_base_class(AstNodePtr<TypeNameNode> base_type_name, AstNodePtr<ClassNode> &class_out, peff::Set<AstNodePtr<MemberNode>> *walked_nodes) {
	do {
		if (base_type_name && (base_type_name->tn_kind == TypeNameKind::Custom)) {
			AstNodePtr<MemberNode> base_type;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, base_type_name->document->shared_from_this(), base_type_name.cast_to<CustomTypeNameNode>(), base_type, walked_nodes));

			if (base_type && (base_type->get_ast_node_type() == AstNodeType::Class)) {
				AstNodePtr<ClassNode> b = base_type.cast_to<ClassNode>();
				bool is_cyclic_inherited;

				SLKC_RETURN_IF_COMP_ERROR(b->is_cyclic_inherited(is_cyclic_inherited));

				if (((!walked_nodes) || (!walked_nodes->contains(base_type))) && (!is_cyclic_inherited)) {
					class_out = b;
					break;
				}
			}
		}
		class_out = {};
	} while (false);

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::visit_base_interface(AstNodePtr<TypeNameNode> base_type_name, AstNodePtr<InterfaceNode> &class_out, peff::Set<AstNodePtr<MemberNode>> *walked_nodes) {
	do {
		if (base_type_name && (base_type_name->tn_kind == TypeNameKind::Custom)) {
			AstNodePtr<MemberNode> base_type;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(nullptr, base_type_name->document->shared_from_this(), base_type_name.cast_to<CustomTypeNameNode>(), base_type, walked_nodes));

			if (base_type && (base_type->get_ast_node_type() == AstNodeType::Interface)) {
				AstNodePtr<InterfaceNode> b = base_type.cast_to<InterfaceNode>();
				bool is_cyclic_inherited;

				SLKC_RETURN_IF_COMP_ERROR(b->is_cyclic_inherited(is_cyclic_inherited));

				if (((!walked_nodes) || (!walked_nodes->contains(base_type))) && (!is_cyclic_inherited)) {
					class_out = b;
					break;
				}
			}
		}
		class_out = {};
	} while (false);

	return {};
}
