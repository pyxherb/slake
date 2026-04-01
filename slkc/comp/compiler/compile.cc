#include "../compiler.h"

using namespace slkc;

#undef max

SLKC_API peff::Option<CompilationError> slkc::compile_type_name(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	AstNodePtr<TypeNameNode> type_name,
	slake::TypeRef &type_out) {
	type_out = slake::TypeId::Void;

	switch (type_name->tn_kind) {
		case TypeNameKind::Void:
			type_out = slake::TypeRef(slake::TypeId::Void);
			break;
		case TypeNameKind::I8:
			type_out = slake::TypeRef(slake::TypeId::I8);
			break;
		case TypeNameKind::I16:
			type_out = slake::TypeRef(slake::TypeId::I16);
			break;
		case TypeNameKind::I32:
			type_out = slake::TypeRef(slake::TypeId::I32);
			break;
		case TypeNameKind::I64:
			type_out = slake::TypeRef(slake::TypeId::I64);
			break;
		case TypeNameKind::U8:
			type_out = slake::TypeRef(slake::TypeId::U8);
			break;
		case TypeNameKind::U16:
			type_out = slake::TypeRef(slake::TypeId::U16);
			break;
		case TypeNameKind::U32:
			type_out = slake::TypeRef(slake::TypeId::U32);
			break;
		case TypeNameKind::U64:
			type_out = slake::TypeRef(slake::TypeId::U64);
			break;
		case TypeNameKind::F32:
			type_out = slake::TypeRef(slake::TypeId::F32);
			break;
		case TypeNameKind::F64:
			type_out = slake::TypeRef(slake::TypeId::F64);
			break;
		case TypeNameKind::String:
			type_out = slake::TypeRef(slake::TypeId::String);
			break;
		case TypeNameKind::Bool:
			type_out = slake::TypeRef(slake::TypeId::Bool);
			break;
		case TypeNameKind::Object:
			type_out = slake::TypeRef(slake::TypeId::Instance, nullptr);
			break;
		case TypeNameKind::Any:
			type_out = slake::TypeRef(slake::TypeId::Any);
			break;
		case TypeNameKind::BCCustom: {
			AstNodePtr<bc::BCCustomTypeNameNode> t = type_name.cast_to<bc::BCCustomTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::CustomTypeDefObject> type_def;

			if (!(type_def = slake::CustomTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			slake::HostObjectRef<slake::IdRefObject> obj;

			SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, t->id_ref_ptr->entries.data(), t->id_ref_ptr->entries.size(), nullptr, 0, false, {}, obj));

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			type_def->type_object = obj.get();

			type_out = slake::TypeRef(slake::TypeId::Instance, type_def.get());
			break;
		}
		case TypeNameKind::Custom: {
			AstNodePtr<CustomTypeNameNode> t = type_name.cast_to<CustomTypeNameNode>();
			peff::SharedPtr<Document> doc = t->document->shared_from_this();
			AstNodePtr<MemberNode> m;

			SLKC_RETURN_IF_COMP_ERROR(resolve_custom_type_name(compile_env, doc, t, m));

			if (!m) {
				return CompilationError(type_name->token_range, CompilationErrorKind::ExpectingTypeName);
			}

			switch (m->get_ast_node_type()) {
				case AstNodeType::Class:
				case AstNodeType::Interface: {
					slake::HostObjectRef<slake::CustomTypeDefObject> type_def;

					if (!(type_def = slake::CustomTypeDefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					IdRefPtr full_name;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(doc->allocator.get(), m, full_name));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
						return gen_out_of_memory_comp_error();
					}

					type_def->type_object = obj.get();

					type_out = slake::TypeRef(slake::TypeId::Instance, type_def.get());
					break;
				}
				case AstNodeType::Struct: {
					slake::HostObjectRef<slake::CustomTypeDefObject> type_def;

					if (!(type_def = slake::CustomTypeDefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					IdRefPtr full_name;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(doc->allocator.get(), m, full_name));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
						return gen_out_of_memory_comp_error();
					}

					type_def->type_object = obj.get();

					type_out = slake::TypeRef(slake::TypeId::StructInstance, type_def.get());
					break;
				}
				case AstNodeType::ScopedEnum: {
					slake::HostObjectRef<slake::CustomTypeDefObject> type_def;

					if (!(type_def = slake::CustomTypeDefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					IdRefPtr full_name;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(doc->allocator.get(), m, full_name));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
						return gen_out_of_memory_comp_error();
					}

					type_def->type_object = obj.get();

					type_out = slake::TypeRef(m.cast_to<ScopedEnumNode>()->base_type ? slake::TypeId::ScopedEnum : slake::TypeId::TypelessScopedEnum, type_def.get());
					break;
				}
				case AstNodeType::UnionEnum: {
					slake::HostObjectRef<slake::CustomTypeDefObject> type_def;

					if (!(type_def = slake::CustomTypeDefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					IdRefPtr full_name;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(doc->allocator.get(), m, full_name));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
						return gen_out_of_memory_comp_error();
					}

					type_def->type_object = obj.get();

					type_out = slake::TypeRef(slake::TypeId::UnionEnum, type_def.get());
					break;
				}
				case AstNodeType::UnionEnumItem: {
					slake::HostObjectRef<slake::CustomTypeDefObject> type_def;

					if (!(type_def = slake::CustomTypeDefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					IdRefPtr full_name;

					SLKC_RETURN_IF_COMP_ERROR(get_full_id_ref(doc->allocator.get(), m, full_name));

					slake::HostObjectRef<slake::IdRefObject> obj;

					SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, compilation_context, full_name->entries.data(), full_name->entries.size(), nullptr, 0, false, {}, obj));

					if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
						return gen_out_of_memory_comp_error();
					}

					type_def->type_object = obj.get();

					type_out = slake::TypeRef(slake::TypeId::UnionEnumItem, type_def.get());
					break;
				}
				case AstNodeType::GenericParam: {
					slake::HostObjectRef<slake::GenericArgTypeDefObject> type_def;

					if (!(type_def = slake::GenericArgTypeDefObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					slake::HostObjectRef<slake::StringObject> obj;

					if (!(obj = slake::StringObject::alloc(compile_env->runtime))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (!obj->data.build(m->name)) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
						return gen_out_of_memory_comp_error();
					}

					type_def->owner_object = nullptr;
					type_def->name_object = obj.get();

					type_out = slake::TypeRef(slake::TypeId::GenericArg, type_def.get());
					break;
				}
				default:
					return CompilationError(type_name->token_range, CompilationErrorKind::ExpectingTypeName);
			}

			break;
		}
		case TypeNameKind::Array: {
			AstNodePtr<ArrayTypeNameNode> t = type_name.cast_to<ArrayTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::ArrayTypeDefObject> type_def;

			if (!(type_def = slake::ArrayTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			slake::HostObjectRef<slake::HeapTypeObject> obj;

			if (!(obj = slake::HeapTypeObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, t->element_type, obj->type_ref));

			if (!(compile_env->host_ref_holder.add_object(type_def.get()))) {
				return gen_out_of_memory_comp_error();
			}

			type_def->element_type = obj.get();

			type_out = slake::TypeRef(slake::TypeId::Array, type_def.get());
			break;
		}
		case TypeNameKind::Ref: {
			AstNodePtr<RefTypeNameNode> t = type_name.cast_to<RefTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::RefTypeDefObject> type_def;

			if (!(type_def = slake::RefTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			if (!(compile_env->host_ref_holder.add_object(type_def.get()))) {
				return gen_out_of_memory_comp_error();
			}

			slake::HostObjectRef<slake::HeapTypeObject> obj;

			if (!(obj = slake::HeapTypeObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, t->referenced_type, obj->type_ref));

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			type_def->referenced_type = obj.get();

			type_out = slake::TypeRef(slake::TypeId::Ref, type_def.get());
			break;
		}
		case TypeNameKind::Tuple: {
			AstNodePtr<TupleTypeNameNode> t = type_name.cast_to<TupleTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::TupleTypeDefObject> obj;

			if (!(obj = slake::TupleTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			if (!obj->element_types.resize(t->element_types.size())) {
				return gen_out_of_runtime_memory_comp_error();
			}

			for (size_t i = 0; i < t->element_types.size(); ++i) {
				slake::HostObjectRef<slake::HeapTypeObject> heap_type;

				if (!(heap_type = slake::HeapTypeObject::alloc(compile_env->runtime)))
					return gen_out_of_runtime_memory_comp_error();

				SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, t->element_types.at(i), heap_type->type_ref));

				if (!(compile_env->host_ref_holder.add_object(heap_type.get())))
					return gen_out_of_memory_comp_error();

				obj->element_types.at(i) = heap_type.get();
			}

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			type_out = slake::TypeRef(slake::TypeId::Tuple, obj.get());
			break;
		}
		case TypeNameKind::SIMD: {
			AstNodePtr<SIMDTypeNameNode> t = type_name.cast_to<SIMDTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::SIMDTypeDefObject> obj;

			if (!(obj = slake::SIMDTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			slake::HostObjectRef<slake::HeapTypeObject> heap_type;

			if (!(heap_type = slake::HeapTypeObject::alloc(compile_env->runtime)))
				return gen_out_of_runtime_memory_comp_error();

			SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, t->element_type, heap_type->type_ref));

			if (!(compile_env->host_ref_holder.add_object(heap_type.get())))
				return gen_out_of_memory_comp_error();

			AstNodePtr<ExprNode> width;

			PathEnv path_env(compile_env->allocator.get());

			SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, &path_env, t->width, width));

			if (!width) {
				return CompilationError(t->width->token_range, CompilationErrorKind::RequiresCompTimeExpr);
			}

			if (width->expr_kind != ExprKind::U32) {
				AstNodePtr<CastExprNode> ce;

				if (!(ce = make_ast_node<CastExprNode>(t->document->allocator.get(), t->document->allocator.get(), t->document->shared_from_this()))) {
					return gen_out_of_memory_comp_error();
				}

				AstNodePtr<U32TypeNameNode> u32_type;

				if (!(u32_type = make_ast_node<U32TypeNameNode>(t->document->allocator.get(), t->document->allocator.get(), t->document->shared_from_this()))) {
					return gen_out_of_memory_comp_error();
				}

				ce->source = width;
				ce->target_type = u32_type.cast_to<TypeNameNode>();

				PathEnv path_env(compile_env->allocator.get());

				SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, compilation_context, &path_env, ce.cast_to<ExprNode>(), width));

				if (!width) {
					return CompilationError(t->width->token_range, CompilationErrorKind::TypeArgTypeMismatched);
				}
			}

			obj->type = heap_type.get();
			obj->width = width.cast_to<U32LiteralExprNode>()->data;

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			type_out = slake::TypeRef(slake::TypeId::SIMD, obj.get());
			break;
		}
		case TypeNameKind::ParamTypeList: {
			AstNodePtr<ParamTypeListTypeNameNode> t = type_name.cast_to<ParamTypeListTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::ParamTypeListTypeDefObject> obj;

			if (!(obj = slake::ParamTypeListTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			if (!obj->param_types.resize(t->param_types.size())) {
				return gen_out_of_runtime_memory_comp_error();
			}
			for (size_t i = 0; i < obj->param_types.size(); ++i) {
				slake::HostObjectRef<slake::HeapTypeObject> heap_type;

				if (!(heap_type = slake::HeapTypeObject::alloc(compile_env->runtime)))
					return gen_out_of_runtime_memory_comp_error();

				SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, t->param_types.at(i), heap_type->type_ref));

				if (!(compile_env->host_ref_holder.add_object(heap_type.get())))
					return gen_out_of_memory_comp_error();

				obj->param_types.at(i) = heap_type.get();
			}

			obj->has_var_arg = t->has_var_args;

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			type_out = slake::TypeRef(slake::TypeId::ParamTypeList, obj.get());
			break;
		}
		case TypeNameKind::Unpacking: {
			AstNodePtr<UnpackingTypeNameNode> t = type_name.cast_to<UnpackingTypeNameNode>();
			AstNodePtr<Document> doc = t->document->shared_from_this();

			slake::HostObjectRef<slake::UnpackingTypeDefObject> obj;

			if (!(obj = slake::UnpackingTypeDefObject::alloc(compile_env->runtime))) {
				return gen_out_of_runtime_memory_comp_error();
			}

			slake::HostObjectRef<slake::HeapTypeObject> heap_type;

			if (!(heap_type = slake::HeapTypeObject::alloc(compile_env->runtime)))
				return gen_out_of_runtime_memory_comp_error();

			SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, t->inner_type_name, heap_type->type_ref));

			if (!(compile_env->host_ref_holder.add_object(heap_type.get())))
				return gen_out_of_memory_comp_error();

			if (!(compile_env->host_ref_holder.add_object(obj.get()))) {
				return gen_out_of_memory_comp_error();
			}

			obj->type = heap_type.get();

			type_out = slake::TypeRef(slake::TypeId::Unpacking, obj.get());
			break;
		}
		default:
			std::terminate();
	}

	if (type_name->is_final)
		type_out.set_final();
	if (type_name->is_local)
		type_out.set_local();
	if (type_name->is_nullable)
		type_out.set_nullable();

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_id_ref(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	const IdRefEntry *entries,
	size_t num_entries,
	AstNodePtr<TypeNameNode> *param_types,
	size_t num_params,
	bool has_var_args,
	AstNodePtr<TypeNameNode> overriden_type,
	slake::HostObjectRef<slake::IdRefObject> &id_ref_out) {
	slake::HostObjectRef<slake::IdRefObject> id;
	assert(num_entries);

	if (!(id = slake::IdRefObject::alloc(compile_env->runtime))) {
		return gen_out_of_runtime_memory_comp_error();
	}
	if (!id->entries.resize_uninit(num_entries)) {
		return gen_out_of_runtime_memory_comp_error();
	}

	for (size_t i = 0; i < id->entries.size(); ++i) {
		peff::construct_at<slake::IdRefEntry>(&id->entries.at(i), slake::IdRefEntry(compile_env->runtime->get_cur_gen_alloc()));
	}

	for (size_t i = 0; i < num_entries; ++i) {
		const IdRefEntry &ce = entries[i];
		slake::IdRefEntry &e = id->entries.at(i);

		if (!e.name.build(ce.name)) {
			return gen_out_of_runtime_memory_comp_error();
		}

		if (!e.generic_args.resize(ce.generic_args.size())) {
			return gen_out_of_runtime_memory_comp_error();
		}

		for (size_t i = 0; i < ce.generic_args.size(); ++i) {
			slake::Value value;
			if (ce.generic_args.at(i)->get_ast_node_type() == AstNodeType::Expr) {
				SLKC_RETURN_IF_COMP_ERROR(compile_value_expr(compile_env, compilation_context, ce.generic_args.at(i).cast_to<ExprNode>(), value));
			} else {
				slake::TypeRef type_ref;
				SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, ce.generic_args.at(i).cast_to<TypeNameNode>(), type_ref));
				assert(type_ref.type_id != slake::TypeId::Invalid);
				value = type_ref;
			}
			assert(value != slake::ValueType::Invalid);
			e.generic_args.at(i) = value;
		}
	}

	if (param_types) {
		id->param_types = peff::DynArray<slake::TypeRef>(compile_env->runtime->get_cur_gen_alloc());

		if (!id->param_types->resize(num_params)) {
			return gen_out_of_memory_comp_error();
		}

		for (size_t i = 0; i < num_params; ++i) {
			SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, param_types[i], id->param_types->at(i)));
		}
	}

	id->has_var_args = has_var_args;

	id->overriden_type = slake::TypeId::Void;

	if (overriden_type) {
		SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, overriden_type, id->overriden_type));
	}

	id_ref_out = id;

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_value_expr(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	AstNodePtr<ExprNode> expr,
	slake::Value &value_out) {
	switch (expr->expr_kind) {
		case ExprKind::IdRef: {
			AstNodePtr<IdRefExprNode> e = expr.cast_to<IdRefExprNode>();
			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(
				compile_id_ref(
					compile_env,
					compilation_context,
					e->id_ref_ptr->entries.data(),
					e->id_ref_ptr->entries.size(),
					nullptr,
					0,
					false,
					{},
					id));

			if (!compile_env->host_ref_holder.add_object(id.get())) {
				return gen_out_of_memory_comp_error();
			}

			slake::Reference entity_ref = slake::Reference(id.get());

			value_out = slake::Value(entity_ref);
			break;
		}
		case ExprKind::I8: {
			AstNodePtr<I8LiteralExprNode> e = expr.cast_to<I8LiteralExprNode>();

			value_out = slake::Value((int8_t)e->data);
			break;
		}
		case ExprKind::I16: {
			AstNodePtr<I16LiteralExprNode> e = expr.cast_to<I16LiteralExprNode>();

			value_out = slake::Value((int16_t)e->data);
			break;
		}
		case ExprKind::I32: {
			AstNodePtr<I32LiteralExprNode> e = expr.cast_to<I32LiteralExprNode>();

			value_out = slake::Value((int32_t)e->data);
			break;
		}
		case ExprKind::I64: {
			AstNodePtr<I64LiteralExprNode> e = expr.cast_to<I64LiteralExprNode>();

			value_out = slake::Value((int64_t)e->data);
			break;
		}
		case ExprKind::U8: {
			AstNodePtr<U8LiteralExprNode> e = expr.cast_to<U8LiteralExprNode>();

			value_out = slake::Value((uint8_t)e->data);
			break;
		}
		case ExprKind::U16: {
			AstNodePtr<U16LiteralExprNode> e = expr.cast_to<U16LiteralExprNode>();

			value_out = slake::Value((uint16_t)e->data);
			break;
		}
		case ExprKind::U32: {
			AstNodePtr<U32LiteralExprNode> e = expr.cast_to<U32LiteralExprNode>();

			value_out = slake::Value((uint32_t)e->data);
			break;
		}
		case ExprKind::U64: {
			AstNodePtr<U64LiteralExprNode> e = expr.cast_to<U64LiteralExprNode>();

			value_out = slake::Value((uint64_t)e->data);
			break;
		}
		case ExprKind::F32: {
			AstNodePtr<F32LiteralExprNode> e = expr.cast_to<F32LiteralExprNode>();

			value_out = slake::Value(e->data);
			break;
		}
		case ExprKind::F64: {
			AstNodePtr<F64LiteralExprNode> e = expr.cast_to<F64LiteralExprNode>();

			value_out = slake::Value(e->data);
			break;
		}
		case ExprKind::String: {
			AstNodePtr<StringLiteralExprNode> e = expr.cast_to<StringLiteralExprNode>();

			slake::HostObjectRef<slake::StringObject> s;

			if (!(s = slake::StringObject::alloc(compile_env->runtime))) {
				return gen_out_of_memory_comp_error();
			}

			if (!s->data.build(e->data)) {
				return gen_out_of_runtime_memory_comp_error();
			}

			if (!compile_env->host_ref_holder.add_object(s.get())) {
				return gen_out_of_memory_comp_error();
			}

			slake::Reference entity_ref = slake::Reference(s.get());

			value_out = slake::Value(entity_ref);
			break;
		}
		case ExprKind::Bool: {
			AstNodePtr<BoolLiteralExprNode> e = expr.cast_to<BoolLiteralExprNode>();

			value_out = slake::Value(e->data);
			break;
		}
		case ExprKind::Null: {
			AstNodePtr<NullLiteralExprNode> e = expr.cast_to<NullLiteralExprNode>();

			slake::Reference entity_ref = slake::Reference(nullptr);

			value_out = slake::Value(entity_ref);
			break;
		}
		case ExprKind::RegIndex: {
			AstNodePtr<RegIndexExprNode> e = expr.cast_to<RegIndexExprNode>();

			value_out = slake::Value(slake::ValueType::RegIndex, e->reg);
			break;
		}
		case ExprKind::BCLabel: {
			AstNodePtr<bc::BCLabelExprNode> e = expr.cast_to<bc::BCLabelExprNode>();

			value_out = slake::Value(compilation_context->get_label_offset(*(compilation_context->get_label_index_by_name(e->name))));
			break;
		}
		case ExprKind::TypeName: {
			AstNodePtr<TypeNameExprNode> e = expr.cast_to<TypeNameExprNode>();

			slake::TypeRef t;

			SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, compilation_context, e->type, t));

			value_out = slake::Value(t);
			break;
		}
		case ExprKind::InitializerList:
			return CompilationError(expr->token_range, CompilationErrorKind::RequiresCompTimeExpr);
		default:
			std::terminate();
	}

	return {};
}

