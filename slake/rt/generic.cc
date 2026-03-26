#include <slake/runtime.h>

using namespace slake;

struct TypeSlotGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	TypeRef *slot;
};

struct ValueSlotGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Value *slot;
};

struct ValueInitGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Reference dest;
	Value value;
};

struct ObjectGenericInstantiationTask {
	peff::RcObjectPtr<Runtime::GenericInstantiationContext> context;
	Object *obj;
};

struct Runtime::GenericInstantiationDispatcher {
	bool has_next = false;

	peff::List<TypeSlotGenericInstantiationTask> next_walk_type_slots;
	peff::List<ValueSlotGenericInstantiationTask> next_walk_value_slots;
	peff::List<ValueInitGenericInstantiationTask> next_walk_value_inits;
	peff::List<ObjectGenericInstantiationTask> next_walk_objects;

	SLAKE_FORCEINLINE GenericInstantiationDispatcher(peff::Alloc *self_allocator) : next_walk_type_slots(self_allocator), next_walk_value_slots(self_allocator), next_walk_value_inits(self_allocator), next_walk_objects(self_allocator) {}

	SLAKE_FORCEINLINE InternalExceptionPointer push_type_slot(TypeSlotGenericInstantiationTask &&slot) noexcept {
		if (!next_walk_type_slots.push_back(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		has_next = true;

		return {};
	}

	SLAKE_FORCEINLINE InternalExceptionPointer push_type_slot(ValueSlotGenericInstantiationTask &&slot) noexcept {
		if (!next_walk_value_slots.push_back(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		has_next = true;

		return {};
	}

	SLAKE_FORCEINLINE InternalExceptionPointer push_value_init(ValueInitGenericInstantiationTask &&slot) noexcept {
		if (!next_walk_value_inits.push_back(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		has_next = true;

		return {};
	}

	SLAKE_FORCEINLINE InternalExceptionPointer push_object(ObjectGenericInstantiationTask &&slot) noexcept {
		if (!next_walk_objects.push_back(std::move(slot))) {
			return OutOfMemoryError::alloc();
		}

		has_next = true;

		return {};
	}
};

SLAKE_API void Runtime::invalidate_generic_cache(MemberObject *i) {
	if (_generic_cache_lookup_table.contains(i)) {
		// Remove the value from generic cache if it is unreachable.
		auto &lookup_entry = _generic_cache_lookup_table.at(i);

		auto &table = _generic_cache_dir.at(lookup_entry.original_object);
		table.remove(lookup_entry.generic_args);

		if (!table.size())
			_generic_cache_dir.remove(lookup_entry.original_object);

		_generic_cache_lookup_table.remove(i);
	}
}

SLAKE_API InternalExceptionPointer Runtime::set_generic_cache(MemberObject *object, const peff::DynArray<Value> &generic_args, MemberObject *instantiated_object) {
	if (!_generic_cache_dir.contains(object)) {
		if (!_generic_cache_dir.insert(+object, GenericCacheTable(get_fixed_alloc(), GenericArgListComparator())))
			return OutOfMemoryError::alloc();
	}
	// Store the instance into the cache.
	auto &cache_table = _generic_cache_dir.at(object);

	if (!cache_table.contains(generic_args)) {
		peff::DynArray<Value> copied_generic_args(get_fixed_alloc());
		if (!copied_generic_args.build(generic_args)) {
			return OutOfMemoryError::alloc();
		}

		cache_table.insert(std::move(copied_generic_args), +instantiated_object);
	} else {
		cache_table.at(generic_args) = instantiated_object;
	}

	{
		peff::DynArray<Value> copied_generic_arg_list(get_fixed_alloc());
		if (!copied_generic_arg_list.build(generic_args)) {
			return OutOfMemoryError::alloc();
		}
		_generic_cache_lookup_table.insert(+instantiated_object, { object, std::move(copied_generic_arg_list) });
	}

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiate_module_fields(GenericInstantiationDispatcher &dispatcher, BasicModuleObject *mod, GenericInstantiationContext *instantiation_context) {
	size_t sz_relocated_local_field_storage = 0;

	HostObjectRef<ModuleObject> tmp_mod;

	if (!(tmp_mod = ModuleObject::alloc(this))) {
		return OutOfMemoryError::alloc();
	}

	const size_t num_records = mod->field_records.size();

	tmp_mod->field_records = std::move(mod->field_records);
	tmp_mod->local_field_storage = std::move(mod->local_field_storage);

	mod->field_record_indices.clear_and_shrink();

	for (size_t i = 0; i < num_records; ++i) {
		const FieldRecord &cur_old_field_record = tmp_mod->field_records.at(i);

		{
			FieldRecord cur_field_record(tmp_mod->self_allocator.get());
			cur_field_record.type = cur_old_field_record.type;
			cur_field_record.access_modifier = cur_old_field_record.access_modifier;
			if (!cur_field_record.name.build(cur_old_field_record.name)) {
				return OutOfMemoryError::alloc();
			}

			if (!mod->append_field_record(std::move(cur_field_record))) {
				return OutOfMemoryError::alloc();
			}
		}

		FieldRecord &cur_field_record = mod->field_records.back();

		SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, cur_field_record.type, instantiation_context));
		Value data;
		read_var(StaticFieldRef(mod, i), data);
		SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, StaticFieldRef(mod, i), data, instantiation_context));
	}

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, Reference dest, Value value, GenericInstantiationContext *instantiation_context) {
	return dispatcher.push_value_init({ instantiation_context, dest, value });
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, TypeRef &type, GenericInstantiationContext *instantiation_context) {
	return dispatcher.push_type_slot({ instantiation_context, &type });
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, Value &value, GenericInstantiationContext *instantiation_context) {
	switch (value.value_type) {
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
		case ValueType::Reference:
		case ValueType::RegIndex:
			break;
		case ValueType::TypeName:
			SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, value.get_type_name(), instantiation_context));
			break;
		case ValueType::Undefined:
			break;
		default:
			throw std::logic_error("Unhandled value type");
	}
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, Object *v, GenericInstantiationContext *instantiation_context) {
	return dispatcher.push_object({ instantiation_context, v });
}

InternalExceptionPointer Runtime::_map_generic_params(const Object *v, GenericInstantiationContext *instantiation_context) const {
	instantiation_context->mapped_object = v;

	switch (v->get_object_kind()) {
		case ObjectKind::Class: {
			ClassObject *value = (ClassObject *)v;

			if (instantiation_context->generic_args->size() != value->generic_params.size()) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MismatchedGenericArgumentNumberError::alloc(
						const_cast<Runtime *>(this)->get_fixed_alloc()));
			}

			for (size_t i = 0; i < value->generic_params.size(); ++i) {
				if (!instantiation_context->mapped_generic_args.insert(value->generic_params.at(i).name, Value(instantiation_context->generic_args->at(i))))
					return OutOfMemoryError::alloc();
			}
			break;
		}
		case ObjectKind::Interface: {
			InterfaceObject *value = (InterfaceObject *)v;

			if (instantiation_context->generic_args->size() != value->generic_params.size()) {
				return alloc_out_of_memory_error_if_alloc_failed(
					MismatchedGenericArgumentNumberError::alloc(
						const_cast<Runtime *>(this)->get_fixed_alloc()));
			}

			for (size_t i = 0; i < value->generic_params.size(); ++i) {
				if (!instantiation_context->mapped_generic_args.insert(value->generic_params.at(i).name, Value(instantiation_context->generic_args->at(i))))
					return OutOfMemoryError::alloc();
			}
			break;
		}
		case ObjectKind::FnOverloading:
			SLAKE_RETURN_IF_EXCEPT(_map_generic_params(static_cast<const FnOverloadingObject *>(v), instantiation_context));
			break;
		case ObjectKind::Fn:
			break;
		default:
			std::terminate();
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_map_generic_params(const FnOverloadingObject *ol, GenericInstantiationContext *instantiation_context) const {
	if (instantiation_context->generic_args->size() != ol->generic_params.size()) {
		return alloc_out_of_memory_error_if_alloc_failed(
			MismatchedGenericArgumentNumberError::alloc(
				const_cast<Runtime *>(this)->get_fixed_alloc()));
	}

	for (size_t i = 0; i < ol->generic_params.size(); ++i) {
		Value copied_value = instantiation_context->generic_args->at(i);

		if (auto it = instantiation_context->mapped_generic_args.find(ol->generic_params.at(i).name);
			it != instantiation_context->mapped_generic_args.end()) {
			it.value() = copied_value;
		} else {
			if (!instantiation_context->mapped_generic_args.insert(ol->generic_params.at(i).name, std::move(copied_value)))
				return OutOfMemoryError::alloc();
		}
	}
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::instantiate_generic_object(MemberObject *object, MemberObject *&object_out, GenericInstantiationContext *instantiation_context) {
	// Try to look up in the cache.
	if (_generic_cache_dir.contains(object)) {
		auto &table = _generic_cache_dir.at(object);
		if (auto it = table.find(*instantiation_context->generic_args); it != table.end()) {
			// Cache hit, return.
			object_out = it.value();
			return {};
		}
		// Cache missed, go to the fallback.
	}

	Duplicator duplicator(this, get_cur_gen_alloc());

	// Cache missed, instantiate the value.
	MemberObject *duplicated_value = (MemberObject *)object->duplicate(&duplicator);	 // Make a duplicate of the original value.

	while (duplicator.tasks.size()) {
		if (!duplicator.exec()) {
			gc();
			return OutOfMemoryError::alloc();
		}
	}
	SLAKE_RETURN_IF_EXCEPT(_map_generic_params(duplicated_value, instantiation_context));

	GenericInstantiationDispatcher dispatcher(get_fixed_alloc());
	// Instantiate the value.
	SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, duplicated_value, instantiation_context));

