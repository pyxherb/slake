#include "../runtime.h"

using namespace slake;

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, MethodTable *method_table) {
	if (!method_table)
		return;
	for (auto i : method_table->methods) {
		context->push_object(i.second);
	}
	for (auto i : method_table->destructors) {
		context->push_object(i);
	}
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, GenericParamList &generic_param_list) {
	for (auto &i : generic_param_list) {
		// i.base_type.load_deferred_type(this);
		_gc_walk(context, i.base_type);

		for (auto &j : i.interfaces) {
			// j.load_deferred_type(this);
			_gc_walk(context, i.base_type);
		}
	}
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, const TypeRef &type) {
	context->push_object(type.type_def);
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, const Value &i) {
	bool is_walkable_object_detected = false;
	switch (i.value_type) {
		case ValueType::I8:
		case ValueType::I16:
		case ValueType::I32:
		case ValueType::I64:
		case ValueType::U8:
		case ValueType::U16:
		case ValueType::U32:
		case ValueType::U64:
		case ValueType::F32:
		case ValueType::F64:
		case ValueType::Bool:
			break;
		case ValueType::Reference: {
			const Reference &entity_ref = i.get_reference();

			switch (entity_ref.kind) {
				case ReferenceKind::StaticFieldRef:
					context->push_object(entity_ref.as_static_field.module_object);
					break;
				case ReferenceKind::ArrayElementRef:
					context->push_object(entity_ref.as_array_element.array_object);
					break;
				case ReferenceKind::ObjectRef:
					context->push_object(entity_ref.as_object);
					break;
				case ReferenceKind::ObjectFieldRef:
					context->push_object(entity_ref.as_object_field.instance_object);
					break;
				case ReferenceKind::LocalVarRef: {
					Value data;
					_gc_walk(context, *entity_ref.as_local_var.context);
					read_var(entity_ref, data);
					_gc_walk(context, data);
					break;
				}
				case ReferenceKind::CoroutineLocalVarRef: {
					Value data;
					context->push_object(entity_ref.as_coroutine_local_var.coroutine);
					read_var(entity_ref, data);
					_gc_walk(context, data);
					break;
				}
				case ReferenceKind::ArgRef:
					break;
				case ReferenceKind::CoroutineArgRef:
					context->push_object(entity_ref.as_coroutine_arg.coroutine);
					break;
			}
			break;
		}
		case ValueType::RegIndex:
			break;
		case ValueType::TypeName:
			_gc_walk(context, i.get_type_name());
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
}

SLAKE_API void GCWalkContext::remove_from_walkable_list(Object *v) {
	MutexGuard access_mutex_guard(access_mutex);

	if (v == walkable_list)
		walkable_list = v->next_same_gcset;

	remove_from_cur_gcset(v);
}

SLAKE_API void GCWalkContext::remove_from_destructible_list(Object *v) {
	MutexGuard access_mutex_guard(access_mutex);
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, char *stack_top, size_t sz_stack, ResumableContextData &value) {
	context->push_object(value.this_object);
	{
		Value *arg_stack = _fetch_arg_stack(stack_top, sz_stack, nullptr, value.off_args);
		for (size_t i = 0; i < value.num_args; ++i)
			_gc_walk(context, arg_stack[i]);
	}
	{
		Value *arg_stack = _fetch_arg_stack(stack_top, sz_stack, nullptr, value.off_next_args);
		for (size_t i = 0; i < value.num_next_args; ++i)
			_gc_walk(context, arg_stack[i]);
	};
	char *const stack_base = stack_top + sz_stack;
	for (auto k = value.off_cur_minor_frame; k != SIZE_MAX;) {
		MinorFrame *mjf = (MinorFrame *)(stack_base - k);
		for (auto l = mjf->off_except_handler; l != SIZE_MAX;) {
			ExceptHandler *eh = (ExceptHandler *)(stack_base - l);

			_gc_walk(context, eh->type);

			l = eh->off_next;
		}
		k = mjf->off_last_minor_frame;
	}
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, Object *v) {
	if (!v)
		return;

	switch (v->gc_status) {
		case ObjectGCStatus::Unwalked:
			throw std::logic_error("Cannot walk on an unwalked object");
		case ObjectGCStatus::ReadyToWalk:
			v->gc_status = ObjectGCStatus::Walked;

			switch (v->get_object_kind()) {
				case ObjectKind::String:
					break;
				case ObjectKind::HeapType:
					_gc_walk(context, ((HeapTypeObject *)v)->type_ref);
					break;
				case ObjectKind::TypeDef: {
					auto type_def = (TypeDefObject *)v;

					switch (type_def->get_type_def_kind()) {
						case TypeDefKind::CustomTypeDef:
							context->push_object(((CustomTypeDefObject *)type_def)->type_object);
							break;
						case TypeDefKind::ArrayTypeDef:
							context->push_object(((ArrayTypeDefObject *)type_def)->element_type);
							break;
						case TypeDefKind::RefTypeDef:
							context->push_object(((RefTypeDefObject *)type_def)->referenced_type);
							break;
						case TypeDefKind::GenericArgTypeDef:
							context->push_object(((GenericArgTypeDefObject *)type_def)->owner_object);
							context->push_object(((GenericArgTypeDefObject *)type_def)->name_object);
							break;
						case TypeDefKind::FnTypeDef: {
							auto td = ((FnTypeDefObject *)type_def);
							context->push_object(td->return_type);
							for (auto i : td->param_types)
								context->push_object(i);
							break;
						}
						case TypeDefKind::ParamTypeListTypeDef: {
							auto td = ((ParamTypeListTypeDefObject *)type_def);
							for (auto i : td->param_types)
								context->push_object(i);
							break;
						}
						case TypeDefKind::TupleTypeDef: {
							auto td = ((TupleTypeDefObject *)type_def);
							for (auto i : td->element_types)
								context->push_object(i);
							break;
						}
						case TypeDefKind::SIMDTypeDef: {
							auto td = ((SIMDTypeDefObject *)type_def);
							context->push_object(td->type);
							break;
						}
						case TypeDefKind::UnpackingTypeDef: {
							auto td = ((UnpackingTypeDefObject *)type_def);
							context->push_object(td->type);
							break;
						}
						default:
							std::terminate();
					}
					break;
				}
				case ObjectKind::Instance: {
					auto value = (InstanceObject *)v;

					context->push_object(value->_class);

					if (value->_class->cached_object_layout) {
						for (auto &i : value->_class->cached_object_layout->field_records) {
							switch (i.type.type_id) {
								case TypeId::String:
								case TypeId::Instance:
								case TypeId::Array:
								case TypeId::Ref:
									context->push_object(*((Object **)(value->raw_field_data + i.offset)));
									break;
							}
						}
					}

					context->remove_from_destructible_list(value);
					break;
				}
				case ObjectKind::Array: {
					auto value = (ArrayObject *)v;

					_gc_walk(context, value->element_type);

					switch (value->element_type.type_id) {
						case TypeId::Instance: {
							for (size_t i = 0; i < value->length; ++i)
								context->push_object(((Object **)value->data)[i]);
							break;
						}
					}
					break;
				}
				case ObjectKind::Module: {
					for (auto i = ((ModuleObject *)v)->members.begin(); i != ((ModuleObject *)v)->members.end(); ++i) {
						context->push_object(i.value());
					}
					Value data;
					for (size_t i = 0; i < ((ModuleObject *)v)->field_records.size(); ++i) {
						read_var(StaticFieldRef((ModuleObject *)v, i), data);
						_gc_walk(context, data);
					}

					context->push_object(((ModuleObject *)v)->get_parent());

					for (auto i : ((ModuleObject *)v)->unnamed_imports)
						context->push_object(i);

					break;
				}
				case ObjectKind::Class: {
					for (auto i = ((ClassObject *)v)->members.begin(); i != ((ClassObject *)v)->members.end(); ++i) {
						context->push_object(i.value());
					}
					context->push_object(((ClassObject *)v)->get_parent());

					ClassObject *value = (ClassObject *)v;

					if (auto mt = value->cached_instantiated_method_table.get(); mt) {
						_gc_walk(context, mt);
					}

					Value data;
					for (size_t i = 0; i < value->field_records.size(); ++i) {
						_gc_walk(context, value->field_records.at(i).type);
						read_var(StaticFieldRef(value, i), data);
						_gc_walk(context, data);
					}

					for (auto &i : value->impl_types) {
						// i.load_deferred_type(this);
						_gc_walk(context, i);
					}
					for (auto &i : value->generic_params) {
						// i.base_type.load_deferred_type(this);
						_gc_walk(context, i.base_type);
						for (auto &j : i.interfaces) {
							// j.load_deferred_type(this);
							_gc_walk(context, j);
						}
					}
					for (auto &i : value->generic_args) {
						// i.load_deferred_type(this);
						_gc_walk(context, i);
					}

					// value->parent_class.load_deferred_type(this);
					_gc_walk(context, value->base_type);

					_gc_walk(context, value->generic_params);

					break;
				}
				case ObjectKind::Struct: {
					for (auto i = ((StructObject *)v)->members.begin(); i != ((StructObject *)v)->members.end(); ++i) {
						context->push_object(i.value());
					}
					context->push_object(((StructObject *)v)->get_parent());

					StructObject *value = (StructObject *)v;

					Value data;
					for (size_t i = 0; i < value->field_records.size(); ++i) {
						_gc_walk(context, value->field_records.at(i).type);
						read_var(StaticFieldRef(value, i), data);
						_gc_walk(context, data);
					}

					for (auto &i : value->generic_params) {
						// i.base_type.load_deferred_type(this);
						_gc_walk(context, i.base_type);
						for (auto &j : i.interfaces) {
							// j.load_deferred_type(this);
							_gc_walk(context, j);
						}
					}
					for (auto &i : value->generic_args) {
						// i.load_deferred_type(this);
						_gc_walk(context, i);
					}

					_gc_walk(context, value->generic_params);

					break;
				}
				case ObjectKind::ScopedEnum: {
					for (auto i = ((ScopedEnumObject *)v)->members.begin(); i != ((ScopedEnumObject *)v)->members.end(); ++i) {
						context->push_object(i.value());
					}
					context->push_object(((ScopedEnumObject *)v)->get_parent());

					ScopedEnumObject *value = (ScopedEnumObject *)v;

					if (value->base_type) {
						Value data;
						for (size_t i = 0; i < value->field_records.size(); ++i) {
							_gc_walk(context, value->field_records.at(i).type);
							read_var(StaticFieldRef(value, i), data);
							_gc_walk(context, data);
						}
					}

					break;
				}
				case ObjectKind::UnionEnum: {
					UnionEnumObject *value = (UnionEnumObject *)v;

					for (auto i = value->members.begin(); i != value->members.end(); ++i) {
						context->push_object(i.value());
					}
					context->push_object(value->get_parent());

					for (auto &i : value->generic_params) {
						// i.base_type.load_deferred_type(this);
						_gc_walk(context, i.base_type);
						for (auto &j : i.interfaces) {
							// j.load_deferred_type(this);
							_gc_walk(context, j);
						}
					}
					for (auto &i : value->generic_args) {
						// i.load_deferred_type(this);
						_gc_walk(context, i);
					}

					_gc_walk(context, value->generic_params);

					break;
				}
				case ObjectKind::UnionEnumItem: {
					UnionEnumItemObject *value = (UnionEnumItemObject *)v;

					context->push_object(value->get_parent());

					Value data;
					for (size_t i = 0; i < value->field_records.size(); ++i) {
						_gc_walk(context, value->field_records.at(i).type);
						read_var(StaticFieldRef(value, i), data);
						_gc_walk(context, data);
					}
					break;
				}
				case ObjectKind::Interface: {
					// TODO: Walk generic parameters.
					for (auto i = ((InterfaceObject *)v)->members.begin(); i != ((InterfaceObject *)v)->members.end(); ++i) {
						context->push_object(i.value());
					}
					context->push_object(((InterfaceObject *)v)->get_parent());

					InterfaceObject *value = (InterfaceObject *)v;

					Value data;
					for (size_t i = 0; i < value->field_records.size(); ++i) {
						read_var(StaticFieldRef(value, i), data);
						_gc_walk(context, data);
					}

					for (auto &i : value->impl_types) {
						// i.load_deferred_type(this);
						context->push_object(i.type_def);
					}
					for (auto &i : value->generic_params) {
						// i.base_type.load_deferred_type(this);
						_gc_walk(context, i.base_type);
						for (auto &j : i.interfaces) {
							_gc_walk(context, j);
						}
					}
					for (auto &i : value->generic_args) {
						// i.load_deferred_type(this);
						_gc_walk(context, i);
					}

					_gc_walk(context, value->generic_params);

					break;
				}
				case ObjectKind::Fn: {
					auto fn = (FnObject *)v;

					context->push_object(fn->get_parent());

					for (auto i : fn->overloadings) {
						context->push_object(i.second);
					}

					break;
				}
				case ObjectKind::FnOverloading: {
					auto fn_overloading = (FnOverloadingObject *)v;

					context->push_object(fn_overloading->fn_object);

					for (auto &i : fn_overloading->generic_params) {
						// i.base_type.load_deferred_type(this);
						_gc_walk(context, i.base_type);
						for (auto &j : i.interfaces) {
							// j.load_deferred_type(this);
							_gc_walk(context, j);
						}
					}
					for (auto i = fn_overloading->mapped_generic_args.begin(); i != fn_overloading->mapped_generic_args.end(); ++i) {
						// i.value().load_deferred_type(this);
						_gc_walk(context, i.value());
					}

					for (auto &j : fn_overloading->param_types)
						_gc_walk(context, j);
					_gc_walk(context, fn_overloading->return_type);

					switch (fn_overloading->overloading_kind) {
						case FnOverloadingKind::Regular: {
							RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn_overloading;

							for (auto &i : ol->instructions) {
								for (size_t j = 0; j < i.num_operands; ++j) {
									_gc_walk(context, i.operands[j]);
								}
							}

							break;
						}
						case FnOverloadingKind::Native: {
							NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)fn_overloading;

							break;
						}
						default:
							throw std::logic_error("Invalid overloading kind");
					}

					_gc_walk(context, fn_overloading->generic_params);

					break;
				}
				case ObjectKind::IdRef: {
					auto value = (IdRefObject *)v;

					for (auto &i : value->entries) {
						for (auto &j : i.generic_args) {
							_gc_walk(context, j);
						}
					}

					if (value->param_types.has_value()) {
						for (auto &j : *value->param_types) {
							_gc_walk(context, j);
						}
					}
					break;
				}
				case ObjectKind::Context: {
					auto value = (ContextObject *)v;

					_gc_walk(context, value->_context);
					break;
				}
				case ObjectKind::Coroutine: {
					auto value = (CoroutineObject *)v;

					context->push_object((FnOverloadingObject *)value->overloading);

					if (value->resumable.has_value()) {
						_gc_walk(context, value->stack_data, value->len_stack_data, value->resumable.value());

						if (value->stack_data) {
							for (size_t i = 0; i < value->resumable->num_regs; ++i)
								_gc_walk(context, *((Value *)(value->stack_data + value->len_stack_data - sizeof(Value) * i)));
						}
					}

					if (value->is_done()) {
						_gc_walk(context, value->final_result);
					}
					break;
				}
				default:
					throw std::logic_error("Unhandled object type");
			}

			context->remove_from_walkable_list(v);
			context->push_walked(v);

			v->gc_spinlock.unlock();

			break;
		case ObjectGCStatus::Walked:
			break;
	}
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, char *data_stack, size_t stack_size, MajorFrame *major_frame) {
	context->push_object((FnOverloadingObject *)major_frame->cur_fn);
	if (major_frame->cur_coroutine)
		context->push_object(major_frame->cur_coroutine);

	_gc_walk(context, data_stack, stack_size, major_frame->resumable_context_data);

	size_t num_regs = major_frame->resumable_context_data.num_regs;
	for (size_t i = 0; i < num_regs; ++i)
		_gc_walk(context, *calc_stack_addr(data_stack, stack_size, (major_frame->off_regs + sizeof(Value) * i)));
}

