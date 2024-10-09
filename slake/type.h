#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <variant>

namespace slake {
	enum class ValueType : uint8_t {
		I8 = 0,		// Signed 8-bit integer
		I16,		// Signed 16-bit integer
		I32,		// Signed 32-bit integer
		I64,		// Signed 64-bit integer
		U8,			// Unsigned 8-bit integer
		U16,		// Unsigned 16-bit integer
		U32,		// Unsigned 32-bit integer
		U64,		// Unsigned 64-bit integer
		F32,		// 32-bit floating point number
		F64,		// 64-bit floating point number
		Bool,		// Boolean
		ObjectRef,	// Object reference

		RegRef,	   // Register reference
		TypeName,  // Type name
		VarRef,	   // Variable reference

		Undefined = UINT8_MAX,
	};

	enum class TypeId : uint8_t {
		None,  // None, aka `null'

		Value,		 // Value type
		String,		 // String
		Instance,	 // Object instance
		GenericArg,	 // Generic argument

		Array,	// Array
		Ref,	// Reference

		Any	 // Any
	};

	class Runtime;
	class Object;
	class IdRefObject;
	class StringObject;

	union TypeExData {
		ValueType valueType;
		Object *ptr;
		struct {
			StringObject *nameObject;
			Object *ownerObject;
		} genericArg;
	};

	struct Type final {
		TypeId typeId;	// Type ID

		TypeExData exData;

		SLAKE_FORCEINLINE Type() noexcept : typeId(TypeId::None) {}
		SLAKE_FORCEINLINE Type(const Type &x) noexcept { *this = x; }
		SLAKE_FORCEINLINE Type(ValueType valueType) noexcept : typeId(TypeId::Value) { exData.valueType = valueType; }
		SLAKE_FORCEINLINE Type(TypeId type) noexcept : typeId(type) {}
		SLAKE_FORCEINLINE Type(TypeId type, Object *destObject) noexcept : typeId(type) {
			exData.ptr = destObject;
		}
		SLAKE_FORCEINLINE Type(StringObject *nameObject, Object *ownerObject) noexcept : typeId(TypeId::GenericArg) {
			exData.genericArg.nameObject = nameObject;
			exData.genericArg.ownerObject = ownerObject;
		}
		SLAKE_API Type(IdRefObject *ref);

		SLAKE_API static Type makeArrayTypeName(Runtime *runtime, const Type &elementType);
		SLAKE_API static Type makeRefTypeName(Runtime *runtime, const Type &elementType);

		SLAKE_API Type duplicate() const;

		SLAKE_FORCEINLINE ValueType getValueTypeExData() const { return exData.valueType; }
		SLAKE_FORCEINLINE Object *getCustomTypeExData() const { return exData.ptr; }
		SLAKE_API Type &getArrayExData() const;
		SLAKE_API Type &getRefExData() const;

		SLAKE_API bool isLoadingDeferred() const noexcept;
		SLAKE_API void loadDeferredType(const Runtime *rt);

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
	SLAKE_FORCEINLINE string to_string(slake::Type &&type, const slake::Runtime *rt) {
		slake::Type t = type;
		return to_string(t, rt);
	}
}

#endif
