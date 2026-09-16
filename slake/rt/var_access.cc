#include "../runtime.h"

using namespace slake;

SLAKE_FORCEINLINE static void *calc_coroutine_local_var_ref_stack_base_ptr(const CoroutineLocalVarRef &local_var_ref) noexcept {
	if (local_var_ref.coroutine->cur_context) {
		return calc_stack_addr(local_var_ref.coroutine->cur_context->data_stack,
			local_var_ref.coroutine->cur_context->stack_size,
			local_var_ref.stack_off + local_var_ref.coroutine->off_stack_top);
	} else {
		return calc_stack_addr(local_var_ref.coroutine->stack_data,
			local_var_ref.coroutine->len_stack_data,
			local_var_ref.stack_off);
	};
}
SLAKE_FORCEINLINE static void *calc_local_var_ref_stack_base_ptr(const LocalVarRef &local_var_ref) noexcept {
	return calc_stack_addr(local_var_ref.context->data_stack,
		local_var_ref.context->stack_size,
		local_var_ref.stack_off);
}
SLAKE_FORCEINLINE static void *calc_local_var_ref_stack_raw_data_ptr(void *p) noexcept {
	return static_cast<char *>(p) +
		   (sizeof(TypeId) + sizeof(TypeModifier));
}
SLAKE_FORCEINLINE static const void *calc_local_var_ref_stack_raw_data_ptr(const void *p) noexcept {
	return static_cast<const char *>(p) +
		   (sizeof(TypeId) + sizeof(TypeModifier));
}

SLAKE_API void *Runtime::locate_value_base_ptr(const Reference &entity_ref) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &field_record = entity_ref.as_static_field.module_object->field_records.at(entity_ref.as_static_field.index);

			return entity_ref.as_static_field.module_object->local_field_storage.data() + field_record.offset;
		}
		case ReferenceKind::LocalVarRef: {
			char *raw_data_ptr = static_cast<char *>(calc_local_var_ref_stack_base_ptr(entity_ref.as_local_var));

			switch (*reinterpret_cast<const TypeId *>(raw_data_ptr)) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::StructInstance:
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::Ref:
					raw_data_ptr += sizeof(void *);
					break;
				default:
					// All fields should be checked during the instantiation.
					break;
			}

			// We have to add size of the type modifier and type ID back, size of other extra information is already added.
			return static_cast<void *>(raw_data_ptr + (sizeof(TypeModifier) + sizeof(TypeId)));
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *raw_data_ptr = static_cast<char *>(calc_local_var_ref_stack_raw_data_ptr(calc_coroutine_local_var_ref_stack_base_ptr(entity_ref.as_coroutine_local_var)));

			switch (*reinterpret_cast<const TypeId *>(raw_data_ptr - (sizeof(TypeModifier) + sizeof(TypeId)))) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::StructInstance:
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::Ref:
					raw_data_ptr += sizeof(void *);
					break;
				default:
					break;
			}

			return static_cast<void *>(raw_data_ptr);
		}
		case ReferenceKind::ObjectFieldRef: {
			ObjectFieldRecord &field_record =
				entity_ref.as_object_field.instance_object->_class->cached_object_layout->field_records.at(
					entity_ref.as_object_field.field_index);

			return entity_ref.as_object_field.instance_object->raw_field_data + field_record.offset;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entity_ref.as_array_element.index < entity_ref.as_array_element.array_object->length);

			return (static_cast<char *>(entity_ref.as_array_element.array_object->data) + entity_ref.as_array_element.index * entity_ref.as_array_element.array_object->element_size);
		}
		case ReferenceKind::ArgRef:
			std::terminate();
		case ReferenceKind::CoroutineArgRef:
			std::terminate();
		case ReferenceKind::InitObjectLayoutFieldRef: {
			ObjectFieldRecord &field_record =
				entity_ref.as_init_object_layout_field.object_layout->field_records.at(
					entity_ref.as_init_object_layout_field.index);

			char *init_data = entity_ref.as_init_object_layout_field.object_layout->get_init_data();
			assert(init_data);
			return init_data + field_record.offset;
		}
		case ReferenceKind::DefaultStructValueRef: {
			char *init_data = entity_ref.as_init_object_layout_field.object_layout->get_init_data();
			assert(init_data);
			return init_data;
		}
		default:
			switch (entity_ref.kind) {
				case ReferenceKind::StaticFieldStructFieldRef:
				case ReferenceKind::LocalVarStructFieldRef:
				case ReferenceKind::CoroutineLocalVarStructFieldRef:
				case ReferenceKind::ObjectFieldStructFieldRef:
				case ReferenceKind::ArrayElementStructFieldRef:
				case ReferenceKind::ArgStructFieldRef:
				case ReferenceKind::CoroutineArgStructFieldRef: {
					Reference inner_ref = entity_ref;
					((uint8_t &)inner_ref.kind) &= ~0x80;
					TypeRef actual_type = typeof_var(inner_ref);

					Object *const type_object = static_cast<CustomTypeDefObject *>(actual_type.type_def)->type_object;
					char *base_ptr = static_cast<char *>(locate_value_base_ptr(inner_ref));

					assert(type_object->get_object_kind() == ObjectKind::Struct);

					return base_ptr + static_cast<StructObject *>(type_object)->field_records.at(entity_ref.struct_field_index).offset;
				}
				default:
					std::terminate();
			}
	}
}

