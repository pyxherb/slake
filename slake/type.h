#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <string>

namespace Slake {
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

	class Value;
	class RefValue;

	struct Type final {
		ValueType valueType;  // Value type
		union {
			Value *customType;	 // Pointer to the type object
			Type *array;		 // Element type of the array
			RefValue *deferred;	 // Reference to the type object
			struct {
				Type *k;		 // Key type
				Type *v;		 // Value type
			} map;				 // Extra data for maps
			uint8_t genericArg;	 // Index in generic arguments
		} exData = {};			 // Extra data

		inline Type() noexcept : valueType(ValueType::NONE) {}
		inline Type(const Type &x) noexcept { *this = x; }
		inline Type(const Type &&x) noexcept { *this = x; }
		inline Type(ValueType valueType) noexcept : valueType(valueType) {}
		inline Type(ValueType valueType, Value *classObject) noexcept : valueType(valueType) { exData.customType = classObject; }
		inline Type(ValueType valueType, Type elementType) : valueType(valueType) { exData.array = new Type(elementType); }
		Type(RefValue *ref);

		inline Type(Type k, Type v) : valueType(ValueType::MAP) {
			exData.map.k = new Type(k), exData.map.v = new Type(v);
		}

		~Type();

		bool isDeferred() noexcept;

		inline operator bool() noexcept {
			return valueType != ValueType::NONE;
		}

		inline bool operator==(const Type &&x) noexcept {
			if (x.valueType != valueType)
				return false;
			switch (x.valueType) {
				case ValueType::CLASS:
				case ValueType::STRUCT:
				case ValueType::OBJECT:
					return exData.customType == x.exData.customType;
				case ValueType::ARRAY:
					return exData.array == x.exData.array;
				case ValueType::MAP:
					return (exData.map.k == x.exData.map.k) &&
						   (exData.map.v == x.exData.map.v);
			}
			return true;
		}

		inline bool operator==(const Type &x) noexcept {
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
			std::memcpy(&exData, &(x.exData), sizeof(exData));
			return *this;
		}

		inline Type &operator=(const Type &x) noexcept {
			return *this = std::move(x);
		}

		inline Value *resolveCustomType() {
			if (valueType == ValueType::CLASS || valueType == ValueType::STRUCT)
				return exData.customType;
			return nullptr;
		}
	};

	class ClassValue;
	class InterfaceValue;
	class TraitValue;

	bool hasImplemented(ClassValue *c, InterfaceValue *i);
	bool isCompatibleWith(ClassValue *c, TraitValue *t);
	bool isConvertible(Type a, Type b);
}

namespace std {
	inline std::string to_string(Slake::Type &&type) {
		std::string s = "{\"valueType\":" + std::to_string((uint8_t)type.valueType);

		switch (type.valueType) {
			case Slake::ValueType::ARRAY: {
				s += ",\"elementType\":" + std::to_string(*type.exData.array);
				break;
			}
			case Slake::ValueType::STRUCT:
			case Slake::ValueType::CLASS:
			case Slake::ValueType::OBJECT: {
				s += ",\"typeValue\":" + std::to_string((uintptr_t)type.exData.customType);
				break;
			}
			case Slake::ValueType::FN: {
				// Unimplemented yet
				break;
			}
			case Slake::ValueType::VAR: {
				// Unimplemented yet
				break;
			}
			case Slake::ValueType::MAP: {
				s += ",\"keyType\":" + std::to_string(*type.exData.map.k);
				s += ",\"valueType\":" + std::to_string(*type.exData.map.v);
				break;
			}
		}
		s += "}";

		return s;
	}
	inline std::string to_string(Slake::Type &type) {
		return to_string(move(type));
	}
	inline std::string to_string(Slake::Type *type) {
		return to_string(*type);
	}
}

#endif