SLKC_API peff::Option<CompilationError> slkc::compile_generic_params(
	CompileEnv *compile_env,
	CompilationContext *compilation_context,
	AstNodePtr<ModuleNode> mod,
	AstNodePtr<GenericParamNode> *generic_params,
	size_t num_generic_params,
	slake::GenericParamList &generic_param_list_out) {
	peff::Option<CompilationError> e;

	for (size_t j = 0; j < num_generic_params; ++j) {
		auto gp_node = generic_params[j];
		slake::GenericParam gp(compile_env->runtime->get_cur_gen_alloc());

		if (!gp.name.build(gp_node->name)) {
			return gen_out_of_memory_comp_error();
		}

		if (gp_node->input_type) {
			// TODO: Detect if the input type is compatible with the compile-time expressions.
			if ((e = compile_type_name(compile_env, compilation_context, gp_node->input_type, gp.input_type))) {
				if (e->error_kind == CompilationErrorKind::OutOfMemory)
					return e;
				if (!compile_env->errors.push_back(std::move(*e))) {
					return gen_out_of_memory_comp_error();
				}
				e.reset();
			}
		} else {
			if (gp_node->is_param_type_list) {
				// TODO: Implement it.
			} else if (gp_node->generic_constraint) {
				if (gp_node->generic_constraint->base_type) {
					if ((e = compile_type_name(compile_env, compilation_context, gp_node->generic_constraint->base_type, gp.base_type))) {
						if (e->error_kind == CompilationErrorKind::OutOfMemory)
							return e;
						if (!compile_env->errors.push_back(std::move(*e))) {
							return gen_out_of_memory_comp_error();
						}
						e.reset();
					}
				}

				if (!gp.interfaces.resize(gp_node->generic_constraint->impl_types.size())) {
					return gen_out_of_runtime_memory_comp_error();
				}

				for (size_t k = 0; k < gp_node->generic_constraint->impl_types.size(); ++k) {
					if ((e = compile_type_name(compile_env, compilation_context, gp_node->generic_constraint->impl_types.at(k), gp.interfaces.at(k)))) {
						if (e->error_kind == CompilationErrorKind::OutOfMemory)
							return e;
						if (!compile_env->errors.push_back(std::move(*e))) {
							return gen_out_of_memory_comp_error();
						}
						e.reset();
					}
				}
			}
		}

		if (!generic_param_list_out.push_back(std::move(gp))) {
			return gen_out_of_runtime_memory_comp_error();
		}
	}

	return {};
}