SLAKE_API TypeRef Runtime::typeof_var(const Reference &entity_ref) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &field_record = entity_ref.as_static_field.module_object->field_records.at(entity_ref.as_static_field.index);

			const char *const raw_data_ptr = entity_ref.as_static_field.module_object->local_field_storage.data() + field_record.offset;

			return field_record.type;
		}
		case ReferenceKind::LocalVarRef: {
			const char *const raw_data_ptr = static_cast<const char *>(
				calc_local_var_ref_stack_base_ptr(entity_ref.as_local_var));

			TypeRef t = TypeRef(
				*static_cast<const TypeId *>(static_cast<const void *>(raw_data_ptr)),
				*static_cast<const TypeModifier *>(static_cast<const void *>(raw_data_ptr + sizeof(TypeId))));

			switch (t.type_id) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				case TypeId::StructInstance:
				case TypeId::UnionEnum:
				case TypeId::UnionEnumItem:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				case TypeId::TypelessScopedEnum:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				case TypeId::Ref:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				default:
					break;
			}

			return t;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *const raw_data_ptr = static_cast<const char *>(
				calc_local_var_ref_stack_raw_data_ptr(calc_coroutine_local_var_ref_stack_base_ptr(entity_ref.as_coroutine_local_var)));

			TypeRef t = TypeRef(*static_cast<const TypeId *>(
									static_cast<const void *>(raw_data_ptr - (sizeof(TypeModifier) + sizeof(TypeId)))),
				*static_cast<const TypeModifier *>(static_cast<const void *>(raw_data_ptr - sizeof(TypeModifier))));

			switch (t.type_id) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				case TypeId::StructInstance:
				case TypeId::UnionEnum:
				case TypeId::UnionEnumItem:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				case TypeId::TypelessScopedEnum:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				case TypeId::Ref:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					break;
				default:
					break;
			}

			return t;
		}
		case ReferenceKind::ObjectFieldRef: {
			ObjectFieldRecord &field_record =
				entity_ref.as_object_field.instance_object->_class->cached_object_layout->field_records.at(
					entity_ref.as_object_field.field_index);

			return field_record.type;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entity_ref.as_array_element.index < entity_ref.as_array_element.array_object->length);

			return entity_ref.as_array_element.array_object->element_type;
		}
		case ReferenceKind::ArgRef: {
			auto overloading = entity_ref.as_arg.major_frame->cur_fn;

			if (entity_ref.as_arg.arg_index >= overloading->param_types.size()) {
				assert(overloading->overloading_flags & OL_VARG);
				return TypeId::Any;
			}

			return overloading->param_types.at(entity_ref.as_arg.arg_index);
		}
		case ReferenceKind::CoroutineArgRef: {
			auto coroutine = entity_ref.as_coroutine_arg.coroutine;
			if (coroutine->cur_context) {
				auto overloading = coroutine->bound_major_frame->cur_fn;

				if (entity_ref.as_coroutine_arg.arg_index >= overloading->param_types.size()) {
					assert(overloading->overloading_flags & OL_VARG);
					return TypeId::Any;
				}

				return overloading->param_types.at(entity_ref.as_coroutine_arg.arg_index);
			} else {
				auto overloading = coroutine->overloading;

				if (entity_ref.as_coroutine_arg.arg_index >= overloading->param_types.size()) {
					assert(overloading->overloading_flags & OL_VARG);
					return TypeId::Any;
				}

				return overloading->param_types.at(entity_ref.as_coroutine_arg.arg_index);
			}
			break;
		}
		case ReferenceKind::InitObjectLayoutFieldRef: {
			ObjectFieldRecord &field_record =
				entity_ref.as_init_object_layout_field.object_layout->field_records.at(
					entity_ref.as_init_object_layout_field.index);

			return field_record.type;
		}
		case ReferenceKind::DefaultStructValueRef:
			// The user should have known type of the value to be written if they want to write the default value.
			std::terminate();
		default:
			switch (entity_ref.kind) {
				case ReferenceKind::StaticFieldStructFieldRef:
				case ReferenceKind::LocalVarStructFieldRef:
				case ReferenceKind::CoroutineLocalVarStructFieldRef:
				case ReferenceKind::ObjectFieldStructFieldRef:
				case ReferenceKind::ArrayElementStructFieldRef:
				case ReferenceKind::ArgStructFieldRef:
				case ReferenceKind::CoroutineArgStructFieldRef: {
					Reference inner_ref = entity_ref;
					((uint8_t &)inner_ref.kind) &= ~0x80;
					TypeRef actual_type = typeof_var(inner_ref);

					Object *const type_object = static_cast<CustomTypeDefObject *>(actual_type.type_def)->type_object;

					assert(type_object->get_object_kind() == ObjectKind::Struct);

					return static_cast<StructObject *>(type_object)->field_records.at(entity_ref.struct_field_index).type;
				}
				default:
					std::terminate();
			}
	}
}

