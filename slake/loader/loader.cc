#include "loader.h"

using namespace slake;
using namespace slake::loader;

SLAKE_API LoaderContext::LoaderContext(peff::Alloc *allocator)
	: allocator(allocator),
	  loaded_id_refs(allocator),
	  loaded_custom_type_defs(allocator),
	  loaded_interfaces(allocator),
	  loaded_structs(allocator),
	  loaded_classes(allocator),
	  loaded_scoped_enums(allocator),
	  loaded_union_enums(allocator),
	  loaded_union_enum_items(allocator),
	  loaded_fns(allocator),
	  loaded_modules(allocator),
	  init_var_data(allocator),
	  host_ref_holder(allocator) {
}

SLAKE_API LoaderContext::~LoaderContext() {
}

SLAKE_FORCEINLINE static InternalExceptionPointer _normalize_read_result(Runtime *runtime, ReadResult read_result) {
	switch (read_result) {
		case ReadResult::Succeeded:
			break;
		case ReadResult::ReadError:
			return alloc_out_of_memory_error_if_alloc_failed(ReadError::alloc(runtime->get_fixed_alloc()));
		default:
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_type(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, TypeRef &type_out) noexcept {
	slxfmt::TypeId type_id;

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8((uint8_t &)type_id)));

	switch (type_id) {
		case slake::slxfmt::TypeId::Void:
			type_out = TypeId::Void;
			break;
		case slake::slxfmt::TypeId::Any:
			type_out = TypeId::Any;
			break;
		case slake::slxfmt::TypeId::I8:
			type_out = TypeId::I8;
			break;
		case slake::slxfmt::TypeId::I16:
			type_out = TypeId::I16;
			break;
		case slake::slxfmt::TypeId::I32:
			type_out = TypeId::I32;
			break;
		case slake::slxfmt::TypeId::I64:
			type_out = TypeId::I64;
			break;
		case slake::slxfmt::TypeId::U8:
			type_out = TypeId::U8;
			break;
		case slake::slxfmt::TypeId::U16:
			type_out = TypeId::U16;
			break;
		case slake::slxfmt::TypeId::U32:
			type_out = TypeId::U32;
			break;
		case slake::slxfmt::TypeId::U64:
			type_out = TypeId::U64;
			break;
		case slake::slxfmt::TypeId::F32:
			type_out = TypeId::F32;
			break;
		case slake::slxfmt::TypeId::F64:
			type_out = TypeId::F64;
			break;
		case slake::slxfmt::TypeId::String:
			type_out = TypeId::String;
			break;
		case slake::slxfmt::TypeId::Bool:
			type_out = TypeId::Bool;
			break;
		case slake::slxfmt::TypeId::Array: {
			HostObjectRef<ArrayTypeDefObject> type_def;

			if (!(type_def = ArrayTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			HostObjectRef<HeapTypeObject> heap_type;

			if (!(heap_type = HeapTypeObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, heap_type->type_ref));

			type_def->element_type = heap_type.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::Array, td);
			} else {
				type_out = TypeRef(TypeId::Array, type_def.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Object: {
			HostObjectRef<CustomTypeDefObject> type_def;

			if (!(type_def = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> id_ref;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref));

			type_def->type_object = id_ref.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::Instance, td);
			} else {
				type_out = TypeRef(TypeId::Instance, type_def.get());
				if (!context.loaded_custom_type_defs.insert(type_def.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Struct: {
			HostObjectRef<CustomTypeDefObject> type_def;

			if (!(type_def = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> id_ref;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref));

			type_def->type_object = id_ref.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::StructInstance, td);
			} else {
				type_out = TypeRef(TypeId::StructInstance, type_def.get());
				if (!context.loaded_custom_type_defs.insert(type_def.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::ScopedEnum: {
			HostObjectRef<CustomTypeDefObject> type_def;

			if (!(type_def = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> id_ref;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref));

			type_def->type_object = id_ref.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::ScopedEnum, td);
			} else {
				type_out = TypeRef(TypeId::ScopedEnum, type_def.get());
				if (!context.loaded_custom_type_defs.insert(type_def.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::TypelessScopedEnum: {
			HostObjectRef<CustomTypeDefObject> type_def;

			if (!(type_def = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> id_ref;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref));

			type_def->type_object = id_ref.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::TypelessScopedEnum, td);
			} else {
				type_out = TypeRef(TypeId::TypelessScopedEnum, type_def.get());
				if (!context.loaded_custom_type_defs.insert(type_def.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::UnionEnum: {
			HostObjectRef<CustomTypeDefObject> type_def;

			if (!(type_def = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> id_ref;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref));

			type_def->type_object = id_ref.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::UnionEnum, td);
			} else {
				type_out = TypeRef(TypeId::UnionEnum, type_def.get());
				if (!context.loaded_custom_type_defs.insert(type_def.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::UnionEnumItem: {
			HostObjectRef<CustomTypeDefObject> type_def;

			if (!(type_def = CustomTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}
			HostObjectRef<IdRefObject> id_ref;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref));

			type_def->type_object = id_ref.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::UnionEnumItem, td);
			} else {
				type_out = TypeRef(TypeId::UnionEnumItem, type_def.get());
				if (!context.loaded_custom_type_defs.insert(type_def.get()))
					return OutOfMemoryError::alloc();
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::GenericArg: {
			HostObjectRef<StringObject> name_object;

			if (!(name_object = StringObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t name_len;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(name_len)));

			if (!name_object->data.resize(name_len)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(name_object->data.data(), name_len)));

			HostObjectRef<GenericArgTypeDefObject> type_def;

			if (!(type_def = GenericArgTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			type_def->owner_object = member;
			type_def->name_object = name_object.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::GenericArg, td);
			} else {
				type_out = TypeRef(TypeId::GenericArg, type_def.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Ref: {
			HostObjectRef<RefTypeDefObject> type_def;

			if (!(type_def = RefTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			HostObjectRef<HeapTypeObject> heap_type;

			if (!(heap_type = HeapTypeObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, heap_type->type_ref));

			type_def->referenced_type = heap_type.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::Ref, td);
			} else {
				type_out = TypeRef(TypeId::Ref, type_def.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::ParamTypeList: {
			HostObjectRef<ParamTypeListTypeDefObject> type_def;

			if (!(type_def = ParamTypeListTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t num_params;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_params)));

			if (!type_def->param_types.resize(num_params)) {
				return OutOfMemoryError::alloc();
			}

			for (uint32_t i = 0; i < num_params; ++i) {
				HostObjectRef<HeapTypeObject> heap_type;

				if (!(heap_type = HeapTypeObject::alloc(runtime))) {
					return OutOfMemoryError::alloc();
				}

				SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, heap_type->type_ref));

				type_def->param_types.at(i) = heap_type.get();
			}

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_bool(type_def->has_var_arg)));

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::ParamTypeList, td);
			} else {
				type_out = TypeRef(TypeId::ParamTypeList, type_def.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		case slake::slxfmt::TypeId::Unpacking: {
			HostObjectRef<UnpackingTypeDefObject> type_def;

			if (!(type_def = UnpackingTypeDefObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			HostObjectRef<HeapTypeObject> heap_type;

			if (!(heap_type = HeapTypeObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, heap_type->type_ref));

			type_def->type = heap_type.get();

			if (auto td = runtime->get_equal_type_def(type_def.get()); td) {
				type_out = TypeRef(TypeId::Unpacking, td);
			} else {
				type_out = TypeRef(TypeId::Unpacking, type_def.get());
				SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(type_def.get()));
			}
			break;
		}
		default:
			std::terminate();
	}

	uint8_t modifier;
	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(modifier)));

	if (modifier & slxfmt::TYPE_FINAL)
		type_out.type_modifier |= TYPE_FINAL;
	if (modifier & slxfmt::TYPE_LOCAL)
		type_out.type_modifier |= TYPE_LOCAL;
	if (modifier & slxfmt::TYPE_NULLABLE)
		type_out.type_modifier |= TYPE_NULLABLE;

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_generic_param(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, GenericParam &generic_param_out) noexcept {
	uint32_t len_name;

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(len_name)));
	if (!generic_param_out.name.resize(len_name)) {
		return OutOfMemoryError::alloc();
	}
	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(generic_param_out.name.data(), len_name)));

	bool b;
	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_bool(b)));

	if (b) {
		SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, generic_param_out.input_type));
	} else {
		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_bool(b)));
		if (b) {
			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, generic_param_out.base_type));
		}

		uint32_t num_impl_interfaces;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_impl_interfaces)));

		if (!generic_param_out.interfaces.resize(num_impl_interfaces)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t i = 0; i < num_impl_interfaces; ++i) {
			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, generic_param_out.interfaces.at(i)));
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_value(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, Value &value_out) noexcept {
	slxfmt::ValueType vt;

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8((uint8_t &)vt)));

	switch (vt) {
		case slake::slxfmt::ValueType::None: {
			value_out = Value(Reference(nullptr));
			break;
		}
		case slake::slxfmt::ValueType::I8: {
			int8_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_i8(data)));
			value_out = Value((int8_t)data);
			break;
		}
		case slake::slxfmt::ValueType::I16: {
			int16_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_i16(data)));
			value_out = Value((int16_t)data);
			break;
		}
		case slake::slxfmt::ValueType::I32: {
			int32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_i32(data)));
			value_out = Value((int32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::I64: {
			int64_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_i64(data)));
			value_out = Value((int64_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U8: {
			uint8_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(data)));
			value_out = Value((uint8_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U16: {
			uint16_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u16(data)));
			value_out = Value((uint16_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U32: {
			uint32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(data)));
			value_out = Value((uint32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::U64: {
			uint64_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u64(data)));
			value_out = Value((uint64_t)data);
			break;
		}
		case slake::slxfmt::ValueType::F32: {
			float data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_f32(data)));
			value_out = Value((float)data);
			break;
		}
		case slake::slxfmt::ValueType::F64: {
			double data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_f64(data)));
			value_out = Value((double)data);
			break;
		}
		case slake::slxfmt::ValueType::Bool: {
			bool data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_bool(data)));
			value_out = Value((bool)data);
			break;
		}
		case slake::slxfmt::ValueType::TypeName: {
			TypeRef type;

			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, type));

			value_out = Value(type);
			break;
		}
		case slake::slxfmt::ValueType::Reg: {
			uint32_t data;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(data)));
			value_out = Value(ValueType::RegIndex, (uint32_t)data);
			break;
		}
		case slake::slxfmt::ValueType::String: {
			HostObjectRef<StringObject> str_obj;

			if (!(str_obj = StringObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t len_name;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(len_name)));
			if (!str_obj->data.resize(len_name)) {
				return OutOfMemoryError::alloc();
			}

			if (len_name) {
				SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(str_obj->data.data(), len_name)));
			}

			value_out = Value(Reference(str_obj.get()));
			break;
		}
		case slake::slxfmt::ValueType::IdRef: {
			HostObjectRef<IdRefObject> id_ref_obj;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, member, id_ref_obj));

			value_out = Value(Reference(id_ref_obj.get()));
			break;
		}
		case slake::slxfmt::ValueType::Array: {
			HostObjectRef<ArrayObject> a;

			TypeRef element_type;

			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, element_type));

			uint32_t num_elements;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_elements)));

			if (!(a = runtime->new_array_instance(runtime, element_type, num_elements))) {
				return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)a->data, a->element_size * num_elements)));

			value_out = a.get();
			break;
		}
		default:
			// TODO: Use InvalidValueTypeError.
			return ReadError::alloc(runtime->get_fixed_alloc());
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_id_ref_entries(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, peff::DynArray<IdRefEntry> &entries_out) noexcept {
	uint32_t num_entries;

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_entries)));
	for (size_t i = 0; i < num_entries; ++i) {
		IdRefEntry cur_entry(runtime->get_cur_gen_alloc());

		uint32_t len_name;
		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(len_name)));
		if (!cur_entry.name.resize(len_name)) {
			return OutOfMemoryError::alloc();
		}

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(cur_entry.name.data(), len_name)));

		uint8_t num_generic_args;
		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(num_generic_args)));

		if (!cur_entry.generic_args.resize(num_generic_args)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t j = 0; j < num_generic_args; ++j) {
			SLAKE_RETURN_IF_EXCEPT(load_value(context, runtime, reader, member, cur_entry.generic_args.at(j)));
		}

		if (!entries_out.push_back(std::move(cur_entry))) {
			return OutOfMemoryError::alloc();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_id_ref(LoaderContext &context, Runtime *runtime, Reader *reader, Object *member, HostObjectRef<IdRefObject> &id_ref_out) noexcept {
	if (!(id_ref_out = IdRefObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	SLAKE_RETURN_IF_EXCEPT(load_id_ref_entries(context, runtime, reader, member, id_ref_out->entries));

	uint32_t num_param_types;

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_param_types)));

	if (num_param_types != UINT32_MAX) {
		peff::DynArray<TypeRef> param_types(id_ref_out->self_allocator.get());

		if (!param_types.resize(num_param_types)) {
			return OutOfMemoryError::alloc();
		}
		for (size_t i = 0; i < num_param_types; ++i) {
			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, param_types.at(i)));
		}

		id_ref_out->param_types = std::move(param_types);
	}

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_bool(id_ref_out->has_var_args)));

	SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, member, id_ref_out->overriden_type));

	if (auto it = context.loaded_id_refs.find(id_ref_out.get()); it != context.loaded_id_refs.end()) {
		id_ref_out = *it;
	} else {
		if (!(context.loaded_id_refs.insert(id_ref_out.get()))) {
			return OutOfMemoryError::alloc();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_module_members(LoaderContext &context, Runtime *runtime, Reader *reader, BasicModuleObject *module_object) noexcept {
	{
		uint32_t num_classes;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_classes)));
		for (size_t i = 0; i < num_classes; ++i) {
			slake::slxfmt::ClassTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<ClassObject> cls_object;

			if (!(cls_object = ClassObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			if (!cls_object->resize_name(desc.len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(cls_object->get_name_raw_ptr(), desc.len_name)));

			AccessModifier access = 0;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
			if (!is_valid_access_modifier(access))
				abort();
			cls_object->set_access(access);

			for (size_t j = 0; j < desc.num_generic_params; ++j) {
				GenericParam gp(cls_object->self_allocator.get());

				SLAKE_RETURN_IF_EXCEPT(load_generic_param(context, runtime, reader, module_object, gp));

				if (!cls_object->generic_params.push_back(std::move(gp)))
					return OutOfMemoryError::alloc();
				if (!cls_object->mapped_generic_params.insert(cls_object->generic_params.back().name, +j))
					return OutOfMemoryError::alloc();
			}

			if (desc.flags & slxfmt::CTD_DERIVED) {
				SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, module_object, cls_object->base_type));
			}

			if (!cls_object->impl_types.resize(desc.num_impls)) {
				return OutOfMemoryError::alloc();
			}
			for (size_t j = 0; j < desc.num_impls; ++j) {
				SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, module_object, cls_object->impl_types.at(j)));
			}

			SLAKE_RETURN_IF_EXCEPT(load_module_members(context, runtime, reader, cls_object.get()));

			if (!context.loaded_classes.insert(cls_object.get())) {
				return OutOfMemoryError::alloc();
			}

			if (!module_object->add_member(cls_object.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t num_interfaces;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_interfaces)));
		for (size_t i = 0; i < num_interfaces; ++i) {
			slake::slxfmt::InterfaceTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<InterfaceObject> interface_object;

			if (!(interface_object = InterfaceObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			if (!interface_object->resize_name(desc.len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(interface_object->get_name_raw_ptr(), desc.len_name)));

			AccessModifier access = 0;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
			if (!is_valid_access_modifier(access))
				abort();
			interface_object->set_access(access);

			for (size_t j = 0; j < desc.num_generic_params; ++j) {
				GenericParam gp(interface_object->self_allocator.get());

				SLAKE_RETURN_IF_EXCEPT(load_generic_param(context, runtime, reader, module_object, gp));

				if (!interface_object->generic_params.push_back(std::move(gp))) {
					return OutOfMemoryError::alloc();
				}
				if (!interface_object->mapped_generic_params.insert(interface_object->generic_params.back().name, +j))
					return OutOfMemoryError::alloc();
			}

			if (!interface_object->impl_types.resize(desc.num_parents)) {
				return OutOfMemoryError::alloc();
			}
			for (size_t j = 0; j < desc.num_parents; ++j) {
				SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, module_object, interface_object->impl_types.at(j)));
			}

			SLAKE_RETURN_IF_EXCEPT(load_module_members(context, runtime, reader, interface_object.get()));

			if (!context.loaded_interfaces.insert(interface_object.get())) {
				return OutOfMemoryError::alloc();
			}

			if (!module_object->add_member(interface_object.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t num_structs;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_structs)));
		for (size_t i = 0; i < num_structs; ++i) {
			slake::slxfmt::StructTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<StructObject> struct_object;

			if (!(struct_object = StructObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			if (!struct_object->resize_name(desc.len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(struct_object->get_name_raw_ptr(), desc.len_name)));

			AccessModifier access = 0;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
			if (!is_valid_access_modifier(access))
				abort();
			struct_object->set_access(access);

			for (size_t j = 0; j < desc.num_generic_params; ++j) {
				GenericParam gp(struct_object->self_allocator.get());

				SLAKE_RETURN_IF_EXCEPT(load_generic_param(context, runtime, reader, module_object, gp));

				if (!struct_object->generic_params.push_back(std::move(gp))) {
					return OutOfMemoryError::alloc();
				}
				if (!struct_object->mapped_generic_params.insert(struct_object->generic_params.back().name, +j))
					return OutOfMemoryError::alloc();
			}

			SLAKE_RETURN_IF_EXCEPT(load_module_members(context, runtime, reader, struct_object.get()));

			if (!context.loaded_structs.insert(struct_object.get())) {
				return OutOfMemoryError::alloc();
			}

			if (!module_object->add_member(struct_object.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t num_scoped_enums;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_scoped_enums)));
		for (size_t i = 0; i < num_scoped_enums; ++i) {
			slake::slxfmt::ScopedEnumTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<ScopedEnumObject> enum_object;

			if (!(enum_object = ScopedEnumObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			if (!enum_object->resize_name(desc.len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(enum_object->get_name_raw_ptr(), desc.len_name)));

			AccessModifier access = 0;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
			if (!is_valid_access_modifier(access))
				abort();
			enum_object->set_access(access);

			// TODO: Implement it.
			uint32_t num_fields;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_fields)));

			if (desc.flags & slxfmt::SETD_BASE) {
				SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, module_object, enum_object->base_type));

				for (size_t j = 0; j < num_fields; ++j) {
					slxfmt::EnumItemDesc eid;
					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&eid, sizeof(eid))));

					FieldRecord fr(module_object->self_allocator.get());

					if (!fr.name.resize(desc.len_name)) {
						return OutOfMemoryError::alloc();
					}
					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(fr.name.data(), eid.len_name)));

					fr.type = enum_object->base_type;

					Value initial_value;

					SLAKE_RETURN_IF_EXCEPT(load_value(context, runtime, reader, enum_object.get(), initial_value));

					if (!is_compatible(fr.type, initial_value))
						std::terminate();

					Reference ref = StaticFieldRef(enum_object.get(), i);

					if (!context.init_var_data.push_back({ ref, std::move(initial_value) }))
						return OutOfMemoryError::alloc();

					if (!enum_object->append_field_record_without_alloc(std::move(fr))) {
						return OutOfMemoryError::alloc();
					}
				}
			} else {
				for (size_t j = 0; j < num_fields; ++j) {
					slxfmt::EnumItemDesc eid;
					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&eid, sizeof(eid))));

					FieldRecord fr(module_object->self_allocator.get());

					if (!fr.name.resize(desc.len_name)) {
						return OutOfMemoryError::alloc();
					}
					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(fr.name.data(), eid.len_name)));

					if (!enum_object->append_field_record_without_alloc(std::move(fr))) {
						return OutOfMemoryError::alloc();
					}
				}
			}

			if (!context.loaded_scoped_enums.insert(enum_object.get())) {
				return OutOfMemoryError::alloc();
			}

			if (!module_object->add_member(enum_object.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t num_union_enums;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_union_enums)));
		for (size_t i = 0; i < num_union_enums; ++i) {
			slake::slxfmt::UnionEnumTypeDesc desc;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&desc, sizeof(desc))));

			HostObjectRef<UnionEnumObject> enum_object;

			if (!(enum_object = UnionEnumObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			if (!enum_object->resize_name(desc.len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(enum_object->get_name_raw_ptr(), desc.len_name)));

			AccessModifier access = 0;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
			if (!is_valid_access_modifier(access))
				abort();
			enum_object->set_access(access);

			// TODO: Implement it.
			uint32_t num_items;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_items)));
			for (uint32_t j = 0; j < num_items; ++j) {
				HostObjectRef<UnionEnumItemObject> item_object;

				if (!(item_object = UnionEnumItemObject::alloc(runtime))) {
					return OutOfMemoryError::alloc();
				}

				uint32_t len_item_name;
				SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(len_item_name)));

				if (!item_object->resize_name(len_item_name)) {
					return OutOfMemoryError::alloc();
				}

				SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(item_object->get_name_raw_ptr(), len_item_name)));

				uint32_t num_fields;

				SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_fields)));
				for (uint32_t k = 0; k < num_fields; ++k) {
					uint32_t len_field_name;
					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(len_field_name)));

					FieldRecord fr(module_object->self_allocator.get());

					if (!fr.name.resize(len_field_name)) {
						return OutOfMemoryError::alloc();
					}
					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(fr.name.data(), len_field_name)));

					SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, enum_object.get(), fr.type));

					if (!item_object->append_field_record_without_alloc(std::move(fr))) {
						return OutOfMemoryError::alloc();
					}
				}

				if (!context.loaded_union_enum_items.insert(item_object.get())) {
					return OutOfMemoryError::alloc();
				}

				if (!enum_object->add_member(item_object.get())) {
					return OutOfMemoryError::alloc();
				}
			}

			if (!context.loaded_union_enums.insert(enum_object.get())) {
				return OutOfMemoryError::alloc();
			}

			if (!module_object->add_member(enum_object.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t num_fns;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_fns)));
		for (size_t i = 0; i < num_fns; ++i) {
			HostObjectRef<FnObject> fn_object;

			if (!(fn_object = FnObject::alloc(runtime))) {
				return OutOfMemoryError::alloc();
			}

			uint32_t len_name;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(len_name)));
			if (!fn_object->resize_name(len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(fn_object->get_name_raw_ptr(), len_name)));

			uint32_t num_overloadings;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_overloadings)));
			for (size_t j = 0; j < num_overloadings; ++j) {
				HostObjectRef<RegularFnOverloadingObject> fn_overloading_object;

				if (!(fn_overloading_object = RegularFnOverloadingObject::alloc(fn_object.get()))) {
					return OutOfMemoryError::alloc();
				}

				slake::slxfmt::FnDesc fnd;

				SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&fnd, sizeof(fnd))));

				AccessModifier access;
				SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
				if (!is_valid_access_modifier(access))
					abort();
				fn_overloading_object->set_access(access);

				if (fnd.flags & slxfmt::FND_VARG) {
					fn_overloading_object->set_var_args();
				}
				if (fnd.flags & slxfmt::FND_GENERATOR) {
					fn_overloading_object->set_coroutine();
				}
				if (fnd.flags & slxfmt::FND_VIRTUAL) {
					fn_overloading_object->set_virtual_flag();
				}

				fn_overloading_object->set_register_number(fnd.num_registers);

				for (size_t k = 0; k < fnd.num_generic_params; ++k) {
					GenericParam gp(fn_overloading_object->self_allocator.get());

					SLAKE_RETURN_IF_EXCEPT(load_generic_param(context, runtime, reader, fn_overloading_object.get(), gp));

					if (!fn_overloading_object->generic_params.push_back(std::move(gp))) {
						return OutOfMemoryError::alloc();
					}
					if (!fn_overloading_object->mapped_generic_params.insert(fn_overloading_object->generic_params.back().name, +j))
						return OutOfMemoryError::alloc();
				}

				if (!fn_overloading_object->param_types.resize(fnd.num_params)) {
					return OutOfMemoryError::alloc();
				}
				for (size_t k = 0; k < fnd.num_params; ++k) {
					SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, fn_overloading_object.get(), fn_overloading_object->param_types.at(k)));
				}

				// stub
				fn_overloading_object->overriden_type = TypeId::Void;

				SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, fn_overloading_object.get(), fn_overloading_object->return_type));

				for (size_t k = 0; k < fnd.len_body; ++k) {
					Opcode opcode;

					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8((uint8_t &)opcode)));

					uint32_t output;

					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(output)));

					uint32_t num_operands;

					SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_operands)));

					Instruction ins;

					ins.opcode = opcode;
					ins.output = output;
					if (!ins.reserve_operands(fn_overloading_object->self_allocator.get(), num_operands)) {
						return OutOfMemoryError::alloc();
					}

					for (size_t l = 0; l < num_operands; ++l) {
						SLAKE_RETURN_IF_EXCEPT(load_value(context, runtime, reader, fn_overloading_object.get(), ins.operands[l]));
					}

					if (!fn_overloading_object->instructions.push_back(std::move(ins))) {
						return OutOfMemoryError::alloc();
					}
				}

				if (!fn_object->overloadings.insert(
						FnSignature{ fn_overloading_object->param_types,
							fn_overloading_object->is_with_var_args(),
							fn_overloading_object->generic_params.size(),
							fn_overloading_object->overriden_type },
						fn_overloading_object.get())) {
					return OutOfMemoryError::alloc();
				}
			}

			if (!context.loaded_fns.insert(fn_object.get()))
				return OutOfMemoryError::alloc();

			if (!module_object->add_member(fn_object.get())) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	{
		uint32_t num_fields;

		SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u32(num_fields)));

		for (size_t i = 0; i < num_fields; ++i) {
			slake::slxfmt::VarDesc vad;

			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&vad, sizeof(vad))));

			FieldRecord fr(module_object->self_allocator.get());

			if (!fr.name.resize(vad.len_name)) {
				return OutOfMemoryError::alloc();
			}
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read(fr.name.data(), vad.len_name)));

			AccessModifier access = 0;
			SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read_u8(access)));
			if (!is_valid_access_modifier(access))
				abort();
			fr.access_modifier = access;

			SLAKE_RETURN_IF_EXCEPT(load_type(context, runtime, reader, module_object, fr.type));

			switch (fr.type.type_id) {
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
					Value initial_value;
					SLAKE_RETURN_IF_EXCEPT(load_value(context, runtime, reader, module_object, initial_value));

					if (!is_compatible(fr.type, initial_value))
						std::terminate();

					Reference ref = StaticFieldRef(module_object, i);

					if (!context.init_var_data.push_back({ ref, std::move(initial_value) }))
						return OutOfMemoryError::alloc();
					break;
				}
				default:
					break;
			}

			if (!module_object->append_field_record_without_alloc(std::move(fr))) {
				return OutOfMemoryError::alloc();
			}
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_single_module(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &module_object_out) noexcept {
	slxfmt::ImgHeader imh = { 0 };

	SLAKE_RETURN_IF_EXCEPT(_normalize_read_result(runtime, reader->read((char *)&imh, sizeof(imh))));

	if (memcmp(imh.magic, slxfmt::IMH_MAGIC, sizeof(imh.magic))) {
		return alloc_out_of_memory_error_if_alloc_failed(BadMagicError::alloc(runtime->get_fixed_alloc()));
	}

	if (!(module_object_out = ModuleObject::alloc(runtime))) {
		return OutOfMemoryError::alloc();
	}

	{
		peff::DynArray<IdRefEntry> module_full_name(runtime->get_cur_gen_alloc());
		SLAKE_RETURN_IF_EXCEPT(load_id_ref_entries(context, runtime, reader, module_object_out.get(), module_full_name));

		for (size_t i = 0; i < imh.num_imports; ++i) {
			HostObjectRef<IdRefObject> path;

			SLAKE_RETURN_IF_EXCEPT(load_id_ref(context, runtime, reader, module_object_out.get(), path));

			if (path->param_types.has_value()) {
				std::terminate();
			}

			for (size_t i = 0; i < path->entries.size(); ++i) {
				if (path->entries.at(i).generic_args.size()) {
					std::terminate();
				}
			}

			if (!module_object_out->unnamed_imports.push_back(path.get())) {
				return OutOfMemoryError::alloc();
			}
		}

		SLAKE_RETURN_IF_EXCEPT(load_module_members(context, runtime, reader, module_object_out.get()));

		SLAKE_RETURN_IF_EXCEPT(complete_parent_namespaces(context, runtime, module_object_out.get(), module_full_name));
	}

	if (!context.loaded_modules.insert(module_object_out.get()))
		return OutOfMemoryError::alloc();

	return {};
}

