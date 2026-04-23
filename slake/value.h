#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>
#include <cassert>
#include "type.h"
#include <slake/generated/config.h>

namespace slake {
	class CoroutineObject;
	class Object;
	struct Context;

	// Value type definitions are defined in <slake/type.h>.

	enum class ReferenceKind : uint8_t {
		Invalid = 0,

		StaticFieldRef,
		LocalVarRef,
		CoroutineLocalVarRef,
		ObjectFieldRef,
		ArrayElementRef,
		ArgRef,
		CoroutineArgRef,
		InitObjectLayoutFieldRef,
		DefaultStructValueRef,

		ObjectRef,
		ArgPackRef,
		AotPtrRef,

		StaticFieldStructFieldRef = StaticFieldRef | 0x80,
		LocalVarStructFieldRef = LocalVarRef | 0x80,
		CoroutineLocalVarStructFieldRef = CoroutineLocalVarRef | 0x80,
		ObjectFieldStructFieldRef = ObjectFieldRef | 0x80,
		ArrayElementStructFieldRef = ArrayElementRef | 0x80,
		ArgStructFieldRef = ArgRef | 0x80,
		CoroutineArgStructFieldRef = CoroutineArgRef | 0x80,
	};

	class StructObject;
	struct ObjectLayout;

	struct StaticFieldRef {
		BasicModuleObject *module_object;
		uint32_t index;

		StaticFieldRef() = default;
		SLAKE_FORCEINLINE StaticFieldRef(BasicModuleObject *module_object, uint32_t index) : module_object(module_object), index(index) {}
	};

	struct ArrayElementRef {
		ArrayObject *array_object;
		size_t index;

		ArrayElementRef() = default;
		SLAKE_FORCEINLINE ArrayElementRef(ArrayObject *array_object, uint32_t index) : array_object(array_object), index(index) {}
	};

	struct ObjectFieldRef {
		InstanceObject *instance_object;
		uint32_t field_index;

		ObjectFieldRef() = default;
		SLAKE_FORCEINLINE ObjectFieldRef(InstanceObject *instance_object, uint32_t field_index) : instance_object(instance_object), field_index(field_index) {}
	};

	struct LocalVarRef {
		Context *context;
		size_t stack_off;

		LocalVarRef() = default;
		SLAKE_FORCEINLINE LocalVarRef(Context *context, size_t stack_off) : context(context), stack_off(stack_off) {}
	};

	struct CoroutineLocalVarRef {
		CoroutineObject *coroutine;
		size_t stack_off;

		CoroutineLocalVarRef() = default;
		SLAKE_FORCEINLINE CoroutineLocalVarRef(CoroutineObject *coroutine, size_t stack_off) : coroutine(coroutine), stack_off(stack_off) {}
	};

	struct LocalVarStructRef {
		void *ptr;
		StructObject *struct_object;

		LocalVarStructRef() = default;
		SLAKE_FORCEINLINE LocalVarStructRef(void *ptr, StructObject *struct_object) : ptr(ptr), struct_object(struct_object) {}
	};

	struct ArgRef {
		const MajorFrame *major_frame;
		uint32_t arg_index;

		ArgRef() = default;
		SLAKE_FORCEINLINE ArgRef(const MajorFrame *major_frame, uint32_t arg_index) : major_frame(major_frame), arg_index(arg_index) {}
	};

	struct ArgPackRef {
		MajorFrame *major_frame;
		uint32_t begin;
		uint32_t end;

		ArgPackRef() = default;
	};

	struct CoroutineArgRef {
		CoroutineObject *coroutine;
		uint32_t arg_index;

		CoroutineArgRef() = default;
		SLAKE_FORCEINLINE CoroutineArgRef(CoroutineObject *coroutine, uint32_t arg_index) : coroutine(coroutine), arg_index(arg_index) {}
	};

	struct InitObjectLayoutFieldRef {
		ObjectLayout *object_layout;
		uint32_t index;