SLAKE_FORCEINLINE void _read_local_var(const Reference &entity_ref, const char *raw_data_ptr, const TypeRef &t, Value &value_out) {
	switch (t.type_id) {
		case TypeId::I8:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_i8 = *(reinterpret_cast<const int8_t *>(raw_data_ptr));
			value_out.value_type = ValueType::I8;
			break;
		case TypeId::I16:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_i16 = *(reinterpret_cast<const int16_t *>(raw_data_ptr));
			value_out.value_type = ValueType::I16;
			break;
		case TypeId::I32:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_i32 = *(reinterpret_cast<const int32_t *>(raw_data_ptr));
			value_out.value_type = ValueType::I32;
			break;
		case TypeId::I64:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_i64 = *(reinterpret_cast<const int64_t *>(raw_data_ptr));
			value_out.value_type = ValueType::I64;
			break;
		case TypeId::ISize:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_isize = *(reinterpret_cast<const ssize_t *>(raw_data_ptr));
			value_out.value_type = ValueType::ISize;
			break;
		case TypeId::U8:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_u8 = *(reinterpret_cast<const uint8_t *>(raw_data_ptr));
			value_out.value_type = ValueType::U8;
			break;
		case TypeId::U16:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_u16 = *(reinterpret_cast<const uint16_t *>(raw_data_ptr));
			value_out.value_type = ValueType::U16;
			break;
		case TypeId::U32:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_u32 = *(reinterpret_cast<const uint32_t *>(raw_data_ptr));
			value_out.value_type = ValueType::U32;
			break;
		case TypeId::U64:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_u64 = *(reinterpret_cast<const uint64_t *>(raw_data_ptr));
			value_out.value_type = ValueType::U64;
			break;
		case TypeId::USize:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_usize = *(reinterpret_cast<const size_t *>(raw_data_ptr));
			value_out.value_type = ValueType::USize;
			break;
		case TypeId::F32:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_f32 = *(reinterpret_cast<const float *>(raw_data_ptr));
			value_out.value_type = ValueType::F32;
			break;
		case TypeId::F64:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_f64 = *(reinterpret_cast<const double *>(raw_data_ptr));
			value_out.value_type = ValueType::F64;
			break;
		case TypeId::Bool:
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}
			value_out.as_bool = *(reinterpret_cast<const bool *>(raw_data_ptr));
			value_out.value_type = ValueType::Bool;
			break;
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::Fn:
			value_out.as_object = (*((Object **)(raw_data_ptr)));
			value_out.value_type = ValueType::Object;
			if (t.is_local())
				value_out.set_local();
			break;
		case TypeId::StructInstance: {
			if (t.is_nullable()) {
				if (*reinterpret_cast<const bool *>(raw_data_ptr)) {
					value_out = nullptr;
					break;
				}
				raw_data_ptr += sizeof(bool);
			}

			value_out.as_reference = entity_ref;
			value_out.value_type = ValueType::Reference;
			if (t.is_local())
				value_out.set_local();
			break;
		}
		case TypeId::Ref:
			value_out.as_reference = *(reinterpret_cast<const Reference *>(raw_data_ptr));
			value_out.value_type = ValueType::Reference;
			if (t.is_local())
				value_out.set_local();
			break;
		case TypeId::Any:
			value_out = *(reinterpret_cast<const Value *>(raw_data_ptr));
			if (t.is_local())
				value_out.set_local();
			break;
		default:
			// All fields should be checked during the instantiation.
			std::terminate();
	}
}

