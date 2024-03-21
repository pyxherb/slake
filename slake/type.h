#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <variant>
#include "valdef/base.h"

namespace slake {
	enum class TypeId : uint8_t {
		None,  // None, aka `null'

		U8,		  // Unsigned 8-bit integer
		U16,	  // Unsigned 16-bit integer
		U32,	  // Unsigned 32-bit integer
		U64,	  // Unsigned 64-bit integer
		I8,		  // Signed 8-bit integer
		I16,	  // Signed 16-bit integer
		I32,	  // Signed 32-bit integer
		I64,	  // Signed 64-bit integer
		F32,	  // 32-bit floating point number
		F64,	  // 64-bit floating point number
		Bool,	  // Boolean
		String,	  // String
		WString,  // UTF-32 string
		Char,	  // ASCII character
		WChar,	  // UTF-32 character

		Fn,		 // Function
		Module,	 // Module
		Var,	 // Variable
		Array,	 // Array
		Map,	 // Map

		Class,		// Class
		Interface,	// Interface
		Trait,		// Trait
		Object,		// Object instance

		Any,  // Any

		Alias,	// Alias

		Ref,		 // Reference
		GenericArg,	 // Generic argument
		RootValue,	 // Root value
		TypeName,	 // Type name
		Context,	 // Context

		LocalVarRef,  // Local variable reference
		RegRef,		  // Register reference
		ArgRef,		  // Argument reference
	};

	template <typename T>
	constexpr inline TypeId getValueType() {
		if constexpr (std::is_same<T, std::int8_t>::value)
			return TypeId::I8;
		else if constexpr (std::is_same<T, std::int16_t>::value)
			return TypeId::I16;
		else if constexpr (std::is_same<T, std::int32_t>::value)
			return TypeId::I32;
		else if constexpr (std::is_same<T, std::int64_t>::value)
			return TypeId::I64;
		else if constexpr (std::is_same<T, uint8_t>::value)
			return TypeId::U8;
		else if constexpr (std::is_same<T, uint16_t>::value)
			return TypeId::U16;
		else if constexpr (std::is_same<T, uint32_t>::value)
			return TypeId::U32;
		else if constexpr (std::is_same<T, uint64_t>::value)
			return TypeId::U64;
		else if constexpr (std::is_same<T, float>::value)
			return TypeId::F32;
		else if constexpr (std::is_same<T, double>::value)
			return TypeId::F64;
		else if constexpr (std::is_same<T, bool>::value)
			return TypeId::Bool;
		else if constexpr (std::is_same<T, std::string>::value)
			return TypeId::String;
		else
			// We didn't use false as the condition directly because some
			// compilers will evaluate the condition prematurely.
			static_assert(!std::is_same<T, T>::value);
	}

	class Runtime;
	class Value;
	class RefValue;
	class MemberValue;

	using TypeFlags = uint8_t;
	constexpr static TypeFlags
		// Determines if the type is with a const access, it is valid to object types only.
		TYPE_CONST = 0x01;

	struct Type final {
		TypeId typeId;	// Type ID
		mutable std::variant<std::monostate, Value *, Type *, std::pair<Type *, Type *>, uint8_t> exData;
		TypeFlags flags = 0;

		inline Type() noexcept : typeId(TypeId::None) {}
		inline Type(const Type &x) noexcept { *this = x; }
		inline Type(Type &&x) noexcept { *this = x; }
		inline Type(TypeId type, TypeFlags flags = 0) noexcept : typeId(type), flags(flags) {}
		inline Type(TypeId type, Value *classObject, TypeFlags flags = 0) noexcept : typeId(type), flags(flags) {
			exData = classObject;
		}
		inline Type(TypeId type, Type elementType, TypeFlags flags = 0) : typeId(type), flags(flags) {
			exData = elementType;
		}
		inline Type(uint8_t idxGenericArg, TypeFlags flags = 0) : typeId(TypeId::GenericArg), flags(flags) {
			exData = idxGenericArg;
		}
		Type(RefValue *ref, TypeFlags flags = 0);

		inline Type(Type k, Type v, TypeFlags flags = 0) : typeId(TypeId::Map), flags(flags) {
			exData = std::pair<Type *, Type *>(new Type(k), new Type(v));
		}

		~Type();

		inline Value *getCustomTypeExData() const { return std::get<Value *>(exData); }
		inline Type &getArrayExData() const { return *std::get<Type *>(exData); }
		inline std::pair<Type *, Type *> getMapExData() const { return std::get<std::pair<Type *, Type *>>(exData); }
		inline uint8_t getGenericArgExData() const { return std::get<uint8_t>(exData); }

		bool isLoadingDeferred() const noexcept;
		void loadDeferredType(const Runtime *rt) const;

		inline operator bool() const noexcept {
			return typeId != TypeId::None;
		}

		inline bool operator<(const Type &rhs) const {
			if (typeId < rhs.typeId)
				return true;

			if (typeId > rhs.typeId)
				return false;

			switch (rhs.typeId) {
				case TypeId::Class:
				case TypeId::Interface:
				case TypeId::Trait:
				case TypeId::Object: {
					auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();
					assert(lhsType->getType() != TypeId::Ref &&
						   rhsType->getType() != TypeId::Ref);

					return lhsType < rhsType;
				}
				case TypeId::Array:
					return getArrayExData() < rhs.getArrayExData();
				case TypeId::Map:
					return *(getMapExData().first) < *(rhs.getMapExData().first) &&
						   *(getMapExData().second) < *(rhs.getMapExData().second);
			}
			return false;
		}
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

		inline bool operator==(const Type &rhs) const noexcept {
			if (rhs.typeId != typeId)
				return false;

			switch (rhs.typeId) {
				case TypeId::Class:
				case TypeId::Interface:
				case TypeId::Trait:
				case TypeId::Object: {
					auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();
					assert(lhsType->getType() != TypeId::Ref &&
						   rhsType->getType() != TypeId::Ref);

					return lhsType == rhsType;
				}
				case TypeId::Array:
					return getArrayExData() == rhs.getArrayExData();
				case TypeId::Map:
					return *(getMapExData().first) == *(rhs.getMapExData().first) &&
						   *(getMapExData().second) == *(rhs.getMapExData().second);
			}
			return true;
		}

		inline bool operator!=(Type &&rhs) noexcept { return !(*this == rhs); }
		inline bool operator!=(const Type &rhs) noexcept { return !(*this == rhs); }

		inline bool operator==(TypeId rhs) noexcept {
			return this->typeId == rhs;
		}
		inline bool operator!=(TypeId rhs) noexcept {
			return this->typeId != rhs;
		}

		inline Type &operator=(Type &&rhs) noexcept {
			typeId = rhs.typeId;
			exData = std::move(rhs.exData);
			return *this;
		}

		inline Type &operator=(const Type &rhs) noexcept {
			typeId = rhs.typeId;
			exData = rhs.exData;
			return *this;
		}

		inline Value *resolveCustomType() {
			if (typeId == TypeId::Class)
				return (Value *)getCustomTypeExData();
			return nullptr;
		}
	};

	class ClassValue;
	class InterfaceValue;
	class TraitValue;

	bool hasImplemented(ClassValue *c, InterfaceValue *i);
	bool hasTrait(ClassValue *c, TraitValue *t);
	bool isConvertible(Type a, Type b);
	bool isCompatible(Type a, Type b);

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