SLAKE_API InternalExceptionPointer loader::load_module(LoaderContext &context, Runtime *runtime, Reader *reader, HostObjectRef<ModuleObject> &module_object_out) noexcept {
	HostObjectRef<ModuleObject> mod;
	SLAKE_RETURN_IF_EXCEPT(load_single_module(context, runtime, reader, mod));

	module_object_out = mod;

	peff::Set<IdRefObject *, IdRefComparator, true> mod_names_to_be_loaded(context.allocator.get());

	for (auto i : mod->unnamed_imports) {
		if (!mod_names_to_be_loaded.insert(+i))
			return OutOfMemoryError::alloc();
	}

load_dependencies:
	peff::Set<IdRefObject *, IdRefComparator, true> new_mod_names(context.allocator.get());

	for (auto i : mod_names_to_be_loaded) {
		slake::Reference ref;
		SLAKE_RETURN_IF_EXCEPT(runtime->resolve_id_ref(i, ref));

		if (ref) {
			if (ref.kind != ReferenceKind::ObjectRef)
				// TODO: Handle it.
				std::terminate();
		} else {
			HostObjectRef<ModuleObject> imported_mod;
			peff::UniquePtr<Reader, peff::DeallocableDeleter<Reader>> imported_reader;
			SLAKE_RETURN_IF_EXCEPT(context.locate_module(runtime, i->entries, imported_reader.get_ref()));
			SLAKE_RETURN_IF_EXCEPT(load_single_module(context, runtime, imported_reader.get(), imported_mod));

			for (auto &j : imported_mod->unnamed_imports) {
				if (!new_mod_names.insert(+j))
					return OutOfMemoryError::alloc();
			}
		}
	}

	if (new_mod_names.size()) {
		mod_names_to_be_loaded = std::move(new_mod_names);
		goto load_dependencies;
	}

	for (auto i : context.loaded_custom_type_defs) {
		runtime->unregister_type_def(i);
	}

	for (auto i : context.loaded_custom_type_defs) {
		SLAKE_RETURN_IF_EXCEPT(runtime->load_deferred_custom_type_def(i));

		SLAKE_RETURN_IF_EXCEPT(runtime->register_type_def(i));
	}

	context.loaded_custom_type_defs.clear();

	for (auto i : context.loaded_interfaces) {
		SLAKE_RETURN_IF_EXCEPT(i->update_inheritance_relationship(runtime->get_fixed_alloc()));
	}

	context.loaded_interfaces.clear();

	for (auto i : context.loaded_structs) {
		SLAKE_RETURN_IF_EXCEPT(i->is_recursed(runtime->get_fixed_alloc()));
	}

	context.loaded_interfaces.clear();

	for (auto i : context.loaded_fns) {
		SLAKE_RETURN_IF_EXCEPT(i->resort_overloadings());
	}

	context.loaded_fns.clear();

	for (auto i : context.loaded_scoped_enums) {
		if (i->base_type)
			if (!i->realloc_field_spaces())
				return OutOfMemoryError::alloc();
	}

	for (auto i : context.loaded_structs) {
		if (!i->realloc_field_spaces())
			return OutOfMemoryError::alloc();
	}

	for (auto i : context.loaded_classes) {
		if (!i->realloc_field_spaces())
			return OutOfMemoryError::alloc();
	}

	for (auto i : context.loaded_union_enum_items) {
		if (!i->realloc_field_spaces())
			return OutOfMemoryError::alloc();
	}

	for (auto i : context.loaded_union_enums) {
		// ...
	}

	for (auto i : context.loaded_modules) {
		if (!i->realloc_field_spaces())
			return OutOfMemoryError::alloc();
	}

	for (auto &i : context.init_var_data) {
		Runtime::write_var(i.first, i.second);
	}

	context.init_var_data.clear();

	runtime->gc();

	return {};
}