SLAKE_API void Runtime::read_var(const Reference &entity_ref, Value &value_out) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::LocalVarRef: {
			const char *raw_data_ptr = static_cast<char *>(calc_local_var_ref_stack_base_ptr(entity_ref.as_local_var));

			TypeRef t = TypeRef(
				*static_cast<const TypeId *>(static_cast<const void *>(raw_data_ptr)),
				*static_cast<const TypeModifier *>(static_cast<const void *>(raw_data_ptr + sizeof(TypeId))));

			switch (t.type_id) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::StructInstance:
				case TypeId::UnionEnum:
				case TypeId::UnionEnumItem:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::TypelessScopedEnum:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::Ref:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				default:
					break;
			}

			_read_local_var(entity_ref, raw_data_ptr + (sizeof(TypeModifier) + sizeof(TypeId)), t, value_out);

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *raw_data_ptr = static_cast<char *>(calc_coroutine_local_var_ref_stack_base_ptr(entity_ref.as_coroutine_local_var));

			TypeRef t = TypeRef(
				*static_cast<const TypeId *>(static_cast<const void *>(raw_data_ptr)),
				*static_cast<const TypeModifier *>(static_cast<const void *>(raw_data_ptr + sizeof(TypeId))));

			switch (t.type_id) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::StructInstance:
				case TypeId::UnionEnum:
				case TypeId::UnionEnumItem:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::TypelessScopedEnum:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::Ref:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				default:
					break;
			}

			_read_local_var(entity_ref, raw_data_ptr + (sizeof(TypeModifier) + sizeof(TypeId)), t, value_out);

			break;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entity_ref.as_array_element.index < entity_ref.as_array_element.array_object->length);

			TypeRef t = entity_ref.as_array_element.array_object->element_type;

			if (t.is_nullable())
				// TODO: Handle it.
				std::terminate();
			switch (t.type_id) {
				case TypeId::I8:
					value_out = (static_cast<const int8_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::I16:
					value_out = (static_cast<const int16_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::I32:
					value_out = (static_cast<const int32_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::I64:
					value_out = (static_cast<const int64_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U8:
					value_out = (static_cast<const uint8_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U16:
					value_out = (static_cast<const uint16_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U32:
					value_out = (static_cast<const uint32_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U64:
					value_out = (static_cast<const uint64_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::F32:
					value_out = (static_cast<const float *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::F64:
					value_out = (static_cast<const double *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::Bool:
					value_out = (static_cast<const bool *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::Instance:
				case TypeId::String:
				case TypeId::Array:
				case TypeId::Fn:
					value_out = static_cast<Object **>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index];
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::StructInstance:
					value_out = entity_ref;
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::Ref:
					value_out = static_cast<const Reference *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index];
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::Any:
					value_out = static_cast<const Value *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index];
					if (t.is_local())
						std::terminate();
					break;
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			TypeRef t = typeof_var(entity_ref);
			value_out = _fetch_arg_stack(
				entity_ref.as_arg.major_frame->cur_context->get_context().data_stack,
				entity_ref.as_arg.major_frame->cur_context->get_context().stack_size,
				entity_ref.as_arg.major_frame,
				entity_ref.as_arg.major_frame->resumable_context_data.off_args)[entity_ref.as_arg.arg_index];

			if (t.is_local())
				value_out.set_local();
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			TypeRef t = typeof_var(entity_ref);
			if (entity_ref.as_coroutine_arg.coroutine->cur_context) {
				MajorFrame *mf = _fetch_major_frame(entity_ref.as_coroutine_arg.coroutine->cur_context, entity_ref.as_coroutine_arg.coroutine->cur_context->off_cur_major_frame);
				value_out = _fetch_arg_stack(
					entity_ref.as_coroutine_arg.coroutine->cur_context->data_stack,
					entity_ref.as_coroutine_arg.coroutine->cur_context->stack_size,
					mf,
					mf->resumable_context_data.off_args)[entity_ref.as_coroutine_arg.arg_index];

				if (t.is_local())
					value_out.set_local();
			} else {
				// TODO: Implement it.
				std::terminate();

				if (t.is_local())
					value_out.set_local();
			}
			break;
		}
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ObjectFieldRef:
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			const char *const raw_data_ptr = static_cast<char *>(locate_value_base_ptr(entity_ref));

			TypeRef t = typeof_var(entity_ref);

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>((raw_data_ptr + sizeof(int8_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i8 = *(reinterpret_cast<const int8_t *>(raw_data_ptr));
					value_out.value_type = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int16_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i16 = *(reinterpret_cast<const int16_t *>(raw_data_ptr));
					value_out.value_type = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int32_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i32 = *(reinterpret_cast<const int32_t *>(raw_data_ptr));
					value_out.value_type = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int64_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i64 = *(reinterpret_cast<const int64_t *>(raw_data_ptr));
					value_out.value_type = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(ssize_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_isize = *(reinterpret_cast<const ssize_t *>(raw_data_ptr));
					value_out.value_type = ValueType::ISize;
					break;
				case TypeId::U8:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint8_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u8 = *(reinterpret_cast<const uint8_t *>(raw_data_ptr));
					value_out.value_type = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint16_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u16 = *(reinterpret_cast<const uint16_t *>(raw_data_ptr));
					value_out.value_type = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint32_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u32 = *(reinterpret_cast<const uint32_t *>(raw_data_ptr));
					value_out.value_type = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint64_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u64 = *(reinterpret_cast<const uint64_t *>(raw_data_ptr));
					value_out.value_type = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(size_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_usize = *(reinterpret_cast<const size_t *>(raw_data_ptr));
					value_out.value_type = ValueType::USize;
					break;
				case TypeId::F32:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(float))) {
							value_out = nullptr;
							break;
						}
					value_out.as_f32 = *(reinterpret_cast<const float *>(raw_data_ptr));
					value_out.value_type = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(double))) {
							value_out = nullptr;
							break;
						}
					value_out.as_f64 = *(reinterpret_cast<const double *>(raw_data_ptr));
					value_out.value_type = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(bool))) {
							value_out = nullptr;
							break;
						}
					value_out.as_bool = *(reinterpret_cast<const bool *>(raw_data_ptr));
					value_out.value_type = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					value_out.as_object = *((Object **)(raw_data_ptr));
					value_out.value_type = ValueType::Object;
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::StructInstance: {
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof_type(t))) {
							value_out = nullptr;
							break;
						}

					value_out.as_reference = entity_ref;
					value_out.value_type = ValueType::Reference;
					break;
				}
				case TypeId::ScopedEnum: {
					CustomTypeDefObject *td = static_cast<CustomTypeDefObject *>(t.type_def);
					assert(td->type_object->get_object_kind() == ObjectKind::ScopedEnum);

					TypeRef type;
					if ((type = (static_cast<ScopedEnumObject *>(td->type_object))->base_type))
						switch (type.type_id) {
							case TypeId::I8:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int8_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i8 = *(reinterpret_cast<const int8_t *>(raw_data_ptr));
								value_out.value_type = ValueType::I8;
								break;
							case TypeId::I16:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int16_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i16 = *(reinterpret_cast<const int16_t *>(raw_data_ptr));
								value_out.value_type = ValueType::I16;
								break;
							case TypeId::I32:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int32_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i32 = *(reinterpret_cast<const int32_t *>(raw_data_ptr));
								value_out.value_type = ValueType::I32;
								break;
							case TypeId::I64:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(int64_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i64 = *(reinterpret_cast<const int64_t *>(raw_data_ptr));
								value_out.value_type = ValueType::I64;
								break;
							case TypeId::ISize:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(ssize_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_isize = *(reinterpret_cast<const ssize_t *>(raw_data_ptr));
								value_out.value_type = ValueType::ISize;
								break;
							case TypeId::U8:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint8_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u8 = *(reinterpret_cast<const uint8_t *>(raw_data_ptr));
								value_out.value_type = ValueType::U8;
								break;
							case TypeId::U16:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint16_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u16 = *(reinterpret_cast<const uint16_t *>(raw_data_ptr));
								value_out.value_type = ValueType::U16;
								break;
							case TypeId::U32:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint32_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u32 = *(reinterpret_cast<const uint32_t *>(raw_data_ptr));
								value_out.value_type = ValueType::U32;
								break;
							case TypeId::U64:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint64_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u64 = *(reinterpret_cast<const uint64_t *>(raw_data_ptr));
								value_out.value_type = ValueType::U64;
								break;
							case TypeId::USize:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(size_t))) {
										value_out = nullptr;
										break;
									}
								value_out.as_usize = *(reinterpret_cast<const size_t *>(raw_data_ptr));
								value_out.value_type = ValueType::USize;
								break;
							case TypeId::F32:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(float))) {
										value_out = nullptr;
										break;
									}
								value_out.as_f32 = *(reinterpret_cast<const float *>(raw_data_ptr));
								value_out.value_type = ValueType::F32;
								break;
							case TypeId::F64:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(double))) {
										value_out = nullptr;
										break;
									}
								value_out.as_f64 = *(reinterpret_cast<const double *>(raw_data_ptr));
								value_out.value_type = ValueType::F64;
								break;
							case TypeId::Bool:
								if (t.is_nullable())
									if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(bool))) {
										value_out = nullptr;
										break;
									}
								value_out.as_bool = *(reinterpret_cast<const bool *>(raw_data_ptr));
								value_out.value_type = ValueType::Bool;
								break;
							default:
								std::terminate();
						}
					break;
				}
				case TypeId::TypelessScopedEnum:
					if (t.is_nullable())
						if (*reinterpret_cast<const bool *>(raw_data_ptr + sizeof(uint32_t))) {
							value_out = nullptr;
							break;
						}
					value_out.as_typeless_scoped_enum.type = t;
					value_out.as_typeless_scoped_enum.value = *(reinterpret_cast<const uint32_t *>(raw_data_ptr));
					value_out.value_type = ValueType::TypelessScopedEnum;
					break;
				case TypeId::Ref:
					value_out.as_reference = *(reinterpret_cast<const Reference *>(raw_data_ptr));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::Any:
					value_out = *(reinterpret_cast<const Value *>(raw_data_ptr));
					if (t.is_local())
						std::terminate();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		default:
			std::terminate();
	}
}

