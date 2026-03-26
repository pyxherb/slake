#include "runtime.h"

#include <cassert>

using namespace slake;

SLAKE_API TypeId slake::value_type_to_type_id(ValueType value_type) noexcept {
	switch (value_type) {
		case ValueType::I8:
			return TypeId::I8;
		case ValueType::I16:
			return TypeId::I16;
		case ValueType::I32:
			return TypeId::I32;
		case ValueType::I64:
			return TypeId::I64;
		case ValueType::U8:
			return TypeId::U8;
		case ValueType::U16:
			return TypeId::U16;
		case ValueType::U32:
			return TypeId::U32;
		case ValueType::U64:
			return TypeId::U64;
		case ValueType::F32:
			return TypeId::F32;
		case ValueType::F64:
			return TypeId::F64;
		case ValueType::Bool:
			return TypeId::Bool;
		case ValueType::Reference:
			return TypeId::Ref;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API bool slake::is_value_type_compatible_type_id(TypeId type_id) noexcept {
	switch (type_id) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
			return true;
		default:;
	}
	return false;
}

SLAKE_API ValueType slake::type_id_to_value_type(TypeId type_id) noexcept {
	switch (type_id) {
		case TypeId::I8:
			return ValueType::I8;
		case TypeId::I16:
			return ValueType::I16;
		case TypeId::I32:
			return ValueType::I32;
		case TypeId::I64:
			return ValueType::I64;
		case TypeId::U8:
			return ValueType::U8;
		case TypeId::U16:
			return ValueType::U16;
		case TypeId::U32:
			return ValueType::U32;
		case TypeId::U64:
			return ValueType::U64;
		case TypeId::F32:
			return ValueType::F32;
		case TypeId::F64:
			return ValueType::F64;
		case TypeId::Bool:
			return ValueType::Bool;
		default:;
	}
	std::terminate();
}

SLAKE_API bool Reference::operator==(const Reference &rhs) const {
	if (kind != rhs.kind)
		return false;
	switch (kind) {
		case ReferenceKind::Invalid:
			break;
		case ReferenceKind::StaticFieldRef:
			if (as_static_field.module_object != rhs.as_static_field.module_object)
				return false;
			return as_static_field.index == rhs.as_static_field.index;
		case ReferenceKind::ArrayElementRef:
			if (as_array_element.array_object != rhs.as_array_element.array_object)
				return false;
			return as_static_field.index == rhs.as_static_field.index;
		case ReferenceKind::ObjectRef:
			return as_object == rhs.as_object;
		case ReferenceKind::ObjectFieldRef:
			if (as_object_field.instance_object != rhs.as_object_field.instance_object)
				return false;
			return as_object_field.field_index == rhs.as_object_field.field_index;
		case ReferenceKind::LocalVarRef:
			if (as_local_var.context != rhs.as_local_var.context)
				return false;
			return as_local_var.stack_off == rhs.as_local_var.stack_off;
		case ReferenceKind::ArgRef:
			if (as_arg.major_frame != rhs.as_arg.major_frame)
				return false;
			return as_arg.arg_index == rhs.as_arg.arg_index;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API bool Reference::operator<(const Reference &rhs) const {
	if (kind < rhs.kind)
		return true;
	if (kind > rhs.kind)
		return false;
	switch (kind) {
		case ReferenceKind::Invalid:
			break;
		case ReferenceKind::StaticFieldRef:
			if (as_static_field.module_object < rhs.as_static_field.module_object)
				return true;
			if (as_static_field.module_object > rhs.as_static_field.module_object)
				return false;
			return as_static_field.index < rhs.as_static_field.index;
		case ReferenceKind::ArrayElementRef:
			if (as_array_element.array_object < rhs.as_array_element.array_object)
				return true;
			if (as_array_element.array_object > rhs.as_array_element.array_object)
				return false;
			return as_static_field.index < rhs.as_static_field.index;
		case ReferenceKind::ObjectRef:
			return as_object < rhs.as_object;
		case ReferenceKind::ObjectFieldRef:
			if (as_object_field.instance_object < rhs.as_object_field.instance_object)
				return true;
			if (as_object_field.instance_object > rhs.as_object_field.instance_object)
				return false;
			return as_object_field.field_index < rhs.as_object_field.field_index;
		case ReferenceKind::LocalVarRef:
			if (as_local_var.context < rhs.as_local_var.context)
				return true;
			if (as_local_var.context > rhs.as_local_var.context)
				return false;
			return as_local_var.stack_off < rhs.as_local_var.stack_off;
		case ReferenceKind::ArgRef:
			if (as_arg.major_frame < rhs.as_arg.major_frame)
				return true;
			if (as_arg.major_frame > rhs.as_arg.major_frame)
				return false;
			return as_arg.arg_index < rhs.as_arg.arg_index;
		case ReferenceKind::AotPtrRef:
			return as_aot_ptr < rhs.as_aot_ptr;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API bool Reference::operator>(const Reference &rhs) const {
	if (kind > rhs.kind)
		return true;
	if (kind < rhs.kind)
		return false;
	switch (kind) {
		case ReferenceKind::Invalid:
			break;
		case ReferenceKind::StaticFieldRef:
			if (as_static_field.module_object > rhs.as_static_field.module_object)
				return true;
			if (as_static_field.module_object < rhs.as_static_field.module_object)
				return false;
			return as_static_field.index > rhs.as_static_field.index;
		case ReferenceKind::ArrayElementRef:
			if (as_array_element.array_object > rhs.as_array_element.array_object)
				return true;
			if (as_array_element.array_object < rhs.as_array_element.array_object)
				return false;
			return as_static_field.index > rhs.as_static_field.index;
		case ReferenceKind::ObjectRef:
			return as_object > rhs.as_object;
		case ReferenceKind::ObjectFieldRef:
			if (as_object_field.instance_object > rhs.as_object_field.instance_object)
				return true;
			if (as_object_field.instance_object < rhs.as_object_field.instance_object)
				return false;
			return as_object_field.field_index > rhs.as_object_field.field_index;
		case ReferenceKind::LocalVarRef:
			if (as_local_var.context > rhs.as_local_var.context)
				return true;
			if (as_local_var.context < rhs.as_local_var.context)
				return false;
			return as_local_var.stack_off > rhs.as_local_var.stack_off;
		case ReferenceKind::ArgRef:
			if (as_arg.major_frame > rhs.as_arg.major_frame)
				return true;
			if (as_arg.major_frame < rhs.as_arg.major_frame)
				return false;
			return as_arg.arg_index > rhs.as_arg.arg_index;
		case ReferenceKind::AotPtrRef:
			return as_aot_ptr > rhs.as_aot_ptr;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API int TypeRef::compares_to(const TypeRef &rhs) const noexcept {
	if (type_id < rhs.type_id)
		return -1;
	if (type_id > rhs.type_id)
		return 1;
	if (type_modifier < rhs.type_modifier)
		return -1;
	if (type_modifier > rhs.type_modifier)
		return 1;
	if (is_fundamental_type(type_id))
		return 0;
	if (type_def < rhs.type_def)
		return -1;
	if (type_def > rhs.type_def)
		return 1;
	return 0;
}

SLAKE_API TypeRef TypeRef::duplicate(bool &succeeded_out) const {
	TypeRef new_type(*this);

	switch (type_id) {
		case TypeId::Array:
		case TypeId::Ref:
		case TypeId::GenericArg:
			new_type.type_def = (TypeDefObject *)type_def->duplicate(nullptr);
			if (!succeeded_out) {
				return {};
			}
			break;
		default:;
	}

	succeeded_out = true;

	return new_type;
}

SLAKE_API bool slake::is_compatible(const TypeRef &type, const Value &value) noexcept {
	if (type.is_nullable() && (value.is_null()))
		return true;
	switch (type.type_id) {
		case TypeId::I8:
			return value.value_type == ValueType::I8;
		case TypeId::I16:
			return value.value_type == ValueType::I16;
		case TypeId::I32:
			return value.value_type == ValueType::I32;
		case TypeId::I64:
			return value.value_type == ValueType::I64;
		case TypeId::ISize:
			return value.value_type == ValueType::ISize;
		case TypeId::U8:
			return value.value_type == ValueType::U8;
		case TypeId::U16:
			return value.value_type == ValueType::U16;
		case TypeId::U32:
			return value.value_type == ValueType::U32;
		case TypeId::U64:
			return value.value_type == ValueType::U64;
		case TypeId::USize:
			return value.value_type == ValueType::USize;
		case TypeId::F32:
			return value.value_type == ValueType::F32;
		case TypeId::F64:
			return value.value_type == ValueType::F64;
		case TypeId::Bool:
			return value.value_type == ValueType::Bool;
		case TypeId::String: {
			if (value.value_type != ValueType::Reference)
				return false;
			if (value.is_local() && !type.is_local())
				return false;
			const Reference &entity_ref = value.get_reference();
			if (entity_ref.kind != ReferenceKind::ObjectRef)
				return false;
			if (!entity_ref.as_object)
				return true;
			if (entity_ref.as_object->get_object_kind() != ObjectKind::String)
				return false;
			return true;
		}
		case TypeId::Instance: {
			if (value.value_type != ValueType::Reference)
				return false;
			if (value.is_local() && !type.is_local())
				return false;

			const Reference &entity_ref = value.get_reference();
			if (entity_ref.kind != ReferenceKind::ObjectRef)
				return false;
			Object *object_ptr = entity_ref.as_object;

			if (!object_ptr)
				return true;

			Object *type_object = type.get_custom_type_def()->type_object;
			switch (type_object->get_object_kind()) {
				case ObjectKind::Class: {
					ClassObject *this_class = (ClassObject *)type_object;

					ClassObject *value_class = ((InstanceObject *)object_ptr)->_class;

					if (type.is_final()) {
						if (this_class != value_class)
							return false;
					} else {
						if (!this_class->is_base_of(value_class))
							return false;
					}
					break;
				}
				case ObjectKind::Interface: {
					InterfaceObject *this_interface = (InterfaceObject *)type_object;

					ClassObject *value_class = ((InstanceObject *)object_ptr)->_class;

					assert(!type.is_final());
					if (!value_class->has_implemented(this_interface))
						return false;
					break;
				}
				default:
					break;
			}

			return true;
		}
		case TypeId::StructInstance:
			// Cannot pass structure instance as value.
			return false;
		case TypeId::GenericArg:
			return false;
		case TypeId::Array: {
			if (value.value_type != ValueType::Reference) {
				return false;
			}
			if (value.is_local() && !type.is_local())
				return false;

			const Reference &entity_ref = value.get_reference();
			if (entity_ref.kind != ReferenceKind::ObjectRef) {
				return false;
			}
			Object *object_ptr = entity_ref.as_object;
			if (!object_ptr)
				return false;
			if (object_ptr->get_object_kind() != ObjectKind::Array) {
				return false;
			}

			auto array_object_ptr = ((ArrayObject *)object_ptr);

			if (array_object_ptr->element_type != (type.get_array_type_def()->element_type->type_ref)) {
				return false;
			}
			return true;
		}
		case TypeId::Ref: {
			if (value.is_local() && !type.is_local())
				return false;
			const Reference &ref = value.get_reference();
			switch (ref.kind) {
				case ReferenceKind::Invalid:
				case ReferenceKind::LocalVarRef:
				case ReferenceKind::CoroutineLocalVarRef:
				case ReferenceKind::ArgRef:
				case ReferenceKind::CoroutineArgRef:
					return false;
				default:
					break;
			}
			return is_compatible(((RefTypeDefObject *)type.type_def)->referenced_type->type_ref, Runtime::typeof_var(value.get_reference()));
		}
		case TypeId::Any:
			if (value.is_local() && !type.is_local())
				return false;
			return true;
		default:
			return false;
	}
	return true;
}

int TypeRefComparator::operator()(const TypeRef &lhs, const TypeRef &rhs) const noexcept {
	if (lhs.type_id < rhs.type_id)
		return -1;
	if (lhs.type_id > rhs.type_id)
		return 1;

	if (lhs.type_def < rhs.type_def)
		return -1;
	if (lhs.type_def > rhs.type_def)
		return 1;

	return 0;
}
