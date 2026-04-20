#include "compiler.h"

using namespace slkc;

size_t slkc::sz_default_parse_thread_stack = 1024 * 1024 * 2;
size_t slkc::sz_default_compile_thread_stack = 1024 * 1024 * 32;

SLKC_API PathPossibility slkc::combine_possibility(PathPossibility outer, PathPossibility inner) noexcept {
	switch (outer) {
		case PathPossibility::Never:
			return PathPossibility::Never;
		case PathPossibility::May:
			switch (inner) {
				case PathPossibility::May:
				case PathPossibility::Must:
					return PathPossibility::May;
				case PathPossibility::Never:
					return PathPossibility::Never;
				default:
					break;
			}
			std::terminate();
		case PathPossibility::Must:
			switch (inner) {
				case PathPossibility::May:
					return PathPossibility::May;
				case PathPossibility::Must:
					return PathPossibility::Must;
				case PathPossibility::Never:
					return PathPossibility::Never;
				default:
					break;
			}
			std::terminate();
		default:
			break;
	}
	std::terminate();
}

SLKC_API peff::Option<CompilationError> slkc::combine_path_env(PathEnv &outer, const PathEnv &inner) noexcept {
	switch (inner.exec_possibility) {
		case PathPossibility::Never:
			break;
		case PathPossibility::May: {
			outer.no_return_possibility = combine_possibility(outer.no_return_possibility, combine_possibility(inner.exec_possibility, inner.no_return_possibility));
			outer.return_possibility = combine_possibility(outer.return_possibility, combine_possibility(inner.exec_possibility, inner.return_possibility));
			outer.break_possibility = combine_possibility(outer.break_possibility, combine_possibility(inner.exec_possibility, inner.break_possibility));

			for (auto i : inner.local_var_nullity_overrides) {
				if (i.second == NullOverrideType::Uncertain) {
					SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, NullOverrideType::Uncertain));
				} else if (i.second == NullOverrideType::Nullify) {
					SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, NullOverrideType::Nullify));
				}
			}
			break;
		}
		case PathPossibility::Must: {
			outer.no_return_possibility = combine_possibility(outer.no_return_possibility, inner.no_return_possibility);
			outer.return_possibility = combine_possibility(outer.return_possibility, inner.return_possibility);
			outer.break_possibility = combine_possibility(outer.break_possibility, inner.break_possibility);

			for (auto i : inner.local_var_nullity_overrides) {
				if (i.second == NullOverrideType::Uncertain) {
					SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, NullOverrideType::Uncertain));
				} else
					SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, i.second));
			}
			break;
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::combine_parallel_path_env(peff::Alloc *allocator, CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv &outer, PathEnv *const *inners, size_t num_inners) noexcept {
	peff::Map<AstNodePtr<VarNode>, NullOverrideType> local_var_nullity_overrides(allocator);
	peff::Set<size_t> idx_may_paths(allocator);

	for (size_t i = 0; i < num_inners; ++i) {
		const PathEnv &inner = *inners[i];

		switch (inner.exec_possibility) {
			case PathPossibility::Never:
				break;
			case PathPossibility::May: {
				if (!idx_may_paths.insert(+i))
					return gen_oom_comp_error();
				break;
			}
			case PathPossibility::Must: {
				outer.no_return_possibility = combine_possibility(outer.no_return_possibility, inner.no_return_possibility);
				outer.return_possibility = combine_possibility(outer.return_possibility, inner.return_possibility);
				outer.break_possibility = combine_possibility(outer.break_possibility, inner.break_possibility);

				for (auto i : inner.local_var_nullity_overrides) {
					if (i.second == NullOverrideType::Uncertain) {
						SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, NullOverrideType::Uncertain));
					} else
						SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, i.second));
				}
				break;
			}
		}
	}

	{
		peff::Map<AstNodePtr<VarNode>, NullOverrideType> common_local_var_nullity_overrides(allocator);

		// Find common local variable null overrides.
		{
			peff::Set<AstNodePtr<VarNode>> nonunifiable_nullity_override_vars(allocator);
			for (auto it = idx_may_paths.begin(); it != idx_may_paths.end(); ++it) {
				const PathEnv &inner = *inners[*it];

				for (auto cur_override : inner.local_var_nullity_overrides) {
					if (!nonunifiable_nullity_override_vars.contains(cur_override.first)) {
						if (auto prev_override = common_local_var_nullity_overrides.find(cur_override.first); prev_override != common_local_var_nullity_overrides.end()) {
							if (prev_override.value() != cur_override.second) {
								if (!nonunifiable_nullity_override_vars.insert(AstNodePtr<VarNode>(prev_override.key())))
									return gen_oom_comp_error();
								common_local_var_nullity_overrides.remove(prev_override);
							}
						} else {
							auto copied_override_type = cur_override.second;
							if (!common_local_var_nullity_overrides.insert(AstNodePtr<VarNode>(cur_override.first), std::move(copied_override_type)))
								return gen_oom_comp_error();
						}
					}
				}
			}

			for (auto i : nonunifiable_nullity_override_vars)
				SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i, NullOverrideType::Uncertain));
		}

		for (auto i : common_local_var_nullity_overrides) {
			SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, i.second));
		}

		// Filter out the common local variable null overrides that are not always happen.
		for (auto it = idx_may_paths.begin(); it != idx_may_paths.end(); ++it) {
			const PathEnv &inner = *inners[*it];

			for (auto i : common_local_var_nullity_overrides) {
				// If a local variable null override is not always happen, its original assumption should be cancelled.
				if (!inner.local_var_nullity_overrides.contains(i.first)) {
					SLKC_RETURN_IF_COMP_ERROR(outer.set_local_var_nullity_override(i.first, NullOverrideType::Uncertain));
				}
			}
		}
	}

	return {};
}

