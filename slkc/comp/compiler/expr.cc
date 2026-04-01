#include "../compiler.h"

using namespace slkc;

template <typename LE, typename DT, typename TN>
static peff::Option<CompilationError> _compile_literal_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	const AstNodePtr<ExprNode> &expr,
	ExprEvalPurpose eval_purpose,
	slake::Opcode opcode,
	CompileExprResult &result_out) {
	AstNodePtr<LE> e = expr.cast_to<LE>();

	switch (eval_purpose) {
		case ExprEvalPurpose::EvalType:
		case ExprEvalPurpose::EvalTypeActual:
			break;
		case ExprEvalPurpose::Stmt:
			SLKC_RETURN_IF_COMP_ERROR(compile_env->push_warning(
				CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
			break;
		case ExprEvalPurpose::RValue: {
			uint32_t output_reg;
			SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(output_reg));

			SLKC_RETURN_IF_COMP_ERROR(
				compilation_context->emit_ins(
					expr->token_range,
					opcode,
					output_reg,
					{ slake::Value((DT)e->data) }));
			result_out.idx_result_reg_out = output_reg;
			break;
		}
		case ExprEvalPurpose::LValue:
			return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
			break;
		case ExprEvalPurpose::Call:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
			break;
		case ExprEvalPurpose::Unpacking:
			return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
		default:
			std::terminate();
	}
	if (!(result_out.evaluated_type = make_ast_node<TN>(
			  compile_env->allocator.get(),
			  compile_env->allocator.get(),
			  compile_env->document)
				.template cast_to<TypeNameNode>())) {
		return gen_out_of_memory_comp_error();
	}
	return {};
}

SLKC_API peff::Option<CompilationError> slkc::_compile_or_cast_operand(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> desired_type,
	AstNodePtr<ExprNode> operand,
	AstNodePtr<TypeNameNode> operand_type,
	CompileExprResult &result_out) {
	bool whether;
	SLKC_RETURN_IF_COMP_ERROR(is_same_type(desired_type, operand_type, whether));
	if (whether) {
		SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, operand, eval_purpose, desired_type, result_out));
		return {};
	}

	SLKC_RETURN_IF_COMP_ERROR(is_convertible(operand_type, desired_type, true, whether));
	if (whether) {
		AstNodePtr<CastExprNode> cast_expr;

		SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
			compile_env,
			operand,
			desired_type,
			cast_expr));

		SLKC_RETURN_IF_COMP_ERROR(compile_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), eval_purpose, desired_type, result_out));
		return {};
	}

	IncompatibleOperandErrorExData ex_data;
	ex_data.desired_type = desired_type;

	return CompilationError(operand->token_range, std::move(ex_data));
}

static peff::Option<CompilationError> _determine_node_type(CompileEnv *compile_env, PathEnv *path_env, ExprEvalPurpose eval_purpose, AstNodePtr<MemberNode> node, AstNodePtr<TypeNameNode> &type_name_out);

static peff::Option<CompilationError> _load_rest_of_id_ref(CompileEnv *compile_env, CompilationContext *compilation_context, PathEnv *path_env, ExprEvalPurpose eval_purpose, uint32_t &result_reg_out, CompileExprResult &result_out, IdRef *id_ref, ResolvedIdRefPartList &parts, uint32_t initial_member_reg, size_t cur_idx, AstNodePtr<FnTypeNameNode> extra_fn_args, uint32_t sld_index) {
	uint32_t idx_reg;

	if (initial_member_reg == UINT32_MAX) {
		SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(idx_reg));
	} else {
		idx_reg = initial_member_reg;
	}

	if (parts.back().member->get_ast_node_type() == AstNodeType::FnOverloading) {
		AstNodePtr<FnOverloadingNode> fn = parts.back().member.cast_to<FnOverloadingNode>();

		if (parts.size() > 1) {
			if (fn->fn_flags & FN_VIRTUAL) {
				slake::HostObjectRef<slake::IdRefObject> id_ref_object;
				AstNodePtr<TypeNameNode> overriden_type;

				// Is calling a virtual instance method.
				for (size_t i = cur_idx; i < parts.size() - 1; ++i) {
					ResolvedIdRefPart &part = parts.at(i);

					if ((i + 1 == parts.size() - 1) &&
						(eval_purpose == ExprEvalPurpose::Call) &&
						(!part.is_static) &&
						(part.member->get_ast_node_type() == AstNodeType::Interface)) {
						// TODO: Add explicit override selection.
						AstNodePtr<CustomTypeNameNode> custom_overriden_type;

						if (!(custom_overriden_type = make_ast_node<CustomTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document)))
							return gen_out_of_memory_comp_error();

						SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), part.member, custom_overriden_type->id_ref_ptr));

						custom_overriden_type->context_node = to_weak_ptr(fn.cast_to<MemberNode>());

						overriden_type = custom_overriden_type.cast_to<TypeNameNode>();
					}

					if (part.num_entries) {
						SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, id_ref->entries.data() + cur_idx, part.num_entries, nullptr, 0, false, {}, id_ref_object));

						if (part.is_static) {
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, idx_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
						} else {
							uint32_t idx_new_reg;

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(idx_new_reg));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::RLOAD, idx_new_reg, { slake::Value(slake::ValueType::RegIndex, idx_reg), slake::Value(slake::Reference(id_ref_object.get())) }));
							idx_reg = idx_new_reg;
						}
						cur_idx += part.num_entries;
					}
				}

				{
					ResolvedIdRefPart &part = parts.back();

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, id_ref->entries.data() + cur_idx, part.num_entries, nullptr, 0, false, overriden_type, id_ref_object));

					if (extra_fn_args) {
						id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

						if (!id_ref_object->param_types->resize(extra_fn_args->param_types.size()))
							return gen_out_of_runtime_memory_comp_error();

						for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
							SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, extra_fn_args->param_types.at(i), id_ref_object->param_types->at(i)));
						}

						if (extra_fn_args->has_var_args)
							id_ref_object->has_var_args = true;
					}

					result_out.idx_this_reg_out = idx_reg;
					uint32_t idx_new_reg;

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(idx_new_reg));

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::RLOAD, idx_new_reg, { slake::Value(slake::ValueType::RegIndex, idx_reg), slake::Value(slake::Reference(id_ref_object.get())) }));
					idx_reg = idx_new_reg;
				}
			} else {
				// Is calling an instance method that is not virtual.
				slake::HostObjectRef<slake::IdRefObject> id_ref_object;

				for (size_t i = cur_idx; i < parts.size() - 1; ++i) {
					ResolvedIdRefPart &part = parts.at(i);

					if (part.num_entries) {
						SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, id_ref->entries.data() + cur_idx, part.num_entries, nullptr, 0, false, {}, id_ref_object));

						if (part.is_static) {
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, idx_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
						} else {
							uint32_t idx_new_reg;

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(idx_new_reg));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::RLOAD, idx_new_reg, { slake::Value(slake::ValueType::RegIndex, idx_reg), slake::Value(slake::Reference(id_ref_object.get())) }));
							idx_reg = idx_new_reg;
						}
						cur_idx += part.num_entries;
					}
				}

				result_out.idx_this_reg_out = idx_reg;
				SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(idx_reg));

				if (fn == compile_env->cur_overloading) {
					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LCURFN, idx_reg, {}));
				} else {
					IdRefPtr full_id_ref;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), fn.cast_to<MemberNode>(), full_id_ref));

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_id_ref->entries.data(), full_id_ref->entries.size(), nullptr, 0, false, {}, id_ref_object));

					if (extra_fn_args) {
						id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

						if (!id_ref_object->param_types->resize(extra_fn_args->param_types.size()))
							return gen_out_of_runtime_memory_comp_error();

						for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
							SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, extra_fn_args->param_types.at(i), id_ref_object->param_types->at(i)));
						}

						if (extra_fn_args->has_var_args)
							id_ref_object->has_var_args = true;
					}

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, idx_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
				}
			}
		} else {
			if (fn == compile_env->cur_overloading) {
				SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LCURFN, idx_reg, {}));
			} else {
				// Is calling a static method.
				IdRefPtr full_id_ref;
				SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), fn.cast_to<MemberNode>(), full_id_ref));

				slake::HostObjectRef<slake::IdRefObject> id_ref_object;
				SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_id_ref->entries.data(), full_id_ref->entries.size(), nullptr, 0, false, {}, id_ref_object));

				if (extra_fn_args) {
					id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

					if (!id_ref_object->param_types->resize(extra_fn_args->param_types.size()))
						return gen_out_of_runtime_memory_comp_error();

					for (size_t i = 0; i < id_ref_object->param_types->size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, extra_fn_args->param_types.at(i), id_ref_object->param_types->at(i)));
					}

					if (extra_fn_args->has_var_args)
						id_ref_object->has_var_args = true;
				}

				SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, idx_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
			}
		}
	} else {
		slake::HostObjectRef<slake::IdRefObject> id_ref_object;

		if (parts.size() > 1) {
			for (size_t i = cur_idx; i < parts.size(); ++i) {
				ResolvedIdRefPart &part = parts.at(i);

				SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, id_ref->entries.data() + cur_idx, part.num_entries, nullptr, 0, false, {}, id_ref_object));

				if (part.is_static) {
					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, idx_reg, { slake::Value(slake::Reference(id_ref_object.get())) }));
				} else {
					uint32_t idx_new_reg;

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(idx_new_reg));

					SLKC_RETURN_IF_COMP_ERROR(compilation_context->emit_ins(sld_index, slake::Opcode::RLOAD, idx_new_reg, { slake::Value(slake::ValueType::RegIndex, idx_reg), slake::Value(slake::Reference(id_ref_object.get())) }));
					idx_reg = idx_new_reg;
				}

				cur_idx += part.num_entries;
			}
		}
	}

	result_reg_out = idx_reg;
	return {};
};
peff::Option<CompilationError> select_single_matching_overloading(CompileEnv *compile_env, const TokenRange &token_range, AstNodePtr<MemberNode> &final_member, AstNodePtr<TypeNameNode> desired_type, bool is_static, CompileExprResult &result_out) {
	AstNodePtr<FnNode> m = final_member.cast_to<FnNode>();

	result_out.call_target_fn_slot = m;

	if (m->overloadings.size() == 1) {
		final_member = m->overloadings.back().cast_to<MemberNode>();
		if (!result_out.call_target_matched_overloadings.push_back(AstNodePtr<FnOverloadingNode>(m->overloadings.back()))) {
			return gen_out_of_memory_comp_error();
		}
		bool accessible;
		SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, {}, final_member.cast_to<MemberNode>(), accessible));
		if (!accessible)
			return CompilationError(
				token_range,
				CompilationErrorKind::MemberIsNotAccessible);
	} else {
		if (desired_type && (desired_type->tn_kind == TypeNameKind::Fn)) {
			AstNodePtr<FnTypeNameNode> tn = desired_type.cast_to<FnTypeNameNode>();
			peff::DynArray<AstNodePtr<FnOverloadingNode>> matched_overloadings(compile_env->allocator.get());

			// TODO: Check tn->is_for_adl and do strictly equality check.
			SLKC_RETURN_IF_COMP_ERROR(determine_fn_overloading(compile_env, m, tn->param_types.data(), tn->param_types.size(), is_static, matched_overloadings));

			switch (matched_overloadings.size()) {
				case 0:
					return CompilationError(token_range, CompilationErrorKind::NoMatchingFnOverloading);
				case 1:
					break;
				default:
					return CompilationError(token_range, CompilationErrorKind::UnableToDetermineOverloading);
			}

			final_member = matched_overloadings.back().cast_to<MemberNode>();

			bool accessible;
			SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, {}, final_member.cast_to<MemberNode>(), accessible));
			if (!accessible)
				return CompilationError(
					token_range,
					CompilationErrorKind::MemberIsNotAccessible);

			result_out.call_target_matched_overloadings = std::move(matched_overloadings);
		} else {
			return CompilationError(token_range, CompilationErrorKind::UnableToDetermineOverloading);
		}
	}

	return {};
};