		InitObjectLayoutFieldRef() = default;
		SLAKE_FORCEINLINE InitObjectLayoutFieldRef(ObjectLayout *object_layout, uint32_t index) : object_layout(object_layout), index(index) {}
	};

	struct DefaultStructValueRef {
		ObjectLayout *object_layout;

		DefaultStructValueRef() = default;
		SLAKE_FORCEINLINE DefaultStructValueRef(ObjectLayout *object_layout) : object_layout(object_layout) {}
	};

	struct Reference {
		union {
			StaticFieldRef as_static_field;
			ArrayElementRef as_array_element;
			Object *as_object;
			ObjectFieldRef as_object_field;
			LocalVarRef as_local_var;
			CoroutineLocalVarRef as_coroutine_local_var;
			ArgRef as_arg;
			ArgPackRef as_arg_pack;
			CoroutineArgRef as_coroutine_arg;
			InitObjectLayoutFieldRef as_init_object_layout_field;
			DefaultStructValueRef as_default_struct_value;
			void *as_aot_ptr;
		};
		uint32_t struct_field_index;
		ReferenceKind kind;

		SLAKE_FORCEINLINE Reference() noexcept = default;
		SLAKE_FORCEINLINE Reference(const Reference &) noexcept = default;

		SLAKE_FORCEINLINE Reference(ReferenceKind reference_kind) noexcept : kind(reference_kind) {}
		SLAKE_FORCEINLINE Reference(std::nullptr_t) noexcept : kind(ReferenceKind::ObjectRef), as_object(nullptr) {}
		SLAKE_FORCEINLINE Reference(Object *object) noexcept : kind(ReferenceKind::ObjectRef), as_object(object) {}
		SLAKE_FORCEINLINE Reference(const StaticFieldRef &ref) noexcept : kind(ReferenceKind::StaticFieldRef), as_static_field(ref) {}
		SLAKE_FORCEINLINE Reference(const ArrayElementRef &ref) noexcept : kind(ReferenceKind::ArrayElementRef), as_array_element(ref) {}
		SLAKE_FORCEINLINE Reference(const ObjectFieldRef &ref) noexcept : kind(ReferenceKind::ObjectFieldRef), as_object_field(ref) {}
		SLAKE_FORCEINLINE Reference(const LocalVarRef &ref) noexcept : kind(ReferenceKind::LocalVarRef), as_local_var(ref) {}
		SLAKE_FORCEINLINE Reference(const CoroutineLocalVarRef &ref) noexcept : kind(ReferenceKind::CoroutineLocalVarRef), as_coroutine_local_var(ref) {}
		SLAKE_FORCEINLINE Reference(const ArgRef &ref) noexcept : kind(ReferenceKind::ArgRef), as_arg(ref) {}
		SLAKE_FORCEINLINE Reference(const ArgPackRef &ref) noexcept : kind(ReferenceKind::ArgPackRef), as_arg_pack(ref) {}
		SLAKE_FORCEINLINE Reference(const CoroutineArgRef &ref) noexcept : kind(ReferenceKind::CoroutineArgRef), as_coroutine_arg(ref) {}
		SLAKE_FORCEINLINE Reference(const InitObjectLayoutFieldRef &ref) noexcept : kind(ReferenceKind::InitObjectLayoutFieldRef), as_init_object_layout_field(ref) {}
		SLAKE_FORCEINLINE Reference(const DefaultStructValueRef &ref) noexcept : kind(ReferenceKind::DefaultStructValueRef), as_default_struct_value(ref) {}

		Reference &operator=(const Reference &) noexcept = default;

