#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <slake/basedefs.h>
#include "except_base.h"

namespace slake {
	enum class ValueType : uint8_t {
		Invalid = 0,  // Used internally
		I8,			  // Signed 8-bit integer
		I16,		  // Signed 16-bit integer
		I32,		  // Signed 32-bit integer
		I64,		  // Signed 64-bit integer
		ISize,		  // Signed platform-dependent integer
		U8,			  // Unsigned 8-bit integer
		U16,		  // Unsigned 16-bit integer
		U32,		  // Unsigned 32-bit integer
		U64,		  // Unsigned 64-bit integer
		USize,		  // Unsigned platform-dependent integer
		F32,		  // 32-bit floating point number
		F64,		  // 64-bit floating point number
		Bool,		  // Boolean

		Reference,	// Reference

		TypelessScopedEnum,	 // Typeless scoped enumeration

		RegIndex,  // Register index
		TypeName,  // Type name

		Label,	// Label

		Undefined = UINT8_MAX,	// For empty registers, etc.
	};

	enum class TypeId : uint8_t {
		Invalid = 0,  // Invalid

		Void,  // Void

		I8,		// Signed 8-bit integer
		I16,	// Signed 16-bit integer
		I32,	// Signed 32-bit integer
		I64,	// Signed 64-bit integer
		ISize,	// Signed platform-dependent integer
		U8,		// Unsigned 8-bit integer
		U16,	// Unsigned 16-bit integer
		U32,	// Unsigned 32-bit integer
		U64,	// Unsigned 64-bit integer
		USize,	// Unsigned platform-dependent integer
		F32,	// 32-bit floating point number
		F64,	// 64-bit floating point number
		Bool,	// Boolean

		Object,				 // Object
		String,				 // String
		Instance,			 // Object instance
		StructInstance,		 // Structure instance
		ScopedEnum,			 // Scoped enumeration
		TypelessScopedEnum,	 // Typeless scoped enumeration
		UnionEnum,			 // Union enumeration
		UnionEnumItem,		 // Union enumeration item
		GenericArg,			 // Generic argument

		Array,	  // Array
		Ref,	  // Reference
		TempRef,  // Temporary reference
		Tuple,	  // Tuple
		SIMD,	  // SIMD

		Fn,				// Function delegation
		ParamTypeList,	// Parameter type list
		Unpacking,		// Unpacking

		Any,  // Any

		Unknown,  // Unknown
	};

	SLAKE_API TypeId value_type_to_type_id(ValueType value_type) noexcept;
	SLAKE_API bool is_value_type_compatible_type_id(TypeId type_id) noexcept;
	SLAKE_API ValueType type_id_to_value_type(TypeId type_id) noexcept;

	class Runtime;
	class Object;
	class IdRefObject;
	class StringObject;
	class BasicModuleObject;
	class ArrayObject;
	class InstanceObject;
	class TypeDefObject;
	struct MajorFrame;
	class CustomTypeDefObject;
	class ArrayTypeDefObject;
	class RefTypeDefObject;
	class GenericArgTypeDefObject;
	class FnTypeDefObject;
	class ParamTypeListTypeDefObject;
	class TupleTypeDefObject;
	class SIMDTypeDefObject;
	class UnpackingTypeDefObject;

	SLAKE_API bool is_type_def_object(Object *object);

	using TypeModifier = uint8_t;
	constexpr static TypeModifier TYPE_FINAL = 0x01, TYPE_LOCAL = 0x02, TYPE_NULLABLE = 0x04;

	struct TypeRef {
		TypeId type_id;
		TypeModifier type_modifier;
		TypeDefObject *type_def;

		TypeRef() noexcept = default;
		TypeRef(const TypeRef &) noexcept = default;
		SLAKE_FORCEINLINE TypeRef(TypeId type_id) : type_id(type_id), type_modifier(0), type_def(nullptr) {
		}
		SLAKE_FORCEINLINE TypeRef(TypeId type_id, TypeModifier type_modifier) : type_id(type_id), type_modifier(type_modifier), type_def(nullptr) {
		}
		SLAKE_FORCEINLINE TypeRef(TypeId type_id, TypeDefObject *type_def) : type_id(type_id), type_modifier(0), type_def(type_def) {
			assert(is_type_def_object((Object *)type_def));
		}
		SLAKE_FORCEINLINE TypeRef(TypeId type_id, TypeDefObject *type_def, TypeModifier type_modifier) : type_id(type_id), type_modifier(type_modifier), type_def(type_def) {
			assert(is_type_def_object((Object *)type_def));
		}
		~TypeRef() = default;