static peff::Option<CompilationError> _determine_node_type(CompileEnv *compile_env, PathEnv *path_env, ExprEvalPurpose eval_purpose, AstNodePtr<MemberNode> node, AstNodePtr<TypeNameNode> &type_name_out) {
	switch (node->get_ast_node_type()) {
		case AstNodeType::This: {
			auto m = node.cast_to<ThisNode>()->this_type;

			IdRefPtr full_id_ref;

			SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(compile_env->allocator.get(), m, full_id_ref));

			auto tn = make_ast_node<CustomTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document);

			if (!tn) {
				return gen_out_of_memory_comp_error();
			}
			tn->context_node = to_weak_ptr(compile_env->document->root_module.cast_to<MemberNode>());

			tn->id_ref_ptr = std::move(full_id_ref);

			type_name_out = tn.cast_to<TypeNameNode>();
			break;
		}
		case AstNodeType::Var: {
			auto original_type = node.cast_to<VarNode>()->type;

			AstNodePtr<TypeNameNode> unpacked_type_name_node;

			SLKC_RETURN_IF_COMP_ERROR(get_unpacked_type_of(original_type, unpacked_type_name_node));

			if ((original_type->tn_kind != TypeNameKind::Ref) && (!unpacked_type_name_node)) {
				AstNodePtr<RefTypeNameNode> t;

				if (!(t = make_ast_node<RefTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document, AstNodePtr<TypeNameNode>()))) {
					return gen_out_of_memory_comp_error();
				}
				t->referenced_type = node.cast_to<VarNode>()->type;

				if ((eval_purpose != ExprEvalPurpose::LValue) &&
					(eval_purpose != ExprEvalPurpose::EvalTypeActual)) {
					auto null_override = path_env->lookup_var_null_override(node.cast_to<VarNode>());

					if (null_override.has_value()) {
						switch (*null_override) {
							case NullOverrideType::Nullify:
								if (!(t->referenced_type = t->referenced_type->duplicate<TypeNameNode>(compile_env->allocator.get())))
									return gen_out_of_memory_comp_error();
								t->referenced_type->is_nullable = true;
								break;
							case NullOverrideType::Denullify:
								if (!(t->referenced_type = t->referenced_type->duplicate<TypeNameNode>(compile_env->allocator.get())))
									return gen_out_of_memory_comp_error();
								t->referenced_type->is_nullable = false;
								break;
							case NullOverrideType::Uncertain:
								break;
						}
					}
				}

				type_name_out = t.cast_to<TypeNameNode>();
			} else {
				type_name_out = original_type;

				if ((eval_purpose != ExprEvalPurpose::LValue) &&
					(eval_purpose != ExprEvalPurpose::EvalTypeActual)) {
					auto null_override = path_env->lookup_var_null_override(node.cast_to<VarNode>());

					if (null_override.has_value()) {
						switch (*null_override) {
							case NullOverrideType::Nullify:
								if (!(type_name_out = type_name_out->duplicate<TypeNameNode>(compile_env->allocator.get())))
									return gen_out_of_memory_comp_error();
								type_name_out->is_nullable = true;
								break;
							case NullOverrideType::Denullify:
								if (!(type_name_out = type_name_out->duplicate<TypeNameNode>(compile_env->allocator.get())))
									return gen_out_of_memory_comp_error();
								type_name_out->is_nullable = false;
								break;
							case NullOverrideType::Uncertain:
								break;
						}
					}
				}
			}

			break;
		}
		case AstNodeType::FnOverloading: {
			AstNodePtr<FnTypeNameNode> tn;
			SLKC_RETURN_IF_COMP_ERROR(fn_to_type_name(compile_env, node.cast_to<FnOverloadingNode>(), tn));
			type_name_out = tn.cast_to<TypeNameNode>();
			break;
		}
		case AstNodeType::Fn:
			type_name_out = {};
			break;
		case AstNodeType::Module:
		case AstNodeType::Class:
		case AstNodeType::Struct:
		case AstNodeType::Interface:
			type_name_out = {};
			break;
		default:
			std::terminate();
	}

	SLKC_RETURN_IF_COMP_ERROR(unwrap_param_list_type_name_tree(type_name_out, compile_env->allocator.get(), type_name_out));

	return {};
};

