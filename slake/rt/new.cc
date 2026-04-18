#include <slake/runtime.h>
#include <peff/base/scope_guard.h>
#include <variant>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::init_method_table_for_class(ClassObject *cls, ClassObject *parent_class) {
	assert(!cls->cached_instantiated_method_table);
	MethodTable *parent_mt = parent_class ? parent_class->cached_instantiated_method_table.get() : nullptr;
	std::unique_ptr<MethodTable, peff::DeallocableDeleter<MethodTable>> method_table(MethodTable::alloc(cls->get_allocator()));

	if (parent_mt) {
		if (!method_table->destructors.resize(parent_mt->destructors.size())) {
			return OutOfMemoryError::alloc();
		}
		memcpy(method_table->destructors.data(), parent_mt->destructors.data(), method_table->destructors.size() * sizeof(void *));
	}

	for (auto it = cls->members.begin(); it != cls->members.end(); ++it) {
		switch (it.value()->get_object_kind()) {
			case ObjectKind::Fn: {
				FnObject *fn = (FnObject *)it.value();

				HostObjectRef<FnObject> fn_slot;

				fn_slot = FnObject::alloc(this);
				if (!fn_slot) {
					return OutOfMemoryError::alloc();
				}
				if (!fn_slot->set_name(((FnObject *)it.value())->get_name())) {
					return OutOfMemoryError::alloc();
				}

				if (it.key() == "delete") {
					peff::DynArray<TypeRef> destructor_param_types(get_fixed_alloc());

					for (auto j : fn->overloadings) {
						bool result;
						static FnSignatureComparator cmp;
						if (cmp(FnSignature(j.second->param_types, j.second->is_with_var_args(), j.second->generic_params.size(), j.second->overriden_type), FnSignature(destructor_param_types, false, 0, TypeId::Void))) {
							if (!method_table->destructors.push_front(+j.second)) {
								return OutOfMemoryError::alloc();
							}
							break;
						}
					}
				} else {
					for (auto j : fn->overloadings) {
						if (!fn_slot->overloadings.insert(FnSignature(j.first), +j.second))
							return OutOfMemoryError::alloc();
					}

					if (parent_mt) {
						if (auto m = parent_mt->get_method(fn->get_name()); m) {
							if (m->overloadings.size()) {
								// Link the method with method inherited from the parent.
								for (auto k : m->overloadings) {
									// If we found a non-duplicated overloading from the parent, add it.
									if (auto it = fn_slot->overloadings.find(k.first); it == fn_slot->overloadings.end()) {
										if (!fn_slot->overloadings.insert(FnSignature(k.first), +k.second))
											return OutOfMemoryError::alloc();
									}
								}
							}
						}
					}
				}

				if (fn_slot->overloadings.size()) {
					if (!method_table->methods.insert(fn_slot->get_name(), fn_slot.get()))
						return OutOfMemoryError::alloc();
				}

				break;
			}
		}
	}

	cls->cached_instantiated_method_table = method_table.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::init_object_layout_for_module(BasicModuleObject *mod, ObjectLayout *object_layout) {
	size_t cnt = 0;
	for (size_t i = 0; i < mod->field_records.size(); ++i) {
		FieldRecord &cls_field_record = mod->field_records.at(i);

		if (cls_field_record.access_modifier & ACCESS_STATIC) {
			continue;
		}

		ObjectFieldRecord field_record(mod->get_allocator());

		TypeRef type = cls_field_record.type;

		size_t size = sizeof_type(type);
		size_t align = alignof_type(type);

		if (align > 1) {
			if (size_t diff = object_layout->total_size % align; diff) {
				object_layout->total_size += align - diff;
			}
		}
		field_record.offset = object_layout->total_size;
		field_record.idx_init_field_record = i;

		field_record.type = type;
		if (!field_record.name.build(cls_field_record.name))
			return OutOfMemoryError::alloc();

		if (!object_layout->field_name_map.insert(field_record.name, object_layout->field_records.size()))
			return OutOfMemoryError::alloc();
		if (!object_layout->field_records.push_back(std::move(field_record)))
			return OutOfMemoryError::alloc();

		object_layout->total_size += size;

		++cnt;
	}

	if (!object_layout->field_record_init_module_fields_number.push_back({ mod, cnt }))
		return OutOfMemoryError::alloc();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::init_object_layout_for_class(ClassObject *cls, ClassObject *parent_class) {
	assert(!cls->cached_object_layout);
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> object_layout;

	if (parent_class && parent_class->cached_object_layout) {
		object_layout = decltype(object_layout)(parent_class->cached_object_layout->duplicate(cls->get_allocator()));
	} else {
		object_layout = decltype(object_layout)(ObjectLayout::alloc(cls->get_allocator()));
	}

	if (!object_layout)
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(init_object_layout_for_module(cls, object_layout.get()));

	// cls->cached_field_init_vars.shrink_to_fit();
	cls->cached_object_layout = object_layout.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::init_object_layout_for_struct(StructObject *s) {
	assert(!s->cached_object_layout);
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> object_layout(ObjectLayout::alloc(s->get_allocator()));

	if (!object_layout)
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(init_object_layout_for_module(s, object_layout.get()));

	// cls->cached_field_init_vars.shrink_to_fit();
	s->cached_object_layout = object_layout.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::init_object_layout_for_union_enum_item(UnionEnumItemObject *s) {
	assert(!s->cached_object_layout);
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> object_layout(ObjectLayout::alloc(s->get_allocator()));

	if (!object_layout)
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(init_object_layout_for_module(s, object_layout.get()));

	// cls->cached_field_init_vars.shrink_to_fit();
	s->cached_object_layout = object_layout.release();

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepare_class_for_instantiation(ClassObject *cls) {
	peff::List<ClassObject *> unprepared_classes(get_fixed_alloc());
	{
		ClassObject *p = cls;

		while (true) {
			if (!unprepared_classes.push_back(+p))
				return OutOfMemoryError::alloc();

			if (!p->base_type)
				break;

			if (p->base_type.type_id != TypeId::Instance)
				return alloc_oom_error_if_alloc_failed(MalformedClassStructureError::alloc(get_fixed_alloc(), p));

			Object *parent_class = (p->base_type.get_custom_type_def())->type_object;
			if (parent_class->get_object_kind() != ObjectKind::Class)
				return alloc_oom_error_if_alloc_failed(MalformedClassStructureError::alloc(get_fixed_alloc(), p));

			p = (ClassObject *)parent_class;
		}
	}

	ClassObject *c, *p = nullptr;

	while (unprepared_classes.size()) {
		c = unprepared_classes.back();

		if (!c->cached_object_layout)
			SLAKE_RETURN_IF_EXCEPT(init_object_layout_for_class(c, p));
		if (!c->cached_instantiated_method_table)
			SLAKE_RETURN_IF_EXCEPT(init_method_table_for_class(c, p));

		p = c;
		unprepared_classes.pop_back();
	}

	return {};
}

struct FieldsOpStructPreparationFrameExData {
	size_t index;
};

struct MembersOpStructPreparationFrameExData {
	BasicModuleObject::MembersMap::ConstIterator iter;
};

struct StructPreparationFrame {
	Object *struct_object;
	std::variant<std::monostate, FieldsOpStructPreparationFrameExData, MembersOpStructPreparationFrameExData> ex_data;
};

struct StructPreparationContext {
	peff::List<StructPreparationFrame> frames;

	SLAKE_FORCEINLINE StructPreparationContext(peff::Alloc *allocator) : frames(allocator) {}
};

SLAKE_FORCEINLINE InternalExceptionPointer _prepare_struct_for_instantiation(StructPreparationContext &context) {
	while (context.frames.size()) {
		StructPreparationFrame &cur_frame = context.frames.back();

		auto push_field_record_corresponding_type_object = [&context](const FieldRecord &cur_record) -> InternalExceptionPointer {
			TypeRef type_ref = cur_record.type;
			switch (cur_record.type.type_id) {
				case TypeId::StructInstance: {
					CustomTypeDefObject *td = type_ref.get_custom_type_def();
					assert(td->type_object->get_object_kind() == ObjectKind::Struct);
					if (!context.frames.push_back(
							{ (StructObject *)td->type_object,
								FieldsOpStructPreparationFrameExData{ 0 } }))
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::UnionEnum: {
					CustomTypeDefObject *td = type_ref.get_custom_type_def();
					assert(td->type_object->get_object_kind() == ObjectKind::UnionEnum);
					if (!context.frames.push_back(
							{ (UnionEnumObject *)td->type_object,
								MembersOpStructPreparationFrameExData{ ((UnionEnumObject *)td->type_object)->get_members().begin_const() } }))
						return OutOfMemoryError::alloc();
					break;
				}
				case TypeId::UnionEnumItem: {
					CustomTypeDefObject *td = type_ref.get_custom_type_def();
					assert(td->type_object->get_object_kind() == ObjectKind::UnionEnumItem);
					if (!context.frames.push_back(
							{ (UnionEnumItemObject *)td->type_object,
								FieldsOpStructPreparationFrameExData{ 0 } }))
						return OutOfMemoryError::alloc();
					break;
				}
			}
			return {};
		};

		switch (cur_frame.struct_object->get_object_kind()) {
			case ObjectKind::Struct: {
				StructObject *struct_object = (StructObject *)cur_frame.struct_object;
				FieldsOpStructPreparationFrameExData &ex_data = std::get<FieldsOpStructPreparationFrameExData>(cur_frame.ex_data);

				auto &field_records = struct_object->get_field_records();

				if (ex_data.index >= field_records.size()) {
					if (!struct_object->cached_object_layout)
						SLAKE_RETURN_IF_EXCEPT(struct_object->associated_runtime->init_object_layout_for_struct(struct_object));
					context.frames.pop_back();
					continue;
				}

				auto &cur_record = field_records.at(ex_data.index);

				SLAKE_RETURN_IF_EXCEPT(push_field_record_corresponding_type_object(cur_record));

				++ex_data.index;
				break;
			}
			case ObjectKind::UnionEnum: {
				UnionEnumObject *enum_object = (UnionEnumObject *)cur_frame.struct_object;
				MembersOpStructPreparationFrameExData &ex_data = std::get<MembersOpStructPreparationFrameExData>(cur_frame.ex_data);
				auto &field_records = enum_object->get_members();
				if (ex_data.iter == enum_object->get_members().end_const()) {
					// TODO: Find out the maximum alignment and the maximum size.
					size_t max_alignment = 0, max_size = 0;
					for (auto i : enum_object->get_members()) {
						if (i.second->get_object_kind() == ObjectKind::UnionEnumItem) {
							UnionEnumItemObject *item = (UnionEnumItemObject *)i.second;
							max_alignment = (std::max)(item->cached_object_layout->alignment, max_alignment);
							max_size = (std::max)(item->cached_object_layout->total_size, max_size);
						}
					}
					enum_object->cached_max_align = max_alignment;
					enum_object->cached_max_size = max_size;
					context.frames.pop_back();
					continue;
				}

				Object *m = ex_data.iter.value();

				assert(m->get_object_kind() == ObjectKind::UnionEnumItem);

				if (!context.frames.push_back({ (UnionEnumItemObject *)m, FieldsOpStructPreparationFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();

				++ex_data.iter;
				break;
			}
			case ObjectKind::UnionEnumItem: {
				UnionEnumItemObject *item_object = (UnionEnumItemObject *)cur_frame.struct_object;
				FieldsOpStructPreparationFrameExData &ex_data = std::get<FieldsOpStructPreparationFrameExData>(cur_frame.ex_data);

				auto &field_records = item_object->get_field_records();
				if (ex_data.index >= field_records.size()) {
					if (!item_object->cached_object_layout)
						SLAKE_RETURN_IF_EXCEPT(item_object->associated_runtime->init_object_layout_for_union_enum_item(item_object));
					context.frames.pop_back();
					continue;
				}

				auto &cur_record = field_records.at(ex_data.index);

				SLAKE_RETURN_IF_EXCEPT(push_field_record_corresponding_type_object(cur_record));

				++ex_data.index;
				break;
			}
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepare_struct_for_instantiation(StructObject *cls) {
	StructPreparationContext context(get_fixed_alloc());

	if (!context.frames.push_back({ cls, FieldsOpStructPreparationFrameExData{ 0 } }))
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(_prepare_struct_for_instantiation(context));

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepare_union_enum_for_instantiation(UnionEnumObject *cls) {
	StructPreparationContext context(get_fixed_alloc());

	if (!context.frames.push_back({ cls, MembersOpStructPreparationFrameExData{ cls->members.begin_const() } }))
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(_prepare_struct_for_instantiation(context));

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::prepare_union_enum_item_for_instantiation(UnionEnumItemObject *cls) {
	StructPreparationContext context(get_fixed_alloc());

	if (!context.frames.push_back({ cls, FieldsOpStructPreparationFrameExData{ 0 } }))
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(_prepare_struct_for_instantiation(context));

	return {};
}

/// @brief Create a new class instance.
/// @param cls Class for instance creation.
/// @return Created instance of the class.
///
/// @note This function normalizes loading-deferred types.
SLAKE_API HostObjectRef<InstanceObject> slake::Runtime::new_class_instance(ClassObject *cls, NewClassInstanceFlags flags) {
	HostObjectRef<InstanceObject> instance;
	InternalExceptionPointer e;

	if ((e = prepare_class_for_instantiation(cls))) {
		e.reset();
		return {};
	}

	instance = InstanceObject::alloc(this);

	if (cls->cached_object_layout->total_size)
		instance->raw_field_data = new char[cls->cached_object_layout->total_size];
	instance->sz_raw_field_data = cls->cached_object_layout->total_size;

	instance->_class = cls;

	//
	// Initialize the fields.
	//
	size_t index = 0, cnt = 0;
	std::pair<BasicModuleObject *, size_t> p = cls->cached_object_layout->field_record_init_module_fields_number.at(0);

	for (size_t i = 0; i < cls->field_records.size(); ++i) {
		const ObjectFieldRecord &field_record = cls->cached_object_layout->field_records.at(i);

		Value data;
		read_var(StaticFieldRef(p.first, field_record.idx_init_field_record), data);
		write_var(ObjectFieldRef(instance.get(), i), data);

		if (cnt++ >= p.second) {
			cnt = 0;
			p = cls->cached_object_layout->field_record_init_module_fields_number.at(++index);
		}
	}

	return instance;
}

SLAKE_API HostObjectRef<ArrayObject> Runtime::new_array_instance(Runtime *rt, const TypeRef &type, size_t length) {
	switch (type.type_id) {
		case TypeId::I8: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int8_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(int8_t) * length, alignof(int8_t))))
				return nullptr;
			obj->element_alignment = alignof(int8_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::I16: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int16_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(int16_t) * length, alignof(int16_t))))
				return nullptr;
			obj->element_alignment = alignof(int16_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::I32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int32_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(int32_t) * length, alignof(int32_t))))
				return nullptr;
			obj->element_alignment = alignof(int32_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::I64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(int64_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(int64_t) * length, alignof(int64_t))))
				return nullptr;
			obj->element_alignment = alignof(int64_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U8: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint8_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(uint8_t) * length, alignof(uint8_t))))
				return nullptr;
			obj->element_alignment = alignof(uint8_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U16: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint16_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(uint16_t) * length, alignof(uint16_t))))
				return nullptr;
			obj->element_alignment = alignof(uint16_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint32_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(uint32_t) * length, alignof(uint32_t))))
				return nullptr;
			obj->element_alignment = alignof(uint32_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::U64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(uint64_t));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(uint64_t) * length, alignof(uint64_t))))
				return nullptr;
			obj->element_alignment = alignof(uint64_t);
			obj->length = length;
			return obj.get();
		}
		case TypeId::F32: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(float));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(float) * length, alignof(float))))
				return nullptr;
			obj->element_alignment = alignof(float);
			obj->length = length;
			return obj.get();
		}
		case TypeId::F64: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(double));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(double) * length, alignof(double))))
				return nullptr;
			obj->element_alignment = alignof(double);
			obj->length = length;
			return obj.get();
		}
		case TypeId::Bool: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(bool));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(bool) * length, alignof(bool))))
				return nullptr;
			obj->element_alignment = alignof(bool);
			obj->length = length;
			return obj.get();
		}
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Reference));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(Reference) * length, alignof(Reference))))
				return nullptr;
			obj->element_alignment = alignof(Reference);
			obj->length = length;
			return obj.get();
		}
		case TypeId::Any: {
			HostObjectRef<ArrayObject> obj = ArrayObject::alloc(this, type, sizeof(Value));
			if (!(obj->data = obj->get_allocator()->alloc(sizeof(Value) * length, alignof(Value))))
				return nullptr;
			obj->element_alignment = alignof(Value);
			obj->length = length;
			return obj.get();
		}
		default:
			throw std::logic_error("Unhandled element type");
	}
}