SLAKE_FORCEINLINE void _write_local_var(const Reference &entity_ref, char *raw_data_ptr, const TypeRef &t, const void *data) noexcept {
	switch (t.type_id) {
		case TypeId::I8:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<int8_t *>((raw_data_ptr))) = *static_cast<const int8_t *>(data);
			break;
		case TypeId::I16:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<int16_t *>((raw_data_ptr))) = *static_cast<const int16_t *>(data);
			break;
		case TypeId::I32:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<int32_t *>((raw_data_ptr))) = *static_cast<const int32_t *>(data);
			break;
		case TypeId::I64:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<int64_t *>((raw_data_ptr))) = *static_cast<const int64_t *>(data);
			break;
		case TypeId::U8:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<uint8_t *>((raw_data_ptr))) = *static_cast<const uint8_t *>(data);
			break;
		case TypeId::U16:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<uint16_t *>((raw_data_ptr))) = *static_cast<const uint16_t *>(data);
			break;
		case TypeId::U32:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<uint32_t *>((raw_data_ptr))) = *static_cast<const uint32_t *>(data);
			break;
		case TypeId::U64:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<uint64_t *>((raw_data_ptr))) = *static_cast<const uint64_t *>(data);
			break;
		case TypeId::F32:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<float *>((raw_data_ptr))) = *static_cast<const float *>(data);
			break;
		case TypeId::F64:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<double *>((raw_data_ptr))) = *static_cast<const double *>(data);
			break;
		case TypeId::Bool:
			if (t.is_nullable()) {
				if (!(*reinterpret_cast<bool *>((raw_data_ptr)) = data))
					break;
				raw_data_ptr += sizeof(bool);
			}
			*(reinterpret_cast<bool *>((raw_data_ptr))) = *static_cast<const bool *>(data);
			break;
		case TypeId::String: {
			const Value &v = *static_cast<const Value *>(data);
			if (v.is_local() && !t.is_local())
				std::terminate();
			*((Object **)(raw_data_ptr)) = v.get_object();
			break;
		}
		case TypeId::Instance:
		case TypeId::Array: {
			const Value &v = *static_cast<const Value *>(data);
			if (v.is_local() && !t.is_local())
				std::terminate();
			*((Object **)(raw_data_ptr)) = v.get_object();
			break;
		}
		case TypeId::StructInstance: {
			const Value &v = *static_cast<const Value *>(data);
			memcpy(raw_data_ptr, Runtime::locate_value_base_ptr(v.get_reference()), Runtime::sizeof_type(t));
			break;
		}
		default:
			// All fields should be checked during the instantiation.
			std::terminate();
	}
}