#define MNEMONIC_NAME_CASE(name)    \
	if (sv == #name) {              \
		return slake::Opcode::name; \
	} else {                        \
	}

SLKC_API peff::Option<slake::Opcode> _get_opcode(const std::string_view &sv) {
	MNEMONIC_NAME_CASE(INVALID);

	MNEMONIC_NAME_CASE(LVALUE);
	MNEMONIC_NAME_CASE(STORE);

	MNEMONIC_NAME_CASE(JMP);
	MNEMONIC_NAME_CASE(BR);

	MNEMONIC_NAME_CASE(PHI);

	MNEMONIC_NAME_CASE(ADD);
	MNEMONIC_NAME_CASE(SUB);
	MNEMONIC_NAME_CASE(MUL);
	MNEMONIC_NAME_CASE(DIV);
	MNEMONIC_NAME_CASE(MOD);
	MNEMONIC_NAME_CASE(AND);
	MNEMONIC_NAME_CASE(OR);
	MNEMONIC_NAME_CASE(XOR);
	MNEMONIC_NAME_CASE(LAND);
	MNEMONIC_NAME_CASE(LOR);
	MNEMONIC_NAME_CASE(EQ);
	MNEMONIC_NAME_CASE(NEQ);
	MNEMONIC_NAME_CASE(LT);
	MNEMONIC_NAME_CASE(GT);
	MNEMONIC_NAME_CASE(LTEQ);
	MNEMONIC_NAME_CASE(GTEQ);
	MNEMONIC_NAME_CASE(LSH);
	MNEMONIC_NAME_CASE(RSH);
	MNEMONIC_NAME_CASE(CMP);

	MNEMONIC_NAME_CASE(NOT);
	MNEMONIC_NAME_CASE(LNOT);
	MNEMONIC_NAME_CASE(NEG);

	MNEMONIC_NAME_CASE(AT);

	MNEMONIC_NAME_CASE(LOAD);
	MNEMONIC_NAME_CASE(RLOAD);

	MNEMONIC_NAME_CASE(LCURFN);

	MNEMONIC_NAME_CASE(COPY);

	MNEMONIC_NAME_CASE(LARG);
	MNEMONIC_NAME_CASE(LAPARG);

	MNEMONIC_NAME_CASE(LVAR);
	MNEMONIC_NAME_CASE(ALLOCA);

	MNEMONIC_NAME_CASE(ENTER);
	MNEMONIC_NAME_CASE(LEAVE);

	MNEMONIC_NAME_CASE(PUSHARG);
	MNEMONIC_NAME_CASE(PUSHAP);

	MNEMONIC_NAME_CASE(CALL);
	MNEMONIC_NAME_CASE(MCALL);
	MNEMONIC_NAME_CASE(CTORCALL);
	MNEMONIC_NAME_CASE(RET);

	MNEMONIC_NAME_CASE(COCALL);
	MNEMONIC_NAME_CASE(COMCALL);
	MNEMONIC_NAME_CASE(YIELD);
	MNEMONIC_NAME_CASE(RESUME);
	MNEMONIC_NAME_CASE(CODONE);

	MNEMONIC_NAME_CASE(LTHIS);

	MNEMONIC_NAME_CASE(NEW);
	MNEMONIC_NAME_CASE(ARRNEW);

	MNEMONIC_NAME_CASE(THROW);
	MNEMONIC_NAME_CASE(PUSHEH);
	MNEMONIC_NAME_CASE(LEXCEPT);

	MNEMONIC_NAME_CASE(CAST);

	MNEMONIC_NAME_CASE(APTOTUPLE);

	MNEMONIC_NAME_CASE(DCMT);

	MNEMONIC_NAME_CASE(TYPEOF);

	MNEMONIC_NAME_CASE(CONSTSW);
	return {};
}
#undef MNEMONIC_NAME_CASE

