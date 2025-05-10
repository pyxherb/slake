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
		U8,		 // Unsigned 8-bit integer
		U16,	 // Unsigned 16-bit integer
		U32,	 // Unsigned 32-bit integer
		U64,	 // Unsigned 64-bit integer
		F32,	 // 32-bit floating point number
		F64,	 // 64-bit floating point number
		Bool,	 // Boolean

		EntityRef,	// Entity reference

		RegRef,	   // Register reference
		TypeName,  // Type name

		Label, // Label reference

		Undefined = UINT8_MAX,
	};

	enum class TypeId : uint8_t {
		None = 0,  // None, aka `null'

		I8,	   // Signed 8-bit integer
		I16,   // Signed 16-bit integer
		I32,   // Signed 32-bit integer
		I64,   // Signed 64-bit integer
		U8,	   // Unsigned 8-bit integer
		U16,   // Unsigned 16-bit integer
		U32,   // Unsigned 32-bit integer
		U64,   // Unsigned 64-bit integer
		F32,   // 32-bit floating point number
		F64,   // 64-bit floating point number
		Bool,  // Boolean

		String,		 // String
		Instance,	 // Object instance
		GenericArg,	 // Generic argument

		Array,	// Array
		Ref,	// Reference

		FnDelegate,	 // Function delegation

		Any	 // Any
	};

	SLAKE_API TypeId valueTypeToTypeId(ValueType valueType);
	SLAKE_API bool isValueTypeCompatibleTypeId(TypeId typeId);
	SLAKE_API ValueType typeIdToValueType(TypeId typeId);

	class Runtime;
	class Object;
	class IdRefObject;
	class StringObject;
	class ModuleObject;
	class ArrayObject;
	class InstanceObject;
	class TypeDefObject;
	struct MajorFrame;

	union TypeExData {
		Object *object;
		TypeDefObject *typeDef;
		struct {
			StringObject *nameObject;
			// For type comparison.
			Object *ownerObject;
		} genericArg;
	};

	struct Type final {
		TypeExData exData;

		TypeId typeId;	// Type ID

		SLAKE_FORCEINLINE Type() = default;
		SLAKE_FORCEINLINE Type(const Type &x) = default;
		SLAKE_FORCEINLINE Type(Type &&x) = default;
		SLAKE_FORCEINLINE constexpr Type(TypeId type) noexcept : typeId(type), exData({}) {}
		SLAKE_FORCEINLINE Type(TypeId type, Object *destObject) noexcept : typeId(type) {
			exData.object = destObject;
		}
		SLAKE_FORCEINLINE Type(StringObject *nameObject, Object *ownerObject) noexcept : typeId(TypeId::GenericArg) {
			exData.genericArg.nameObject = nameObject;
			exData.genericArg.ownerObject = ownerObject;
		}
		SLAKE_API Type(IdRefObject *ref);

		SLAKE_API static Type makeArrayTypeName(Runtime *runtime, const Type &elementType);
		SLAKE_API static Type makeRefTypeName(Runtime *runtime, const Type &elementType);

		SLAKE_API Type duplicate(bool &succeededOut) const;

		SLAKE_FORCEINLINE Object *getCustomTypeExData() const { return exData.object; }
		SLAKE_API Type &getArrayExData() const;
		SLAKE_API Type &getRefExData() const;

		SLAKE_API bool isLoadingDeferred() const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer loadDeferredType(Runtime *rt);

		SLAKE_FORCEINLINE operator bool() const noexcept {
			return typeId != TypeId::None;
		}

		SLAKE_API bool operator<(const Type &rhs) const;
		/// @brief The less than operator is required by containers such as map and set.
		/// @param rhs Right-hand side operand.
		/// @return true if lesser, false otherwise.
		SLAKE_FORCEINLINE bool operator<(Type &&rhs) const noexcept {
			auto r = rhs;
			return *this == r;
		}

		SLAKE_FORCEINLINE bool operator==(Type &&rhs) const noexcept {
			auto r = rhs;
			return *this == r;
		}

		SLAKE_API bool operator==(const Type &rhs) const;

		SLAKE_FORCEINLINE bool operator!=(Type &&rhs) const noexcept { return !(*this == rhs); }
		SLAKE_FORCEINLINE bool operator!=(const Type &rhs) const noexcept { return !(*this == rhs); }

		SLAKE_FORCEINLINE bool operator==(TypeId rhs) const noexcept {
			return this->typeId == rhs;
		}
		SLAKE_FORCEINLINE bool operator!=(TypeId rhs) const noexcept {
			return this->typeId != rhs;
		}

		Type &operator=(const Type &rhs) noexcept = default;
		Type &operator=(Type &&rhs) noexcept = default;

		SLAKE_FORCEINLINE Object *resolveCustomType() const {
			if (typeId == TypeId::Instance)
				return (Object *)getCustomTypeExData();
			return nullptr;
		}
	};

	class ClassObject;
	class InterfaceObject;

	class Runtime;
}

namespace std {
	SLAKE_API string to_string(const slake::Type &type, const slake::Runtime *rt);
}

#endif