SLAKE_API void Runtime::_gc_walk(GCWalkContext *context, Context &ctxt) {
	bool is_walkable_object_detected = false;

	MajorFrame *mf;
	size_t off_cur_major_frame = ctxt.off_cur_major_frame;
	do {
		mf = _fetch_major_frame(&ctxt, off_cur_major_frame);
		context->push_object(mf->cur_coroutine);
		_gc_walk(context, ctxt.data_stack, ctxt.stack_size, mf);
		off_cur_major_frame = mf->off_prev_frame;
	} while (mf->off_prev_frame != SIZE_MAX);
}

SLAKE_API void GCWalkContext::remove_from_cur_gcset(Object *object) {
	if (object->prev_same_gcset)
		object->prev_same_gcset->next_same_gcset = object->next_same_gcset;
	if (object->next_same_gcset)
		object->next_same_gcset->prev_same_gcset = object->prev_same_gcset;
	object->next_same_gcset = nullptr;
	object->prev_same_gcset = nullptr;
}

SLAKE_API void GCWalkContext::push_object(Object *object) {
	if (!object)
		return;

	switch (object->gc_status) {
		case ObjectGCStatus::Unwalked:
			object->gc_status = ObjectGCStatus::ReadyToWalk;

			object->gc_spinlock.lock();

			if (object == unwalked_list)
				unwalked_list = object->next_same_gcset;

			remove_from_cur_gcset(object);
			push_walkable(object);
			break;
		case ObjectGCStatus::ReadyToWalk:
			break;
		case ObjectGCStatus::Walked:
			break;
		default:
			std::terminate();
	}
}

