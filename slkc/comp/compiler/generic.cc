#include "../compiler.h"

#undef min
#undef max

using namespace slkc;

SLKC_API peff::Option<CompilationError> Document::lookup_generic_cache_table(
	AstNodePtr<MemberNode> original_object,
	GenericCacheTable *&table_out) {
	if (auto it = generic_cache_dir.find(original_object.get()); it != generic_cache_dir.end()) {
		table_out = &it.value();
		return {};
	}
	table_out = nullptr;
	return {};
}

SLKC_API peff::Option<CompilationError> Document::lookup_generic_cache(
	AstNodePtr<MemberNode> original_object,
	const peff::DynArray<AstNodePtr<AstNode>> &generic_args,
	AstNodePtr<MemberNode> &member_out) const {
	const GenericCacheTable *tab;

	SLKC_RETURN_IF_COMP_ERROR(lookup_generic_cache_table(original_object, tab));

	if (tab) {
		if (auto it = tab->find(generic_args); it != tab->end_const()) {
			member_out = it.value();
			return {};
		}
	}

	member_out = {};
	return {};
}

static peff::Option<CompilationError> _walk_type_name_for_generic_instantiation(
	AstNodePtr<TypeNameNode> &type_name,
	const GenericInstantiationContext &context);

static peff::Option<CompilationError> _walk_type_name_for_generic_instantiation(
	AstNodePtr<AstNode> &ast_node,
	peff::SharedPtr<GenericInstantiationContext> context) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 8));

	if (!ast_node)
		return {};

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->push_ast_node_task(AstNodeGenericInstantiationTask{ context, ast_node }));

	return {};
}

static peff::Option<CompilationError> _walk_type_name_for_generic_instantiation(
	AstNodePtr<TypeNameNode> &type_name,
	peff::SharedPtr<GenericInstantiationContext> context) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 8));

	if (!type_name) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->push_type_slot_task(TypeSlotGenericInstantiationTask{ context, type_name }));

	return {};
}

static peff::Option<CompilationError> _walk_node_for_generic_instantiation(
	AstNodePtr<MemberNode> ast_node,
	peff::SharedPtr<GenericInstantiationContext> context) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 8));

	if (!ast_node) {
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(context->dispatcher->push_member_task(MemberGenericInstantiationTask{ context, ast_node }));

	return {};
}