		SLAKE_FORCEINLINE Reference &operator=(ReferenceKind reference_kind) noexcept {
			this->kind = reference_kind;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(std::nullptr_t) noexcept {
			this->kind = ReferenceKind::ObjectRef;
			this->as_object = nullptr;
			return *this;
		}
		SLAKE_FORCEINLINE Reference &operator=(const StaticFieldRef &ref) noexcept {
			kind = ReferenceKind::StaticFieldRef;
			as_static_field = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ArrayElementRef &ref) noexcept {
			kind = ReferenceKind::ArrayElementRef;
			as_array_element = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ObjectFieldRef &ref) noexcept {
			kind = ReferenceKind::ObjectFieldRef;
			as_object_field = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const LocalVarRef &ref) noexcept {
			kind = ReferenceKind::LocalVarRef;
			as_local_var = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const CoroutineLocalVarRef &ref) noexcept {
			kind = ReferenceKind::CoroutineLocalVarRef;
			as_coroutine_local_var = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ArgRef &ref) noexcept {
			kind = ReferenceKind::ArgRef;
			as_arg = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ArgPackRef &ref) noexcept {
			kind = ReferenceKind::ArgPackRef;
			as_arg_pack = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const CoroutineArgRef &ref) noexcept {
			kind = ReferenceKind::CoroutineArgRef;
			as_coroutine_arg = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const InitObjectLayoutFieldRef &ref) noexcept {
			kind = ReferenceKind::InitObjectLayoutFieldRef;
			as_init_object_layout_field = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const DefaultStructValueRef &ref) noexcept {
			kind = ReferenceKind::DefaultStructValueRef;
			as_default_struct_value = ref;
			return *this;
		}

		SLAKE_FORCEINLINE bool is_invalid() const noexcept {
			return kind == ReferenceKind::Invalid;
		}

		SLAKE_FORCEINLINE bool is_object_ref() const noexcept {
			return kind == ReferenceKind::ObjectRef;
		}

		SLAKE_FORCEINLINE bool is_static_field_ref() const noexcept {
			return kind == ReferenceKind::StaticFieldRef;
		}

		SLAKE_FORCEINLINE bool is_local_var_ref() const noexcept {
			return kind == ReferenceKind::LocalVarRef;
		}

		SLAKE_FORCEINLINE bool is_coroutine_local_var_ref() const noexcept {
			return kind == ReferenceKind::CoroutineLocalVarRef;
		}

		SLAKE_FORCEINLINE bool is_object_field_ref() const noexcept {
			return kind == ReferenceKind::ObjectFieldRef;
		}

		SLAKE_FORCEINLINE bool is_array_element_ref() const noexcept {
			return kind == ReferenceKind::ArrayElementRef;
		}

		SLAKE_FORCEINLINE bool is_arg_ref() const noexcept {
			return kind == ReferenceKind::ArgRef;
		}

		SLAKE_FORCEINLINE bool is_coroutine_arg_ref() const noexcept {
			return kind == ReferenceKind::CoroutineArgRef;
		}

		SLAKE_FORCEINLINE bool is_struct_field_ref() const noexcept {
			return (((uint8_t)kind) >= (uint8_t)ReferenceKind::StaticFieldStructFieldRef) &&
				   (((uint8_t)kind) <= (uint8_t)ReferenceKind::CoroutineArgStructFieldRef);
		}

		SLAKE_FORCEINLINE bool is_arg_pack_ref() const noexcept {
			return kind == ReferenceKind::ArgPackRef;
		}

		SLAKE_FORCEINLINE bool is_aot_ptr_ref() const noexcept {
			return kind == ReferenceKind::AotPtrRef;
		}

		SLAKE_FORCEINLINE const StaticFieldRef &get_static_field_ref() const noexcept {
			assert(kind == ReferenceKind::StaticFieldRef);
			return as_static_field;
		}

		SLAKE_FORCEINLINE Object *get_object_ref() const noexcept {
			assert(kind == ReferenceKind::ObjectRef);
			return as_object;
		}

		SLAKE_FORCEINLINE Object *&get_object_ref() noexcept {
			assert(kind == ReferenceKind::ObjectRef);
			return as_object;
		}

		SLAKE_FORCEINLINE StaticFieldRef &get_static_field_ref() noexcept {
			assert(kind == ReferenceKind::StaticFieldRef);
			return as_static_field;
		}

		SLAKE_FORCEINLINE const LocalVarRef &get_local_var_ref() const noexcept {
			assert(kind == ReferenceKind::LocalVarRef);
			return as_local_var;
		}

		SLAKE_FORCEINLINE LocalVarRef &get_local_var_ref() noexcept {
			assert(kind == ReferenceKind::LocalVarRef);
			return as_local_var;
		}

		SLAKE_FORCEINLINE const CoroutineLocalVarRef &get_coroutine_local_var_ref() const noexcept {
			assert(kind == ReferenceKind::CoroutineLocalVarRef);
			return as_coroutine_local_var;
		}

		SLAKE_FORCEINLINE CoroutineLocalVarRef &get_coroutine_local_var_ref() noexcept {
			assert(kind == ReferenceKind::CoroutineLocalVarRef);
			return as_coroutine_local_var;
		}

		SLAKE_FORCEINLINE const ObjectFieldRef &get_object_field_ref() const noexcept {
			assert(kind == ReferenceKind::ObjectFieldRef);
			return as_object_field;
		}

		SLAKE_FORCEINLINE ObjectFieldRef &get_object_field_ref() noexcept {
			assert(kind == ReferenceKind::ObjectFieldRef);
			return as_object_field;
		}

		SLAKE_FORCEINLINE const ArrayElementRef &get_array_element_ref() const noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return as_array_element;
		}

