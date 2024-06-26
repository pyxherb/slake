#ifndef _SLAKE_TYPE_H_
#define _SLAKE_TYPE_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <variant>
#include "valdef/object.h"
#include "valdef/value.h"

namespace slake {
	enum class TypeId : uint8_t {
		None,  // None, aka `null'

		Value,	 // Value type
		String,	 // String

		Fn,				// Function
		FnOverloading,	// Function overloading
		Module,			// Module
		Var,			// Variable
		Array,			// Array
		Ref,			// Reference

		Class,		// Class
		Interface,	// Interface
		Struct,		// Structure
		Instance,	// Object instance

		Any,  // Any

		Alias,	// Alias

		IdRef,		 // Reference
		GenericArg,	 // Generic argument
		RootObject,	 // Root value
		Context,	 // Context
	};

	template <typename T>
	constexpr inline TypeId getObjectType() {
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
	class Object;
	class IdRefObject;
	class MemberObject;

	struct Type final {
		TypeId typeId;	// Type ID
		mutable std::variant<
			std::monostate,	 // Simple type
			ValueType,		 // Value type
			Object *,		 // Resolved type
			Type *,			 // Array/Reference
			std::string		 // Generic parameter
			>
			exData;

		inline Type() noexcept : typeId(TypeId::None) {}
		inline Type(const Type &x) noexcept { *this = x; }
		inline Type(ValueType valueType) noexcept : typeId(TypeId::Value) { exData = valueType; }
		inline Type(Type &&x) noexcept { *this = std::move(x); }
		inline Type(TypeId typeId, const Type &elementType) : typeId(typeId) {
			exData = new Type(elementType);
		}
		inline Type(TypeId type) noexcept : typeId(type) {}
		inline Type(TypeId type, Object *destObject) noexcept : typeId(type) {
			exData = destObject;
		}
		inline Type(std::string genericParamName) : typeId(TypeId::GenericArg) {
			exData = genericParamName;
		}
		Type(IdRefObject *ref);

		~Type();

		inline ValueType getValueTypeExData() const { return std::get<ValueType>(exData); }
		inline Object *getCustomTypeExData() const { return std::get<Object *>(exData); }
		inline Type &getArrayExData() const { return *std::get<Type *>(exData); }
		inline Type &getRefExData() const { return *std::get<Type *>(exData); }
		inline Type &getVarExData() const { return *std::get<Type *>(exData); }
		inline std::string getGenericArgExData() const { return std::get<std::string>(exData); }

		bool isLoadingDeferred() const noexcept;
		void loadDeferredType(const Runtime *rt) const;

		inline operator bool() const noexcept {
			return typeId != TypeId::None;
		}

		inline bool operator<(const Type &rhs) const {
			if (typeId < rhs.typeId)
				return true;
			else if (typeId > rhs.typeId)
				return false;
			else {
				switch (rhs.typeId) {
					case TypeId::Class:
					case TypeId::Interface:
					case TypeId::Instance: {
						auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();
						assert(lhsType->getType() != TypeId::IdRef &&
							   rhsType->getType() != TypeId::IdRef);

						return lhsType < rhsType;
					}
					case TypeId::Array:
						return getArrayExData() < rhs.getArrayExData();
					case TypeId::Ref:
						return getRefExData() < rhs.getRefExData();
					case TypeId::Var:
						return getVarExData() < rhs.getVarExData();
				}
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
				case TypeId::Value:
					return getValueTypeExData() == rhs.getValueTypeExData();
				case TypeId::Class:
				case TypeId::Interface:
				case TypeId::Instance: {
					auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();
					assert(lhsType->getType() != TypeId::IdRef &&
						   rhsType->getType() != TypeId::IdRef);

					return lhsType == rhsType;
				}
				case TypeId::Array:
					return getArrayExData() == rhs.getArrayExData();
				case TypeId::Ref:
					return getRefExData() == rhs.getRefExData();
				case TypeId::Var:
					return getVarExData() == rhs.getVarExData();
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

		inline Type &operator=(const Type &rhs) noexcept {
			typeId = rhs.typeId;

			switch (rhs.typeId) {
				case TypeId::Class:
				case TypeId::Interface:
				case TypeId::Instance:
					if (rhs.isLoadingDeferred())
						// Duplicate the reference to the type.
						exData = rhs.getCustomTypeExData()->duplicate();
					else
						exData = rhs.getCustomTypeExData();
					break;
				case TypeId::Array:
					exData = new Type(rhs.getArrayExData());
					break;
				case TypeId::Ref:
					exData = new Type(rhs.getRefExData());
					break;
				case TypeId::Var:
					exData = new Type(rhs.getVarExData());
					break;
				default:
					exData = rhs.exData;
			}

			return *this;
		}

		inline Type &operator=(Type &&rhs) noexcept {
			typeId = rhs.typeId;

			switch (rhs.typeId) {
				case TypeId::Class:
				case TypeId::Interface:
				case TypeId::Instance:
					exData = rhs.getCustomTypeExData();
					break;
				case TypeId::Array:
					exData = std::get<Type *>(rhs.exData);
					break;
				case TypeId::Ref:
					exData = std::get<Type *>(rhs.exData);
					break;
				case TypeId::Var:
					exData = std::get<Type *>(rhs.exData);
					break;
				default:
					exData = rhs.exData;
			}

			rhs.exData = {};
			rhs.typeId = TypeId::None;

			return *this;
		}

		inline Object *resolveCustomType() {
			if (typeId == TypeId::Class)
				return (Object *)getCustomTypeExData();
			return nullptr;
		}
	};

	class ClassObject;
	class InterfaceObject;

	bool hasImplemented(ClassObject *c, InterfaceObject *i);
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