SLKC_API peff::Option<CompilationError> slkc::compile_module_like_node(
	CompileEnv *compile_env,
	AstNodePtr<ModuleNode> mod,
	slake::BasicModuleObject *mod_out) {
	peff::Deferred restore_cur_parent_access_node_guard([compile_env, old_node = compile_env->cur_parent_access_node]() noexcept {
		compile_env->cur_parent_access_node = old_node;
	});
	compile_env->cur_parent_access_node = mod;

	peff::ScopeGuard restore_cur_module_guard([compile_env, old_module = compile_env->cur_module]() noexcept {
		compile_env->cur_module = old_module;
	});
	if (mod->get_ast_node_type() == AstNodeType::Module)
		compile_env->cur_module = mod;
	else
		restore_cur_module_guard.release();

	peff::Option<CompilationError> compilation_error;
	if (mod_out->get_object_kind() == slake::ObjectKind::Module) {
		for (auto i : mod->anonymous_imports) {
			NormalCompilationContext compilation_context(compile_env, nullptr);

			slake::HostObjectRef<slake::IdRefObject> id;

			for (auto &j : compile_env->document->external_module_providers) {
				SLKC_RETURN_IF_COMP_ERROR(j->load_module(compile_env, i->id_ref.get()));
			}

			SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, &compilation_context, i->id_ref->entries.data(), i->id_ref->entries.size(), nullptr, 0, false, {}, id));

			if (!((slake::ModuleObject *)mod_out)->unnamed_imports.push_back(id.get()))
				return gen_out_of_runtime_memory_comp_error();
		}
	}

	for (auto [k, v] : mod->member_indices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		if (m->get_ast_node_type() == AstNodeType::Import) {
			AstNodePtr<ImportNode> import_node = m.cast_to<ImportNode>();

			for (auto &j : compile_env->document->external_module_providers) {
				SLKC_RETURN_IF_COMP_ERROR(j->load_module(compile_env, import_node->id_ref.get()));
			}

			NormalCompilationContext compilation_context(compile_env, nullptr);

			slake::HostObjectRef<slake::IdRefObject> id;

			SLKC_RETURN_IF_COMP_ERROR(compile_id_ref(compile_env, &compilation_context, import_node->id_ref->entries.data(), import_node->id_ref->entries.size(), nullptr, 0, false, {}, id));

			if (mod_out->get_object_kind() == slake::ObjectKind::Module) {
				if (!((slake::ModuleObject *)mod_out)->unnamed_imports.push_back(id.get()))
					return gen_out_of_memory_comp_error();
			}
		}
	}

	// SLKC_RETURN_IF_COMP_ERROR(index_module_var_members(compile_env, compile_env->document->root_module));

	for (auto [k, v] : mod->member_indices) {
		AstNodePtr<MemberNode> m = mod->members.at(v);

		NormalCompilationContext compilation_context(compile_env, nullptr);

		switch (m->get_ast_node_type()) {
			case AstNodeType::BCFn: {
				AstNodePtr<bc::BCFnNode> slot_node = m.cast_to<bc::BCFnNode>();
				slake::HostObjectRef<slake::FnObject> slot_object;

				if (!(slot_object = slake::FnObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				if (!slot_object->set_name(slot_node->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				for (auto &i : slot_node->overloadings) {
					peff::Option<CompilationError> e;

					compile_env->reset();

					switch (mod->get_ast_node_type()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
							if (!(i->access_modifier & slake::ACCESS_STATIC)) {
								if (!(compile_env->this_node = make_ast_node<ThisNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document)))
									return gen_out_of_memory_comp_error();
								compile_env->this_node->this_type = mod->shared_from_this().cast_to<MemberNode>();
							}
							break;
						default:
							break;
					}

					slake::HostObjectRef<slake::RegularFnOverloadingObject> fn_object;

					if (!(fn_object = slake::RegularFnOverloadingObject::alloc(slot_object.get()))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (i->fn_flags & FN_VARG) {
						fn_object->set_var_args();
					}
					if (i->fn_flags & FN_VIRTUAL) {
						fn_object->set_virtual_flag();
					}

					fn_object->set_access(i->access_modifier);

					if (!fn_object->param_types.resize(i->params.size())) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (i->return_type) {
						if ((e = compile_type_name(compile_env, &compilation_context, i->return_type, fn_object->return_type))) {
							if (e->error_kind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compile_env->errors.push_back(std::move(*e))) {
								return gen_out_of_memory_comp_error();
							}
							e.reset();
						}
					} else {
						fn_object->return_type = slake::TypeId::Void;
					}

					for (size_t j = 0; j < i->params.size(); ++j) {
						if ((e = compile_type_name(compile_env, &compilation_context, i->params.at(j)->type, fn_object->param_types.at(j)))) {
							if (e->error_kind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compile_env->errors.push_back(std::move(*e))) {
								return gen_out_of_memory_comp_error();
							}
							e.reset();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(compile_generic_params(compile_env, &compilation_context, mod, i->generic_params.data(), i->generic_params.size(), fn_object->generic_params));

					uint32_t max_reg = UINT32_MAX;
					NormalCompilationContext comp_context(compile_env, nullptr);
					{
						size_t k = 0;
						for (auto j : i->body) {
							switch (j->stmt_kind) {
								case bc::BCStmtKind::Instruction:
									++k;
									break;
								case bc::BCStmtKind::Label: {
									AstNodePtr<bc::LabelBCStmtNode> ins = j.cast_to<bc::LabelBCStmtNode>();

									if (auto it = comp_context.label_name_indices.find(ins->name); it != comp_context.label_name_indices.end())
										comp_context.set_label_offset(it.value(), k);
									else {
										uint32_t label_id;
										SLKC_RETURN_IF_COMP_ERROR(comp_context.alloc_label(label_id));
										SLKC_RETURN_IF_COMP_ERROR(comp_context.set_label_name(label_id, ins->name));
										comp_context.set_label_offset(label_id, k);
									}
									break;
								}
								default:
									std::terminate();
							}
						}
						k = 0;
						for (auto j : i->body) {
							switch (j->stmt_kind) {
								case bc::BCStmtKind::Instruction: {
									++k;
									AstNodePtr<bc::InstructionBCStmtNode> ins = j.cast_to<bc::InstructionBCStmtNode>();

									uint32_t sld_index;
									SLKC_RETURN_IF_COMP_ERROR(comp_context.register_source_loc_desc(slake::slxfmt::SourceLocDesc{ ins->line, ins->column }, sld_index));

									peff::Option<slake::Opcode> opcode_result = _get_opcode(ins->mnemonic);
									if (!opcode_result.has_value())
										return CompilationError(ins->token_range, CompilationErrorKind::InvalidMnemonic);
									slake::Opcode opcode = *opcode_result;
									peff::DynArray<slake::Value> operands(compile_env->allocator.get());

									if (!operands.resize(ins->operands.size()))
										return gen_out_of_memory_comp_error();

									for (size_t k = 0; k < operands.size(); ++k) {
										SLKC_RETURN_IF_COMP_ERROR(compile_value_expr(compile_env, &comp_context, ins->operands.at(k), operands.at(k)));
									}

									SLKC_RETURN_IF_COMP_ERROR(comp_context.emit_ins(sld_index, opcode, ins->reg_out, operands.data(), operands.size()));
									break;
								}
								case bc::BCStmtKind::Label:
									break;
								default:
									std::terminate();
							}
						}
					}
					if (!fn_object->instructions.resize(comp_context.generated_instructions.size())) {
						return gen_out_of_runtime_memory_comp_error();
					}
					for (size_t i = 0; i < comp_context.generated_instructions.size(); ++i) {
						fn_object->instructions.at(i) = std::move(comp_context.generated_instructions.at(i));
					}
					comp_context.generated_instructions.clear_and_shrink();

					if (!fn_object->source_loc_descs.resize(comp_context.source_loc_descs.size()))
						return gen_out_of_memory_comp_error();
					memcpy(fn_object->source_loc_descs.data(), comp_context.source_loc_descs.data(), comp_context.source_loc_descs.size() * sizeof(slake::slxfmt::SourceLocDesc));
					comp_context.source_loc_descs.clear_and_shrink();
					comp_context.source_loc_descs_map.clear();

					for (auto &j : fn_object->instructions) {
						for (size_t k = 0; k < j.num_operands; ++k) {
							if (j.operands[k].value_type == slake::ValueType::RegIndex) {
								if (max_reg == UINT32_MAX)
									max_reg = j.operands[k].get_reg_index();
								else if (j.operands[k].get_reg_index() != UINT32_MAX) {
									max_reg = std::max(max_reg, j.operands[k].get_reg_index());
								}
							}
						}
					}

					if (max_reg != UINT32_MAX)
						fn_object->num_registers = max_reg + 1;
					else
						fn_object->num_registers = 0;

					if (!slot_object->overloadings.insert(slake::FnSignature{ fn_object->param_types, fn_object->is_with_var_args(), fn_object->generic_params.size(), slake::TypeId::Void }, fn_object.get())) {
						return gen_out_of_runtime_memory_comp_error();
					}
				}

				if (!mod_out->add_member(slot_object.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}
				break;
			}
			case AstNodeType::Var: {
				AstNodePtr<VarNode> var_node = m.cast_to<VarNode>();

				slake::FieldRecord fr(compile_env->runtime->get_cur_gen_alloc());

				if (!fr.name.build(k)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				fr.access_modifier = m->access_modifier;
				fr.offset = mod_out->get_local_field_storage_size();

				slake::TypeRef type;

				SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, &compilation_context, var_node->type, type));

				fr.type = type;

				switch (type.type_id) {
					case slake::TypeId::Any:
					case slake::TypeId::I8:
					case slake::TypeId::I16:
					case slake::TypeId::I32:
					case slake::TypeId::I64:
					case slake::TypeId::ISize:
					case slake::TypeId::U8:
					case slake::TypeId::U16:
					case slake::TypeId::U32:
					case slake::TypeId::U64:
					case slake::TypeId::USize:
					case slake::TypeId::F32:
					case slake::TypeId::F64:
					case slake::TypeId::Bool:
					case slake::TypeId::String:
					case slake::TypeId::Instance:
					case slake::TypeId::Array:
					case slake::TypeId::Tuple:
					case slake::TypeId::SIMD:
					case slake::TypeId::Fn: {
						slake::Value default_value;
						if (var_node->initial_value)
							SLKC_RETURN_IF_COMP_ERROR(compile_value_expr(compile_env, &compilation_context, var_node->initial_value, default_value));
						else
							default_value = mod_out->associated_runtime->default_value_of(type);
						if (!mod_out->append_field_record(std::move(fr))) {
							return gen_out_of_runtime_memory_comp_error();
						}
						slake::Runtime::write_var(slake::StaticFieldRef(mod_out, mod_out->get_number_of_fields() - 1), default_value);
						break;
					}
					case slake::TypeId::StructInstance:
					case slake::TypeId::ScopedEnum:
					case slake::TypeId::UnionEnum:
					case slake::TypeId::UnionEnumItem:
						if (!mod_out->append_field_record_without_alloc(std::move(fr))) {
							return gen_out_of_runtime_memory_comp_error();
						}
						// Note that we don't allocate space for types which
						// may have not been compiled, which means we cannot
						// or hard to evaluate their size.
						break;
					case slake::TypeId::GenericArg:
					case slake::TypeId::Ref:
					case slake::TypeId::TempRef:
						if (var_node->initial_value)
							return CompilationError(var_node->initial_value->token_range, CompilationErrorKind::TypeIsNotInitializable);
						if (!mod_out->append_field_record(std::move(fr))) {
							return gen_out_of_runtime_memory_comp_error();
						}
						break;
					case slake::TypeId::ParamTypeList:
					case slake::TypeId::Unpacking:
					case slake::TypeId::Unknown:
					default:
						std::terminate();
				}

				break;
			}
			case AstNodeType::Class: {
				AstNodePtr<ClassNode> cls_node = m.cast_to<ClassNode>();

				slake::HostObjectRef<slake::ClassObject> cls;

				if (!(cls = slake::ClassObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				cls->set_access(mod->access_modifier);

				if (!cls->set_name(m->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_generic_params(compile_env, &compilation_context, mod, cls_node->generic_params.data(), cls_node->generic_params.size(), cls->generic_params));

				if (cls_node->base_type) {
					AstNodePtr<MemberNode> base_type_node;

					if (cls_node->base_type->tn_kind == TypeNameKind::Custom) {
						if (!(compilation_error = resolve_custom_type_name(compile_env, cls_node->document->shared_from_this(), cls_node->base_type.cast_to<CustomTypeNameNode>(), base_type_node))) {
							if (base_type_node) {
								if (base_type_node->get_ast_node_type() != AstNodeType::Class) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->base_type->token_range, CompilationErrorKind::ExpectingClassName)));
								}

								bool is_cyclic_inherited = false;
								SLKC_RETURN_IF_COMP_ERROR(is_base_of(cls_node->document->shared_from_this(), cls_node, cls_node, is_cyclic_inherited));

								if (is_cyclic_inherited) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, CompilationErrorKind::CyclicInheritedClass)));
									continue;
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->base_type->token_range, CompilationErrorKind::ExpectingClassName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(std::move(compilation_error.value())));
							compilation_error.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->base_type->token_range, CompilationErrorKind::ExpectingClassName)));
					}

					bool is_cyclic_inherited = false;
					SLKC_RETURN_IF_COMP_ERROR(cls_node->is_cyclic_inherited(is_cyclic_inherited));

					if (is_cyclic_inherited) {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, CompilationErrorKind::CyclicInheritedClass)));
						continue;
					}

					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, &compilation_context, cls_node->base_type, cls->base_type));
				}

				peff::Set<AstNodePtr<InterfaceNode>> involved_interfaces(compile_env->allocator.get());

				for (auto &i : cls_node->impl_types) {
					AstNodePtr<MemberNode> implemented_type_node;

					if (i->tn_kind == TypeNameKind::Custom) {
						if (!(compilation_error = resolve_custom_type_name(compile_env, cls_node->document->shared_from_this(), i.cast_to<CustomTypeNameNode>(), implemented_type_node))) {
							if (implemented_type_node) {
								if (implemented_type_node->get_ast_node_type() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collect_involved_interfaces(compile_env->document, implemented_type_node.cast_to<InterfaceNode>(), involved_interfaces, true); e) {
										if (e->error_kind != CompilationErrorKind::CyclicInheritedInterface)
											return e;
									}
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(std::move(compilation_error.value())));
							compilation_error.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
					}

					slake::TypeRef t;

					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, &compilation_context, i, t));

					if (!cls->impl_types.push_back(std::move(t))) {
						return gen_out_of_runtime_memory_comp_error();
					}
				}

				auto cmp = [](const std::pair<AstNodePtr<InterfaceNode>, AstNodePtr<InterfaceNode>> &lhs,
							   const std::pair<AstNodePtr<InterfaceNode>, AstNodePtr<InterfaceNode>> &rhs) -> int {
					if (lhs.first < rhs.first)
						return -1;
					if (lhs.first > rhs.first)
						return 1;
					if (lhs.second < rhs.second)
						return -1;
					if (lhs.second > rhs.second)
						return 1;
					return 0;
				};

				peff::Set<std::pair<AstNodePtr<InterfaceNode>, AstNodePtr<InterfaceNode>>, decltype(cmp), true> reported_conflicting_interfaces_set(
					compile_env->allocator.get(),
					std::move(cmp));

				bool conflicted_interfaces_detected = false;
				for (auto lhs_it = cls_node->impl_types.begin(); lhs_it != cls_node->impl_types.end(); ++lhs_it) {
					for (auto rhs_it = lhs_it + 1; rhs_it != cls_node->impl_types.end(); ++rhs_it) {
						AstNodePtr<InterfaceNode> lhs_parent, rhs_parent;

						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(*lhs_it, lhs_parent, nullptr));
						SLKC_RETURN_IF_COMP_ERROR(visit_base_interface(*rhs_it, rhs_parent, nullptr));

						if ((!lhs_parent) || (!rhs_parent))
							continue;

						if (reported_conflicting_interfaces_set.contains({ lhs_parent, rhs_parent }) ||
							reported_conflicting_interfaces_set.contains({ rhs_parent, lhs_parent }))
							continue;
						if (!reported_conflicting_interfaces_set.insert({ lhs_parent, rhs_parent }))
							return gen_out_of_memory_comp_error();

						for (auto &lhs_member : lhs_parent->members) {
							if (lhs_member->get_ast_node_type() == AstNodeType::Fn) {
								AstNodePtr<FnNode> lhs = lhs_member.cast_to<FnNode>();

								if (auto rhs_member = rhs_parent->member_indices.find(lhs_member->name); rhs_member != rhs_parent->member_indices.end()) {
									AstNodePtr<MemberNode> corresponding_member = rhs_parent->members.at(rhs_member.value());

									if (corresponding_member->get_ast_node_type() != AstNodeType::Fn) {
										// Corresponding member should not be not a function.
										std::terminate();
									}
									AstNodePtr<FnNode> rhs = corresponding_member.cast_to<FnNode>();

									for (auto &cur_lhs_overloading : lhs->overloadings) {
										bool b = false;

										for (auto &cur_rhs_overloading : rhs->overloadings) {
											if (cur_lhs_overloading->params.size() != cur_rhs_overloading->params.size()) {
												continue;
											}

											SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_same(cur_lhs_overloading->params.data(), cur_rhs_overloading->params.data(), cur_lhs_overloading->params.size(), {}, {}, b));

											// No conflict, continue.
											if (!b)
												continue;

											if (auto overriden_it = cls_node->member_indices.find(lhs_member->name); overriden_it != cls_node->member_indices.end()) {
												AstNodePtr<MemberNode> corresponding_overriden_member = cls_node->members.at(overriden_it.value());

												if (corresponding_overriden_member->get_ast_node_type() == AstNodeType::Fn) {
													AstNodePtr<FnNode> corresponding_overriden_method = corresponding_overriden_member.cast_to<FnNode>();

													bool overriden_whether;

													for (auto &cur_overriden_overloading : corresponding_overriden_method->overloadings) {
														SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_same(
															cur_lhs_overloading->params.data(),
															cur_overriden_overloading->params.data(),
															cur_lhs_overloading->params.size(),
															cur_overriden_overloading->overriden_type,
															{},
															overriden_whether));
														// Unspecialized overriden, continue.
														if (overriden_whether) {
															b = false;
															goto check_interface_methods_conflicted;
														}

														SLKC_RETURN_IF_COMP_ERROR(
															is_fn_signature_same(
																cur_lhs_overloading->params.data(),
																cur_overriden_overloading->params.data(),
																cur_lhs_overloading->params.size(),
																*rhs_it,
																cur_overriden_overloading->overriden_type,
																overriden_whether));
														// Specialized overriden, continue.
														if (overriden_whether) {
															b = false;
															goto check_interface_methods_conflicted;
														}
													}
												}
											}

										check_interface_methods_conflicted:
											if (b) {
												conflicted_interfaces_detected = true;
												SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError((*lhs_it)->token_range, CompilationErrorKind::InterfaceMethodsConflicted)));
												continue;
											}
										}
									}
								}
							}
						}
					}
				}

				if (!conflicted_interfaces_detected) {
					for (auto &i : involved_interfaces) {
						for (auto &j : i->members) {
							if (j->get_ast_node_type() == AstNodeType::Fn) {
								AstNodePtr<FnNode> method = j.cast_to<FnNode>();

								if (auto it = cls_node->member_indices.find(j->name); it != cls_node->member_indices.end()) {
									AstNodePtr<MemberNode> corresponding_member = cls_node->members.at(it.value());

									if (corresponding_member->get_ast_node_type() != AstNodeType::Fn) {
										for (auto &k : method->overloadings) {
											SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, AbstractMethodNotImplementedErrorExData{ k })));
										}
									} else {
										AstNodePtr<FnNode> corresponding_method = corresponding_member.cast_to<FnNode>();

										for (auto &k : method->overloadings) {
											bool b = false;

											for (auto &l : corresponding_method->overloadings) {
												if (k->params.size() != l->params.size()) {
													continue;
												}

												SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_same(k->params.data(), l->params.data(), k->params.size(), {}, {}, b));

												if (b) {
													goto class_matched;
												}
											}

											SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, AbstractMethodNotImplementedErrorExData{ k })));

										class_matched:;
										}
									}
								} else {
									for (auto &k : method->overloadings) {
										SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, AbstractMethodNotImplementedErrorExData{ k })));
									}
								}
							}
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_module_like_node(compile_env, cls_node.cast_to<ModuleNode>(), cls.get()));

				if (!mod_out->add_member(cls.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}

				break;
			}
			case AstNodeType::Interface: {
				AstNodePtr<InterfaceNode> cls_node = m.cast_to<InterfaceNode>();

				slake::HostObjectRef<slake::InterfaceObject> cls;

				if (!(cls = slake::InterfaceObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				cls->set_access(mod->access_modifier);

				if (!cls->set_name(m->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_generic_params(compile_env, &compilation_context, mod, cls_node->generic_params.data(), cls_node->generic_params.size(), cls->generic_params));

				for (auto &i : cls_node->impl_types) {
					AstNodePtr<MemberNode> implemented_type_node;

					if (i->tn_kind == TypeNameKind::Custom) {
						if (!(compilation_error = resolve_custom_type_name(compile_env, cls_node->document->shared_from_this(), i.cast_to<CustomTypeNameNode>(), implemented_type_node))) {
							if (implemented_type_node) {
								if (implemented_type_node->get_ast_node_type() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(std::move(compilation_error.value())));
							compilation_error.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
					}

					slake::TypeRef t;

					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, &compilation_context, i, t));

					if (!cls->impl_types.push_back(std::move(t))) {
						return gen_out_of_runtime_memory_comp_error();
					}
				}

				bool is_cyclic_inherited = false;
				SLKC_RETURN_IF_COMP_ERROR(cls_node->is_cyclic_inherited(is_cyclic_inherited));

				if (is_cyclic_inherited) {
					if (cls_node->cyclic_inheritance_error.has_value()) {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(std::move(*cls_node->cyclic_inheritance_error)));
						cls_node->cyclic_inheritance_error.reset();
						continue;
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_module_like_node(compile_env, cls_node.cast_to<ModuleNode>(), cls.get()));

				if (!mod_out->add_member(cls.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}

				break;
			}
			case AstNodeType::ScopedEnum: {
				AstNodePtr<ScopedEnumNode> cls_node = m.cast_to<ScopedEnumNode>();

				slake::TypeRef base_type = slake::TypeId::Invalid;

				if (cls_node->base_type) {
					bool b = false;
					SLKC_RETURN_IF_COMP_ERROR(is_scoped_enum_base_type(cls_node->base_type, b));

					if (!b) {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(
							CompilationError(
								cls_node->base_type->token_range,
								CompilationErrorKind::InvalidEnumBaseType)));
					}

					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, &compilation_context, cls_node->base_type, base_type));
				}

				SLKC_RETURN_IF_COMP_ERROR(fill_scoped_enum(compile_env, &compilation_context, cls_node));

				slake::HostObjectRef<slake::ScopedEnumObject> cls;

				if (!(cls = slake::ScopedEnumObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				cls->set_access(mod->access_modifier);

				if (!cls->set_name(m->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				cls->base_type = base_type;

				if (base_type != slake::TypeId::Invalid) {
					for (auto i : cls_node->members) {
						switch (i->get_ast_node_type()) {
							case AstNodeType::EnumItem: {
								AstNodePtr<EnumItemNode> item_node = i.cast_to<EnumItemNode>();

								slake::FieldRecord fr(compile_env->runtime->get_cur_gen_alloc());

								if (!fr.name.build(i->name)) {
									return gen_out_of_runtime_memory_comp_error();
								}

								fr.access_modifier = m->access_modifier;
								fr.offset = mod_out->get_local_field_storage_size();

								fr.type = base_type;

								slake::Value item_value;

								AstNodePtr<ExprNode> enum_value;
								AstNodePtr<TypeNameNode> enum_value_type;

								assert(item_node->filled_value);

								{
									PathEnv path_env(compile_env->allocator.get());
									SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, &compilation_context, &path_env, item_node->filled_value, enum_value));
								}

								if (!enum_value)
									return CompilationError(item_node->token_range, CompilationErrorKind::RequiresCompTimeExpr);
								{
									PathEnv path_env(compile_env->allocator.get());
									SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, &compilation_context, &path_env, item_node->filled_value, enum_value));
								}

								{
									PathEnv root_path_env(compile_env->allocator.get());
									SLKC_RETURN_IF_COMP_ERROR(eval_expr_type(compile_env, &compilation_context, &root_path_env, enum_value, enum_value_type, cls_node->base_type));
								}

								bool is_same;
								SLKC_RETURN_IF_COMP_ERROR(is_same_type(enum_value_type, cls_node->base_type, is_same));

								if (!is_same) {
									AstNodePtr<CastExprNode> cast_expr;

									if (!(cast_expr = make_ast_node<CastExprNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document)))
										return gen_out_of_memory_comp_error();

									cast_expr->target_type = cls_node->base_type;
									cast_expr->source = enum_value;
									cast_expr->token_range = item_node->filled_value->token_range;

									PathEnv path_env(compile_env->allocator.get());
									SLKC_RETURN_IF_COMP_ERROR(eval_const_expr(compile_env, &compilation_context, &path_env, cast_expr.cast_to<ExprNode>(), enum_value));
									if (!enum_value) {
										SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(
											CompilationError(
												item_node->filled_value->token_range,
												CompilationErrorKind::IncompatibleInitialValueType)));
									}
								}

								SLKC_RETURN_IF_COMP_ERROR(compile_value_expr(compile_env, &compilation_context, enum_value, item_value));

								if (!cls->append_field_record(std::move(fr))) {
									return gen_out_of_runtime_memory_comp_error();
								}
								slake::Runtime::write_var(slake::StaticFieldRef(cls.get(), cls->get_number_of_fields() - 1), item_value);
								break;
							}
							default:
								std::terminate();
						}
					}
				} else {
					for (auto i : cls_node->members) {
						switch (i->get_ast_node_type()) {
							case AstNodeType::EnumItem: {
								AstNodePtr<EnumItemNode> item_node = i.cast_to<EnumItemNode>();

								slake::FieldRecord fr(compile_env->runtime->get_cur_gen_alloc());

								if (!fr.name.build(i->name)) {
									return gen_out_of_runtime_memory_comp_error();
								}

								fr.access_modifier = m->access_modifier;
								fr.offset = mod_out->get_local_field_storage_size();

								fr.type = base_type;

								if (item_node->filled_value) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(
										CompilationError(
											item_node->token_range,
											CompilationErrorKind::EnumItemIsNotAssignable)));
								}
								if (!cls->append_field_record_without_alloc(std::move(fr))) {
									return gen_out_of_runtime_memory_comp_error();
								}
								break;
							}
							default:
								std::terminate();
						}
					}
				}

				if (!mod_out->add_member(cls.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}

				break;
			}
			case AstNodeType::UnionEnum: {
				AstNodePtr<UnionEnumNode> cls_node = m.cast_to<UnionEnumNode>();

				bool is_cyclic_inherited = false;
				SLKC_RETURN_IF_COMP_ERROR(cls_node->is_recursed_type(is_cyclic_inherited));

				if (is_cyclic_inherited) {
					SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(
						CompilationError(
							cls_node->token_range,
							CompilationErrorKind::RecursedValueType)));
				}

				slake::HostObjectRef<slake::UnionEnumObject> cls;

				if (!(cls = slake::UnionEnumObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				cls->set_access(mod->access_modifier);

				if (!cls->set_name(m->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_generic_params(compile_env, &compilation_context, mod, cls_node->generic_params.data(), cls_node->generic_params.size(), cls->generic_params));

				for (auto i : cls_node->members) {
					switch (i->get_ast_node_type()) {
						case AstNodeType::UnionEnumItem: {
							AstNodePtr<UnionEnumItemNode> item_node = i.cast_to<UnionEnumItemNode>();

							slake::HostObjectRef<slake::UnionEnumItemObject> item;

							if (!(item = slake::UnionEnumItemObject::alloc(compile_env->runtime))) {
								return gen_out_of_runtime_memory_comp_error();
							}

							item->set_access(mod->access_modifier);

							if (!item->set_name(item_node->name)) {
								return gen_out_of_runtime_memory_comp_error();
							}

							SLKC_RETURN_IF_COMP_ERROR(compile_module_like_node(compile_env, item_node.cast_to<ModuleNode>(), item.get()));

							if (!cls->add_member(item.get())) {
								return gen_out_of_runtime_memory_comp_error();
							}

							break;
						}
						default:
							std::terminate();
					}
				}

				if (!mod_out->add_member(cls.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}

				break;
			}
			case AstNodeType::Struct: {
				AstNodePtr<StructNode> cls_node = m.cast_to<StructNode>();

				slake::HostObjectRef<slake::StructObject> cls;

				if (!(cls = slake::StructObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				cls->set_access(mod->access_modifier);

				if (!cls->set_name(m->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_generic_params(compile_env, &compilation_context, mod, cls_node->generic_params.data(), cls_node->generic_params.size(), cls->generic_params));

				bool is_cyclic_inherited = false;
				SLKC_RETURN_IF_COMP_ERROR(cls_node->is_recursed_type(is_cyclic_inherited));

				if (is_cyclic_inherited) {
					SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, CompilationErrorKind::RecursedValueType)));
					continue;
				}

				peff::Set<AstNodePtr<InterfaceNode>> involved_interfaces(compile_env->allocator.get());

				for (auto &i : cls_node->impl_types) {
					AstNodePtr<MemberNode> implemented_type_node;

					if (i->tn_kind == TypeNameKind::Custom) {
						if (!(compilation_error = resolve_custom_type_name(compile_env, cls_node->document->shared_from_this(), i.cast_to<CustomTypeNameNode>(), implemented_type_node))) {
							if (implemented_type_node) {
								if (implemented_type_node->get_ast_node_type() != AstNodeType::Interface) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
								} else {
									if (auto e = collect_involved_interfaces(compile_env->document, implemented_type_node.cast_to<InterfaceNode>(), involved_interfaces, true); e) {
										if (e->error_kind != CompilationErrorKind::CyclicInheritedInterface)
											return e;
									}
								}
							} else {
								SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
							}
						} else {
							SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(std::move(compilation_error.value())));
							compilation_error.reset();
						}
					} else {
						SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::ExpectingInterfaceName)));
					}

					slake::TypeRef t;

					SLKC_RETURN_IF_COMP_ERROR(compile_type_name(compile_env, &compilation_context, i, t));

					if (!cls->impl_types.push_back(std::move(t))) {
						return gen_out_of_runtime_memory_comp_error();
					}
				}

				for (auto &i : involved_interfaces) {
					for (auto &j : i->members) {
						if (j->get_ast_node_type() == AstNodeType::Fn) {
							AstNodePtr<FnNode> method = j.cast_to<FnNode>();

							if (auto it = cls_node->member_indices.find(j->name); it != cls_node->member_indices.end()) {
								AstNodePtr<MemberNode> corresponding_member = cls_node->members.at(it.value());

								if (corresponding_member->get_ast_node_type() != AstNodeType::Fn) {
									for (auto &k : method->overloadings) {
										SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, AbstractMethodNotImplementedErrorExData{ k })));
									}
								} else {
									AstNodePtr<FnNode> corresponding_method = corresponding_member.cast_to<FnNode>();

									for (auto &k : method->overloadings) {
										bool b = false;

										for (auto &l : corresponding_method->overloadings) {
											if (k->params.size() != l->params.size()) {
												continue;
											}

											SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_same(k->params.data(), l->params.data(), k->params.size(), {}, {}, b));

											if (b) {
												goto struct_matched;
											}
										}

										SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, AbstractMethodNotImplementedErrorExData{ k })));

									struct_matched:;
									}
								}
							} else {
								for (auto &k : method->overloadings) {
									SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(cls_node->token_range, AbstractMethodNotImplementedErrorExData{ k })));
								}
							}
						}
					}
				}

				SLKC_RETURN_IF_COMP_ERROR(compile_module_like_node(compile_env, cls_node.cast_to<ModuleNode>(), cls.get()));

				if (!mod_out->add_member(cls.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}

				break;
			}
			case AstNodeType::Fn: {
				AstNodePtr<FnNode> slot_node = m.cast_to<FnNode>();
				slake::HostObjectRef<slake::FnObject> slot_object;

				if (!(slot_object = slake::FnObject::alloc(compile_env->runtime))) {
					return gen_out_of_runtime_memory_comp_error();
				}

				if (!slot_object->set_name(slot_node->name)) {
					return gen_out_of_runtime_memory_comp_error();
				}

				for (auto &i : slot_node->overloadings) {
					peff::Option<CompilationError> e;

					for (size_t j = &i - slot_node->overloadings.data() + 1; j < slot_node->overloadings.size(); ++j) {
						bool whether;
						SLKC_RETURN_IF_COMP_ERROR(is_fn_signature_duplicated(i, slot_node->overloadings.at(j), whether));
						if (whether) {
							SLKC_RETURN_IF_COMP_ERROR(compile_env->push_error(CompilationError(i->token_range, CompilationErrorKind::FunctionOverloadingDuplicated)));
							break;
						}
					}

					compile_env->reset();

					switch (mod->get_ast_node_type()) {
						case AstNodeType::Class:
						case AstNodeType::Interface:
						case AstNodeType::Struct:
							if (!(i->access_modifier & slake::ACCESS_STATIC)) {
								if (!(compile_env->this_node = make_ast_node<ThisNode>(compile_env->allocator.get(), compile_env->allocator.get(), compile_env->document)))
									return gen_out_of_memory_comp_error();
								compile_env->this_node->this_type = mod->shared_from_this().cast_to<MemberNode>();
							}
							break;
						default:
							break;
					}

					slake::HostObjectRef<slake::RegularFnOverloadingObject> fn_object;

					if (!(fn_object = slake::RegularFnOverloadingObject::alloc(slot_object.get()))) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (i->fn_flags & FN_VARG) {
						fn_object->set_var_args();
					}
					if (i->fn_flags & FN_VIRTUAL) {
						fn_object->set_virtual_flag();
					}

					fn_object->set_access(i->access_modifier);

					compile_env->cur_overloading = i;

					if (!fn_object->param_types.resize(i->params.size())) {
						return gen_out_of_runtime_memory_comp_error();
					}

					if (i->return_type) {
						if ((e = compile_type_name(compile_env, &compilation_context, i->return_type, fn_object->return_type))) {
							if (e->error_kind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compile_env->errors.push_back(std::move(*e))) {
								return gen_out_of_memory_comp_error();
							}
							e.reset();
						}
					} else {
						fn_object->return_type = slake::TypeId::Void;
					}

					for (size_t j = 0; j < i->params.size(); ++j) {
						if ((e = compile_type_name(compile_env, &compilation_context, i->params.at(j)->type, fn_object->param_types.at(j)))) {
							if (e->error_kind == CompilationErrorKind::OutOfMemory)
								return e;
							if (!compile_env->errors.push_back(std::move(*e))) {
								return gen_out_of_memory_comp_error();
							}
							e.reset();
						}
					}

					SLKC_RETURN_IF_COMP_ERROR(compile_generic_params(compile_env, &compilation_context, mod, i->generic_params.data(), i->generic_params.size(), fn_object->generic_params));

					if (i->body) {
						NormalCompilationContext comp_context(compile_env, nullptr);

						PathEnv root_path_env(compile_env->allocator.get());

						for (auto j : i->body->body) {
							if ((e = compile_stmt(compile_env, &comp_context, &root_path_env, j))) {
								if (e->error_kind == CompilationErrorKind::OutOfMemory)
									return e;
								if (!compile_env->errors.push_back(std::move(*e))) {
									return gen_out_of_memory_comp_error();
								}
								e.reset();
							}
						}
						if (!fn_object->instructions.resize(comp_context.generated_instructions.size())) {
							return gen_out_of_runtime_memory_comp_error();
						}
						for (size_t i = 0; i < comp_context.generated_instructions.size(); ++i) {
							fn_object->instructions.at(i) = std::move(comp_context.generated_instructions.at(i));
						}
						comp_context.generated_instructions.clear_and_shrink();

						if (!fn_object->source_loc_descs.resize(comp_context.source_loc_descs.size()))
							return gen_out_of_memory_comp_error();
						memcpy(fn_object->source_loc_descs.data(), comp_context.source_loc_descs.data(), comp_context.source_loc_descs.size() * sizeof(slake::slxfmt::SourceLocDesc));
						comp_context.source_loc_descs.clear_and_shrink();
						comp_context.source_loc_descs_map.clear();

						for (auto &j : fn_object->instructions) {
							for (size_t k = 0; k < j.num_operands; ++k) {
								if (j.operands[k].value_type == slake::ValueType::Label) {
									j.operands[k] = slake::Value(comp_context.get_label_offset(j.operands[k].get_label()));
								}
							}
						}

						fn_object->num_registers = comp_context.num_total_regs;
					}

					if (!slot_object->overloadings.insert(slake::FnSignature{ fn_object->param_types, fn_object->is_with_var_args(), fn_object->generic_params.size(), slake::TypeId::Void }, fn_object.get())) {
						return gen_out_of_runtime_memory_comp_error();
					}
				}

				if (!mod_out->add_member(slot_object.get())) {
					return gen_out_of_runtime_memory_comp_error();
				}
				break;
			}
			default:
				break;
		}
	}

	return {};
}