		SLAKE_FORCEINLINE ArrayElementRef &get_array_element_ref() noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return as_array_element;
		}

		SLAKE_FORCEINLINE const ArgRef &get_arg_ref() const noexcept {
			assert(kind == ReferenceKind::ArgRef);
			return as_arg;
		}

		SLAKE_FORCEINLINE ArgRef &get_arg_ref() noexcept {
			assert(kind == ReferenceKind::ArgRef);
			return as_arg;
		}

		SLAKE_FORCEINLINE const CoroutineArgRef &get_coroutine_arg_ref() const noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return as_coroutine_arg;
		}

		SLAKE_FORCEINLINE CoroutineArgRef &get_coroutine_arg_ref() noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return as_coroutine_arg;
		}

		SLAKE_FORCEINLINE const ArgPackRef &get_arg_pack_ref() const noexcept {
			assert(kind == ReferenceKind::ArgPackRef);
			return as_arg_pack;
		}

		SLAKE_FORCEINLINE ArgPackRef &get_arg_pack_ref() noexcept {
			assert(kind == ReferenceKind::ArgPackRef);
			return as_arg_pack;
		}

		SLAKE_FORCEINLINE const void *get_aot_ptr_ref() const noexcept {
			assert(kind == ReferenceKind::AotPtrRef);
			return as_aot_ptr;
		}

		SLAKE_FORCEINLINE void *&get_aot_ptr_ref() noexcept {
			assert(kind == ReferenceKind::AotPtrRef);
			return as_aot_ptr;
		}

		SLAKE_FORCEINLINE bool is_valid() const noexcept {
			return kind != ReferenceKind::Invalid;
		}
		explicit SLAKE_FORCEINLINE operator bool() const noexcept {
			return is_valid();
		}

		SLAKE_API bool operator==(const Reference &rhs) const;
		SLAKE_API bool operator!=(const Reference &rhs) const;
		SLAKE_API bool operator<(const Reference &rhs) const;
		SLAKE_API bool operator>(const Reference &rhs) const;
	};

#ifndef ssize_t
	using ssize_t = std::make_signed_t<size_t>;
