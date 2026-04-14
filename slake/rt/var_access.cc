#include "../runtime.h"

using namespace slake;

SLAKE_FORCEINLINE static char *calc_coroutine_local_var_ref_stack_base_ptr(const CoroutineLocalVarRef &local_var_ref) noexcept {
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
SLAKE_FORCEINLINE static char *calc_local_var_ref_stack_base_ptr(const LocalVarRef &local_var_ref) noexcept {
	return calc_stack_addr(local_var_ref.context->data_stack,
		local_var_ref.context->stack_size,
		local_var_ref.stack_off);
}
SLAKE_FORCEINLINE static char *calc_local_var_ref_stack_raw_data_ptr(char *p) noexcept {
	return p +
		   (sizeof(TypeId) + sizeof(TypeModifier));
}
SLAKE_FORCEINLINE static const char *calc_local_var_ref_stack_raw_data_ptr(const char *p) noexcept {
	return p +
		   (sizeof(TypeId) + sizeof(TypeModifier));
}

SLAKE_API void *Runtime::locate_value_base_ptr(const Reference &entity_ref) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &field_record = entity_ref.as_static_field.module_object->field_records.at(entity_ref.as_static_field.index);

			return entity_ref.as_static_field.module_object->local_field_storage.data() + field_record.offset;
		}
		case ReferenceKind::LocalVarRef: {
			const char *raw_data_ptr = calc_local_var_ref_stack_raw_data_ptr(calc_local_var_ref_stack_base_ptr(entity_ref.as_local_var));

			switch (*reinterpret_cast<const TypeId *>(raw_data_ptr - (sizeof(TypeModifier) + sizeof(TypeId)))) {
				case TypeId::I8:
				case TypeId::I16:
				case TypeId::I32:
				case TypeId::I64:
				case TypeId::ISize:
				case TypeId::U8:
				case TypeId::U16:
				case TypeId::U32:
				case TypeId::U64:
				case TypeId::USize:
				case TypeId::F32:
				case TypeId::F64:
				case TypeId::Bool:
				case TypeId::String:
					break;
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
				case TypeId::Any:
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			return (void *)raw_data_ptr;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *raw_data_ptr = calc_local_var_ref_stack_raw_data_ptr(calc_coroutine_local_var_ref_stack_base_ptr(entity_ref.as_coroutine_local_var));

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

			return (void *)raw_data_ptr;
		}
		case ReferenceKind::ObjectFieldRef: {
			ObjectFieldRecord &field_record =
				entity_ref.as_object_field.instance_object->_class->cached_object_layout->field_records.at(
					entity_ref.as_object_field.field_index);

			return entity_ref.as_object_field.instance_object->raw_field_data + field_record.offset;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entity_ref.as_array_element.index < entity_ref.as_array_element.array_object->length);

			return ((char *)entity_ref.as_array_element.array_object->data) + entity_ref.as_array_element.index * entity_ref.as_array_element.array_object->element_size;
		}
		case ReferenceKind::ArgRef:
			std::terminate();
		case ReferenceKind::CoroutineArgRef:
			std::terminate();
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

			Object *const type_object = ((CustomTypeDefObject *)actual_type.type_def)->type_object;
			char *base_ptr = (char *)locate_value_base_ptr(inner_ref);

			assert(type_object->get_object_kind() == ObjectKind::Struct);

			return base_ptr + ((StructObject *)type_object)->field_records.at(entity_ref.struct_field_index).offset;
		}
		default:
			break;
	}

	std::terminate();
}

SLAKE_API TypeRef Runtime::typeof_var(const Reference &entity_ref) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef: {
			FieldRecord &field_record = entity_ref.as_static_field.module_object->field_records.at(entity_ref.as_static_field.index);

			const char *const raw_data_ptr = entity_ref.as_static_field.module_object->local_field_storage.data() + field_record.offset;

			return field_record.type;
		}
		case ReferenceKind::LocalVarRef: {
			const char *const raw_data_ptr = calc_local_var_ref_stack_raw_data_ptr(calc_local_var_ref_stack_base_ptr(entity_ref.as_local_var));

			TypeRef t = TypeRef(*(TypeId *)(raw_data_ptr - (sizeof(TypeModifier) + sizeof(TypeId))), *(TypeModifier *)(raw_data_ptr - sizeof(TypeModifier)));

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
				case TypeId::Any:
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			return t;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *const raw_data_ptr = calc_local_var_ref_stack_raw_data_ptr(calc_coroutine_local_var_ref_stack_base_ptr(entity_ref.as_coroutine_local_var));

			TypeRef t = TypeRef(*(TypeId *)(raw_data_ptr - (sizeof(TypeModifier) + sizeof(TypeId))), *(TypeModifier *)(raw_data_ptr - sizeof(TypeModifier)));

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

			Object *const type_object = ((CustomTypeDefObject *)actual_type.type_def)->type_object;

			assert(type_object->get_object_kind() == ObjectKind::Struct);

			return ((StructObject *)type_object)->field_records.at(entity_ref.struct_field_index).type;
		}
		default:
			break;
	}
	std::terminate();
}

