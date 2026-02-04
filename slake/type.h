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

	SLAKE_API TypeId valueTypeToTypeId(ValueType valueType) noexcept;
	SLAKE_API bool isValueTypeCompatibleTypeId(TypeId typeId) noexcept;
	SLAKE_API ValueType typeIdToValueType(TypeId typeId) noexcept;

	SLAKE_FORCEINLINE constexpr bool isFundamentalType(TypeId typeId) {
		switch (typeId) {
			case TypeId::Void:
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
			case TypeId::Any:
			case TypeId::Unknown:
				return true;
			default:
				break;
		}

		return false;
	}

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

	SLAKE_API bool isTypeDefObject(Object *object);

	using TypeModifier = uint8_t;
	constexpr static TypeModifier TYPE_FINAL = 0x01, TYPE_LOCAL = 0x02;

	struct TypeRef {
		TypeId typeId;
		TypeModifier typeModifier;
		TypeDefObject *typeDef;

		TypeRef() noexcept = default;
		TypeRef(const TypeRef &) noexcept = default;
		SLAKE_FORCEINLINE TypeRef(TypeId typeId) : typeId(typeId), typeModifier(0), typeDef(nullptr) {
		}
		SLAKE_FORCEINLINE TypeRef(TypeId typeId, TypeModifier typeModifier) : typeId(typeId), typeModifier(typeModifier), typeDef(nullptr) {
		}
		SLAKE_FORCEINLINE TypeRef(TypeId typeId, TypeDefObject *typeDef) : typeId(typeId), typeModifier(0), typeDef(typeDef) {
			assert(isTypeDefObject((Object *)typeDef));
		}
		SLAKE_FORCEINLINE TypeRef(TypeId typeId, TypeDefObject *typeDef, TypeModifier typeModifier) : typeId(typeId), typeModifier(typeModifier), typeDef(typeDef) {
			assert(isTypeDefObject((Object *)typeDef));
		}
		~TypeRef() = default;

		TypeRef &operator=(const TypeRef &) noexcept = default;

		SLAKE_API int comparesTo(const TypeRef &rhs) const noexcept;

		SLAKE_FORCEINLINE bool operator==(const TypeRef &rhs) const {
			return comparesTo(rhs) == 0;
		}

		SLAKE_FORCEINLINE bool operator!=(const TypeRef &rhs) const {
			return comparesTo(rhs) != 0;
		}

		SLAKE_FORCEINLINE bool operator<(const TypeRef &rhs) const {
			return comparesTo(rhs) < 0;
		}

		SLAKE_FORCEINLINE bool operator>(const TypeRef &rhs) const {
			return comparesTo(rhs) > 0;
		}

		SLAKE_FORCEINLINE explicit operator bool() const {
			return typeId != TypeId::Invalid;
		}

		SLAKE_API TypeRef duplicate(bool &succeededOut) const;

		SLAKE_FORCEINLINE bool isFinal() const noexcept {
			return typeModifier & TYPE_FINAL;
		}

		SLAKE_FORCEINLINE bool isLocal() const noexcept {
			return typeModifier & TYPE_LOCAL;
		}

		SLAKE_FORCEINLINE CustomTypeDefObject *getCustomTypeDef() const;
		SLAKE_FORCEINLINE ArrayTypeDefObject *getArrayTypeDef() const;
		SLAKE_FORCEINLINE RefTypeDefObject *getRefTypeDef() const;
		SLAKE_FORCEINLINE GenericArgTypeDefObject *getGenericArgTypeDef() const;
		SLAKE_FORCEINLINE FnTypeDefObject *getFnTypeDef() const;
		SLAKE_FORCEINLINE ParamTypeListTypeDefObject *getParamTypeListTypeDef() const;
		SLAKE_FORCEINLINE TupleTypeDefObject *getTupleTypeDef() const;
		SLAKE_FORCEINLINE SIMDTypeDefObject *getSIMDTypeDef() const;
		SLAKE_FORCEINLINE UnpackingTypeDefObject *getUnpackingTypeDef() const;
	};

	static_assert(std::is_trivially_copyable_v<TypeRef>, "TypeRef must be trivially copyable");
	static_assert(std::is_trivially_copy_assignable_v<TypeRef>, "TypeRef must be trivially copy-assignable");
	static_assert(std::is_trivially_destructible_v<TypeRef>, "TypeRef must be trivially destructible");

	SLAKE_FORCEINLINE bool isFundamentalType(TypeRef type) {
		return isFundamentalType(type.typeId);
	}

	class ClassObject;
	class InterfaceObject;

	class Runtime;

	struct TypeRefComparator {
		SLAKE_API int operator()(const TypeRef &lhs, const TypeRef &rhs) const noexcept;
	};

	struct TypeRefLtComparator {
		TypeRefComparator innerComparator;

		SLAKE_FORCEINLINE bool operator()(const TypeRef &lhs, const TypeRef &rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};
}

/*
namespace std {
	SLAKE_API string to_string(const slake::TypeRef &type, const slake::Runtime *rt);
}*/

#endif