		TypeRef &operator=(const TypeRef &) noexcept = default;

		SLAKE_API int compares_to(const TypeRef &rhs) const noexcept;

		SLAKE_FORCEINLINE bool operator==(const TypeRef &rhs) const {
			return compares_to(rhs) == 0;
		}

		SLAKE_FORCEINLINE bool operator!=(const TypeRef &rhs) const {
			return compares_to(rhs) != 0;
		}

		SLAKE_FORCEINLINE bool operator<(const TypeRef &rhs) const {
			return compares_to(rhs) < 0;
		}

		SLAKE_FORCEINLINE bool operator>(const TypeRef &rhs) const {
			return compares_to(rhs) > 0;
		}

		SLAKE_FORCEINLINE explicit operator bool() const {
			return type_id != TypeId::Invalid;
		}

		SLAKE_API TypeRef duplicate(bool &succeeded_out) const;

		SLAKE_FORCEINLINE bool is_final() const noexcept {
			return type_modifier & TYPE_FINAL;
		}

		SLAKE_FORCEINLINE bool is_local() const noexcept {
			return type_modifier & TYPE_LOCAL;
		}

		SLAKE_FORCEINLINE bool is_nullable() const noexcept {
			return type_modifier & TYPE_NULLABLE;
		}

		SLAKE_FORCEINLINE void set_final() noexcept {
			type_modifier |= TYPE_FINAL;
		}

		SLAKE_FORCEINLINE void set_local() noexcept {
			type_modifier |= TYPE_LOCAL;
		}

		SLAKE_FORCEINLINE void set_nullable() noexcept {
			type_modifier |= TYPE_NULLABLE;
		}

		SLAKE_FORCEINLINE void clear_final() noexcept {
			type_modifier &= ~TYPE_FINAL;
		}

		SLAKE_FORCEINLINE void clear_local() noexcept {
			type_modifier &= ~TYPE_LOCAL;
		}

		SLAKE_FORCEINLINE void clear_nullable() noexcept {
			type_modifier &= ~TYPE_NULLABLE;
		}

		SLAKE_FORCEINLINE CustomTypeDefObject *get_custom_type_def() const;
		SLAKE_FORCEINLINE ArrayTypeDefObject *get_array_type_def() const;
		SLAKE_FORCEINLINE RefTypeDefObject *get_ref_type_def() const;
		SLAKE_FORCEINLINE GenericArgTypeDefObject *get_generic_arg_type_def() const;
		SLAKE_FORCEINLINE FnTypeDefObject *get_fn_type_def() const;
		SLAKE_FORCEINLINE ParamTypeListTypeDefObject *get_param_type_list_type_def() const;
		SLAKE_FORCEINLINE TupleTypeDefObject *get_tuple_type_def() const;
		SLAKE_FORCEINLINE SIMDTypeDefObject *get_simdtype_def() const;
		SLAKE_FORCEINLINE UnpackingTypeDefObject *get_unpacking_type_def() const;
	};

	static_assert(std::is_trivially_copyable_v<TypeRef>, "TypeRef must be trivially copyable");
	static_assert(std::is_trivially_copy_assignable_v<TypeRef>, "TypeRef must be trivially copy-assignable");
	static_assert(std::is_trivially_destructible_v<TypeRef>, "TypeRef must be trivially destructible");

	class ClassObject;
	class InterfaceObject;

	class Runtime;

	struct TypeRefComparator {
		SLAKE_API int operator()(const TypeRef &lhs, const TypeRef &rhs) const noexcept;
	};

	struct TypeRefLtComparator {
		TypeRefComparator inner_comparator;

		SLAKE_FORCEINLINE bool operator()(const TypeRef &lhs, const TypeRef &rhs) const noexcept {
			return inner_comparator(lhs, rhs) < 0;
		}
	};
}

/*
namespace std {
	SLAKE_API string to_string(const slake::TypeRef &type, const slake::Runtime *rt);
}*/

#endif