SLAKE_API void Runtime::write_var_with_type(const Reference &entity_ref, const TypeRef &t, const void *data) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::LocalVarRef: {
			char *raw_data_ptr = static_cast<char *>(calc_local_var_ref_stack_base_ptr(entity_ref.as_local_var));

			TypeRef t = TypeRef(
				*static_cast<const TypeId *>(static_cast<const void *>(raw_data_ptr)),
				*static_cast<const TypeModifier *>(static_cast<const void *>(raw_data_ptr + sizeof(TypeId))));

			switch (t.type_id) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::StructInstance:
				case TypeId::UnionEnum:
				case TypeId::UnionEnumItem:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::TypelessScopedEnum:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::Ref:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				default:
					break;
			}

			_write_local_var(entity_ref, raw_data_ptr + (sizeof(TypeModifier) + sizeof(TypeId)), t, data);
			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *raw_data_ptr = static_cast<char *>(calc_coroutine_local_var_ref_stack_base_ptr(entity_ref.as_coroutine_local_var));

			TypeRef t = TypeRef(
				*static_cast<const TypeId *>(static_cast<const void *>(raw_data_ptr)),
				*static_cast<const TypeModifier *>(static_cast<const void *>(raw_data_ptr + sizeof(TypeId))));

			switch (t.type_id) {
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::StructInstance:
				case TypeId::UnionEnum:
				case TypeId::UnionEnumItem:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::TypelessScopedEnum:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				case TypeId::Ref:
					t.type_def = *((TypeDefObject **)raw_data_ptr);
					raw_data_ptr += sizeof(void *);
					break;
				default:
					break;
			}

			_write_local_var(entity_ref, raw_data_ptr + (sizeof(TypeModifier) + sizeof(TypeId)), t, data);
			break;
		}
		case ReferenceKind::ArrayElementRef: {
			if (t.is_nullable()) {
				if (data) {
					// TODO: Handle this.
					std::terminate();
				}
			}
			switch (t.type_id) {
				case TypeId::I8:
					static_cast<int8_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const int8_t *>(data);
					break;
				case TypeId::I16:
					static_cast<int16_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const int16_t *>(data);
					break;
				case TypeId::I32:
					static_cast<int32_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const int32_t *>(data);
					break;
				case TypeId::I64:
					static_cast<int64_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const int64_t *>(data);
					break;
				case TypeId::U8:
					static_cast<uint8_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const uint8_t *>(data);
					break;
				case TypeId::U16:
					static_cast<uint16_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const uint16_t *>(data);
					break;
				case TypeId::U32:
					static_cast<uint32_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const uint32_t *>(data);
					break;
				case TypeId::U64:
					static_cast<uint64_t *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const uint64_t *>(data);
					break;
				case TypeId::F32:
					static_cast<float *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const float *>(data);
					break;
				case TypeId::F64:
					static_cast<double *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const double *>(data);
					break;
				case TypeId::Bool:
					static_cast<bool *>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = *static_cast<const bool *>(data);
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array: {
					const Value &value = *static_cast<const Value *>(data);
					if (t.is_local())
						std::terminate();
					if (value.is_local())
						std::terminate();
					static_cast<Object **>(entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_object();
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
			const Value &value = *static_cast<const Value *>(data);
			if (value.is_local() && !t.is_local())
				std::terminate();
			_fetch_arg_stack(
				entity_ref.as_arg.major_frame->cur_context->get_context().data_stack,
				entity_ref.as_arg.major_frame->cur_context->get_context().stack_size,
				entity_ref.as_arg.major_frame,
				entity_ref.as_arg.major_frame->resumable_context_data.off_args)[entity_ref.as_arg.arg_index] = value;
			break;
		}
		case ReferenceKind::CoroutineArgRef: {
			const Value &value = *static_cast<const Value *>(data);
			if (value.is_local() && !t.is_local())
				std::terminate();
			if (entity_ref.as_coroutine_arg.coroutine->cur_context) {
				MajorFrame *mf = _fetch_major_frame(entity_ref.as_coroutine_arg.coroutine->cur_context, entity_ref.as_coroutine_arg.coroutine->cur_context->off_cur_major_frame);
				_fetch_arg_stack(
					entity_ref.as_coroutine_arg.coroutine->cur_context->data_stack,
					entity_ref.as_coroutine_arg.coroutine->cur_context->stack_size,
					mf,
					mf->resumable_context_data.off_args)[entity_ref.as_coroutine_arg.arg_index] = value;
			} else {
				// TODO: Implement it.
				std::terminate();
			}
			break;
		}
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ObjectFieldRef:
		case ReferenceKind::InitObjectLayoutFieldRef: {
		write_field:
			char *const raw_data_ptr = static_cast<char *>(locate_value_base_ptr(entity_ref));

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(int8_t))) = data))
							break;
					}
					*reinterpret_cast<int8_t *>(raw_data_ptr) = *static_cast<const int8_t *>(data);
					break;
				case TypeId::I16:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(int16_t))) = data))
							break;
					}
					*reinterpret_cast<int16_t *>(raw_data_ptr) = *static_cast<const int16_t *>(data);
					break;
				case TypeId::I32:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(int32_t))) = data))
							break;
					}
					*reinterpret_cast<int32_t *>(raw_data_ptr) = *static_cast<const int32_t *>(data);
					break;
				case TypeId::I64:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(int64_t))) = data))
							break;
					}
					*reinterpret_cast<int64_t *>(raw_data_ptr) = *static_cast<const int64_t *>(data);
					break;
				case TypeId::U8:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(uint8_t))) = data))
							break;
					}
					*reinterpret_cast<uint8_t *>(raw_data_ptr) = *static_cast<const uint8_t *>(data);
					break;
				case TypeId::U16:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(uint16_t))) = data))
							break;
					}
					*reinterpret_cast<uint16_t *>(raw_data_ptr) = *static_cast<const uint16_t *>(data);
					break;
				case TypeId::U32:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(uint32_t))) = data))
							break;
					}
					*reinterpret_cast<uint32_t *>(raw_data_ptr) = *static_cast<const uint32_t *>(data);
					break;
				case TypeId::U64:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(uint64_t))) = data))
							break;
					}
					*reinterpret_cast<uint64_t *>(raw_data_ptr) = *static_cast<const uint64_t *>(data);
					break;
				case TypeId::F32:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(float))) = data))
							break;
					}
					*reinterpret_cast<float *>(raw_data_ptr) = *static_cast<const float *>(data);
					break;
				case TypeId::F64:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(double))) = data))
							break;
					}
					*reinterpret_cast<double *>(raw_data_ptr) = *static_cast<const double *>(data);
					break;
				case TypeId::Bool:
					if (t.is_nullable()) {
						if (!(*reinterpret_cast<bool *>((raw_data_ptr + sizeof(bool))) = data))
							break;
					}
					*reinterpret_cast<bool *>(raw_data_ptr) = *static_cast<const bool *>(data);
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array: {
					const Value &value = *static_cast<const Value *>(data);
					if (t.is_local())
						std::terminate();
					if (value.is_local())
						std::terminate();
					*static_cast<Object **>(static_cast<void *>(raw_data_ptr)) = value.get_object();
					break;
				}
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		default:
			switch (entity_ref.kind) {
				case ReferenceKind::StaticFieldStructFieldRef:
				case ReferenceKind::LocalVarStructFieldRef:
				case ReferenceKind::CoroutineLocalVarStructFieldRef:
				case ReferenceKind::ObjectFieldStructFieldRef:
				case ReferenceKind::ArrayElementStructFieldRef:
				case ReferenceKind::ArgStructFieldRef:
				case ReferenceKind::CoroutineArgStructFieldRef:
					goto write_field;
				default:
					break;
			}
			std::terminate();
	}
}