SLAKE_API bool GCWalkContext::is_walkable_list_empty() {
	return !walkable_list;
}

SLAKE_API Object *GCWalkContext::get_walkable_list() {
	MutexGuard access_mutex_guard(access_mutex);

	Object *p = walkable_list;
	walkable_list = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::push_walkable(Object *walkable_object) {
	MutexGuard access_mutex_guard(access_mutex);

	if (unwalked_list == walkable_object)
		unwalked_list = walkable_object->next_same_gcset;

	walkable_object->next_same_gcset = walkable_list;
	walkable_list = walkable_object;
}

SLAKE_API Object *GCWalkContext::get_unwalked_list(bool clear_list) {
	MutexGuard access_mutex_guard(access_mutex);

	Object *p = unwalked_list;
	if (clear_list)
		unwalked_list = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::push_unwalked(Object *walkable_object) {
	MutexGuard access_mutex_guard(access_mutex);

	if (unwalked_list)
		unwalked_list->prev_same_gcset = walkable_object;

	walkable_object->next_same_gcset = unwalked_list;

	unwalked_list = walkable_object;
}

SLAKE_API void GCWalkContext::update_unwalked_list(Object *deleted_object) {
	MutexGuard access_mutex_guard(access_mutex);

	if (unwalked_list == deleted_object)
		unwalked_list = deleted_object->next_same_gcset;

	remove_from_cur_gcset(deleted_object);
}

SLAKE_API Object *GCWalkContext::get_walked_list() {
	return walked_list;
}

SLAKE_API void GCWalkContext::push_walked(Object *walked_object) {
	MutexGuard access_mutex_guard(access_mutex);

	if (walked_list)
		walked_list->prev_same_gcset = walked_object;

	walked_object->next_same_gcset = walked_list;

	walked_list = walked_object;
}

SLAKE_API InstanceObject *GCWalkContext::get_destructible_list() {
	MutexGuard access_mutex_guard(access_mutex);

	auto p = destructible_list;
	destructible_list = nullptr;

	return p;
}

SLAKE_API void GCWalkContext::push_destructible(InstanceObject *v) {
	MutexGuard access_mutex_guard(access_mutex);
}

SLAKE_API void GCWalkContext::reset() {
	walkable_list = nullptr;
	unwalked_instance_list = nullptr;
	destructible_list = nullptr;
	unwalked_list = nullptr;
}

enum class GCTarget : uint8_t {
	TypeDef = 0,
	All
};

SLAKE_API void Runtime::_gc_serial(Object *&object_list, Object *&end_object_out, size_t &num_objects, ObjectGeneration new_generation, peff::Alloc *new_generation_allocator) {
	size_t iteration_times = 0;
rescan:
	++iteration_times;
	GCWalkContext context;

	Object *host_ref_list = nullptr;

	{
		Object *prev = nullptr;

		for (Object *i = object_list; i; i = i->next_same_gen_object) {
			i->gc_status = ObjectGCStatus::Unwalked;

			i->next_same_gcset = nullptr;
			i->prev_same_gcset = nullptr;

			// Check if the object is referenced by the host, if so, exclude them into a separated list.
			size_t host_ref_count = i->host_ref_count;
			switch (host_ref_count) {
				case 0:
					context.push_unwalked(i);
					break;
				default:
					if (host_ref_list)
						host_ref_list->prev_same_gcset = i;
					i->next_same_gcset = host_ref_list;
					host_ref_list = i;
			}

			switch (i->get_object_kind()) {
				case ObjectKind::Instance: {
					InstanceObject *value = (InstanceObject *)i;

					/* if (!(value->_flags & VF_DESTRUCTED)) {
						context.push_destructible(value);
					}*/
					break;
				}
				default:
					break;
			}

			prev = i;
		}

		end_object_out = prev;
	}

	for (Object *i = host_ref_list, *next; i; i = next) {
		next = i->next_same_gcset;
		context.push_object(i);
	}

	for (auto i : _generic_cache_dir) {
		for (auto j : i.second) {
			context.push_object(i.first);
			context.push_object(j.second);
		}
	}

	for (auto i : managed_thread_runnables) {
		context.push_object(i.second->context.get());
	}

	if (!(runtime_flags & _RT_DEINITING)) {
		// Walk the root node.
		context.push_object(_root_object);

		// Walk contexts for each thread.
		for (auto &i : active_contexts)
			context.push_object(i.second);
	}

	for (Object *p = context.get_walkable_list(), *i; p;) {
		i = p;

		while (i) {
			Object *next = i->next_same_gcset;

			switch (i->gc_status) {
				case ObjectGCStatus::Unwalked:
					std::terminate();
					break;
				case ObjectGCStatus::ReadyToWalk:
					_gc_walk(&context, i);
					break;
				case ObjectGCStatus::Walked:
					std::terminate();
					break;
			}

			i = next;
		}

		p = context.get_walkable_list();
	}

	bool is_rescan_needed = false;

	/*
	if (InstanceObject *p = context.get_destructible_list(); p) {
		_destruct_destructible_objects(p);
		is_rescan_needed = true;
	}*/

	if (is_rescan_needed) {
		goto rescan;
	}

	size_t num_deleted_objects = 0;
	// Delete unreachable objects.
	GCTarget cur_gc_target = GCTarget::TypeDef;
	bool clear_unwalked_list = false;

rescan_deletables:
	Object *i = context.get_unwalked_list(clear_unwalked_list);

	bool update_unwalked_list;
	switch (cur_gc_target) {
		case GCTarget::TypeDef: {
			if (!is_type_def_object(i))
				update_unwalked_list = true;
			else
				update_unwalked_list = false;
			break;
		}
		case GCTarget::All:
			update_unwalked_list = false;
			break;
		default:
			std::terminate();
	}

	for (Object *next; i; i = next) {
		next = i->next_same_gcset;

		switch (cur_gc_target) {
			case GCTarget::TypeDef:
				if (!is_type_def_object(i))
					continue;
				unregister_type_def((TypeDefObject *)i);
				break;
			case GCTarget::All:
				break;
			default:
				std::terminate();
		}

		if (i == object_list) {
			object_list = i->next_same_gen_object;
			assert(!i->prev_same_gen_object);
		}

		if (end_object_out == i) {
			if (i->prev_same_gen_object) {
				end_object_out = i->prev_same_gen_object;
			} else {
				end_object_out = nullptr;
			}
			assert(!i->next_same_gen_object);
		}

		if (i->prev_same_gen_object) {
			i->prev_same_gen_object->next_same_gen_object = i->next_same_gen_object;
		}

		if (i->next_same_gen_object) {
			i->next_same_gen_object->prev_same_gen_object = i->prev_same_gen_object;
		}

		if (update_unwalked_list)
			context.update_unwalked_list(i);
		i->dealloc();

		++num_deleted_objects;
	}

	switch (cur_gc_target) {
		case GCTarget::TypeDef:
			cur_gc_target = GCTarget::All;
			clear_unwalked_list = true;
			goto rescan_deletables;
		case GCTarget::All:
			break;
		default:
			std::terminate();
	}

	num_objects -= num_deleted_objects;

	if (new_generation_allocator) {
		for (Object *i = context.get_walked_list(), *next; i; i = next) {
			next = i->next_same_gcset;
			context.remove_from_cur_gcset(i);
			i->object_generation = new_generation;
			i->replace_allocator(new_generation_allocator);
			i->gc_status = ObjectGCStatus::Unwalked;
		}
	} else {
		for (Object *i = context.get_walked_list(), *next; i; i = next) {
			next = i->next_same_gcset;
			context.remove_from_cur_gcset(i);
			i->object_generation = new_generation;
			i->gc_status = ObjectGCStatus::Unwalked;
		}
	}
}

SLAKE_API void Runtime::ParallelGcThreadRunnable::run() {
	thread_state = ParallelGcThreadState::Alive;
	for (;;) {
		while (!is_active) {
			active_cond.wait();

			yield_current_thread();
		}

		is_active = false;

		if (thread_state == ParallelGcThreadState::NotifyTermination) {
			break;
		}

		// TODO: Implement it.

		is_done = true;
		while (is_done) {
			done_cond.notify_all();
			yield_current_thread();
		}
	}

	thread_state = ParallelGcThreadState::Terminated;
	return;
}

SLAKE_API void Runtime::ParallelGcThreadRunnable::dealloc() {
	peff::destroy_and_release<ParallelGcThreadRunnable>(runtime->get_fixed_alloc(), this, alignof(ParallelGcThreadRunnable));
}

SLAKE_API void Runtime::_gc_parallel_heapless(Object *&object_list, Object *&end_object_out, size_t &num_objects, ObjectGeneration new_generation) {
rescan:
	size_t num_recorded_objects = num_objects;

	parallel_gc_thread_runnable->context.reset();
	parallel_gc_thread_runnable->is_active = true;
	parallel_gc_thread_runnable->active_cond.notify_all();

	while (!parallel_gc_thread_runnable->is_done) {
		yield_current_thread();
		parallel_gc_thread_runnable->done_cond.wait();
	}
	parallel_gc_thread_runnable->is_done = false;
}

SLAKE_API Runtime::ParallelGcThreadRunnable::ParallelGcThreadRunnable(Runtime *runtime) : runtime(runtime) {
}

SLAKE_API bool Runtime::_alloc_parallel_gc_resources() {
	if (!(parallel_gc_thread_runnable = std::unique_ptr<ParallelGcThreadRunnable, peff::DeallocableDeleter<ParallelGcThreadRunnable>>(peff::alloc_and_construct<ParallelGcThreadRunnable>(get_fixed_alloc(), alignof(ParallelGcThreadRunnable), this)))) {
		return false;
	}

	if (!(parallel_gc_thread = std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>>(Thread::alloc(get_fixed_alloc(), parallel_gc_thread_runnable.get(), 4096)))) {
		return false;
	}

	parallel_gc_thread->start();

	return true;
}

SLAKE_API void Runtime::_release_parallel_gc_resources() {
	if (parallel_gc_thread_runnable->thread_state != ParallelGcThreadState::Uninit) {
		parallel_gc_thread_runnable->thread_state = ParallelGcThreadState::NotifyTermination;

		parallel_gc_thread_runnable->is_active = true;

		parallel_gc_thread_runnable->active_cond.notify_all();

		while (parallel_gc_thread_runnable->thread_state == ParallelGcThreadState::NotifyTermination)
			yield_current_thread();
	}

	parallel_gc_thread_runnable.reset();
	parallel_gc_thread.reset();
}