	size_t iterate_times = 0;

	peff::Set<CustomTypeDefObject *> deferred_type_defs(get_fixed_alloc());
	peff::Set<FnObject *> fns(get_fixed_alloc());

	while (dispatcher.has_next) {
		dispatcher.has_next = false;

		++iterate_times;

		{
			auto next_walk_type_slots = std::move(dispatcher.next_walk_type_slots);

			dispatcher.next_walk_type_slots = peff::List<TypeSlotGenericInstantiationTask>(get_fixed_alloc());

			for (const auto &i : next_walk_type_slots) {
				TypeRef &type = *i.slot;

				switch (type.type_id) {
					/* case TypeId::Instance: {
						auto type_def = (type.type_def.get_custom_type_def());

						if (type_def->is_loading_deferred()) {
							IdRefObject *ex_data = (IdRefObject *)type_def->type_object;
							for (auto &j : ex_data->entries) {
								for (auto &k : j.generic_args) {
									SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, k, i.context.get()));
								}
							}

							if (!deferred_type_defs.insert(+type_def))
								return OutOfMemoryError::alloc();
						} else {
							HostObjectRef<IdRefObject> id_ref_object = IdRefObject::alloc((Runtime *)this);

							if (!id_ref_object) {
								return OutOfMemoryError::alloc();
							}

							if (!get_full_ref(id_ref_object->self_allocator.get(), (MemberObject *)type_def->type_object, id_ref_object->entries))
								return OutOfMemoryError::alloc();

							HostObjectRef<CustomTypeDefObject> type_def_object = CustomTypeDefObject::alloc((Runtime *)this);

							if (!type_def_object) {
								return OutOfMemoryError::alloc();
							}

							type_def_object->type_object = id_ref_object.get();

							// TODO: Add HostRefHolder for id_ref_object.
							type = TypeRef(type.type_id, type_def_object.get());

							SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, type, i.context.get()));
						}
						break;
					}*/
					case TypeId::Array: {
						bool is_succeeded;
						type = type.duplicate(is_succeeded);
						if (!is_succeeded)
							return OutOfMemoryError::alloc();
						SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, (type.get_array_type_def())->element_type->type_ref, i.context.get()));
						if (auto td = get_equal_type_def(type.type_def); td)
							type.type_def = td;
						else
							register_type_def(type.type_def);
						break;
					}
					case TypeId::Ref: {
						bool is_succeeded;
						type = type.duplicate(is_succeeded);
						if (!is_succeeded)
							return OutOfMemoryError::alloc();
						SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, (type.get_ref_type_def())->referenced_type->type_ref, i.context.get()));
						if (auto td = get_equal_type_def(type.type_def); td)
							type.type_def = td;
						else
							register_type_def(type.type_def);
						break;
					}
					case TypeId::ParamTypeList: {
						bool is_succeeded;
						type = type.duplicate(is_succeeded);
						if (!is_succeeded)
							return OutOfMemoryError::alloc();
						for (auto j : ((ParamTypeListTypeDefObject*)type.type_def)->param_types)
							SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, j->type_ref, i.context.get()));
						if (auto td = get_equal_type_def(type.type_def); td)
							type.type_def = td;
						else
							register_type_def(type.type_def);
						break;
					}
					case TypeId::Unpacking: {
						bool is_succeeded;
						type = type.duplicate(is_succeeded);
						if (!is_succeeded)
							return OutOfMemoryError::alloc();
						SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, (type.get_unpacking_type_def())->type->type_ref, i.context.get()));
						if (auto td = get_equal_type_def(type.type_def); td)
							type.type_def = td;
						else
							register_type_def(type.type_def);
						break;
					}
					case TypeId::GenericArg: {
						HostObjectRef<GenericArgTypeDefObject> type_def = type.get_generic_arg_type_def();
						HostObjectRef<StringObject> name_object = type_def->name_object;

						if (auto it = i.context->mapped_generic_args.find(name_object->data); it != i.context->mapped_generic_args.end()) {
							Value fetched_arg = it.value();
							if (fetched_arg.value_type != ValueType::Invalid) {
								if (fetched_arg.value_type != ValueType::TypeName) {
									peff::String name(get_fixed_alloc());
									if (!name.build(name_object->data))
										return OutOfMemoryError::alloc();
									return alloc_out_of_memory_error_if_alloc_failed(
										GenericArgTypeError::alloc(
											const_cast<Runtime *>(this)->get_fixed_alloc(),
											std::move(name)));
								}
								type = fetched_arg.get_type_name();
							}
						}
						else {
							peff::String param_name(get_fixed_alloc());
							if (!param_name.build(name_object->data)) {
								return OutOfMemoryError::alloc();
							}

							return alloc_out_of_memory_error_if_alloc_failed(
								GenericParameterNotFoundError::alloc(
									const_cast<Runtime*>(this)->get_fixed_alloc(),
									std::move(param_name)));
						}
						break;
					}
				}
			}
		}

		{
			auto next_walk_value_slots = std::move(dispatcher.next_walk_value_slots);

			dispatcher.next_walk_type_slots = peff::List<TypeSlotGenericInstantiationTask>(get_fixed_alloc());

			for (const auto &i : next_walk_value_slots) {
				Value &value = *i.slot;

				switch (value.value_type) {
					case ValueType::TypeName: {
						TypeRef type = value.get_type_name();

						if (type.type_id == TypeId::GenericArg) {
							HostObjectRef<GenericArgTypeDefObject> type_def = type.get_generic_arg_type_def();
							HostObjectRef<StringObject> name_object = type_def->name_object;

							if (auto it = i.context->mapped_generic_args.find(name_object->data); it != i.context->mapped_generic_args.end()) {
								if (it.value().value_type != ValueType::Invalid)
									value = it.value();
							} else {
								peff::String param_name(get_fixed_alloc());
								if (!param_name.build(name_object->data)) {
									return OutOfMemoryError::alloc();
								}

								return alloc_out_of_memory_error_if_alloc_failed(
									GenericParameterNotFoundError::alloc(
										const_cast<Runtime *>(this)->get_fixed_alloc(),
										std::move(param_name)));
							}
						}
					}
				}
			}
		}

		{
			auto next_walk_value_inits = std::move(dispatcher.next_walk_value_inits);

			dispatcher.next_walk_value_inits = peff::List<ValueInitGenericInstantiationTask>(get_fixed_alloc());

			for (auto &i : next_walk_value_inits) {
				if (auto e = write_var_checked(i.dest, i.value); e) {
					e.reset();
					return GenericFieldInitError::alloc(const_cast<Runtime *>(this)->get_fixed_alloc(), i.dest.as_static_field.module_object, i.dest.as_static_field.index);
				}
			}
		}

		{
			auto next_walk_objects = std::move(dispatcher.next_walk_objects);

			dispatcher.next_walk_objects = peff::List<ObjectGenericInstantiationTask>(get_fixed_alloc());

			for (const auto &i : next_walk_objects) {
				Object *v = i.obj;

				switch (v->get_object_kind()) {
					case ObjectKind::Class: {
						ClassObject *const value = (ClassObject *)v;

						if (value->generic_params.size() && value != i.context->mapped_object) {
							peff::HashMap<std::string_view, Value> copied_mapped_generic_args(get_fixed_alloc());

							for (auto [k, v] : i.context->mapped_generic_args) {
								if (!(copied_mapped_generic_args.insert(std::string_view(k), Value(v))))
									return OutOfMemoryError::alloc();
							}

							peff::RcObjectPtr<GenericInstantiationContext> new_instantiation_context;

							if (!(new_instantiation_context = peff::alloc_and_construct<GenericInstantiationContext>(
									  get_fixed_alloc(), alignof(GenericInstantiationContext),
									  get_fixed_alloc(),
									  i.context->mapped_object,
									  i.context->generic_args,
									  std::move(copied_mapped_generic_args)))) {
								return OutOfMemoryError::alloc();
							}

							// Mark nested generic parameters which have the same name as
							// irreplaceable, thus they will not be replaced.
							for (size_t i = 0; i < value->generic_params.size(); ++i) {
								if (!new_instantiation_context->mapped_generic_args.insert(value->generic_params.at(i).name, ValueType::Invalid))
									return OutOfMemoryError::alloc();
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, value->base_type, new_instantiation_context.get()));

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, it.value(), new_instantiation_context.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiate_module_fields(dispatcher, value, new_instantiation_context.get()));
						} else {
							// Save mapped generic arguments.
							if (value == i.context->mapped_object) {
								if (!value->generic_args.build(*i.context->generic_args))
									return OutOfMemoryError::alloc();
								for (auto i : i.context->mapped_generic_args) {
									if (!value->mapped_generic_args.insert(std::string_view(i.first), Value(i.second)))
										return OutOfMemoryError::alloc();
								}
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, value->base_type, i.context.get()));

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, it.value(), i.context.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiate_module_fields(dispatcher, value, i.context.get()));
						}

						break;
					}
					case ObjectKind::Interface: {
						InterfaceObject *const value = (InterfaceObject *)v;

						if (value->generic_params.size() && value != i.context->mapped_object) {
							peff::HashMap<std::string_view, Value> copied_mapped_generic_args(get_fixed_alloc());

							for (auto [k, v] : i.context->mapped_generic_args) {
								if (!(copied_mapped_generic_args.insert(std::string_view(k), Value(v))))
									return OutOfMemoryError::alloc();
							}

							peff::RcObjectPtr<GenericInstantiationContext> new_instantiation_context;

							if (!(new_instantiation_context = peff::alloc_and_construct<GenericInstantiationContext>(
									  get_fixed_alloc(), alignof(GenericInstantiationContext),
									  get_fixed_alloc(),
									  i.context->mapped_object,
									  i.context->generic_args,
									  std::move(copied_mapped_generic_args)))) {
								return OutOfMemoryError::alloc();
							}

							// Mark nested generic parameters which have the same name as
							// irreplaceable, thus they will not be replaced.
							for (size_t i = 0; i < value->generic_params.size(); ++i) {
								if (!new_instantiation_context->mapped_generic_args.insert(value->generic_params.at(i).name, ValueType::Invalid))
									return OutOfMemoryError::alloc();
							}

							for (auto it = value->impl_types.begin(); it != value->impl_types.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, *it, new_instantiation_context.get()));
							}

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, it.value(), new_instantiation_context.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiate_module_fields(dispatcher, value, new_instantiation_context.get()));
						} else {
							// Save mapped generic arguments.
							if (value == i.context->mapped_object) {
								if (!value->generic_args.build(*i.context->generic_args))
									return OutOfMemoryError::alloc();
								for (auto [k, v] : i.context->mapped_generic_args) {
									if (!(value->mapped_generic_args.insert(std::string_view(k), Value(v))))
										return OutOfMemoryError::alloc();
								}
							}

							for (auto it = value->impl_types.begin(); it != value->impl_types.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, *it, i.context.get()));
							}

							for (auto it = value->members.begin(); it != value->members.end(); ++it) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, it.value(), i.context.get()));
							}

							SLAKE_RETURN_IF_EXCEPT(_instantiate_module_fields(dispatcher, value, i.context.get()));
						}

						break;
					}
					case ObjectKind::Fn: {
						FnObject *value = (FnObject *)v;

						if (!fns.contains(value)) {
							if (!fns.insert(+value))
								return OutOfMemoryError::alloc();
						}

						if (i.context->mapped_object == value) {
							peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true> overloadings(value->overloadings.allocator());

							for (auto [signature, overloading] : value->overloadings) {
								if (overloading->generic_params.size() == instantiation_context->generic_args->size()) {
									peff::HashMap<std::string_view, Value> copied_mapped_generic_args(get_fixed_alloc());

									for (auto [k, v] : i.context->mapped_generic_args) {
										if (!(copied_mapped_generic_args.insert(std::string_view(k), Value(v))))
											return OutOfMemoryError::alloc();
									}

									peff::RcObjectPtr<GenericInstantiationContext> new_instantiation_context;

									if (!(new_instantiation_context = peff::alloc_and_construct<GenericInstantiationContext>(
											  get_fixed_alloc(), alignof(GenericInstantiationContext),
											  get_fixed_alloc(),
											  i.context->mapped_object,
											  i.context->generic_args,
											  std::move(copied_mapped_generic_args)))) {
										return OutOfMemoryError::alloc();
									}

									SLAKE_RETURN_IF_EXCEPT(_map_generic_params(overloading, new_instantiation_context.get()));

									SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, overloading, new_instantiation_context.get()));
									if (overloadings.contains({ overloading->param_types,
											overloading->is_with_var_args(),
											overloading->generic_params.size(),
											overloading->overriden_type }))
										return GenericDuplicatedFnOverloadingError::alloc(
											instantiation_context->self_allocator.get(),
											value);
									if (!overloadings.insert(
											{ overloading->param_types,
												overloading->is_with_var_args(),
												overloading->generic_params.size(),
												overloading->overriden_type },
											+overloading))
										return OutOfMemoryError::alloc();
								}
							}

							value->overloadings = std::move(overloadings);
						} else {
							peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true> overloadings(value->overloadings.allocator());

							for (auto j : value->overloadings) {
								size_t original_type_slot_size = dispatcher.next_walk_type_slots.size();
								if (j.second->generic_params.size()) {
									peff::HashMap<std::string_view, Value> copied_mapped_generic_args(get_fixed_alloc());

									for (auto [k, v] : i.context->mapped_generic_args) {
										if (!(copied_mapped_generic_args.insert(std::string_view(k), Value(v))))
											return OutOfMemoryError::alloc();
									}

									peff::RcObjectPtr<GenericInstantiationContext> new_instantiation_context;

									if (!(new_instantiation_context = peff::alloc_and_construct<GenericInstantiationContext>(
											  get_fixed_alloc(), alignof(GenericInstantiationContext),
											  get_fixed_alloc(),
											  i.context->mapped_object,
											  i.context->generic_args,
											  std::move(copied_mapped_generic_args)))) {
										return OutOfMemoryError::alloc();
									}

									// Mark nested generic parameters which have the same name as
									// irreplaceable, thus they will not be replaced.
									for (size_t i = 0; i < j.second->generic_params.size(); ++i) {
										if (!new_instantiation_context->mapped_generic_args.insert(j.second->generic_params.at(i).name, ValueType::Invalid))
											return OutOfMemoryError::alloc();
									}

									SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, j.second, new_instantiation_context.get()));
								} else {
									SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, j.second, i.context.get()));
								}
								if (overloadings.contains({ j.second->param_types,
										j.second->is_with_var_args(),
										j.second->generic_params.size(),
										j.second->overriden_type }))
									return GenericDuplicatedFnOverloadingError::alloc(
										instantiation_context->self_allocator.get(),
										value);
								if (!overloadings.insert(
										{ j.second->param_types,
											j.second->is_with_var_args(),
											j.second->generic_params.size(),
											j.second->overriden_type },
										+j.second))
									return OutOfMemoryError::alloc();
							}

							value->overloadings = std::move(overloadings);
						}
						break;
					}
					case ObjectKind::String:
						break;
					case ObjectKind::IdRef: {
						IdRefObject *value = (IdRefObject *)v;

						for (auto &j : value->entries) {
							for (auto &k : j.generic_args) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, k, i.context.get()));
							}
						}

						if (value->param_types.has_value()) {
							for (auto &j : *value->param_types) {
								SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, j, i.context.get()));
							}
						}

						break;
					}
					default:
						throw std::logic_error("Unhandled object type");
				}
			}
		}
	}

	for (auto i : fns) {
		peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true> overloadings(i->overloadings.allocator());

		for (auto j : i->overloadings) {
			// Expand the parameter pack.
			size_t original_param_num;
			size_t num_params;
			do {
				original_param_num = j.second->param_types.size();
				num_params = j.second->param_types.size();
				for (size_t k = 0; k < num_params; ++k) {
					auto &param = j.second->param_types.at(k);
					if (param.type_id == TypeId::Unpacking) {
						UnpackingTypeDefObject *type_def = ((UnpackingTypeDefObject *)param.type_def);
						ParamTypeListTypeDefObject *param_type_list = (ParamTypeListTypeDefObject *)type_def->type->type_ref.type_def;

						if (param_type_list->get_type_def_kind() != TypeDefKind::ParamTypeListTypeDef)
							std::terminate();

						if (!j.second->param_types.insert_range_uninit(k, param_type_list->param_types.size() - 1))
							return OutOfMemoryError::alloc();

						for (size_t l = 0; l < param_type_list->param_types.size(); ++l) {
							j.second->param_types.at(k + l) = param_type_list->param_types.at(l)->type_ref;
						}
						num_params = j.second->param_types.size();
					}
				}
			} while (original_param_num != num_params);
			if (overloadings.contains({ j.second->param_types,
					j.second->is_with_var_args(),
					j.second->generic_params.size(),
					j.second->overriden_type }))
				return GenericDuplicatedFnOverloadingError::alloc(
					instantiation_context->self_allocator.get(),
					i);
			if (!overloadings.insert(
					{ j.second->param_types,
						j.second->is_with_var_args(),
						j.second->generic_params.size(),
						j.second->overriden_type },
					+j.second))
				return OutOfMemoryError::alloc();
		}
	}

	/* for (auto i : deferred_type_defs) {
		slake::Reference entity_ref;
		SLAKE_RETURN_IF_EXCEPT(resolve_id_ref((IdRefObject*)i->type_object, entity_ref));

		if (!entity_ref)
			std::terminate();

		if (entity_ref.kind != ReferenceKind::ObjectRef)
			std::terminate();

		i->type_object = entity_ref.as_object;
	}*/

	SLAKE_RETURN_IF_EXCEPT(set_generic_cache(object, *instantiation_context->generic_args, duplicated_value));

	object_out = duplicated_value;

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_instantiate_generic_object(GenericInstantiationDispatcher &dispatcher, FnOverloadingObject *ol, GenericInstantiationContext *instantiation_context) {
	if (ol->generic_params.size() && ol->fn_object != instantiation_context->mapped_object) {
		peff::HashMap<std::string_view, Value> copied_mapped_generic_args(get_fixed_alloc());

		for (auto [k, v] : instantiation_context->mapped_generic_args) {
			if (!(copied_mapped_generic_args.insert(std::string_view(k), Value(v))))
				return OutOfMemoryError::alloc();
		}

		peff::NullAlloc tmp_alloc;
		GenericInstantiationContext new_instantiation_context = {
			&tmp_alloc,
			instantiation_context->mapped_object,
			instantiation_context->generic_args,
			std::move(copied_mapped_generic_args)
		};

		// Map irreplaceable parameters to corresponding generic parameter reference type
		// and thus the generic types will keep unchanged.
		for (size_t i = 0; i < ol->generic_params.size(); ++i) {
			if (!new_instantiation_context.mapped_generic_args.insert(ol->generic_params.at(i).name, ValueType::Invalid))
				return OutOfMemoryError::alloc();
		}

		new_instantiation_context.mapped_object = ol->fn_object;

		SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, ol, &new_instantiation_context));
	} else {
		for (auto &i : ol->param_types) {
			SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, i, instantiation_context));
		}

		SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, ol->return_type, instantiation_context));

		switch (ol->overloading_kind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *overloading = (RegularFnOverloadingObject *)ol;

				for (auto &i : overloading->instructions) {
					for (size_t j = 0; j < i.num_operands; ++j) {
						SLAKE_RETURN_IF_EXCEPT(_instantiate_generic_object(dispatcher, i.operands[j], instantiation_context));
					}
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *overloading = (NativeFnOverloadingObject *)ol;

				break;
			}
		}
	}
	return {};
}