SLKC_API PathEnv::PathEnv(peff::Alloc *allocator) noexcept : local_var_nullity_overrides(allocator) {
}

SLAKE_API peff::Option<NullOverrideType> PathEnv::lookup_var_nullity_override(const AstNodePtr<VarNode> &var_node) {
	for (const PathEnv *i = this; i; i = i->parent) {
		if (auto it = i->local_var_nullity_overrides.find(var_node);
			it != i->local_var_nullity_overrides.end()) {
			NullOverrideType v = it.value();
			return std::move(v);
		}
	}
	return {};
}

SLAKE_API peff::Option<CompilationError> PathEnv::set_local_var_nullity_override(AstNodePtr<VarNode> var_node, NullOverrideType type) {
	if (auto it = local_var_nullity_overrides.find(var_node); it != local_var_nullity_overrides.end()) {
		it.value() = type;
	} else if (!local_var_nullity_overrides.insert(std::move(var_node), std::move(type)))
		return gen_oom_comp_error();
	return {};
}

SLAKE_API void PathEnv::remove_var_nullity_override(const AstNodePtr<VarNode> &var_node) {
	local_var_nullity_overrides.remove(var_node);
}

SLKC_API CompilationContext::CompilationContext(CompilationContext *parent) : parent(parent) {
}
SLKC_API CompilationContext::~CompilationContext() {
}

SLKC_API uint32_t CompilationContext::get_break_label() const {
	for (const CompilationContext *i = this; i; i = i->parent) {
		if (uint32_t l = i->do_get_break_label(); l != UINT32_MAX)
			return l;
	}
	return UINT32_MAX;
}
SLKC_API uint32_t CompilationContext::get_continue_label() const {
	for (const CompilationContext *i = this; i; i = i->parent) {
		if (uint32_t l = i->do_get_continue_label(); l != UINT32_MAX)
			return l;
	}
	return UINT32_MAX;
}

SLKC_API uint32_t CompilationContext::get_break_label_block_level() const {
	for (const CompilationContext *i = this; i; i = i->parent) {
		if (i->do_get_break_label() != UINT32_MAX)
			return i->do_get_break_label_block_level();
	}
	return 0;
}
SLKC_API uint32_t CompilationContext::get_continue_label_block_level() const {
	for (const CompilationContext *i = this; i; i = i->parent) {
		if (i->do_get_continue_label() != UINT32_MAX)
			return i->do_get_continue_label_block_level();
	}
	return 0;
}