SLKC_API peff::Option<CompilationError> Document::instantiate_generic_object(
	AstNodePtr<MemberNode> original_object,
	const peff::DynArray<AstNodePtr<AstNode>> &generic_args,
	AstNodePtr<MemberNode> &member_out) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 16));

	AstNodePtr<MemberNode> duplicated_object;
	SLKC_RETURN_IF_COMP_ERROR(lookup_generic_cache(original_object, generic_args, duplicated_object));
	if (duplicated_object) {
		member_out = duplicated_object;
		return {};
	}

	peff::DynArray<AstNodePtr<AstNode>> duplicated_generic_args(allocator.get());

	if (!duplicated_generic_args.resize(generic_args.size())) {
		return gen_out_of_memory_comp_error();
	}

	for (size_t i = 0; i < duplicated_generic_args.size(); ++i) {
		if (!(duplicated_generic_args.at(i) = generic_args.at(i)->duplicate<AstNode>(allocator.get()))) {
			return gen_out_of_memory_comp_error();
		}
	}

	duplicated_object = original_object->duplicate<MemberNode>(allocator.get());

	if (!duplicated_object) {
		return gen_out_of_memory_comp_error();
	}

	duplicated_object->set_parent(original_object->parent);

	GenericCacheTable *cache_table;

	{
		peff::ScopeGuard remove_cache_dir_entry_guard([this, original_object]() noexcept {
			generic_cache_dir.remove(original_object.get());
		});

		if (auto it = generic_cache_dir.find(original_object.get());
			it != generic_cache_dir.end()) {
			cache_table = &it.value();
			remove_cache_dir_entry_guard.release();
		} else {
			peff::RcObjectPtr<CompileEnv> compile_env;

			if (!(compile_env = peff::alloc_and_construct<CompileEnv>(allocator.get(), alignof(CompileEnv), nullptr, shared_from_this(), allocator.get(), allocator.get())))
				return gen_out_of_memory_comp_error();
			if (!generic_cache_dir.insert(
					original_object.get(),
					GenericCacheTable(allocator.get(),
						GenericArgListCmp(this, compile_env.get())))) {
				return gen_out_of_memory_comp_error();
			}
			cache_table = &generic_cache_dir.at(original_object.get());
		}

		auto compile_env = cache_table->comparator().compile_env.get();
		NormalCompilationContext compilation_context(compile_env, nullptr);
		for (size_t i = 0; i < generic_args.size(); ++i) {
			AstNodePtr<AstNode> cur_arg = generic_args.at(i);

			if (cur_arg->get_ast_node_type() == AstNodeType::Expr) {
				AstNodePtr<ExprNode> evaluated_arg;
				PathEnv path_env(compile_env->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, &compilation_context, &path_env, cur_arg.cast_to<ExprNode>(), evaluated_arg));
				if (!evaluated_arg)
					return CompilationError(
						cur_arg->token_range,
						CompilationErrorKind::RequiresCompTimeExpr);

				evaluated_arg->token_range = cur_arg->token_range;
				duplicated_generic_args.at(i) = evaluated_arg.cast_to<AstNode>();
			}
		}
		{
			// Map generic arguments.
			GenericInstantiationDispatcher dispatcher(allocator.get());
			peff::SharedPtr<GenericInstantiationContext> context;

			if (!(context = peff::make_shared<GenericInstantiationContext>(allocator.get(), allocator.get(), &duplicated_generic_args, &dispatcher)))
				return gen_out_of_memory_comp_error();

			switch (original_object->get_ast_node_type()) {
				case AstNodeType::Fn: {
					AstNodePtr<FnNode> obj = duplicated_object.cast_to<FnNode>();

					peff::DynArray<AstNodePtr<FnOverloadingNode>> overloadings(allocator.get());

					for (auto i : obj->overloadings) {
						context->mapped_node = i.cast_to<MemberNode>();

						if (duplicated_generic_args.size() != i->generic_params.size())
							continue;

						for (size_t j = 0; j < duplicated_generic_args.size(); ++j) {
							AstNodePtr<AstNode> cur_arg = duplicated_generic_args.at(j);

							if (i->generic_params.at(j)->input_type) {
								if (cur_arg->get_ast_node_type() != AstNodeType::Expr)
									return CompilationError(
										cur_arg->token_range,
										CompilationErrorKind::RequiresCompTimeExpr);

								AstNodePtr<TypeNameNode> arg_type;
								{
									PathEnv root_path_env(compile_env->allocator.get());
									SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, &compilation_context, &root_path_env, cur_arg.cast_to<ExprNode>(), arg_type));
								}

								bool same = false;
								SLKC_RETURN_IF_COMP_ERROR(is_same_type(arg_type, i->generic_params.at(j)->input_type, same));

								if (!same)
									goto fn_overloading_mismatched;
							} else {
								if (cur_arg->get_ast_node_type() != AstNodeType::TypeName)
									goto fn_overloading_mismatched;
							}
						}

						for (auto [k, v] : i->generic_param_indices) {
							if (!context->mapped_generic_args.insert(
									std::string_view(k),
									AstNodePtr<AstNode>(duplicated_generic_args.at(v)))) {
								return gen_out_of_memory_comp_error();
							}
						}

						SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(i.cast_to<MemberNode>(), context));

						if (!overloadings.push_back(AstNodePtr<FnOverloadingNode>(i))) {
							return gen_out_of_memory_comp_error();
						}
					fn_overloading_mismatched:;
					}

					if (!overloadings.shrink_to_fit()) {
						return gen_out_of_memory_comp_error();
					}

					obj->overloadings = std::move(overloadings);

					break;
				}
				case AstNodeType::Class: {
					AstNodePtr<ClassNode> obj = duplicated_object.cast_to<ClassNode>();

					context->mapped_node = obj.cast_to<MemberNode>();

					if (duplicated_generic_args.size() != obj->generic_params.size()) {
						return CompilationError(
							TokenRange{
								duplicated_generic_args.front()->token_range.module_node,
								duplicated_generic_args.front()->token_range.begin_index,
								duplicated_generic_args.back()->token_range.end_index },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (size_t i = 0; i < duplicated_generic_args.size(); ++i) {
						AstNodePtr<AstNode> cur_arg = duplicated_generic_args.at(i);

						if (obj->generic_params.at(i)->input_type) {
							if (cur_arg->get_ast_node_type() != AstNodeType::Expr)
								return CompilationError(
									cur_arg->token_range,
									CompilationErrorKind::RequiresCompTimeExpr);

							AstNodePtr<TypeNameNode> arg_type;
							{
								PathEnv root_path_env(compile_env->allocator.get());
								SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, &compilation_context, &root_path_env, cur_arg.cast_to<ExprNode>(), arg_type));
							}

							bool same = false;
							SLKC_RETURN_IF_COMP_ERROR(is_same_type(arg_type, obj->generic_params.at(i)->input_type, same));

							if (!same)
								return CompilationError(
									cur_arg->token_range,
									CompilationErrorKind::TypeArgTypeMismatched);
						} else {
							if (cur_arg->get_ast_node_type() != AstNodeType::TypeName)
								return CompilationError(
									cur_arg->token_range,
									CompilationErrorKind::ExpectingTypeName);
						}
					}

					for (auto [k, v] : obj->generic_param_indices) {
						if (!context->mapped_generic_args.insert(
								std::string_view(k),
								AstNodePtr<AstNode>(duplicated_generic_args.at(v)))) {
							return gen_out_of_memory_comp_error();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(duplicated_object, context));
					break;
				}
				case AstNodeType::Interface: {
					AstNodePtr<InterfaceNode> obj = duplicated_object.cast_to<InterfaceNode>();

					context->mapped_node = obj.cast_to<MemberNode>();

					if (duplicated_generic_args.size() != obj->generic_params.size()) {
						return CompilationError(
							TokenRange{
								duplicated_generic_args.front()->token_range.module_node,
								duplicated_generic_args.front()->token_range.begin_index,
								duplicated_generic_args.back()->token_range.end_index },
							CompilationErrorKind::MismatchedGenericArgNumber);
					}

					for (size_t i = 0; i < duplicated_generic_args.size(); ++i) {
						AstNodePtr<AstNode> cur_arg = duplicated_generic_args.at(i);

						if (obj->generic_params.at(i)->input_type) {
							if (cur_arg->get_ast_node_type() != AstNodeType::Expr)
								return CompilationError(
									cur_arg->token_range,
									CompilationErrorKind::RequiresCompTimeExpr);

							AstNodePtr<TypeNameNode> arg_type;
							{
								PathEnv root_path_env(compile_env->allocator.get());
								SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, &compilation_context, &root_path_env, cur_arg.cast_to<ExprNode>(), arg_type));
							}

							bool same = false;
							SLKC_RETURN_IF_COMP_ERROR(is_same_type(arg_type, obj->generic_params.at(i)->input_type, same));

							if (!same)
								return CompilationError(
									cur_arg->token_range,
									CompilationErrorKind::TypeArgTypeMismatched);
						} else {
							if (cur_arg->get_ast_node_type() != AstNodeType::TypeName)
								return CompilationError(
									cur_arg->token_range,
									CompilationErrorKind::ExpectingTypeName);
						}
					}

					for (auto [k, v] : obj->generic_param_indices) {
						if (!context->mapped_generic_args.insert(
								std::string_view(k),
								AstNodePtr<AstNode>(duplicated_generic_args.at(v)))) {
							return gen_out_of_memory_comp_error();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(duplicated_object, context));
					break;
				}
				default:
					return CompilationError(
						TokenRange{
							duplicated_generic_args.front()->token_range.module_node,
							duplicated_generic_args.front()->token_range.begin_index,
							duplicated_generic_args.back()->token_range.end_index },
						CompilationErrorKind::MismatchedGenericArgNumber);
			}

			while (true) {
				auto type_name_tasks = std::move(dispatcher.type_tasks);
				auto member_tasks = std::move(dispatcher.member_tasks);
				auto ast_node_tasks = std::move(dispatcher.ast_node_tasks);

				dispatcher.type_tasks = { allocator.get() };
				dispatcher.member_tasks = { allocator.get() };
				dispatcher.ast_node_tasks = { allocator.get() };

				if ((!type_name_tasks.size() &&
						(!member_tasks.size())) &&
					(!ast_node_tasks.size()))
					break;

				for (auto &task : type_name_tasks) {
					auto &type_name = task.type_name;

					switch (type_name->type_name_kind) {
						case TypeNameKind::Array: {
							AstNodePtr<ArrayTypeNameNode> tn = type_name.cast_to<ArrayTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->element_type, task.context));
							break;
						}
						case TypeNameKind::Ref: {
							AstNodePtr<RefTypeNameNode> tn = type_name.cast_to<RefTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->referenced_type, task.context));
							break;
						}
						case TypeNameKind::TempRef: {
							AstNodePtr<TempRefTypeNameNode> tn = type_name.cast_to<TempRefTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->referenced_type, task.context));
							break;
						}
						case TypeNameKind::Fn: {
							AstNodePtr<FnTypeNameNode> tn = type_name.cast_to<FnTypeNameNode>();

							for (size_t i = 0; i < tn->param_types.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->param_types.at(i), task.context));
							}
							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->return_type, task.context));
							break;
						}
						case TypeNameKind::Custom: {
							AstNodePtr<CustomTypeNameNode> tn = type_name.cast_to<CustomTypeNameNode>();

							if (tn->id_ref_ptr->entries.size() == 1) {
								IdRefEntry &entry = tn->id_ref_ptr->entries.at(0);

								if (!entry.generic_args.size()) {
									if (auto it = task.context->mapped_generic_args.find(entry.name);
										it != task.context->mapped_generic_args.end()) {
										if (it.value()->get_ast_node_type() != AstNodeType::TypeName)
											return CompilationError(it.value()->token_range, CompilationErrorKind::ExpectingTypeName);
										bool nullable = type_name->is_nullable;
										type_name = it.value().cast_to<TypeNameNode>();
										type_name->is_nullable = nullable;
										break;
									}
								}
							}

							for (size_t i = 0; i < tn->id_ref_ptr->entries.size(); ++i) {
								auto &generic_args = tn->id_ref_ptr->entries.at(i).generic_args;
								for (size_t j = 0; j < generic_args.size(); ++j) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(generic_args.at(j), task.context));
								}
							}
							break;
						}
						case TypeNameKind::Unpacking: {
							AstNodePtr<UnpackingTypeNameNode> tn = type_name.cast_to<UnpackingTypeNameNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->inner_type_name, task.context));
							break;
						}
						case TypeNameKind::ParamTypeList: {
							AstNodePtr<ParamTypeListTypeNameNode> tn = type_name.cast_to<ParamTypeListTypeNameNode>();

							for (size_t i = 0; i < tn->param_types.size(); ++i) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->param_types.at(i), task.context));
							}
							break;
						}
					}
				}

				for (auto &task : member_tasks) {
					auto &ast_node = task.member;

					if (task.context->mapped_node == ast_node) {
						if (!ast_node->generic_args.resize(task.context->generic_args->size())) {
							return gen_out_of_memory_comp_error();
						}
						for (size_t i = 0; i < task.context->generic_args->size(); ++i) {
							if (!(ast_node->generic_args.at(i) =
										task.context->generic_args->at(i)
											->duplicate<TypeNameNode>(allocator.get()))) {
								return gen_out_of_memory_comp_error();
							}
						}
					}

					switch (ast_node->get_ast_node_type()) {
						case AstNodeType::FnOverloading: {
							AstNodePtr<FnOverloadingNode> fn_slot = ast_node.cast_to<FnOverloadingNode>();

							for (auto i : fn_slot->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(i.cast_to<MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (fn_slot->generic_params.size())) {
								peff::SharedPtr<GenericInstantiationContext> inner_context;

								if (!(inner_context = peff::make_shared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->generic_args, &dispatcher))) {
									return gen_out_of_memory_comp_error();
								}

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = fn_slot->generic_param_indices.find(k);
										it == fn_slot->generic_param_indices.end()) {
										if (!inner_context->mapped_generic_args.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
											return gen_out_of_memory_comp_error();
										}
									}
								}

								for (auto &i : fn_slot->params) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(i->type, inner_context));
								}

								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(fn_slot->return_type, inner_context));

								// No need to substitute the function body, we just care about the declaration.
							} else {
								for (auto &i : fn_slot->params) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(i->type, task.context));
								}

								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(fn_slot->return_type, task.context));
							}

							if (!dispatcher.collected_fn_overloadings.contains(fn_slot))
								if (!dispatcher.collected_fn_overloadings.insert(std::move(fn_slot)))
									return gen_out_of_memory_comp_error();
							break;
						}
						case AstNodeType::Fn: {
							AstNodePtr<FnNode> fn_slot = ast_node.cast_to<FnNode>();

							for (auto i : fn_slot->overloadings) {
								AstNodePtr<MemberNode> a = i.cast_to<MemberNode>();
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(a, task.context));
							}

							if (!dispatcher.collected_fns.contains(fn_slot))
								if (!dispatcher.collected_fns.insert(std::move(fn_slot)))
									return gen_out_of_memory_comp_error();
							break;
						}
						case AstNodeType::Var: {
							AstNodePtr<VarNode> var_node = ast_node.cast_to<VarNode>();

							SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(var_node->type, task.context));
							break;
						}
						case AstNodeType::Class: {
							AstNodePtr<ClassNode> cls = ast_node.cast_to<ClassNode>();

							for (auto j : cls->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j.cast_to<MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (cls->generic_params.size())) {
								peff::SharedPtr<GenericInstantiationContext> inner_context;

								if (!(inner_context = peff::make_shared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->generic_args, &dispatcher))) {
									return gen_out_of_memory_comp_error();
								}

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = cls->generic_param_indices.find(k);
										it == cls->generic_param_indices.end()) {
										if (!inner_context->mapped_generic_args.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
											return gen_out_of_memory_comp_error();
										}
									}
								}

								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(cls->base_type, inner_context));

								for (auto &k : cls->impl_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k, inner_context));
								}

								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, inner_context));
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(cls->base_type, task.context));

								for (auto &k : cls->impl_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k, task.context));
								}

								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, task.context));
								}
							}
							break;
						}
						case AstNodeType::Struct: {
							AstNodePtr<StructNode> cls = ast_node.cast_to<StructNode>();

							for (auto j : cls->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j.cast_to<MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (cls->generic_params.size())) {
								peff::SharedPtr<GenericInstantiationContext> inner_context;

								if (!(inner_context = peff::make_shared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->generic_args, &dispatcher))) {
									return gen_out_of_memory_comp_error();
								}

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = cls->generic_param_indices.find(k);
										it == cls->generic_param_indices.end()) {
										if (!inner_context->mapped_generic_args.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
											return gen_out_of_memory_comp_error();
										}
									}
								}
								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, inner_context));
								}
							} else {
								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, task.context));
								}
							}
							break;
						}
						case AstNodeType::Interface: {
							AstNodePtr<InterfaceNode> cls = ast_node.cast_to<InterfaceNode>();

							for (auto j : cls->generic_params) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j.cast_to<MemberNode>(), task.context));
							}

							if ((task.context->mapped_node != ast_node) && (cls->generic_params.size())) {
								peff::SharedPtr<GenericInstantiationContext> inner_context;

								if (!(inner_context = peff::make_shared<GenericInstantiationContext>(allocator.get(), allocator.get(), task.context->generic_args, &dispatcher))) {
									return gen_out_of_memory_comp_error();
								}

								for (auto [k, v] : task.context->mapped_generic_args) {
									if (auto it = cls->generic_param_indices.find(k);
										it == cls->generic_param_indices.end()) {
										if (!inner_context->mapped_generic_args.insert(std::string_view(k), AstNodePtr<AstNode>(v))) {
											return gen_out_of_memory_comp_error();
										}
									}
								}

								for (auto &k : cls->impl_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k, inner_context));
								}

								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, inner_context));
								}
							} else {
								for (auto &k : cls->impl_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k, task.context));
								}

								for (auto j : cls->members) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_node_for_generic_instantiation(j, task.context));
								}
							}
							break;
						}
						case AstNodeType::GenericParam: {
							AstNodePtr<GenericParamNode> cls = ast_node.cast_to<GenericParamNode>();

							if (cls->generic_constraint) {
								SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(cls->generic_constraint->base_type, task.context));

								for (auto &k : cls->generic_constraint->impl_types) {
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(k, task.context));
								}
							}
							break;
						}
						default:;
					}
				}

				for (auto &task : ast_node_tasks) {
					auto &ast_node = task.node;

					switch (ast_node->get_ast_node_type()) {
						case AstNodeType::TypeName: {
							const AstNodePtr<TypeNameNode> type_name = ast_node.cast_to<TypeNameNode>();
							switch (type_name->type_name_kind) {
								case TypeNameKind::Array: {
									AstNodePtr<ArrayTypeNameNode> tn = type_name.cast_to<ArrayTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->element_type, task.context));
									break;
								}
								case TypeNameKind::Ref: {
									AstNodePtr<RefTypeNameNode> tn = type_name.cast_to<RefTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->referenced_type, task.context));
									break;
								}
								case TypeNameKind::TempRef: {
									AstNodePtr<TempRefTypeNameNode> tn = type_name.cast_to<TempRefTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->referenced_type, task.context));
									break;
								}
								case TypeNameKind::Fn: {
									AstNodePtr<FnTypeNameNode> tn = type_name.cast_to<FnTypeNameNode>();

									for (size_t i = 0; i < tn->param_types.size(); ++i) {
										SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->param_types.at(i), task.context));
									}
									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->return_type, task.context));
									break;
								}
								case TypeNameKind::Custom: {
									AstNodePtr<CustomTypeNameNode> tn = type_name.cast_to<CustomTypeNameNode>();

									if (tn->id_ref_ptr->entries.size() == 1) {
										IdRefEntry &entry = tn->id_ref_ptr->entries.at(0);

										if (!entry.generic_args.size()) {
											if (auto it = task.context->mapped_generic_args.find(entry.name);
												it != task.context->mapped_generic_args.end()) {
												ast_node = it.value();
												break;
											}
										}
									}

									for (size_t i = 0; i < tn->id_ref_ptr->entries.size(); ++i) {
										auto &generic_args = tn->id_ref_ptr->entries.at(i).generic_args;
										for (size_t j = 0; j < generic_args.size(); ++j) {
											SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(generic_args.at(j), task.context));
										}
									}
									break;
								}
								case TypeNameKind::Unpacking: {
									AstNodePtr<UnpackingTypeNameNode> tn = type_name.cast_to<UnpackingTypeNameNode>();

									SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->inner_type_name, task.context));
									break;
								}
								case TypeNameKind::ParamTypeList: {
									AstNodePtr<ParamTypeListTypeNameNode> tn = type_name.cast_to<ParamTypeListTypeNameNode>();

									for (size_t i = 0; i < tn->param_types.size(); ++i) {
										SLKC_RETURN_IF_COMP_ERROR(_walk_type_name_for_generic_instantiation(tn->param_types.at(i), task.context));
									}
									break;
								}
							}
						}
					}
				}
			}

			for (auto fn_slot : dispatcher.collected_fn_overloadings) {
			rescan_params:
				for (size_t i = 0; i < fn_slot->params.size(); ++i) {
					auto cur_param = fn_slot->params.at(i);
					AstNodePtr<TypeNameNode> cur_param_type = fn_slot->params.at(i)->type;

					if (cur_param_type) {
						if (cur_param_type->type_name_kind == TypeNameKind::Unpacking) {
							AstNodePtr<UnpackingTypeNameNode> unpacking_type = cur_param_type.cast_to<UnpackingTypeNameNode>();

							if (unpacking_type->inner_type_name->type_name_kind == TypeNameKind::ParamTypeList) {
								AstNodePtr<ParamTypeListTypeNameNode> inner_type_name = unpacking_type->inner_type_name.cast_to<ParamTypeListTypeNameNode>();

								if (!fn_slot->params.erase_range_and_shrink(i, i + 1))
									return gen_out_of_memory_comp_error();

								if (!fn_slot->params.insert_range_init(i, inner_type_name->param_types.size())) {
									return gen_out_of_memory_comp_error();
								}

								for (size_t k = 0; k < inner_type_name->param_types.size(); ++k) {
									AstNodePtr<VarNode> p = cur_param->duplicate<VarNode>(allocator.get());

									if (!p) {
										return gen_out_of_memory_comp_error();
									}

									constexpr static size_t len_name = sizeof("arg_") + (sizeof(size_t) << 1) + 1;
									char name_buf[len_name] = { 0 };

									sprintf(name_buf, "arg_%.02zx", i + k);

									if (!p->name.build(name_buf)) {
										return gen_out_of_memory_comp_error();
									}

									p->type = inner_type_name->param_types.at(k);

									fn_slot->params.at(i + k) = p;
								}

								// Note that we use nullptr for we assuming that errors that require a compile task.context will never happen.
								SLKC_RETURN_IF_COMP_ERROR(reindex_fn_params(nullptr, fn_slot));

								if (inner_type_name->has_var_args) {
									if (i + 1 != fn_slot->params.size()) {
										return CompilationError(inner_type_name->token_range, CompilationErrorKind::InvalidVarArgHintDuringInstantiation);
									}

									fn_slot->fn_flags |= FN_VARG;
								}
							}

							goto rescan_params;
						}
					}
				}
			}

			for (auto fn_slot : dispatcher.collected_fns) {
				for (auto it = fn_slot->overloadings.begin(); it != fn_slot->overloadings.end(); ++it) {
					for (auto jt = it + 1; jt != fn_slot->overloadings.end(); ++jt) {
						bool whether;
						SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_duplicated(*it, *jt, whether));

						if (whether) {
							ModuleNode *mod = nullptr;
							size_t idx_min_token = SIZE_MAX, idx_max_token = 0;

							for (auto i : *context->generic_args) {
								if (!mod) {
									mod = i->token_range.module_node;
								} else if (i->token_range.module_node != mod)
									std::terminate();
								idx_min_token = std::min(i->token_range.begin_index, idx_min_token);
								idx_max_token = std::max(i->token_range.end_index, idx_max_token);
							}

							return CompilationError(
								TokenRange(mod, idx_min_token, idx_max_token),
								CompilationErrorKind::FunctionOverloadingDuplicatedDuringInstantiation);
						}
					}
				}
			}

			if (!cache_table->insert(std::move(duplicated_generic_args), AstNodePtr<MemberNode>(duplicated_object))) {
				return gen_out_of_memory_comp_error();
			}
		}

		remove_cache_dir_entry_guard.release();
	}

	member_out = duplicated_object;
	return {};
}
