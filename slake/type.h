#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <variant>
#include "valdef/base.h"

namespace slake {
	enum class ValueType : uint8_t {
		NONE,  // None, aka `null'

		U8,		 // Unsigned 8-bit integer
		U16,	 // Unsigned 16-bit integer
		U32,	 // Unsigned 32-bit integer
		U64,	 // Unsigned 64-bit integer
		I8,		 // Signed 8-bit integer
		I16,	 // Signed 16-bit integer
		I32,	 // Signed 32-bit integer
		I64,	 // Signed 64-bit integer
		F32,	 // 32-bit floating point number
		F64,	 // 64-bit floating point number
		BOOL,	 // Boolean
		STRING,	 // String

		FN,		// Function
		MOD,	// Module
		VAR,	// Variable
		ARRAY,	// Array
		MAP,	// Map

		CLASS,		// Class
		INTERFACE,	// Interface
		TRAIT,		// Trait
		STRUCT,		// Structure
		OBJECT,		// Object instance
		STRUCTOBJ,	// Structure instance

		ANY,  // Any

		REF,		  // Reference
		GENERIC_ARG,  // Generic argument
		ROOT,		  // Root value

		INVALID = 0xff	// Invalid
	};

	template <typename T>
	constexpr inline ValueType getValueType() {
		if constexpr (std::is_same<T, std::int8_t>::value)
			return ValueType::I8;
		else if constexpr (std::is_same<T, std::int16_t>::value)
			return ValueType::I16;
		else if constexpr (std::is_same<T, std::int32_t>::value)
			return ValueType::I32;
		else if constexpr (std::is_same<T, std::int64_t>::value)
			return ValueType::I64;
		else if constexpr (std::is_same<T, uint8_t>::value)
			return ValueType::U8;
		else if constexpr (std::is_same<T, uint16_t>::value)
			return ValueType::U16;
		else if constexpr (std::is_same<T, uint32_t>::value)
			return ValueType::U32;
		else if constexpr (std::is_same<T, uint64_t>::value)
			return ValueType::U64;
		else if constexpr (std::is_same<T, float>::value)
			return ValueType::F32;
		else if constexpr (std::is_same<T, double>::value)
			return ValueType::F64;
		else if constexpr (std::is_same<T, bool>::value)
			return ValueType::BOOL;
		else if constexpr (std::is_same<T, std::string>::value)
			return ValueType::STRING;
		else
			// We don't use `false' as the condition due to the compiler evaluates it
			// prematurely.
			static_assert(!std::is_same<T, T>::value);
	}

	class Runtime;
	class Value;
	class RefValue;
	class MemberValue;

	struct Type final {
		ValueType valueType;  // Value type
		mutable std::variant<std::monostate, ValueRef<>, Type *, std::pair<Type *, Type *>, uint8_t> exData;

		inline Type() noexcept : valueType(ValueType::NONE) {}
		inline Type(const Type &x) noexcept { *this = x; }
		inline Type(const Type &&x) noexcept { *this = x; }
		inline Type(ValueType valueType) noexcept : valueType(valueType) {}
		inline Type(ValueType valueType, Value *classObject) noexcept : valueType(valueType) {
			exData = classObject;
		}
		inline Type(ValueType valueType, Type elementType) : valueType(valueType) {
			exData = elementType;
		}
		Type(RefValue *ref);

		inline Type(Type k, Type v) : valueType(ValueType::MAP) {
			exData = std::pair<Type *, Type *>(new Type(k), new Type(v));
		}

		~Type();

		inline ValueRef<> getCustomTypeExData() const { return std::get<ValueRef<>>(exData); }
		inline Type getArrayExData() const { return *std::get<Type *>(exData); }
		inline std::pair<Type *, Type *> getMapExData() const { return std::get<std::pair<Type *, Type *>>(exData); }
		inline uint8_t getGenericArgExData() const { return std::get<uint8_t>(exData); }

		bool isLoadingDeferred() const noexcept;
		void loadDeferredType(const Runtime *rt) const;

		inline operator bool() const noexcept {
			return valueType != ValueType::NONE;
		}

		inline bool operator==(const Type &&x) const noexcept {
			if (x.valueType != valueType)
				return false;
			switch (x.valueType) {
				case ValueType::CLASS:
				case ValueType::STRUCT:
				case ValueType::OBJECT:
					return getCustomTypeExData() == x.getCustomTypeExData();
				case ValueType::ARRAY:
					return getArrayExData() == x.getArrayExData();
				case ValueType::MAP:
					return *(getMapExData().first) == *(x.getMapExData().first) &&
						   *(getMapExData().second) == *(x.getMapExData().second);
			}
			return true;
		}

		inline bool operator==(const Type &x) const noexcept {
			return *this == std::move(x);
		}

		inline bool operator!=(const Type &&x) noexcept { return !(*this == x); }
		inline bool operator!=(const Type &x) noexcept { return !(*this == x); }

		inline bool operator==(ValueType x) noexcept {
			return this->valueType == x;
		}
		inline bool operator!=(ValueType x) noexcept {
			return this->valueType != x;
		}

		inline Type &operator=(const Type &&x) noexcept {
			valueType = x.valueType;
			exData = x.exData;
			return *this;
		}

		inline Type &operator=(const Type &x) noexcept {
			return *this = std::move(x);
		}

		inline Value *resolveCustomType() {
			if (valueType == ValueType::CLASS || valueType == ValueType::STRUCT)
				return (Value *)*getCustomTypeExData();
			return nullptr;
		}
	};

	class ClassValue;
	class InterfaceValue;
	class TraitValue;

	bool hasImplemented(ClassValue *c, InterfaceValue *i);
	bool isComplyWith(ClassValue *c, TraitValue *t);
	bool isConvertible(Type a, Type b);
	bool isCompatible(Type a, Type b);

	class Runtime;
}

namespace std {
	string to_string(slake::Type &&type, slake::Runtime *rt);
	inline string to_string(slake::Type &type, slake::Runtime *rt) {
		return to_string(move(type), rt);
	}
}

#endif