#endif

	using SizeTypeMarker = bool;
	constexpr static SizeTypeMarker SIZETYPE_MARKER = true;

	struct TypelessScopedEnumValue {
		uint32_t value;
		TypeRef type;
	};

	using ValueFlags = uint8_t;
	constexpr ValueFlags VALUE_LOCAL = 0x01;
	struct Value {
		union {
			int8_t as_i8;
			int16_t as_i16;
			int32_t as_i32;
			int64_t as_i64;
			ssize_t as_isize;
			uint8_t as_u8;
			uint16_t as_u16;
			uint32_t as_u32;
			uint64_t as_u64;
			size_t as_usize;
			float as_f32;
			double as_f64;
			bool as_bool;
			TypeRef as_type;
			Reference as_reference;
			TypelessScopedEnumValue as_typeless_scoped_enum;
		};

		ValueType value_type;
		ValueFlags value_flags;

		SLAKE_FORCEINLINE Value() noexcept = default;
		SLAKE_API Value(const Value &other) noexcept = default;
		SLAKE_FORCEINLINE Value(Value &&other) noexcept = default;
		SLAKE_FORCEINLINE constexpr Value(int8_t data) noexcept : value_type(ValueType::I8), as_i8(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int16_t data) noexcept : value_type(ValueType::I16), as_i16(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int32_t data) noexcept : value_type(ValueType::I32), as_i32(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int64_t data) noexcept : value_type(ValueType::I64), as_i64(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, ssize_t data) noexcept : value_type(ValueType::ISize), as_isize(data), value_flags(0) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(uint8_t data) noexcept : value_type(ValueType::U8), as_u8(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint16_t data) noexcept : value_type(ValueType::U16), as_u16(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint32_t data) noexcept : value_type(ValueType::U32), as_u32(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint64_t data) noexcept : value_type(ValueType::U64), as_u64(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, size_t data) noexcept : value_type(ValueType::USize), as_usize(data), value_flags(0) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(float data) noexcept : value_type(ValueType::F32), as_f32(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(double data) noexcept : value_type(ValueType::F64), as_f64(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr explicit Value(bool data) noexcept : value_type(ValueType::Bool), as_bool(data), value_flags(0) {
		}
		SLAKE_FORCEINLINE Value(const Reference &reference) noexcept : value_type(ValueType::Reference), as_reference(reference), value_flags(0) {
			if (reference.kind == ReferenceKind::Invalid)
				std::terminate();
		}
		SLAKE_FORCEINLINE Value(std::nullptr_t) noexcept : value_type(ValueType::Reference), as_reference(nullptr), value_flags(0) {
		}
		SLAKE_FORCEINLINE Value(const TypelessScopedEnumValue &v) noexcept : value_type(ValueType::TypelessScopedEnum), as_typeless_scoped_enum(v), value_flags(0) {
		}
		SLAKE_FORCEINLINE Value(ValueType vt) noexcept : value_type(vt), value_flags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(ValueType vt, uint32_t index) noexcept : value_type(vt), as_u32(index), value_flags(0) {
		}
		SLAKE_FORCEINLINE Value(const TypeRef &type) noexcept : value_type(ValueType::TypeName), as_type(type), value_flags(0) {
		}

		SLAKE_FORCEINLINE constexpr Value &operator=(ValueType data) noexcept {
			value_type = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int8_t data) noexcept {
			value_type = ValueType::I8;
			this->as_i8 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int16_t data) noexcept {
			value_type = ValueType::I16;
			this->as_i16 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int32_t data) noexcept {
			value_type = ValueType::I32;
			this->as_i32 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int64_t data) noexcept {
			value_type = ValueType::I64;
			this->as_i64 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint8_t data) noexcept {
			value_type = ValueType::U8;
			this->as_u8 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint16_t data) noexcept {
			value_type = ValueType::U16;
			this->as_u16 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint32_t data) noexcept {
			value_type = ValueType::U32;
			this->as_u32 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint64_t data) noexcept {
			value_type = ValueType::U64;
			this->as_u64 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(float data) noexcept {
			value_type = ValueType::F32;
			this->as_f32 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(double data) noexcept {
			value_type = ValueType::F64;
			this->as_f64 = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(bool data) noexcept {
			value_type = ValueType::Bool;
			this->as_bool = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const Reference &entity_ref) noexcept {
			value_type = ValueType::Reference;
			this->as_reference = entity_ref;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(std::nullptr_t entity_ref) noexcept {
			value_type = ValueType::Reference;
			this->as_reference = entity_ref;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const TypelessScopedEnumValue &data) noexcept {
			value_type = ValueType::TypelessScopedEnum;
			this->as_typeless_scoped_enum = data;
			this->value_flags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const TypeRef &type) noexcept {
			value_type = ValueType::TypeName;
			as_type = type;
			this->value_flags = 0;
			return *this;
		}

		SLAKE_FORCEINLINE int8_t get_i8() const noexcept {
			assert(value_type == ValueType::I8);
			return as_i8;
		}

		SLAKE_FORCEINLINE int16_t get_i16() const noexcept {
			assert(value_type == ValueType::I16);
			return as_i16;
		}

		SLAKE_FORCEINLINE int32_t get_i32() const noexcept {
			assert(value_type == ValueType::I32);
			return as_i32;
		}

		SLAKE_FORCEINLINE int64_t get_i64() const noexcept {
			assert(value_type == ValueType::I64);
			return as_i64;
		}

		SLAKE_FORCEINLINE ssize_t get_isize() const noexcept {
			assert(value_type == ValueType::ISize);
			return as_isize;
		}

		SLAKE_FORCEINLINE uint8_t get_u8() const noexcept {
			assert(value_type == ValueType::U8);
			return as_u8;
		}

		SLAKE_FORCEINLINE uint16_t get_u16() const noexcept {
			assert(value_type == ValueType::U16);
			return as_u16;
		}

		SLAKE_FORCEINLINE uint32_t get_u32() const noexcept {
			assert(value_type == ValueType::U32);
			return as_u32;
		}

		SLAKE_FORCEINLINE uint64_t get_u64() const noexcept {
			assert(value_type == ValueType::U64);
			return as_u64;
		}

		SLAKE_FORCEINLINE size_t get_usize() const noexcept {
			assert(value_type == ValueType::USize);
			return as_usize;
		}

		SLAKE_FORCEINLINE float get_f32() const noexcept {
			assert(value_type == ValueType::F32);
			return as_f32;
		}

		SLAKE_FORCEINLINE double get_f64() const noexcept {
			assert(value_type == ValueType::F64);
			return as_f64;
		}

		SLAKE_FORCEINLINE bool get_bool() const noexcept {
			assert(value_type == ValueType::Bool);
			return as_bool;
		}

		SLAKE_FORCEINLINE uint32_t get_reg_index() const noexcept {
			assert(value_type == ValueType::RegIndex);
			return as_u32;
		}

		SLAKE_FORCEINLINE uint32_t get_label() const noexcept {
			assert(value_type == ValueType::Label);
			return as_u32;
		}

		SLAKE_FORCEINLINE TypeRef &get_type_name() noexcept {
			assert(value_type == ValueType::TypeName);
			return as_type;
		}
		SLAKE_FORCEINLINE const TypeRef &get_type_name() const noexcept {
			assert(value_type == ValueType::TypeName);
			return as_type;
		}

		SLAKE_FORCEINLINE Reference &get_reference() noexcept {
			assert(value_type == ValueType::Reference);
			return as_reference;
		}
		SLAKE_FORCEINLINE const Reference &get_reference() const noexcept {
			assert(value_type == ValueType::Reference);
			return as_reference;
		}
		SLAKE_FORCEINLINE TypelessScopedEnumValue &get_typeless_scoped_enum() noexcept {
			assert(value_type == ValueType::TypelessScopedEnum);
			return as_typeless_scoped_enum;
		}
		SLAKE_FORCEINLINE const TypelessScopedEnumValue &get_typeless_scoped_enum() const noexcept {
			assert(value_type == ValueType::TypelessScopedEnum);
			return as_typeless_scoped_enum;
		}

		//
		// Helper APIs.
		//

		SLAKE_FORCEINLINE bool is_i8() const noexcept {
			return value_type == ValueType::I8;
		}

		SLAKE_FORCEINLINE bool is_i16() const noexcept {
			return value_type == ValueType::I16;
		}

		SLAKE_FORCEINLINE bool is_i32() const noexcept {
			return value_type == ValueType::I32;
		}

		SLAKE_FORCEINLINE bool is_i64() const noexcept {
			return value_type == ValueType::I64;
		}

		SLAKE_FORCEINLINE bool is_isize() const noexcept {
			return value_type == ValueType::ISize;
		}

		SLAKE_FORCEINLINE bool is_u8() const noexcept {
			return value_type == ValueType::U8;
		}

		SLAKE_FORCEINLINE bool is_u16() const noexcept {
			return value_type == ValueType::U16;
		}

		SLAKE_FORCEINLINE bool is_u32() const noexcept {
			return value_type == ValueType::U32;
		}

		SLAKE_FORCEINLINE bool is_u64() const noexcept {
			return value_type == ValueType::U64;
		}

		SLAKE_FORCEINLINE bool is_usize() const noexcept {
			return value_type == ValueType::USize;
		}

		SLAKE_FORCEINLINE bool is_f32() const noexcept {
			return value_type == ValueType::F32;
		}

		SLAKE_FORCEINLINE bool is_f64() const noexcept {
			return value_type == ValueType::F64;
		}

		SLAKE_FORCEINLINE bool is_bool() const noexcept {
			return value_type == ValueType::Bool;
		}

		SLAKE_FORCEINLINE bool is_reg_index() const noexcept {
			return value_type == ValueType::RegIndex;
		}

		SLAKE_FORCEINLINE bool is_label() const noexcept {
			return value_type == ValueType::Label;
		}

		SLAKE_FORCEINLINE bool is_type_name() const noexcept {
			return value_type == ValueType::TypeName;
		}

		SLAKE_FORCEINLINE bool is_reference() const noexcept {
			return value_type == ValueType::Reference;
		}
		SLAKE_FORCEINLINE bool is_null() const noexcept {
			return (value_type == ValueType::Reference) && (as_reference.kind == ReferenceKind::ObjectRef) && (!as_reference.as_object);
		}
		SLAKE_FORCEINLINE bool is_typeless_scoped_enum() const noexcept {
			return value_type == ValueType::TypelessScopedEnum;
		}
		SLAKE_FORCEINLINE bool is_invalid() const noexcept {
			return value_type == ValueType::Invalid;
		}

		SLAKE_FORCEINLINE bool is_local() const noexcept {
			return value_flags & VALUE_LOCAL;
		}
		SLAKE_FORCEINLINE void set_local() noexcept {
			value_flags |= VALUE_LOCAL;
		}
		SLAKE_FORCEINLINE void clear_local() noexcept {
			value_flags &= ~VALUE_LOCAL;
		}

		Value &operator=(const Value &other) noexcept = default;
		Value &operator=(Value &&other) noexcept = default;

		SLAKE_API bool operator==(const Value &rhs) const noexcept;

		SLAKE_FORCEINLINE bool operator!=(const Value &rhs) const noexcept {
			return !(*this == rhs);
		}

		SLAKE_API int compares_to(const Value &rhs) const noexcept;
		SLAKE_API bool operator<(const Value &rhs) const noexcept;
		SLAKE_API bool operator>(const Value &rhs) const noexcept;
	};

	SLAKE_API bool is_compatible(const TypeRef &type, const Value &value) noexcept;
}

#endif