SLAKE_API void Runtime::write_var_with_type_and_value(const Reference &entity_ref, const TypeRef &t, const Value &value) noexcept {
	switch (t.type_id) {
		case TypeId::I8:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				write_var_with_type(entity_ref, t, &value.as_i8);
			}
			break;
		case TypeId::I16:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				write_var_with_type(entity_ref, t, &value.as_i16);
			}
			break;
		case TypeId::I32:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				write_var_with_type(entity_ref, t, &value.as_i32);
			}
			break;
		case TypeId::I64:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				write_var_with_type(entity_ref, t, &value.as_i64);
			}
			break;
		case TypeId::U8:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				write_var_with_type(entity_ref, t, &value.as_u8);
			}
			break;
		case TypeId::U16:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				uint16_t data = value.get_u16();
				write_var_with_type(entity_ref, t, &value.as_u16);
			}
			break;
		case TypeId::U32:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				uint32_t data = value.get_u32();
				write_var_with_type(entity_ref, t, &value.as_u32);
			}
			break;
		case TypeId::U64:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				uint64_t data = value.get_u64();
				write_var_with_type(entity_ref, t, &value.as_u64);
			}
			break;
		case TypeId::F32:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				float data = value.get_f32();
				write_var_with_type(entity_ref, t, &value.as_f32);
			}
			break;
		case TypeId::F64:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				double data = value.get_f64();
				write_var_with_type(entity_ref, t, &value.as_f64);
			}
			break;
		case TypeId::Bool:
			if (value.is_null())
				write_var_with_type(entity_ref, t, nullptr);
			else {
				bool data = value.get_bool();
				write_var_with_type(entity_ref, t, &value.as_bool);
			}
			break;
		case TypeId::Object:
		case TypeId::String:
		case TypeId::Array:
		case TypeId::Instance:
		case TypeId::StructInstance:
		case TypeId::ScopedEnum:
		case TypeId::TypelessScopedEnum:
		case TypeId::UnionEnum:
		case TypeId::UnionEnumItem:
		case TypeId::Any:
			write_var_with_type(entity_ref, t, &value);
			break;
		default:
			std::terminate();
	}
}