SLKC_API peff::Option<CompilationError> slkc::compile_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	PathEnv *path_env,
	const AstNodePtr<ExprNode> &expr,
	ExprEvalPurpose eval_purpose,
	AstNodePtr<TypeNameNode> desired_type,
	CompileExprResult &result_out) {
	peff::Option<CompilationError> compilation_error;
	uint32_t sld_index;
	if ((eval_purpose != ExprEvalPurpose::EvalTypeActual) &&
		(eval_purpose != ExprEvalPurpose::EvalType))
		SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->register_source_loc_desc(token_range_to_sld(expr->token_range), sld_index));
	else
		sld_index = UINT32_MAX;

	result_out.reset();

	switch (expr->expr_kind) {
		case ExprKind::Unary:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_unary_expr(compile_env, compilation_context, path_env, expr.cast_to<UnaryExprNode>(), eval_purpose, result_out));
			break;
		case ExprKind::Binary:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_binary_expr(compile_env, compilation_context, path_env, expr.cast_to<BinaryExprNode>(), eval_purpose, result_out));
			break;
		case ExprKind::HeadedIdRef: {
			AstNodePtr<HeadedIdRefExprNode> e = expr.cast_to<HeadedIdRefExprNode>();

			CompileExprResult result(compile_env->allocator.get());

			uint32_t head_register;

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->head, ExprEvalPurpose::RValue, {}, result));

			head_register = result.idx_result_reg_out;

			AstNodePtr<MemberNode> initial_member;

			AstNodePtr<TypeNameNode> tn = result.evaluated_type;

		determine_initial_member:
			switch (tn->tn_kind) {
				case TypeNameKind::Void:
				case TypeNameKind::I8:
				case TypeNameKind::I16:
				case TypeNameKind::I32:
				case TypeNameKind::I64:
				case TypeNameKind::U8:
				case TypeNameKind::U16:
				case TypeNameKind::U32:
				case TypeNameKind::U64:
				case TypeNameKind::F32:
				case TypeNameKind::F64:
				case TypeNameKind::ISize:
				case TypeNameKind::USize:
				case TypeNameKind::String:
				case TypeNameKind::Bool:
				case TypeNameKind::Any:
				case TypeNameKind::Object:
					return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::IdNotFound);
				case TypeNameKind::Custom:
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, resolve_custom_type_name(compile_env, tn->document->shared_from_this(), tn.cast_to<CustomTypeNameNode>(), initial_member));
					break;
				case TypeNameKind::Fn:
				case TypeNameKind::Array:
					return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::IdNotFound);
				case TypeNameKind::Ref:
					tn = tn.cast_to<RefTypeNameNode>()->referenced_type;

					goto determine_initial_member;
				case TypeNameKind::TempRef:
					tn = tn.cast_to<TempRefTypeNameNode>()->referenced_type;

					goto determine_initial_member;
				case TypeNameKind::Bad:
					break;
				default:
					std::terminate();
			}

			if (!initial_member) {
				return CompilationError(e->head->token_range, CompilationErrorKind::IdNotFound);
			}

			AstNodePtr<MemberNode> final_member;

			uint32_t final_register;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(final_register));

			ResolvedIdRefPartList parts(compile_env->document->allocator.get());
			{
				ResolvedIdRefPart initial_part;

				initial_part.is_static = false;
				initial_part.num_entries = 0;
				initial_part.member = initial_member;

				if (!parts.push_back(std::move(initial_part))) {
					return gen_out_of_memory_comp_error();
				}

				peff::Set<AstNodePtr<MemberNode>> walked_member_nodes(compile_env->document->allocator.get());
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
					resolve_id_ref_with_scope_node(
						compile_env,
						compile_env->document,
						walked_member_nodes,
						initial_member,
						e->id_ref_ptr->entries.data(),
						e->id_ref_ptr->entries.size(),
						final_member,
						&parts,
						false));

				if (!final_member) {
					return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::IdNotFound);
				}
			}

			if (!final_member) {
				return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::IdNotFound);
			}

			if (final_member->get_ast_node_type() == AstNodeType::Fn) {
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, select_single_matching_overloading(compile_env, e->id_ref_ptr->token_range, final_member, desired_type, false, result_out));

				AstNodePtr<FnTypeNameNode> fn_type;
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, fn_to_type_name(compile_env, final_member.cast_to<FnOverloadingNode>(), fn_type));

				parts.back().member = final_member;

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _load_rest_of_id_ref(compile_env, compilation_context, path_env, eval_purpose, final_register, result_out, e->id_ref_ptr.get(), parts, head_register, 0, fn_type, sld_index));
			} else {
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _load_rest_of_id_ref(compile_env, compilation_context, path_env, eval_purpose, final_register, result_out, e->id_ref_ptr.get(), parts, head_register, 0, {}, sld_index));
			}

			bool accessible;
			SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, {}, final_member, accessible));
			if (!accessible)
				return CompilationError(
					TokenRange{ compile_env->document->main_module, e->id_ref_ptr->entries.back().name_token_index },
					CompilationErrorKind::MemberIsNotAccessible);

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _determine_node_type(compile_env, path_env, eval_purpose, final_member, result_out.evaluated_type));
			result_out.evaluated_final_member = final_member;
			// TODO: Check if there is this usage while the members are not all initialized.

			switch (eval_purpose) {
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					if (!result_out.evaluated_type) {
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::ExpectingId);
					}
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																			  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(result_out.evaluated_type, b));
					if (!b) {
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::ExpectingLValueExpr);
					}

					result_out.idx_result_reg_out = final_register;
					break;
				}
				case ExprEvalPurpose::RValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(result_out.evaluated_type, b));
					if ((b) && (initial_member != final_member)) {
						uint32_t value_reg_out;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::LVALUE,
								value_reg_out,
								{ slake::Value(slake::ValueType::RegIndex, final_register) }));
						result_out.idx_result_reg_out = value_reg_out;
					} else {
						result_out.idx_result_reg_out = final_register;
					}
					break;
				}
				case ExprEvalPurpose::Call: {
					AstNodePtr<TypeNameNode> decayed_target_type;

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, remove_ref_of_type(result_out.evaluated_type, decayed_target_type));

					if (decayed_target_type->tn_kind != TypeNameKind::Fn) {
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::TargetIsNotCallable);
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(result_out.evaluated_type, b));
					if ((b) && (initial_member != final_member)) {
						uint32_t value_reg_out;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::LVALUE,
								value_reg_out,
								{ slake::Value(slake::ValueType::RegIndex, final_register) }));
						result_out.idx_result_reg_out = value_reg_out;
					} else {
						result_out.idx_result_reg_out = final_register;
					}
					break;
				}
				case ExprEvalPurpose::Unpacking:
					return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::TargetIsNotUnpackable);
				default:
					std::terminate();
			}

			break;
		}
		case ExprKind::IdRef: {
			AstNodePtr<IdRefExprNode> e = expr.cast_to<IdRefExprNode>();

			AstNodePtr<MemberNode> initial_member, final_member;
			IdRefEntry &initial_entry = e->id_ref_ptr->entries.at(0);
			ExprEvalPurpose initial_member_eval_purpose;

			uint32_t initial_member_reg;
			bool is_static = false;

			uint32_t final_register;

			if (!initial_entry.generic_args.size()) {
				if (e->id_ref_ptr->entries.at(0).name == "this") {
					if (!compile_env->this_node)
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::InvalidThisUsage);

					initial_member = compile_env->this_node.cast_to<MemberNode>();

					if (e->id_ref_ptr->entries.size() > 1) {
						initial_member_eval_purpose = ExprEvalPurpose::RValue;
					} else {
						initial_member_eval_purpose = eval_purpose;
					}

					switch (initial_member_eval_purpose) {
						case ExprEvalPurpose::EvalType:
						case ExprEvalPurpose::EvalTypeActual:
							initial_member_reg = UINT32_MAX;
							break;
						case ExprEvalPurpose::LValue:
							return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																					  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
							[[fallthrough]];
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(initial_member_reg));
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LTHIS, initial_member_reg, {}));
							break;
						}
						case ExprEvalPurpose::Unpacking:
							return CompilationError(e->token_range, CompilationErrorKind::TargetIsNotUnpackable);
					}
					goto initial_member_resolved;
				}

				// Check if the entry refers to a local variable.
				if (auto it = compilation_context->lookup_local_var(e->id_ref_ptr->entries.at(0).name);
					it) {
					if (e->id_ref_ptr->entries.size() > 1) {
						initial_member_eval_purpose = ExprEvalPurpose::RValue;
					} else {
						initial_member_eval_purpose = eval_purpose;
					}

					initial_member = it.cast_to<MemberNode>();

					switch (initial_member_eval_purpose) {
						case ExprEvalPurpose::EvalType:
						case ExprEvalPurpose::EvalTypeActual:
							initial_member_reg = UINT32_MAX;
							break;
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																					  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
							[[fallthrough]];
						case ExprEvalPurpose::LValue:
							initial_member_reg = it->idx_reg;
							break;
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(initial_member_reg));
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LVALUE, initial_member_reg, { slake::Value(slake::ValueType::RegIndex, it->idx_reg) }));
							break;
						}
						case ExprEvalPurpose::Unpacking:
							return CompilationError(e->token_range, CompilationErrorKind::TargetIsNotUnpackable);
					}
					goto initial_member_resolved;
				}

				// Check if the entry refers to a function parameter.
				if (auto it = compile_env->cur_overloading->param_indices.find(e->id_ref_ptr->entries.at(0).name);
					it != compile_env->cur_overloading->param_indices.end()) {
					initial_member = compile_env->cur_overloading->params.at(it.value()).cast_to<MemberNode>();

					if (e->id_ref_ptr->entries.size() > 1) {
						initial_member_eval_purpose = ExprEvalPurpose::RValue;
					} else {
						initial_member_eval_purpose = eval_purpose;
					}

					switch (initial_member_eval_purpose) {
						case ExprEvalPurpose::EvalType:
						case ExprEvalPurpose::EvalTypeActual:
							initial_member_reg = UINT32_MAX;
							break;
						case ExprEvalPurpose::LValue: {
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(initial_member_reg));
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LARG, initial_member_reg, { slake::Value((uint32_t)it.value()) }));
							break;
						}
						case ExprEvalPurpose::Stmt:
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																					  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
							[[fallthrough]];
						case ExprEvalPurpose::RValue:
						case ExprEvalPurpose::Call: {
							uint32_t tmp_reg;

							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(tmp_reg));

							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(initial_member_reg));
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LARGV, initial_member_reg, { slake::Value((uint32_t)it.value()) }));

							break;
						}
						case ExprEvalPurpose::Unpacking:
							SLKC_RETURN_IF_COMP_ERROR(compilation_context->alloc_reg(initial_member_reg));
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LAPARG, initial_member_reg, { slake::Value((uint32_t)it.value()) }));

							break;
					}
					goto initial_member_resolved;
				}
			}

		initial_member_resolved:

			if (initial_member) {
				if (e->id_ref_ptr->entries.size() > 1) {
					size_t cur_idx = 1;

					ResolvedIdRefPartList parts(compile_env->document->allocator.get());
					{
						ResolvedIdRefPart initial_part;

						initial_part.is_static = false;
						initial_part.num_entries = 1;
						initial_part.member = initial_member;

						if (!parts.push_back(std::move(initial_part))) {
							return gen_out_of_memory_comp_error();
						}

						peff::Set<AstNodePtr<MemberNode>> walked_member_nodes(compile_env->document->allocator.get());
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							resolve_id_ref_with_scope_node(
								compile_env,
								compile_env->document,
								walked_member_nodes,
								initial_member,
								e->id_ref_ptr->entries.data() + 1,
								e->id_ref_ptr->entries.size() - 1,
								final_member,
								&parts,
								false));

						if (!final_member) {
							return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::IdNotFound);
						}
					}

					if (final_member->get_ast_node_type() == AstNodeType::Fn) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, select_single_matching_overloading(compile_env, e->id_ref_ptr->token_range, final_member, desired_type, false, result_out));

						AstNodePtr<FnTypeNameNode> fn_type;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, fn_to_type_name(compile_env, final_member.cast_to<FnOverloadingNode>(), fn_type));

						parts.back().member = final_member;

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _load_rest_of_id_ref(compile_env, compilation_context, path_env, eval_purpose, final_register, result_out, e->id_ref_ptr.get(), parts, initial_member_reg, 1, fn_type, sld_index));
					} else {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _load_rest_of_id_ref(compile_env, compilation_context, path_env, eval_purpose, final_register, result_out, e->id_ref_ptr.get(), parts, initial_member_reg, 1, {}, sld_index));
					}
				} else {
					final_member = initial_member;
					final_register = initial_member_reg;
				}
			} else {
				size_t cur_idx = 0;

				ResolvedIdRefPartList parts(compile_env->document->allocator.get());
				{
					peff::Set<AstNodePtr<MemberNode>> walked_member_nodes(compile_env->document->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
						resolve_id_ref_with_scope_node(
							compile_env,
							compile_env->document,
							walked_member_nodes,
							compile_env->cur_overloading.cast_to<MemberNode>(),
							e->id_ref_ptr->entries.data(),
							e->id_ref_ptr->entries.size(),
							final_member,
							&parts));
				}

				if (!final_member) {
					return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::IdNotFound);
				}

				if (final_member->get_ast_node_type() == AstNodeType::Fn) {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, select_single_matching_overloading(compile_env, e->id_ref_ptr->token_range, final_member, desired_type, false, result_out));

					AstNodePtr<FnTypeNameNode> fn_type;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, fn_to_type_name(compile_env, final_member.cast_to<FnOverloadingNode>(), fn_type));

					parts.back().member = final_member;

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _load_rest_of_id_ref(compile_env, compilation_context, path_env, eval_purpose, final_register, result_out, e->id_ref_ptr.get(), parts, UINT32_MAX, 0, fn_type, sld_index));
				} else {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _load_rest_of_id_ref(compile_env, compilation_context, path_env, eval_purpose, final_register, result_out, e->id_ref_ptr.get(), parts, UINT32_MAX, 0, {}, sld_index));
				}
			}

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _determine_node_type(compile_env, path_env, eval_purpose, final_member, result_out.evaluated_type));
			result_out.evaluated_final_member = final_member;
			// TODO: Check if there is this usage while the members are not all initialized.

			switch (eval_purpose) {
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					if (!result_out.evaluated_type) {
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::ExpectingId);
					}
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																			  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(result_out.evaluated_type, b));
					if (!b) {
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::ExpectingLValueExpr);
					}

					result_out.idx_result_reg_out = final_register;
					break;
				}
				case ExprEvalPurpose::RValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(result_out.evaluated_type, b));
					if ((b) && (initial_member != final_member)) {
						uint32_t value_reg_out;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::LVALUE,
								value_reg_out,
								{ slake::Value(slake::ValueType::RegIndex, final_register) }));
						result_out.idx_result_reg_out = value_reg_out;
					} else {
						result_out.idx_result_reg_out = final_register;
					}
					break;
				}
				case ExprEvalPurpose::Call: {
					AstNodePtr<TypeNameNode> decayed_target_type;

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, remove_ref_of_type(result_out.evaluated_type, decayed_target_type));

					if (decayed_target_type->tn_kind != TypeNameKind::Fn) {
						return CompilationError(e->id_ref_ptr->token_range, CompilationErrorKind::TargetIsNotCallable);
					}

					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(result_out.evaluated_type, b));
					if ((b) && (initial_member != final_member)) {
						uint32_t value_reg_out;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::LVALUE,
								value_reg_out,
								{ slake::Value(slake::ValueType::RegIndex, final_register) }));
						result_out.idx_result_reg_out = value_reg_out;
					} else {
						result_out.idx_result_reg_out = final_register;
					}
					break;
				}
				case ExprEvalPurpose::Unpacking: {
					result_out.idx_result_reg_out = final_register;
					break;
				}
				default:
					std::terminate();
			}

			break;
		}
		case ExprKind::I8:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<I8LiteralExprNode, int8_t, I8TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYI8, result_out));
			break;
		case ExprKind::I16:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<I16LiteralExprNode, int16_t, I16TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYI16, result_out));
			break;
		case ExprKind::I32:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<I32LiteralExprNode, int32_t, I32TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYI32, result_out));
			break;
		case ExprKind::I64:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<I64LiteralExprNode, int64_t, I64TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYI64, result_out));
			break;
		case ExprKind::U8:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<U8LiteralExprNode, uint8_t, U8TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYU8, result_out));
			break;
		case ExprKind::U16:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<U16LiteralExprNode, uint16_t, U16TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYU16, result_out));
			break;
		case ExprKind::U32:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<U32LiteralExprNode, uint32_t, U32TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYU32, result_out));
			break;
		case ExprKind::U64:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<U64LiteralExprNode, uint64_t, U64TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYU64, result_out));
			break;
		case ExprKind::F32:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<F32LiteralExprNode, float, F32TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYF32, result_out));
			break;
		case ExprKind::F64:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<F64LiteralExprNode, double, F64TypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYF64, result_out));
			break;
		case ExprKind::Bool:
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, _compile_literal_expr<BoolLiteralExprNode, bool, BoolTypeNameNode>(compile_env, compilation_context, path_env, expr, eval_purpose, slake::Opcode::COPYBOOL, result_out));
			break;
		case ExprKind::String: {
			AstNodePtr<StringLiteralExprNode> e = expr.cast_to<StringLiteralExprNode>();

			switch (eval_purpose) {
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																			  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					slake::HostObjectRef<slake::StringObject> sl;

					{
						if (!(sl = slake::StringObject::alloc(compile_env->runtime))) {
							return gen_out_of_runtime_memory_comp_error();
						}

						if (!sl->data.build(e->data)) {
							return gen_out_of_runtime_memory_comp_error();
						}
					}

					uint32_t value_reg_out;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
						compilation_context->emit_ins(
							sld_index,
							slake::Opcode::COPY,
							value_reg_out,
							{ slake::Value(slake::Reference(sl.get())) }));
					result_out.idx_result_reg_out = value_reg_out;
					break;
				}
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
					break;
				case ExprEvalPurpose::Call:
					return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
					break;
				default:
					std::terminate();
			}
			if (!(result_out.evaluated_type = make_ast_node<StringTypeNameNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(),
					  compile_env->document)
						.cast_to<TypeNameNode>())) {
				return gen_out_of_memory_comp_error();
			}
			break;
		}
		case ExprKind::Null: {
			AstNodePtr<NullLiteralExprNode> e = expr.cast_to<NullLiteralExprNode>();

			switch (eval_purpose) {
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																			  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					uint32_t value_reg_out;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
						compilation_context->emit_ins(
							sld_index,
							slake::Opcode::COPYNULL,
							value_reg_out,
							{}));
					result_out.idx_result_reg_out = value_reg_out;
					break;
				}
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
					break;
				case ExprEvalPurpose::Call:
					return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
					break;
				default:
					std::terminate();
			}

			if (!(result_out.evaluated_type = make_ast_node<NullTypeNameNode>(
					  compile_env->allocator.get(),
					  compile_env->allocator.get(),
					  compile_env->document)
						.cast_to<TypeNameNode>())) {
				return gen_out_of_memory_comp_error();
			}
			break;
		}
		case ExprKind::InitializerList: {
			AstNodePtr<InitializerListExprNode> e = expr.cast_to<InitializerListExprNode>();

			AstNodePtr<TypeNameNode> tn;
			if (!desired_type) {
				for (auto i : e->elements) {
					AstNodePtr<TypeNameNode> t;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, i, t));

					if (t) {
						if (tn) {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, infer_common_type(t, tn, tn));
							if (!tn)
								break;
						} else {
							tn = t;
						}
					}
				}

				if (!tn) {
					return CompilationError(expr->token_range, CompilationErrorKind::ErrorDeducingInitializerListType);
				}

				if (!(result_out.evaluated_type = make_ast_node<ArrayTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document, tn).cast_to<TypeNameNode>())) {
					return gen_out_of_memory_comp_error();
				}
			} else {
				switch (desired_type->tn_kind) {
					case TypeNameKind::Array:
						tn = desired_type.cast_to<ArrayTypeNameNode>()->element_type;
						break;
					default:
						return CompilationError(expr->token_range, CompilationErrorKind::InvalidInitializerListUsage);
				}

				if (!(result_out.evaluated_type = desired_type)) {
					return gen_out_of_memory_comp_error();
				}
			}

			switch (eval_purpose) {
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					break;
				case ExprEvalPurpose::Stmt:
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_env->push_warning(
																			  CompilationWarning(e->token_range, CompilationWarningKind::UnusedExprResult)));
					break;
				case ExprEvalPurpose::RValue: {
					slake::TypeRef element_type;

					uint32_t value_reg_out;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_type_name(compile_env, compilation_context, tn, element_type));
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
						compilation_context->emit_ins(
							sld_index,
							slake::Opcode::ARRNEW,
							value_reg_out,
							{ slake::Value(element_type), slake::Value((uint32_t)e->elements.size()) }));

					for (size_t i = 0; i < e->elements.size(); ++i) {
						uint32_t cur_element_slot_reg_index;

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(cur_element_slot_reg_index));

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::AT,
								cur_element_slot_reg_index,
								{ slake::Value(slake::ValueType::RegIndex, value_reg_out), slake::Value((uint32_t)i) }));

						CompileExprResult element_result(compile_env->allocator.get());

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->elements.at(i), ExprEvalPurpose::RValue, tn.cast_to<TypeNameNode>(), element_result));

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::STORE,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegIndex, cur_element_slot_reg_index), slake::Value(slake::ValueType::RegIndex, element_result.idx_result_reg_out) }));
					}
					result_out.idx_result_reg_out = value_reg_out;
					break;
				}
				case ExprEvalPurpose::LValue:
					return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
				case ExprEvalPurpose::Call:
					return CompilationError(expr->token_range, CompilationErrorKind::TargetIsNotCallable);
				default:
					std::terminate();
			}

			break;
		}
		case ExprKind::Call: {
			AstNodePtr<CallExprNode> e = expr.cast_to<CallExprNode>();

			uint32_t target_reg;

			peff::DynArray<AstNodePtr<TypeNameNode>> arg_types(compile_env->allocator.get());

			if (!arg_types.resize(e->args.size())) {
				return gen_out_of_memory_comp_error();
			}

			CompileExprResult result(compile_env->allocator.get());
			AstNodePtr<TypeNameNode> fn_type;
			if (auto error = eval_expr_type(compile_env, compilation_context, path_env, e->target, fn_type); error) {
				switch (error->error_kind) {
					case CompilationErrorKind::OutOfMemory:
					case CompilationErrorKind::OutOfRuntimeMemory:
						return error;
					default:
						break;
				}

				for (size_t i = 0, j = 0; i < e->args.size(); ++i, ++j) {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->args.at(i), arg_types.at(j)));

					if (arg_types.at(j)->tn_kind == TypeNameKind::UnpackedArgs) {
						AstNodePtr<UnpackedArgsTypeNameNode> t = arg_types.at(i).cast_to<UnpackedArgsTypeNameNode>();

						if (!arg_types.resize(arg_types.size() + t->param_types.size() - 1)) {
							return gen_out_of_memory_comp_error();
						}

						--j;
						for (size_t k = 0; k < t->param_types.size(); ++k, ++j) {
							arg_types.at(j) = t->param_types.at(k);
						}

						if (t->has_var_args) {
							// Varidic arguments must be the trailing one.
							if (i + 1 != e->args.size()) {
								return CompilationError(expr->token_range, CompilationErrorKind::CannotBeUnpackedInThisContext);
							}
						}
					}
				}

				AstNodePtr<FnTypeNameNode> fn_prototype;

				if (!(fn_prototype = make_ast_node<FnTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document))) {
					return gen_out_of_memory_comp_error();
				}

				fn_prototype->param_types = std::move(arg_types);
				fn_prototype->is_for_adl = true;

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->target, ExprEvalPurpose::Call, fn_prototype.cast_to<TypeNameNode>(), result));

				target_reg = result.idx_result_reg_out;

				arg_types = std::move(fn_prototype->param_types);
				fn_type = result.evaluated_type;
			} else {
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->target, ExprEvalPurpose::Call, {}, result));

				target_reg = result.idx_result_reg_out;

				fn_type = result.evaluated_type;

				AstNodePtr<FnTypeNameNode> tn = fn_type.cast_to<FnTypeNameNode>();

				for (size_t i = 0, j = 0; i < e->args.size(); ++i, ++j) {
					if (i < tn->param_types.size()) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->args.at(i), arg_types.at(j), tn->param_types.at(i)));
					} else {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->args.at(i), arg_types.at(j), {}));
					}

					if (!arg_types.at(j)) {
						return CompilationError(e->args.at(i)->token_range, CompilationErrorKind::ErrorDeducingArgType);
					}

					// SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, simplify_param_list_type_name_tree(arg_types.at(j), compile_env->allocator.get(), arg_types.at(j)));

					if (arg_types.at(j)->tn_kind == TypeNameKind::UnpackedArgs) {
						AstNodePtr<UnpackedArgsTypeNameNode> t = arg_types.at(i).cast_to<UnpackedArgsTypeNameNode>();

						if (!arg_types.resize(arg_types.size() + t->param_types.size() - 1)) {
							return gen_out_of_memory_comp_error();
						}

						--j;
						for (size_t k = 0; k < t->param_types.size(); ++k, ++j) {
							arg_types.at(j) = t->param_types.at(k);
						}

						if (t->has_var_args) {
							// Varidic arguments must be the trailing one.
							if (i + 1 != e->args.size()) {
								return CompilationError(expr->token_range, CompilationErrorKind::CannotBeUnpackedInThisContext);
							}
						}
					}

					assert(arg_types.at(j));
				}
			}

			if (result.call_target_fn_slot) {
				peff::DynArray<AstNodePtr<FnOverloadingNode>> matched_overloading_indices(compile_env->allocator.get());
				auto matched_overloading = result.call_target_matched_overloadings.back();
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, determine_fn_overloading(compile_env,
																		  result.call_target_fn_slot,
																		  arg_types.data(),
																		  arg_types.size(),
																		  matched_overloading->access_modifier & slake::ACCESS_STATIC,
																		  matched_overloading_indices));
				if (!matched_overloading_indices.size()) {
					return CompilationError(e->token_range, CompilationErrorKind::ArgsMismatched);
				}
			}

			auto tn = fn_type.cast_to<FnTypeNameNode>();

			peff::DynArray<std::pair<AstNodePtr<TypeNameNode>, uint32_t>> arg_passing_info(compile_env->allocator.get());

			for (size_t i = 0; i < e->args.size(); ++i) {
				CompileExprResult arg_result(compile_env->allocator.get());

				if (i < tn->param_types.size()) {
					bool is_same;

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_same_type(arg_types.at(i), tn->param_types.at(i), is_same));
					if (is_same)
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->args.at(i), ExprEvalPurpose::RValue, {}, arg_result));
					else {
						AstNodePtr<CastExprNode> cast_expr;

						SLKC_RETURN_IF_COMP_ERROR(gen_implicit_cast_expr(
							compile_env,
							e->args.at(i),
							tn->param_types.at(i),
							cast_expr));

						if (i < tn->param_types.size()) {
							bool b = false;
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(tn->param_types.at(i), b));

							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, tn->param_types.at(i), arg_result));
						} else {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, cast_expr.cast_to<ExprNode>(), ExprEvalPurpose::RValue, {}, arg_result));
						}
					}
				} else {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->args.at(i), ExprEvalPurpose::RValue, {}, arg_result));
				}

				AstNodePtr<TypeNameNode> arg_type = arg_result.evaluated_type;

				if (!arg_passing_info.push_back({ std::move(arg_type), arg_result.idx_result_reg_out }))
					return gen_out_of_memory_comp_error();

				// SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, simplify_param_list_type_name_tree(arg_result.evaluated_type, compile_env->allocator.get(), arg_type));
			}

			for (size_t i = e->args.size(); i; --i) {
				const auto &passing_info = arg_passing_info.at(i - 1);
				switch (passing_info.first->tn_kind) {
					case TypeNameKind::UnpackedArgs:
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::PUSHAP,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegIndex, passing_info.second) }));
						break;
					default:
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
							compilation_context->emit_ins(
								sld_index,
								slake::Opcode::PUSHARG,
								UINT32_MAX,
								{ slake::Value(slake::ValueType::RegIndex, passing_info.second) }));
				}
			}

			uint32_t this_reg = UINT32_MAX;

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, remove_ref_of_type(fn_type, fn_type));

			if (fn_type.cast_to<FnTypeNameNode>()->this_type) {
				if (result.idx_this_reg_out == UINT32_MAX) {
					if (!e->with_object) {
						return CompilationError(expr->token_range, CompilationErrorKind::MissingBindingObject);
					}

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->with_object, ExprEvalPurpose::RValue, fn_type.cast_to<FnTypeNameNode>()->this_type, result));

					this_reg = result.idx_this_reg_out;
				} else {
					this_reg = result.idx_this_reg_out;
				}
			} else {
				if (e->with_object) {
					return CompilationError(expr->token_range, CompilationErrorKind::RedundantWithObject);
				}
			}

			switch (eval_purpose) {
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					break;
				case ExprEvalPurpose::Stmt:
					if (this_reg != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, target_reg), slake::Value(slake::ValueType::RegIndex, this_reg) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::CALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, target_reg) }));
					}
					break;
				case ExprEvalPurpose::LValue: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(fn_type.cast_to<FnTypeNameNode>()->return_type, b));
					if (!b) {
						return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
					}

					uint32_t value_reg_out;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));

					if (this_reg != UINT32_MAX) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, value_reg_out, { slake::Value(slake::ValueType::RegIndex, target_reg), slake::Value(slake::ValueType::RegIndex, this_reg) }));
					} else {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::CALL, value_reg_out, { slake::Value(slake::ValueType::RegIndex, target_reg) }));
					}

					result_out.idx_result_reg_out = value_reg_out;
					break;
				}
				case ExprEvalPurpose::RValue:
				case ExprEvalPurpose::Call: {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(fn_type.cast_to<FnTypeNameNode>()->return_type, b));

					if (b) {
						uint32_t tmp_reg_index;

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(tmp_reg_index));

						if (result.idx_this_reg_out != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, tmp_reg_index, { slake::Value(slake::ValueType::RegIndex, target_reg), slake::Value(slake::ValueType::RegIndex, result.idx_this_reg_out) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::CALL, tmp_reg_index, { slake::Value(slake::ValueType::RegIndex, target_reg) }));
						}

						uint32_t value_reg_out;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LVALUE, value_reg_out, { slake::Value(slake::ValueType::RegIndex, tmp_reg_index) }));

						result_out.idx_result_reg_out = value_reg_out;
					} else {
						uint32_t value_reg_out;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));

						if (result.idx_this_reg_out != UINT32_MAX) {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::MCALL, value_reg_out, { slake::Value(slake::ValueType::RegIndex, target_reg), slake::Value(slake::ValueType::RegIndex, result.idx_this_reg_out) }));
						} else {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::CALL, value_reg_out, { slake::Value(slake::ValueType::RegIndex, target_reg) }));
						}

						result_out.idx_result_reg_out = value_reg_out;
					}

					break;
				}
			}

			if (auto rt = fn_type.cast_to<FnTypeNameNode>()->return_type; rt) {
				result_out.evaluated_type = fn_type.cast_to<FnTypeNameNode>()->return_type;
			} else {
				if (!(result_out.evaluated_type = make_ast_node<VoidTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document).cast_to<TypeNameNode>())) {
					return gen_out_of_memory_comp_error();
				}
			}
			break;
		}
		case ExprKind::New: {
			AstNodePtr<NewExprNode> e = expr.cast_to<NewExprNode>();

			if (e->target_type->tn_kind != TypeNameKind::Custom) {
				return CompilationError(e->target_type->token_range, CompilationErrorKind::TypeIsNotConstructible);
			}

			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, resolve_custom_type_name(compile_env, compile_env->document, e->target_type.cast_to<CustomTypeNameNode>(), m));

			if (m->get_ast_node_type() != AstNodeType::Class) {
				return CompilationError(e->target_type->token_range, CompilationErrorKind::TypeIsNotConstructible);
			}

			AstNodePtr<ClassNode> c = m.cast_to<ClassNode>();

			slake::TypeRef type;
			{
				IdRefPtr full_id_ref;
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, get_full_id_ref(compile_env->allocator.get(), c.cast_to<MemberNode>(), full_id_ref));

				auto tn = make_ast_node<CustomTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document);

				if (!tn) {
					return gen_out_of_memory_comp_error();
				}
				tn->context_node = to_weak_ptr(compile_env->document->root_module.cast_to<MemberNode>());

				tn->id_ref_ptr = std::move(full_id_ref);

				tn->token_range = e->target_type->token_range;

				auto e = compile_type_name(compile_env, compilation_context, tn.cast_to<TypeNameNode>(), type);
				/* if (e) {
					std::terminate();
				}*/
			}

			uint32_t value_reg_out;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::NEW, value_reg_out, { slake::Value(type) }));

			if (auto it = c->member_indices.find("new"); it != c->member_indices.end()) {
				if (c->members.at(it.value())->get_ast_node_type() != AstNodeType::Fn) {
					return CompilationError(e->target_type->token_range, CompilationErrorKind::TypeIsNotConstructible);
				}
				AstNodePtr<FnNode> constructor = c->members.at(it.value()).cast_to<FnNode>();

				peff::DynArray<AstNodePtr<TypeNameNode>> arg_types(compile_env->allocator.get());

				if (!arg_types.resize(e->args.size())) {
					return gen_out_of_memory_comp_error();
				}

				AstNodePtr<FnOverloadingNode> overloading;
				if (constructor->overloadings.size() == 1) {
					overloading = constructor->overloadings.back();

					for (size_t i = 0; i < e->args.size(); ++i) {
						if (i < overloading->params.size()) {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->args.at(i), arg_types.at(i), overloading->params.at(i)->type));
						} else {
							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->args.at(i), arg_types.at(i), {}));
						}
					}

					peff::DynArray<AstNodePtr<FnOverloadingNode>> matched_overloading_indices(compile_env->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, determine_fn_overloading(compile_env,
																			  constructor,
																			  arg_types.data(),
																			  arg_types.size(),
																			  false,
																			  matched_overloading_indices));
					if (!matched_overloading_indices.size()) {
						return CompilationError(e->token_range, CompilationErrorKind::ArgsMismatched);
					}
				} else {
					for (size_t i = 0; i < e->args.size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->args.at(i), arg_types.at(i), {}));
					}

					peff::DynArray<AstNodePtr<FnOverloadingNode>> matched_overloading_indices(compile_env->allocator.get());
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, determine_fn_overloading(compile_env,
																			  constructor,
																			  arg_types.data(),
																			  arg_types.size(),
																			  false,
																			  matched_overloading_indices));
					switch (matched_overloading_indices.size()) {
						case 0:
							return CompilationError(e->token_range, CompilationErrorKind::NoMatchingFnOverloading);
						case 1:
							break;
						default:
							return CompilationError(e->token_range, CompilationErrorKind::UnableToDetermineOverloading);
					}

					overloading = matched_overloading_indices.back();

					bool accessible;
					SLKC_RETURN_IF_COMP_ERROR(is_member_accessible(compile_env, {}, overloading.cast_to<MemberNode>(), accessible));
					if (!accessible)
						return CompilationError(
							TokenRange{ compile_env->document->main_module, e->target_type->token_range },
							CompilationErrorKind::MemberIsNotAccessible);
				}

				slake::HostObjectRef<slake::IdRefObject> id_ref_object;
				{
					IdRefPtr full_id_ref;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, get_full_id_ref(compile_env->allocator.get(), constructor.cast_to<MemberNode>(), full_id_ref));

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_id_ref(compile_env, compilation_context, full_id_ref->entries.data(), full_id_ref->entries.size(), nullptr, 0, false, {}, id_ref_object));

					id_ref_object->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

					if (!id_ref_object->param_types->resize(overloading->params.size()))
						return gen_out_of_runtime_memory_comp_error();

					for (size_t i = 0; i < overloading->params.size(); ++i) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_type_name(compile_env, compilation_context, overloading->params.at(i)->type, id_ref_object->param_types->at(i)));
					}

					if (overloading->fn_flags & FN_VARG)
						id_ref_object->has_var_args = true;
				}

				uint32_t ctor_call_target;

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(ctor_call_target));

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::LOAD, ctor_call_target, { slake::Value(slake::Reference(id_ref_object.get())) }));

				for (size_t i = 0; i < e->args.size(); ++i) {
					CompileExprResult arg_result(compile_env->allocator.get());

					if (i < overloading->params.size()) {
						bool b = false;
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(overloading->params.at(i)->type, b));

						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->args.at(i), b ? ExprEvalPurpose::LValue : ExprEvalPurpose::RValue, overloading->params.at(i)->type, arg_result));
					} else {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->args.at(i), ExprEvalPurpose::RValue, {}, arg_result));
					}

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error,
						compilation_context->emit_ins(
							sld_index,
							slake::Opcode::PUSHARG,
							UINT32_MAX,
							{ slake::Value(slake::ValueType::RegIndex, arg_result.idx_result_reg_out) }));
				}

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::CTORCALL, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, ctor_call_target), slake::Value(slake::ValueType::RegIndex, value_reg_out) }));
			} else {
			}

			result_out.idx_result_reg_out = value_reg_out;
			result_out.evaluated_type = e->target_type;
			break;
		}
		case ExprKind::Cast: {
			AstNodePtr<CastExprNode> e = expr.cast_to<CastExprNode>();

			AstNodePtr<TypeNameNode> expr_type, target_type = e->target_type;

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, e->source, expr_type, target_type));

			bool b;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_convertible(expr_type, target_type, false, b));

			if (!b) {
				return CompilationError(e->source->token_range, CompilationErrorKind::InvalidCast);
			}

			bool left_value;

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(target_type, left_value));

			AstNodePtr<TypeNameNode> decayed_expr_type;

			bool expr_left_value;

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(expr_type, expr_left_value));

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, remove_ref_of_type(expr_type, decayed_expr_type));

			bool same_type;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_same_type(decayed_expr_type, target_type, same_type));
			if (!same_type) {
				CompileExprResult result(compile_env->allocator.get());
				if (!left_value) {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->source, ExprEvalPurpose::RValue, {}, result));
				} else {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->source, ExprEvalPurpose::LValue, {}, result));
				}

				uint32_t idx_reg = result.idx_result_reg_out;

				slake::TypeRef type;
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_type_name(compile_env, compilation_context, target_type, type));

				uint32_t value_reg_out;
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::CAST, value_reg_out, { slake::Value(type), slake::Value(slake::ValueType::RegIndex, idx_reg) }));

				result_out.evaluated_type = target_type;

				// Convert to subtypes may fail.
				bool subtype;
				if (is_subtype_of(target_type, decayed_expr_type, subtype)) {
					if (!(result_out.evaluated_type = result_out.evaluated_type->duplicate<TypeNameNode>(compile_env->allocator.get())))
						return gen_out_of_memory_comp_error();
					result_out.evaluated_type->is_nullable = true;
				}

				result_out.idx_result_reg_out = value_reg_out;
			} else {
				if (!left_value) {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->source, ExprEvalPurpose::RValue, {}, result_out));
				} else {
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->source, ExprEvalPurpose::LValue, {}, result_out));
				}
				result_out.evaluated_type = target_type;
			}

			break;
		}
		case ExprKind::Match: {
			AstNodePtr<MatchExprNode> e = expr.cast_to<MatchExprNode>();

			AstNodePtr<TypeNameNode> return_type = e->return_type;

			// Infer common return type from the cases.
			if (!return_type) {
				if (desired_type)
					return_type = desired_type;
				else {
					AstNodePtr<TypeNameNode> common_type, t;

					for (auto &i : e->cases) {
						SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, i.second, t));

						if (t) {
							if (common_type) {
								SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, infer_common_type(t, common_type, common_type));
								if (!common_type)
									break;
							} else {
								common_type = t;
							}
						}
					}

					if (!common_type)
						return CompilationError(expr->token_range, CompilationErrorKind::ErrorDeducingMatchResultType);
					return_type = common_type;
				}
			}

			// Check if the return value requires an L-Value expression.
			bool is_lvalue;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_lvalue_type(return_type, is_lvalue));
			switch (eval_purpose) {
				case ExprEvalPurpose::LValue:
					if (!is_lvalue)
						return CompilationError(expr->token_range, CompilationErrorKind::ExpectingLValueExpr);
					break;
				case ExprEvalPurpose::EvalType:
				case ExprEvalPurpose::EvalTypeActual:
					result_out.evaluated_type = return_type;
					return {};
				default:
					break;
			}

			// Compile the match condition.
			uint32_t condition_reg;

			CompileExprResult result(compile_env->allocator.get());
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, e->condition, ExprEvalPurpose::RValue, {}, result));
			condition_reg = result.idx_result_reg_out;
			if (!result.evaluated_type)
				return CompilationError(expr->token_range, CompilationErrorKind::ErrorDeducingMatchConditionType);

			AstNodePtr<TypeNameNode> cond_type = result.evaluated_type;

			// Allocate the end label.
			uint32_t end_label;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_label(end_label));

			uint32_t default_label;

			bool is_default_set = false;

			// Key = jump source, value = value register
			peff::DynArray<uint32_t> match_value_eval_labels(compile_env->allocator.get());
			peff::DynArray<std::pair<uint32_t, uint32_t>> phi_register_value_map(compile_env->allocator.get());

			if (!match_value_eval_labels.resize(e->cases.size())) {
				return gen_out_of_memory_comp_error();
			}

			if (!phi_register_value_map.resize(e->cases.size())) {
				return gen_out_of_memory_comp_error();
			}

			size_t idx_default_branch_case;

			// Check and prepare for each case.
			for (size_t i = 0; i < e->cases.size(); ++i) {
				auto &cur_case = e->cases.at(i);

				uint32_t eval_value_label;
				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_label(eval_value_label));

				if (cur_case.first) {
					// Evaluate the case conition as a comptime expression.
					AstNodePtr<ExprNode> evaluated_case_cond_expr;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_const_expr(compile_env, compilation_context, path_env, cur_case.first, evaluated_case_cond_expr));
					if (!evaluated_case_cond_expr) {
						return CompilationError(cur_case.first->token_range, CompilationErrorKind::ErrorEvaluatingConstMatchCaseCondition);
					}

					// Try to infer the case condition's type.
					AstNodePtr<TypeNameNode> case_cond_expr_type;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, eval_expr_type(compile_env, compilation_context, path_env, evaluated_case_cond_expr, case_cond_expr_type));
					if (!case_cond_expr_type) {
						return CompilationError(cur_case.first->token_range, CompilationErrorKind::MismatchedMatchCaseConditionType);
					}

					// Check if the case condition's type matches the match condition's type.
					bool b;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, is_same_type(cond_type, case_cond_expr_type, b));
					if (!b)
						return CompilationError(cur_case.first->token_range, CompilationErrorKind::MismatchedMatchCaseConditionType);

					uint32_t cmp_result_reg;
					{
						AstNodePtr<BinaryExprNode> cmp_expr;

						AstNodePtr<RegIndexExprNode> reg_index_expr;
						if (!(reg_index_expr = make_ast_node<RegIndexExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document, condition_reg, cond_type))) {
							return gen_out_of_memory_comp_error();
						}
						reg_index_expr->token_range = cur_case.first->token_range;

						AstNodePtr<BoolTypeNameNode> desired_bool_type_name;
						if (!(desired_bool_type_name = make_ast_node<BoolTypeNameNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document)))
							return gen_out_of_memory_comp_error();

						SLKC_RETURN_IF_COMP_ERROR(gen_binary_op_expr(compile_env, BinaryOp::Eq, reg_index_expr.cast_to<ExprNode>(), cur_case.first, cur_case.first->token_range, cmp_expr));

						{
							CompileExprResult cmp_expr_result(compile_env->allocator.get());

							SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, path_env, cmp_expr.cast_to<ExprNode>(), ExprEvalPurpose::RValue, desired_bool_type_name.cast_to<TypeNameNode>(), cmp_expr_result));

							cmp_result_reg = cmp_expr_result.idx_result_reg_out;
						}
					}

					uint32_t succeeded_value_label;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_label(succeeded_value_label));

					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::BR, UINT32_MAX, { slake::Value(slake::ValueType::RegIndex, cmp_result_reg), slake::Value(slake::ValueType::Label, eval_value_label), slake::Value(slake::ValueType::Label, succeeded_value_label) }));

					compilation_context->set_label_offset(succeeded_value_label, compilation_context->get_cur_ins_off());
				} else {
					if (is_default_set)
						return CompilationError(cur_case.first->token_range, CompilationErrorKind::DuplicatedMatchCaseBranch);

					default_label = eval_value_label;

					idx_default_branch_case = i;

					is_default_set = true;
				}

				match_value_eval_labels.at(i) = eval_value_label;
			}

			if (!is_default_set)
				return CompilationError(e->token_range, CompilationErrorKind::MissingDefaultMatchCaseBranch);

			peff::DynArray<PathEnv> body_path_envs(compile_env->allocator.get());

			if (!body_path_envs.resize_uninit(e->cases.size()))
				return gen_out_of_memory_comp_error();

			for (size_t i = 0; i < body_path_envs.size(); ++i)
				peff::construct_at<PathEnv>(&body_path_envs.at(i), compile_env->allocator.get());

			for (size_t i = 0; i < e->cases.size(); ++i) {
				auto &cur_case = e->cases.at(i);

				compilation_context->set_label_offset(match_value_eval_labels.at(i), compilation_context->get_cur_ins_off());

				uint32_t expr_value_register;

				{
					PathEnv &body_path_env = body_path_envs.at(i);
					body_path_env.set_parent(path_env);
					body_path_env.exec_possibility = PathPossibility::May;	 // TODO: Check if the path will always reached hit after an assignment (if exists).
					body_path_env.no_return_possibility = PathPossibility::May;
					body_path_env.break_possibility = PathPossibility::Never;
					SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compile_expr(compile_env, compilation_context, &body_path_env, cur_case.second, eval_purpose, return_type, result));
				}

				expr_value_register = result.idx_result_reg_out;

				if (i == idx_default_branch_case)
					phi_register_value_map.at(idx_default_branch_case) = { UINT32_MAX, expr_value_register };
				else
					phi_register_value_map.at(i) = { compilation_context->get_cur_ins_off(), expr_value_register };

				SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::JMP, UINT32_MAX, { slake::Value(slake::ValueType::Label, end_label) }));
			}

			SLKC_RETURN_IF_COMP_ERROR(combine_parallel_path_env(compile_env->allocator.get(), compile_env, compilation_context, *path_env, body_path_envs.data(), body_path_envs.size()));

			compilation_context->set_label_offset(end_label, compilation_context->get_cur_ins_off());

			peff::DynArray<slake::Value> operands(compile_env->allocator.get());

			if (!operands.resize(phi_register_value_map.size() * 2)) {
				return gen_out_of_memory_comp_error();
			}

			for (size_t i = 0, j = 0; i < phi_register_value_map.size(); ++i, j += 2) {
				operands.at(j) = phi_register_value_map.at(i).first;
				operands.at(j + 1) = slake::Value(slake::ValueType::RegIndex, phi_register_value_map.at(i).second);
			}

			uint32_t value_reg_out;
			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->alloc_reg(value_reg_out));

			SLKC_RETURN_IF_COMP_ERROR_WITH_LVAR(compilation_error, compilation_context->emit_ins(sld_index, slake::Opcode::PHI, value_reg_out, operands.data(), operands.size()));
			result_out.idx_result_reg_out = value_reg_out;

			result_out.evaluated_type = return_type;
			break;
		}
		case ExprKind::Wrapper:
			return compile_expr(compile_env, compilation_context, path_env, expr.cast_to<WrapperExprNode>()->target, eval_purpose, desired_type, result_out);
		case ExprKind::RegIndex: {
			auto e = expr.cast_to<RegIndexExprNode>();

			result_out.idx_result_reg_out = e->reg;
			result_out.evaluated_type = e->type;

			break;
		}
		default:
			std::terminate();
	}

	return {};
}