SLKC_API AstNodePtr<VarNode> CompilationContext::lookup_local_var(const std::string_view &name) const {
	for (const CompilationContext *i = this; i; i = i->parent) {
		AstNodePtr<VarNode> var_node = i->get_local_var(name);

		if (var_node) {
			return var_node;
		}
	}
	return {};
}

SLKC_API NormalCompilationContext::BlockLayer::~BlockLayer() {
}

SLKC_API NormalCompilationContext::NormalCompilationContext(CompileEnv *compile_env, CompilationContext *parent)
	: CompilationContext(parent),
	  allocator(compile_env->allocator),
	  saved_block_layers(compile_env->allocator.get()),
	  cur_block_layer(compile_env->allocator.get()),
	  labels(compile_env->allocator.get()),
	  label_name_indices(compile_env->allocator.get()),
	  generated_instructions(compile_env->allocator.get()),
	  document(compile_env->get_document()),
	  base_block_level(parent ? parent->get_block_level() : 0),
	  base_ins_off(parent ? parent->get_cur_ins_off() : 0),
	  source_loc_descs(compile_env->allocator.get()),
	  source_loc_descs_map(compile_env->allocator.get()) {
}
SLKC_API NormalCompilationContext::~NormalCompilationContext() {
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::alloc_label(uint32_t &label_id_out) {
	peff::SharedPtr<Label> label = peff::make_shared<Label>(allocator.get(), peff::String(allocator.get()));

	if (!label) {
		return gen_oom_comp_error();
	}

	label_id_out = labels.size();

	if (!labels.push_back(peff::SharedPtr<Label>(label))) {
		return gen_oom_comp_error();
	}

	return {};
}
SLKC_API void NormalCompilationContext::set_label_offset(uint32_t label_id, uint32_t offset) const {
	labels.at(label_id)->offset = offset;
}
SLKC_API peff::Option<CompilationError> NormalCompilationContext::set_label_name(uint32_t label_id, const std::string_view &name) {
	if (!labels.at(label_id)->name.build(name)) {
		return gen_oom_comp_error();
	}
	if (!label_name_indices.insert(labels.at(label_id)->name, +label_id))
		return gen_oom_comp_error();
	return {};
}
SLKC_API uint32_t NormalCompilationContext::get_label_offset(uint32_t label_id) const {
	return labels.at(label_id)->offset;
}

SLKC_API peff::Option<uint32_t> NormalCompilationContext::get_label_index_by_name(const std::string_view &sv) const {
	if (auto it = label_name_indices.find(sv); it != label_name_indices.end())
		return it.value();
	return {};
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::alloc_reg(uint32_t &reg_out) {
	if (num_total_regs < UINT32_MAX) {
		reg_out = num_total_regs++;
		return {};
	}

	return CompilationError({ document->main_module, 0 }, CompilationErrorKind::RegLimitExceeded);
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::emit_ins(uint32_t idx_sld, slake::Opcode opcode, uint32_t output_reg_index, const std::initializer_list<slake::Value> &operands) {
	slake::Instruction ins_out;

	ins_out.off_source_loc_desc = idx_sld;
	ins_out.opcode = opcode;
	ins_out.output = output_reg_index;
	if (!ins_out.reserve_operands(allocator.get(), operands.size())) {
		return gen_oom_comp_error();
	}

	auto it = operands.begin();
	for (size_t i = 0; i < operands.size(); ++i) {
		ins_out.operands[i] = *it++;
	}

	if (!generated_instructions.push_back(std::move(ins_out))) {
		return gen_out_of_runtime_memory_comp_error();
	}

	return {};
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::emit_ins(uint32_t idx_sld, slake::Opcode opcode, uint32_t output_reg_index, slake::Value *operands, size_t num_operands) {
	slake::Instruction ins_out;

	ins_out.off_source_loc_desc = idx_sld;
	ins_out.opcode = opcode;
	ins_out.output = output_reg_index;
	if (!ins_out.reserve_operands(allocator.get(), num_operands)) {
		return gen_oom_comp_error();
	}

	memcpy(ins_out.operands, operands, sizeof(slake::Value) * num_operands);

	if (!generated_instructions.push_back(std::move(ins_out))) {
		return gen_out_of_runtime_memory_comp_error();
	}

	return {};
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::alloc_local_var(const TokenRange &token_range, const std::string_view &name, uint32_t reg, AstNodePtr<TypeNameNode> type, AstNodePtr<VarNode> &local_var_out) {
	AstNodePtr<VarNode> new_var;

	if (!(new_var = make_ast_node<VarNode>(allocator.get(), allocator.get(), document))) {
		return gen_oom_comp_error();
	}

	if (!new_var->name.build(name)) {
		return gen_oom_comp_error();
	}

	new_var->type = type;

	new_var->idx_reg = reg;

	if (!cur_block_layer.local_vars.insert(new_var->name, AstNodePtr<VarNode>(new_var))) {
		return gen_oom_comp_error();
	}

	local_var_out = new_var;

	return {};
}
SLKC_API AstNodePtr<VarNode> NormalCompilationContext::get_local_var_in_cur_level(const std::string_view &name) const {
	if (auto it = cur_block_layer.local_vars.find(name); it != cur_block_layer.local_vars.end()) {
		return it.value();
	}

	return {};
}
SLKC_API AstNodePtr<VarNode> NormalCompilationContext::get_local_var(const std::string_view &name) const {
	if (auto v = get_local_var_in_cur_level(name); v)
		return v;

	for (auto i = saved_block_layers.begin_const_reversed(); i != saved_block_layers.end_const_reversed(); ++i) {
		if (auto it = i->local_vars.find(name); it != i->local_vars.end()) {
			return it.value();
		}
	}

	return {};
}

SLKC_API void NormalCompilationContext::set_break_label(uint32_t label_id, uint32_t block_level) {
	break_stmt_jump_dest_label = label_id;
	break_stmt_block_level = block_level;
}
SLKC_API void NormalCompilationContext::set_continue_label(uint32_t label_id, uint32_t block_level) {
	continue_stmt_jump_dest_label = label_id;
	continue_stmt_block_level = block_level;
}

SLKC_API uint32_t NormalCompilationContext::do_get_break_label() const {
	return break_stmt_jump_dest_label;
}
SLKC_API uint32_t NormalCompilationContext::do_get_continue_label() const {
	return continue_stmt_jump_dest_label;
}

SLKC_API uint32_t NormalCompilationContext::do_get_break_label_block_level() const {
	return break_stmt_block_level;
}

SLKC_API uint32_t NormalCompilationContext::do_get_continue_label_block_level() const {
	return continue_stmt_block_level;
}

SLKC_API uint32_t NormalCompilationContext::get_cur_ins_off() const {
	return base_ins_off + generated_instructions.size();
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::enter_block() {
	if (!saved_block_layers.push_back(std::move(cur_block_layer))) {
		return gen_oom_comp_error();
	}

	cur_block_layer = BlockLayer(allocator.get());

	return {};
}
SLKC_API void NormalCompilationContext::leave_block() {
	cur_block_layer = std::move(saved_block_layers.back());
	saved_block_layers.pop_back();
}

SLKC_API uint32_t NormalCompilationContext::get_block_level() {
	return base_block_level + saved_block_layers.size();
}

SLKC_API peff::Option<CompilationError> NormalCompilationContext::register_source_loc_desc(slake::slxfmt::SourceLocDesc sld, uint32_t &index_out) {
	if (!source_loc_descs_map.insert(slake::slxfmt::SourceLocDesc(sld), source_loc_descs.size()))
		return gen_oom_comp_error();
	peff::ScopeGuard remove_source_loc_descs_map_guard([this, &sld]() noexcept {
		source_loc_descs_map.remove(sld);
	});
	if (!source_loc_descs.push_back(slake::slxfmt::SourceLocDesc(sld)))
		return gen_oom_comp_error();
	remove_source_loc_descs_map_guard.release();
	index_out = source_loc_descs.size() - 1;
	return {};
}

SLKC_API CompileEnv::~CompileEnv() {
}

SLKC_API peff::Option<CompilationError> slkc::eval_expr_type(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	const AstNodePtr<ExprNode> &expr,
	AstNodePtr<TypeNameNode> &type_out,
	AstNodePtr<TypeNameNode> desired_type) {
	NormalCompilationContext tmp_context(compile_env, compilation_context);

	CompileExprResult result(compile_env->allocator.get());

	SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, &tmp_context, path_env, expr, ExprEvalPurpose::EvalType, desired_type, result));

	type_out = result.evaluated_type;
	return {};
}

[[nodiscard]] SLKC_API peff::Option<CompilationError> slkc::eval_ref_removed_expr_type(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	const AstNodePtr<ExprNode> &expr,
	AstNodePtr<TypeNameNode> &type_out,
	AstNodePtr<TypeNameNode> desired_type) {
	SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, compilation_context, path_env, expr, type_out, desired_type));
	if (!type_out)
		return {};
	SLKC_RETURN_IF_COMP_ERROR(remove_ref_of_type(type_out, type_out));
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::eval_actual_expr_type(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	const AstNodePtr<ExprNode> &expr,
	AstNodePtr<TypeNameNode> &type_out,
	AstNodePtr<TypeNameNode> desired_type) {
	NormalCompilationContext tmp_context(compile_env, compilation_context);

	CompileExprResult result(compile_env->allocator.get());

	ExprEvalPurpose eval_purpose;

	SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, &tmp_context, path_env, expr, ExprEvalPurpose::EvalTypeActual, desired_type, result));

	type_out = result.evaluated_type;
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::complete_parent_modules(
	CompileEnv *compile_env,
	IdRef *module_path,
	AstNodePtr<ModuleNode> leaf) {
	peff::DynArray<AstNodePtr<ModuleNode>> modules(compile_env->allocator.get());
	size_t idx_new_modules_begin = 0;

	if (!modules.resize(module_path->entries.size())) {
		return gen_oom_comp_error();
	}

	AstNodePtr<ModuleNode> node = compile_env->get_document()->root_module;

	for (size_t i = 0; i < modules.size(); ++i) {
		if (auto it = node->scope->_member_indices.find(module_path->entries.at(i).name); it != node->scope->_member_indices.end()) {
			node = node->scope->_members.at(it.value()).cast_to<ModuleNode>();
			modules.at(i) = node;
			idx_new_modules_begin = i + 1;
		} else {
			if (i + 1 == modules.size()) {
				node = leaf;
			} else {
				if (!(node = make_ast_node<ModuleNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document()))) {
					return gen_oom_comp_error();
				}
				if (!node->alloc_scope())
					return gen_oom_comp_error();
				node->access_modifier = slake::make_access_modifier(slake::AccessMode::Public, slake::ACCESS_STATIC);
			}
			modules.at(i) = node;
			if (!node->name.build(module_path->entries.at(i).name)) {
				return gen_oom_comp_error();
			}
		}
	}

	if (!leaf->name.build(module_path->entries.back().name)) {
		return gen_oom_comp_error();
	}

	for (size_t i = idx_new_modules_begin; i < modules.size(); ++i) {
		auto &current_entry = module_path->entries.at(i);

		if (i) {
			auto m1 = modules.at(i - 1), m2 = modules.at(i);
			if (!modules.at(i - 1)->scope->add_member(modules.at(i).cast_to<MemberNode>())) {
				return gen_oom_comp_error();
			}
			modules.at(i)->set_parent(modules.at(i - 1).get());
		} else {
			if (!compile_env->get_document()->root_module->scope->add_member(modules.at(i).cast_to<MemberNode>())) {
				return gen_oom_comp_error();
			}
			modules.at(i)->set_parent(compile_env->get_document()->root_module.get());
		}
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::cleanup_unused_module_tree(
	CompileEnv *compile_env,
	AstNodePtr<ModuleNode> leaf) {
	AstNodePtr<ModuleNode> cur = leaf;

	for (;;) {
		for (auto &i : cur->scope->_members) {
			if (i->get_ast_node_type() == AstNodeType::Module) {
				return {};
			}
		}

		if (!cur->outer) {
			break;
		}

		if (cur->outer->get_ast_node_type() != AstNodeType::Module)
			std::terminate();

		AstNodePtr<ModuleNode> parent = cur->outer->shared_from_this().cast_to<ModuleNode>();

		parent->scope->remove_member(cur->name);

		cur = parent;
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::check_stack_bounds(size_t reserved_size) {
	void *base;
	size_t size;
	slake::get_current_thread_stack_bounds(base, size);

	void *ptr = slake::estimate_current_stack_pointer();

	assert(base < ptr);

	if (((char *)ptr - (char *)base) < reserved_size)
		return gen_stack_overflow();

	return {};
}

ExternalModuleProvider::ExternalModuleProvider(const char *provider_name) : provider_name(provider_name) {
}

ExternalModuleProvider::~ExternalModuleProvider() {
}

FileSystemExternalModuleProvider::FileSystemExternalModuleProvider(peff::Alloc *allocator) : ExternalModuleProvider("filesystem"), import_paths(allocator) {
}

FileSystemExternalModuleProvider::~FileSystemExternalModuleProvider() {
}

SLKC_API peff::Option<CompilationError> FileSystemExternalModuleProvider::load_module(CompileEnv *compile_env, IdRef *module_name) {
	peff::String suffix_path(compile_env->allocator.get());

	{
		bool is_loaded = true;
		AstNodePtr<ModuleNode> node = compile_env->get_document()->root_module;

		for (size_t i = 0; i < module_name->entries.size(); ++i) {
			if (auto it = node->scope->_member_indices.find(module_name->entries.at(i).name); it != node->scope->_member_indices.end()) {
				node = node->scope->_members.at(it.value()).cast_to<ModuleNode>();
				continue;
			}

			is_loaded = false;
			break;
		}

		if (is_loaded) {
			return {};
		}
	}

	for (size_t i = 0; i < module_name->entries.size(); ++i) {
		auto &current_entry = module_name->entries.at(i);

		if (current_entry.generic_args.size()) {
			return CompilationError(module_name->token_range, CompilationErrorKind::MalformedModuleName);
		}

		size_t begin_index = suffix_path.size();

		if (!suffix_path.resize(begin_index + sizeof('/') + current_entry.name.size())) {
			return gen_oom_comp_error();
		}

		suffix_path.at(begin_index) = '/';

		memcpy(suffix_path.data() + begin_index + 1, current_entry.name.data(), current_entry.name.size());
	}

	for (size_t i = 0; i < import_paths.size(); ++i) {
		const peff::String &cur_path = import_paths.at(i);

		{
			peff::String full_path(compile_env->allocator.get());

			const static char extension[] = ".slk";

			if (!full_path.resize(cur_path.size() + suffix_path.size() + strlen(extension))) {
				return gen_oom_comp_error();
			}

			memcpy(full_path.data(), cur_path.data(), cur_path.size());
			memcpy(full_path.data() + cur_path.size(), suffix_path.data(), suffix_path.size());
			memcpy(full_path.data() + cur_path.size() + suffix_path.size(), extension, strlen(extension));

			FILE *fp = fopen(full_path.data(), "rb");
			if (fp) {
				peff::ScopeGuard close_fp_guard([fp]() noexcept {
					if (fp) {
						fclose(fp);
					}
				});

				fseek(fp, 0, SEEK_END);
				long file_size = ftell(fp);
				if (file_size < 0) {
					goto fail;
				}
				fseek(fp, 0, SEEK_SET);

				auto deleter = [compile_env, file_size](void *ptr) {
					if (ptr) {
						compile_env->allocator->release(ptr, (size_t)file_size, 1);
					}
				};
				std::unique_ptr<char, decltype(deleter)> file_content((char *)malloc((size_t)file_size + 1), std::move(deleter));
				if (!file_content) {
					goto fail;
				}

				if (fread(file_content.get(), (size_t)file_size, 1, fp) < 1) {
					goto fail;
				}

				file_content.get()[file_size] = '\0';

				AstNodePtr<ModuleNode> mod;

				if (!(mod = make_ast_node<ModuleNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->get_document()))) {
					return gen_oom_comp_error();
				}
				if (!mod->alloc_scope()) {
					return gen_oom_comp_error();
				}
				mod->access_modifier = slake::make_access_modifier(slake::AccessMode::Public, slake::ACCESS_STATIC);

				slkc::TokenList token_list(compile_env->allocator.get());
				{
					slkc::Lexer lexer(compile_env->allocator.get());

					std::string_view sv(file_content.get(), file_size);

					if (auto e = lexer.lex(mod.get(), sv, peff::default_allocator(), compile_env->get_document()); e) {
						auto ce = CompilationError(module_name->token_range, ErrorParsingImportedModuleErrorExData(std::move(*e)));
						e.reset();
						return std::move(ce);
					}

					token_list = std::move(lexer.token_list);
				}

				peff::SharedPtr<slkc::Parser> parser;
				if (!(parser = peff::make_shared<slkc::Parser>(compile_env->allocator.get(), compile_env->get_document(), std::move(token_list), compile_env->allocator.get()))) {
					return gen_oom_comp_error();
				}

				IdRefPtr module_name;
				if (auto e = parser->parse(mod, module_name); e) {
					if (!parser->syntax_errors.push_back(std::move(*e))) {
						return gen_oom_comp_error();
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(complete_parent_modules(compile_env, module_name.get(), mod));

				if (parser->syntax_errors.size()) {
					return CompilationError(module_name->token_range, ErrorParsingImportedModuleErrorExData(mod));
				}

				for (auto i : mod->scope->_members) {
					if (i->get_ast_node_type() == AstNodeType::Import) {
						SLKC_RETURN_IF_COMP_ERROR(load_module(compile_env, i.cast_to<ImportNode>()->id_ref.get()));
					}
				}
				for (auto i : mod->scope->anonymous_imports) {
					SLKC_RETURN_IF_COMP_ERROR(load_module(compile_env, i->id_ref.get()));
				}

				return {};
			}
		fail:;
		}

		{
			peff::String full_path(compile_env->allocator.get());

			const static char extension[] = ".slx";

			if (!full_path.resize(cur_path.size() + suffix_path.size() + strlen(extension))) {
				return gen_oom_comp_error();
			}

			memcpy(full_path.data(), cur_path.data(), cur_path.size());
			memcpy(full_path.data() + cur_path.size(), suffix_path.data(), suffix_path.size());
			memcpy(full_path.data() + cur_path.size() + suffix_path.size(), extension, strlen(extension));

			FILE *fp = fopen(full_path.data(), "rb");
			if (fp) {
				peff::ScopeGuard close_fp_guard([fp]() noexcept {
					if (fp) {
						fclose(fp);
					}
				});

				fseek(fp, 0, SEEK_END);
				long file_size = ftell(fp);
				if (file_size < 0) {
					goto module_fail;
				}
				fseek(fp, 0, SEEK_SET);

				auto deleter = [compile_env, file_size](void *ptr) {
					if (ptr) {
						compile_env->allocator->release(ptr, (size_t)file_size, 1);
					}
				};
				std::unique_ptr<char, decltype(deleter)> file_content((char *)malloc((size_t)file_size), std::move(deleter));
				if (!file_content) {
					goto module_fail;
				}

				if (fread(file_content.get(), (size_t)file_size, 1, fp) < 1) {
					goto module_fail;
				}

				/* TODO: Implement it.*/
			}
		module_fail:;
		}
	}

	return CompilationError(module_name->token_range, CompilationErrorKind::ModuleNotFound);
}

SLKC_API bool FileSystemExternalModuleProvider::register_import_path(peff::String &&path) {
	if (!import_paths.push_back(std::move(path))) {
		return false;
	}
	return true;
}