SLAKE_API void Runtime::read_var_with_type(const Reference &entity_ref, const TypeRef &t, Value &value_out) noexcept {
	value_out.value_flags = 0;
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ObjectFieldRef:
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			const char *const raw_data_ptr = (char *)locate_value_base_ptr(entity_ref);

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(int8_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i8 = (*(reinterpret_cast<const int8_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(int16_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i16 = (*(reinterpret_cast<const int16_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(int32_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i32 = (*(reinterpret_cast<const int32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(int64_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_i64 = (*(reinterpret_cast<const int64_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(ssize_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_isize = *(reinterpret_cast<const ssize_t *>(raw_data_ptr));
					value_out.value_type = ValueType::ISize;
					break;
				case TypeId::U8:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(uint8_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u8 = (*(reinterpret_cast<const uint8_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(uint16_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u16 = (*(reinterpret_cast<const uint16_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(uint32_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u32 = (*(reinterpret_cast<const uint32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(uint64_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_u64 = (*(reinterpret_cast<const uint64_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(size_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_usize = *(reinterpret_cast<const size_t *>(raw_data_ptr));
					value_out.value_type = ValueType::USize;
					break;
				case TypeId::F32:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(float)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_f32 = (*(reinterpret_cast<const float *>(raw_data_ptr)));
					value_out.value_type = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(double)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_f64 = (*(reinterpret_cast<const double *>(raw_data_ptr)));
					value_out.value_type = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(bool)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_bool = (*(reinterpret_cast<const bool *>(raw_data_ptr)));
					value_out.value_type = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					value_out.as_reference = (Reference(*((Object **)(raw_data_ptr))));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::StructInstance: {
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof_type(t)))) {
							value_out = nullptr;
							break;
						}

					value_out.as_reference = entity_ref;
					value_out.value_type = ValueType::Reference;
					break;
				}
				case TypeId::ScopedEnum: {
					CustomTypeDefObject *td = (CustomTypeDefObject *)t.type_def;
					assert(td->type_object->get_object_kind() == ObjectKind::ScopedEnum);

					TypeRef type;
					if ((type = ((ScopedEnumObject *)td->type_object)->base_type))
						switch (type.type_id) {
							case TypeId::I8:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(int8_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i8 = (*(reinterpret_cast<const int8_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::I8;
								break;
							case TypeId::I16:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(int16_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i16 = (*(reinterpret_cast<const int16_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::I16;
								break;
							case TypeId::I32:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(int32_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i32 = (*(reinterpret_cast<const int32_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::I32;
								break;
							case TypeId::I64:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(int64_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_i64 = (*(reinterpret_cast<const int64_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::I64;
								break;
							case TypeId::ISize:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(ssize_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_isize = *(reinterpret_cast<const ssize_t *>(raw_data_ptr));
								value_out.value_type = ValueType::ISize;
								break;
							case TypeId::U8:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(uint8_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u8 = (*(reinterpret_cast<const uint8_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::U8;
								break;
							case TypeId::U16:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(uint16_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u16 = (*(reinterpret_cast<const uint16_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::U16;
								break;
							case TypeId::U32:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(uint32_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u32 = (*(reinterpret_cast<const uint32_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::U32;
								break;
							case TypeId::U64:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(uint64_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_u64 = (*(reinterpret_cast<const uint64_t *>(raw_data_ptr)));
								value_out.value_type = ValueType::U64;
								break;
							case TypeId::USize:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(size_t)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_usize = *(reinterpret_cast<const size_t *>(raw_data_ptr));
								value_out.value_type = ValueType::USize;
								break;
							case TypeId::F32:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(float)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_f32 = (*(reinterpret_cast<const float *>(raw_data_ptr)));
								value_out.value_type = ValueType::F32;
								break;
							case TypeId::F64:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(double)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_f64 = (*(reinterpret_cast<const double *>(raw_data_ptr)));
								value_out.value_type = ValueType::F64;
								break;
							case TypeId::Bool:
								if (t.is_nullable())
									if (*(bool *)((raw_data_ptr + sizeof(bool)))) {
										value_out = nullptr;
										break;
									}
								value_out.as_bool = (*(reinterpret_cast<const bool *>(raw_data_ptr)));
								value_out.value_type = ValueType::Bool;
								break;
							default:
								std::terminate();
						}
					break;
				}
				case TypeId::TypelessScopedEnum:
					if (t.is_nullable())
						if (*(bool *)((raw_data_ptr + sizeof(uint32_t)))) {
							value_out = nullptr;
							break;
						}
					value_out.as_typeless_scoped_enum.type = t;
					value_out.as_typeless_scoped_enum.value = (*(reinterpret_cast<const uint32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::TypelessScopedEnum;
					break;
				case TypeId::Ref:
					value_out.as_reference = (*(reinterpret_cast<const Reference *>(raw_data_ptr)));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::Any:
					value_out = (*(reinterpret_cast<const Value *>(raw_data_ptr)));
					if (t.is_local())
						std::terminate();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			const char *raw_data_ptr = (char *)locate_value_base_ptr(entity_ref);

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i8 = (*(reinterpret_cast<const int8_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i16 = (*(reinterpret_cast<const int16_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i32 = (*(reinterpret_cast<const int32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i64 = (*(reinterpret_cast<const int64_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
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
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u8 = (*(reinterpret_cast<const uint8_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u16 = (*(reinterpret_cast<const uint16_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u32 = (*(reinterpret_cast<const uint32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u64 = (*(reinterpret_cast<const uint64_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
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
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_f32 = (*(reinterpret_cast<const float *>(raw_data_ptr)));
					value_out.value_type = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_f64 = (*(reinterpret_cast<const double *>(raw_data_ptr)));
					value_out.value_type = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_bool = (*(reinterpret_cast<const bool *>(raw_data_ptr)));
					value_out.value_type = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					value_out.as_reference = (Reference(*((Object **)(raw_data_ptr))));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						value_out.set_local();
					break;
				case TypeId::StructInstance: {
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
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
					value_out.as_reference = (*(reinterpret_cast<const Reference *>(raw_data_ptr)));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						value_out.set_local();
					break;
				case TypeId::Any:
					value_out = (*(reinterpret_cast<const Value *>(raw_data_ptr)));
					if (t.is_local())
						value_out.set_local();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			const char *raw_data_ptr = (char *)locate_value_base_ptr(entity_ref);

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i8 = (*(reinterpret_cast<const int8_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I8;
					break;
				case TypeId::I16:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i16 = (*(reinterpret_cast<const int16_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I16;
					break;
				case TypeId::I32:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i32 = (*(reinterpret_cast<const int32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I32;
					break;
				case TypeId::I64:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_i64 = (*(reinterpret_cast<const int64_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::I64;
					break;
				case TypeId::ISize:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
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
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u8 = (*(reinterpret_cast<const uint8_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U8;
					break;
				case TypeId::U16:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u16 = (*(reinterpret_cast<const uint16_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U16;
					break;
				case TypeId::U32:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u32 = (*(reinterpret_cast<const uint32_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U32;
					break;
				case TypeId::U64:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_u64 = (*(reinterpret_cast<const uint64_t *>(raw_data_ptr)));
					value_out.value_type = ValueType::U64;
					break;
				case TypeId::USize:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
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
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_f32 = (*(reinterpret_cast<const float *>(raw_data_ptr)));
					value_out.value_type = ValueType::F32;
					break;
				case TypeId::F64:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_f64 = (*(reinterpret_cast<const double *>(raw_data_ptr)));
					value_out.value_type = ValueType::F64;
					break;
				case TypeId::Bool:
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
							value_out = nullptr;
							break;
						}
						raw_data_ptr += sizeof(bool);
					}
					value_out.as_bool = (*(reinterpret_cast<const bool *>(raw_data_ptr)));
					value_out.value_type = ValueType::Bool;
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
				case TypeId::Fn:
					value_out.as_reference = (Reference(*((Object **)(raw_data_ptr))));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						value_out.set_local();
					break;
				case TypeId::StructInstance: {
					if (t.is_nullable()) {
						if (*((bool *)raw_data_ptr)) {
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
					value_out.as_reference = (*(reinterpret_cast<const Reference *>(raw_data_ptr)));
					value_out.value_type = ValueType::Reference;
					if (t.is_local())
						value_out.set_local();
					break;
				case TypeId::Any:
					value_out = (*(reinterpret_cast<const Value *>(raw_data_ptr)));
					if (t.is_local())
						value_out.set_local();
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::ArrayElementRef: {
			assert(entity_ref.as_array_element.index < entity_ref.as_array_element.array_object->length);

			if (t.is_nullable())
				// TODO: Handle it.
				std::terminate();
			switch (t.type_id) {
				case TypeId::I8:
					value_out = (((int8_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::I16:
					value_out = (((int16_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::I32:
					value_out = (((int32_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::I64:
					value_out = (((int64_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U8:
					value_out = (((uint8_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U16:
					value_out = (((uint16_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U32:
					value_out = (((uint32_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::U64:
					value_out = (((uint64_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::F32:
					value_out = (((float *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::F64:
					value_out = (((double *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::Bool:
					value_out = (((bool *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					break;
				case TypeId::Instance:
				case TypeId::String:
				case TypeId::Array:
				case TypeId::Fn:
					value_out = (Reference(((Object **)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]));
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::StructInstance:
					value_out = entity_ref;
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::Ref:
					value_out = (((Reference *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					if (t.is_local())
						std::terminate();
					break;
				case TypeId::Any:
					value_out = (((Value *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index]);
					if (t.is_local())
						std::terminate();
					break;
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
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
		default:
			std::terminate();
	}
}

SLAKE_API void Runtime::write_var_with_type(const Reference &entity_ref, const TypeRef &t, const Value &value) noexcept {
	switch (entity_ref.kind) {
		case ReferenceKind::StaticFieldRef:
		case ReferenceKind::ObjectFieldRef:
		case ReferenceKind::StaticFieldStructFieldRef:
		case ReferenceKind::LocalVarStructFieldRef:
		case ReferenceKind::CoroutineLocalVarStructFieldRef:
		case ReferenceKind::ObjectFieldStructFieldRef:
		case ReferenceKind::ArrayElementStructFieldRef:
		case ReferenceKind::ArgStructFieldRef:
		case ReferenceKind::CoroutineArgStructFieldRef: {
			char *const raw_data_ptr = (char *)locate_value_base_ptr(entity_ref);

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(int8_t))) = value.is_null()))
							break;
					}
					*((int8_t *)raw_data_ptr) = value.get_i8();
					break;
				case TypeId::I16:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(int16_t))) = value.is_null()))
							break;
					}
					*((int16_t *)raw_data_ptr) = value.get_i16();
					break;
				case TypeId::I32:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(int32_t))) = value.is_null()))
							break;
					}
					*((int32_t *)raw_data_ptr) = value.get_i32();
					break;
				case TypeId::I64:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(int64_t))) = value.is_null()))
							break;
					}
					*((int64_t *)raw_data_ptr) = value.get_i64();
					break;
				case TypeId::U8:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(uint8_t))) = value.is_null()))
							break;
					}
					*((uint8_t *)raw_data_ptr) = value.get_u8();
					break;
				case TypeId::U16:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(uint16_t))) = value.is_null()))
							break;
					}
					*((uint16_t *)raw_data_ptr) = value.get_u16();
					break;
				case TypeId::U32:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(uint32_t))) = value.is_null()))
							break;
					}
					*((uint32_t *)raw_data_ptr) = value.get_u32();
					break;
				case TypeId::U64:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(uint64_t))) = value.is_null()))
							break;
					}
					*((uint64_t *)raw_data_ptr) = value.get_u64();
					break;
				case TypeId::F32:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(float))) = value.is_null()))
							break;
					}
					*((float *)raw_data_ptr) = value.get_f32();
					break;
				case TypeId::F64:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(double))) = value.is_null()))
							break;
					}
					*((double *)raw_data_ptr) = value.get_f64();
					break;
				case TypeId::Bool:
					if (t.is_nullable()) {
						if ((*((bool *)(raw_data_ptr + sizeof(bool))) = value.is_null()))
							break;
					}
					*((bool *)raw_data_ptr) = value.get_bool();
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array:
					if (t.is_local())
						std::terminate();
					if (value.is_local())
						std::terminate();
					*((Object **)raw_data_ptr) = value.get_reference().as_object;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::LocalVarRef: {
			char *raw_data_ptr = (char *)locate_value_base_ptr(entity_ref);

			if (t.is_nullable()) {
				if (value.is_null()) {
					*(bool *)((raw_data_ptr + sizeof_type(t))) = false;
					break;
				}
			}
			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int8_t *>(raw_data_ptr)) = value.get_i8();
					break;
				case TypeId::I16:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int16_t *>(raw_data_ptr)) = value.get_i16();
					break;
				case TypeId::I32:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int32_t *>(raw_data_ptr)) = value.get_i32();
					break;
				case TypeId::I64:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int64_t *>(raw_data_ptr)) = value.get_i64();
					break;
				case TypeId::U8:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint8_t *>(raw_data_ptr)) = value.get_u8();
					break;
				case TypeId::U16:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint16_t *>(raw_data_ptr)) = value.get_u16();
					break;
				case TypeId::U32:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint32_t *>(raw_data_ptr)) = value.get_u32();
					break;
				case TypeId::U64:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint64_t *>(raw_data_ptr)) = value.get_u64();
					break;
				case TypeId::F32:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<float *>(raw_data_ptr)) = value.get_f32();
					break;
				case TypeId::F64:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<double *>(raw_data_ptr)) = value.get_f64();
					break;
				case TypeId::Bool:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<bool *>(raw_data_ptr)) = value.get_bool();
					break;
				case TypeId::String:
					if (value.is_local() && !t.is_local())
						std::terminate();
					*((Object **)(raw_data_ptr)) = value.get_reference().as_object;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (value.is_local() && !t.is_local())
						std::terminate();
					*((Object **)(raw_data_ptr)) = value.get_reference().as_object;
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}

			break;
		}
		case ReferenceKind::CoroutineLocalVarRef: {
			char *raw_data_ptr = (char *)locate_value_base_ptr(entity_ref);

			switch (t.type_id) {
				case TypeId::I8:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int8_t *>(raw_data_ptr)) = value.get_i8();
					break;
				case TypeId::I16:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int16_t *>(raw_data_ptr)) = value.get_i16();
					break;
				case TypeId::I32:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int32_t *>(raw_data_ptr)) = value.get_i32();
					break;
				case TypeId::I64:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<int64_t *>(raw_data_ptr)) = value.get_i64();
					break;
				case TypeId::U8:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint8_t *>(raw_data_ptr)) = value.get_u8();
					break;
				case TypeId::U16:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint16_t *>(raw_data_ptr)) = value.get_u16();
					break;
				case TypeId::U32:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint32_t *>(raw_data_ptr)) = value.get_u32();
					break;
				case TypeId::U64:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<uint64_t *>(raw_data_ptr)) = value.get_u64();
					break;
				case TypeId::F32:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<float *>(raw_data_ptr)) = value.get_f32();
					break;
				case TypeId::F64:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<double *>(raw_data_ptr)) = value.get_f64();
					break;
				case TypeId::Bool:
					if (t.is_nullable()) {
						if ((*((bool *)raw_data_ptr) = value.is_null()))
							break;
						raw_data_ptr += sizeof(bool);
					}
					*(reinterpret_cast<bool *>(raw_data_ptr)) = value.get_bool();
					break;
				case TypeId::String:
					if (value.is_local() && !t.is_local())
						std::terminate();
					*((Object **)(raw_data_ptr)) = value.get_reference().as_object;
					break;
				case TypeId::Instance:
				case TypeId::Array:
					if (value.is_local() && !t.is_local())
						std::terminate();
					*((Object **)(raw_data_ptr)) = value.get_reference().as_object;
					break;
				case TypeId::StructInstance:
					memcpy(raw_data_ptr, locate_value_base_ptr(value.get_reference()), sizeof_type(t));
					break;
				default:
					// All fields should be checked during the instantiation.
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArrayElementRef: {
			if (t.is_nullable()) {
				if (value.is_null()) {
					// TODO: Handle this.
					std::terminate();
				}
			}
			switch (t.type_id) {
				case TypeId::I8:
					((int8_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_i8();
					break;
				case TypeId::I16:
					((int16_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_i16();
					break;
				case TypeId::I32:
					((int32_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_i32();
					break;
				case TypeId::I64:
					((int64_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_i64();
					break;
				case TypeId::U8:
					((uint8_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_u8();
					break;
				case TypeId::U16:
					((uint16_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_u16();
					break;
				case TypeId::U32:
					((uint32_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_u32();
					break;
				case TypeId::U64:
					((uint64_t *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_u64();
					break;
				case TypeId::F32:
					((float *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_f32();
					break;
				case TypeId::F64:
					((double *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_f64();
					break;
				case TypeId::Bool:
					((bool *)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_bool();
					break;
				case TypeId::String:
				case TypeId::Instance:
				case TypeId::Array: {
					if (t.is_local())
						std::terminate();
					if (value.is_local())
						std::terminate();
					((Object **)entity_ref.as_array_element.array_object->data)[entity_ref.as_array_element.index] = value.get_reference().as_object;
					break;
				}
				default:
					std::terminate();
			}
			break;
		}
		case ReferenceKind::ArgRef: {
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
		default:
			std::terminate();
	}
}