SLAKE_API InternalExceptionPointer slake::loader::complete_parent_namespaces(LoaderContext &context, Runtime *runtime, BasicModuleObject *module_object, const peff::DynArray<IdRefEntry> &ref) noexcept {
	HostObjectRef<ModuleObject> mod = runtime->get_root_object();

	for (size_t i = 0; i < ref.size() - 1; ++i) {
		std::string_view name = ref.at(i).name;

		if (auto m = mod->get_member(name); m.is_valid()) {
			if ((m.kind != ReferenceKind::ObjectRef) || (m.as_object->get_object_kind() != ObjectKind::Module))
				std::terminate();
			mod = (ModuleObject *)m.as_object;
		} else {
			HostObjectRef<ModuleObject> new_mod;

			if (!(new_mod = ModuleObject::alloc(runtime)))
				return OutOfMemoryError::alloc();

			if (!new_mod->set_name(name))
				return OutOfMemoryError::alloc();

			if (!mod->add_member(new_mod.get()))
				return OutOfMemoryError::alloc();

			mod = new_mod.get();
		}
	}

	if (!module_object->set_name(ref.at(ref.size() - 1).name))
		return OutOfMemoryError::alloc();
	if (!mod->add_member(module_object))
		return OutOfMemoryError::alloc();

	return {};
}
