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

		inline Type() noexcept : typeId(TypeId::None) {}
		inline Type(const Type &x) noexcept { *this = x; }
		inline Type(ValueType valueType) noexcept : typeId(TypeId::Value) { exData.valueType = valueType; }
		inline Type(TypeId type) noexcept : typeId(type) {}
		inline Type(TypeId type, Object *destObject) noexcept : typeId(type) {
			exData.ptr = destObject;
		}
		inline Type(StringObject *nameObject, Object *ownerObject) noexcept : typeId(TypeId::GenericArg) {
			exData.genericArg.nameObject = nameObject;
			exData.genericArg.ownerObject = ownerObject;
		}
		Type(IdRefObject *ref);

		static Type makeArrayTypeName(Runtime *runtime, const Type &elementType);
		static Type makeRefTypeName(Runtime *runtime, const Type &elementType);

		Type duplicate() const;

		inline ValueType getValueTypeExData() const { return exData.valueType; }
		inline Object *getCustomTypeExData() const { return exData.ptr; }
		Type &getArrayExData() const;
		Type &getRefExData() const;

		bool isLoadingDeferred() const noexcept;
		void loadDeferredType(const Runtime *rt);

		inline operator bool() const noexcept {
			return typeId != TypeId::None;
		}

		bool operator<(const Type &rhs) const;
		/// @brief The less than operator is required by containers such as map and set.
		/// @param rhs Right-hand side operand.
		/// @return true if lesser, false otherwise.
		inline bool operator<(Type &&rhs) const noexcept {
			auto r = rhs;
			return *this == r;
		}

		inline bool operator==(Type &&rhs) const noexcept {
			auto r = rhs;
			return *this == r;
		}

		bool operator==(const Type &rhs) const;

		inline bool operator!=(Type &&rhs) const noexcept { return !(*this == rhs); }
		inline bool operator!=(const Type &rhs) const noexcept { return !(*this == rhs); }

		inline bool operator==(TypeId rhs) const noexcept {
			return this->typeId == rhs;
		}
		inline bool operator!=(TypeId rhs) const noexcept {
			return this->typeId != rhs;
		}

		inline Type &operator=(const Type &rhs) noexcept = default;
		inline Type &operator=(Type &&rhs) noexcept = default;

		inline Object *resolveCustomType() const {
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
	string to_string(const slake::Type &type, const slake::Runtime *rt);
	inline string to_string(slake::Type &&type, const slake::Runtime *rt) {
		slake::Type t = type;
		return to_string(t, rt);
	}
}

#endif
