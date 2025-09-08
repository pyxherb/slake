#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <variant>
#include <slake/basedefs.h>
#include "except_base.h"

namespace slake {
	enum class ValueType : uint8_t {
		I8 = 0,	 // Signed 8-bit integer
		I16,	 // Signed 16-bit integer
		I32,	 // Signed 32-bit integer
		I64,	 // Signed 64-bit integer
		ISize,	 // Signed platform-dependent integer
		U8,		 // Unsigned 8-bit integer
		U16,	 // Unsigned 16-bit integer
		U32,	 // Unsigned 32-bit integer
		U64,	 // Unsigned 64-bit integer
		USize,	 // Unsigned platform-dependent integer
		F32,	 // 32-bit floating point number
		F64,	 // 64-bit floating point number
		Bool,	 // Boolean

		EntityRef,	// Entity reference

		RegRef,	   // Register reference
		TypeName,  // Type name

		Label,	// Label reference

		Undefined = UINT8_MAX,
	};

	enum class TypeId : uint8_t {
		Void = 0,  // Void

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

		String,		 // String
		Instance,	 // Object instance
		GenericArg,	 // Generic argument

		Array,	// Array
		Ref,	// Reference
		Tuple,	// Tuple
		SIMD,	// SIMD

		Fn,				// Function delegation
		ParamTypeList,	// Parameter type list
		Unpacking,		// Unpacking

		Any,	  // Any
		Unknown,  // Unknown
	};

	SLAKE_API TypeId valueTypeToTypeId(ValueType valueType);
	SLAKE_API bool isValueTypeCompatibleTypeId(TypeId typeId);
	SLAKE_API ValueType typeIdToValueType(TypeId typeId);

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
	class ModuleObject;
	class ArrayObject;
	class InstanceObject;
	struct MajorFrame;

	SLAKE_API bool isTypeDefObject(Object *object);

	struct TypeRef {
		TypeId typeId;
		Object *typeDef;

		TypeRef() = default;
		TypeRef(const TypeRef &) = default;
		SLAKE_FORCEINLINE TypeRef(TypeId typeId) : typeId(typeId), typeDef(nullptr) {
			assert(isFundamentalType(typeId));
		}
		SLAKE_FORCEINLINE TypeRef(TypeId typeId, Object *typeDef) : typeId(typeId), typeDef(typeDef) {
			assert(!isFundamentalType(typeId));
			assert(isTypeDefObject(typeDef));
		}
		~TypeRef() = default;

		TypeRef &operator=(const TypeRef &) = default;

		SLAKE_FORCEINLINE int comparesTo(const TypeRef &rhs) const noexcept {
			if (typeId < rhs.typeId)
				return -1;
			if(typeId > rhs.typeId)
				return 1;
			if (isFundamentalType(typeId))
				return 0;
			if (typeDef < rhs.typeDef)
				return -1;
			if(typeDef > rhs.typeDef)
				return 1;
			return 0;
		}

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
			return typeId != TypeId::Void;
		}

		SLAKE_API TypeRef TypeRef::duplicate(bool &succeededOut) const;
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

		SLAKE_FORCEINLINE bool operator()(const TypeRef& lhs, const TypeRef& rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};
}

/*
namespace std {
	SLAKE_API string to_string(const slake::TypeRef &type, const slake::Runtime *rt);
}*/

#endif
