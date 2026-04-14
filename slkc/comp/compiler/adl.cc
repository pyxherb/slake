#include "../compiler.h"

using namespace slkc;

static peff::Option<CompilationError> _determine_with_current_slot(
	CompileEnv *compile_env,
	AstNodePtr<FnNode> fn_slot,
	const AstNodePtr<TypeNameNode> *arg_types,
	size_t num_arg_types,
	bool is_static,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matched_overloadings,
	peff::Set<AstNodePtr<MemberNode>> *walked_parents) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 8));

	for (size_t i = 0; i < fn_slot->overloadings.size(); ++i) {
		bool exactly_matched = true;
		AstNodePtr<FnOverloadingNode> current_overloading = fn_slot->overloadings.at(i);

		if (is_static != ((current_overloading->access_modifier & slake::ACCESS_STATIC) == slake::ACCESS_STATIC)) {
			continue;
		}

		if (num_arg_types < current_overloading->params.size()) {
			continue;
		} else if (num_arg_types > current_overloading->params.size()) {
			if (!(current_overloading->fn_flags & FN_VARG)) {
				continue;
			}
		}

		for (size_t j = 0; j < current_overloading->params.size(); ++j) {
			AstNodePtr<VarNode> current_param = current_overloading->params.at(j);

			bool whether = false;
			SLKC_RETURN_IF_COMP_ERROR(is_same_type(current_param->type, arg_types[j], whether));

			if (!whether) {
				SLKC_RETURN_IF_COMP_ERROR(is_convertible(arg_types[j], current_param->type, true, whether));
				if (!whether) {
					goto mismatched;
				}
				exactly_matched = false;
			}
		}

		if (exactly_matched) {
			matched_overloadings.clear_and_shrink();

			if (!matched_overloadings.push_back(AstNodePtr<FnOverloadingNode>(current_overloading))) {
				return gen_out_of_memory_comp_error();
			}

			return {};
		} else {
			if (!matched_overloadings.push_back(AstNodePtr<FnOverloadingNode>(current_overloading))) {
				return gen_out_of_memory_comp_error();
			}
		}

	mismatched:;
	}
	return {};
}

static peff::Option<CompilationError> _determine_with_parent_class(
	CompileEnv *compile_env,
	AstNodePtr<FnNode> fn_slot,
	const AstNodePtr<TypeNameNode> *arg_types,
	size_t num_arg_types,
	bool is_static,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matched_overloadings,
	peff::Set<AstNodePtr<MemberNode>> *walked_parents) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 8));

	AstNodePtr<ClassNode> m = fn_slot->parent->shared_from_this().cast_to<ClassNode>();
	{
		AstNodePtr<ClassNode> base_type;
		SLKC_RETURN_IF_COMP_ERROR(visit_base_class(m->base_type, base_type, nullptr));
		if (base_type) {
			if (auto it = base_type->member_indices.find(fn_slot->name); it != base_type->member_indices.end()) {
				if (base_type->members.at(it.value())->get_ast_node_type() != AstNodeType::Fn) {
					goto class_base_class_malformed;
				}

				if (!walked_parents) {
					peff::Set<AstNodePtr<MemberNode>> walked_parents_set(compile_env->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, base_type->members.at(it.value()).cast_to<FnNode>(), arg_types, num_arg_types, is_static, matched_overloadings, walked_parents));
				} else {
					SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, base_type->members.at(it.value()).cast_to<FnNode>(), arg_types, num_arg_types, is_static, matched_overloadings));
				}
			}
		}
	}

class_base_class_malformed:
	for (auto &i : m->impl_types) {
		{
			AstNodePtr<InterfaceNode> base_type;
			SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, nullptr));
			if (base_type) {
				if (auto it = base_type->member_indices.find(fn_slot->name); it != base_type->member_indices.end()) {
					if (base_type->members.at(it.value())->get_ast_node_type() != AstNodeType::Fn) {
						continue;
					}

					if (!walked_parents) {
						peff::Set<AstNodePtr<MemberNode>> walked_parents_set(compile_env->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, base_type->members.at(it.value()).cast_to<FnNode>(), arg_types, num_arg_types, is_static, matched_overloadings, walked_parents));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, base_type->members.at(it.value()).cast_to<FnNode>(), arg_types, num_arg_types, is_static, matched_overloadings));
					}
				}
			}
		}
	}
	return {};
}

static peff::Option<CompilationError> _determine_with_parent_interface(
	CompileEnv *compile_env,
	AstNodePtr<FnNode> fn_slot,
	const AstNodePtr<TypeNameNode> *arg_types,
	size_t num_arg_types,
	bool is_static,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matched_overloadings,
	peff::Set<AstNodePtr<MemberNode>> *walked_parents) {
	AstNodePtr<InterfaceNode> m = fn_slot->parent->shared_from_this().cast_to<InterfaceNode>();
	for (auto &i : m->impl_types) {
		{
			AstNodePtr<InterfaceNode> base_type;
			SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(i, base_type, nullptr));
			if (base_type) {
				if (auto it = base_type->member_indices.find(fn_slot->name); it != base_type->member_indices.end()) {
					if (base_type->members.at(it.value())->get_ast_node_type() != AstNodeType::Fn) {
						continue;
					}

					if (!walked_parents) {
						peff::Set<AstNodePtr<MemberNode>> walked_parents_set(compile_env->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, base_type->members.at(it.value()).cast_to<FnNode>(), arg_types, num_arg_types, is_static, matched_overloadings, walked_parents));
					} else {
						SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, base_type->members.at(it.value()).cast_to<FnNode>(), arg_types, num_arg_types, is_static, matched_overloadings));
					}
				}
			}
		}
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::determine_fn_overloading(
	CompileEnv *compile_env,
	AstNodePtr<FnNode> fn_slot,
	const AstNodePtr<TypeNameNode> *arg_types,
	size_t num_arg_types,
	bool is_static,
	peff::DynArray<AstNodePtr<FnOverloadingNode>> &matched_overloadings,
	peff::Set<AstNodePtr<MemberNode>> *walked_parents) {
	SLKC_RETURN_IF_COMP_ERROR(check_stack_bounds(1024 * 1));

	SLKC_RETURN_IF_COMP_ERROR(_determine_with_current_slot(compile_env, fn_slot, arg_types, num_arg_types, is_static, matched_overloadings, walked_parents));

	if (is_static) {
		// ...
	} else
		switch (fn_slot->parent->get_ast_node_type()) {
			case AstNodeType::Class:
				SLKC_RETURN_IF_COMP_ERROR(_determine_with_parent_class(compile_env, fn_slot, arg_types, num_arg_types, is_static, matched_overloadings, walked_parents));
				break;
			case AstNodeType::Interface:
				SLKC_RETURN_IF_COMP_ERROR(_determine_with_parent_interface(compile_env, fn_slot, arg_types, num_arg_types, is_static, matched_overloadings, walked_parents));
				break;
			default:
				break;
		}

	return {};
}
