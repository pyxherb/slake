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

	class Runtime;
	class Object;
	class IdRefObject;
	class MemberObject;

	union BasicTypeExData {
		ValueType valueType;
		Object *ptr;
	};

	struct ArrayExData {
		TypeId typeId;
		BasicTypeExData exData;
		uint32_t nDimensions;
	};

	struct RefExData {
		TypeId typeId;
		union {
			BasicTypeExData basicExData;
			ArrayExData arrayExData;
		} exData;
	};

	struct Type final {
		TypeId typeId;	// Type ID

		mutable union {
			BasicTypeExData basicExData;
			ArrayExData arrayExData;
			RefExData refExData;
		} exData;

		inline Type() noexcept : typeId(TypeId::None) {}
		inline Type(const Type &x) noexcept { *this = x; }
		inline Type(ValueType valueType) noexcept : typeId(TypeId::Value) { exData.basicExData.valueType = valueType; }
		inline Type(TypeId type) noexcept : typeId(type) {}
		inline Type(TypeId type, Object *destObject) noexcept : typeId(type) {
			exData.basicExData.ptr = destObject;
		}
		inline Type(TypeId type, const BasicTypeExData &exData) noexcept : typeId(type) {
			this->exData.basicExData = exData;
		}
		inline Type(TypeId type, const ArrayExData &exData) noexcept : typeId(type) {
			this->exData.arrayExData = exData;
		}
		Type(IdRefObject *ref);

		static inline Type makeArrayTypeName(const Type &elementType, uint32_t nDimensions) {
			assert(elementType.typeId != TypeId::Array);
			assert(elementType.typeId != TypeId::Ref);

			Type type;

			type.typeId = TypeId::Array;
			type.exData.arrayExData.typeId = elementType.typeId;
			type.exData.arrayExData.exData = elementType.exData.basicExData;
			type.exData.arrayExData.nDimensions = nDimensions;

			return type;
		}

		static inline Type makeRefTypeName(const Type &elementType) {
			assert(elementType.typeId != TypeId::Ref);

			Type type;

			type.typeId = TypeId::Ref;
			type.exData.refExData.typeId = elementType.typeId;
			if (elementType.typeId == TypeId::Array) {
				type.exData.refExData.exData.arrayExData = elementType.exData.arrayExData;
			} else
				type.exData.refExData.exData.basicExData = elementType.exData.basicExData;

			return type;
		}

		inline ValueType getValueTypeExData() const { return exData.basicExData.valueType; }
		inline Object *getCustomTypeExData() const { return exData.basicExData.ptr; }
		inline Type getArrayExData() const { return Type(exData.arrayExData.typeId, exData.arrayExData.exData); }
		inline Type getRefExData() const {
			if (exData.refExData.typeId == TypeId::Array)
				return Type(exData.refExData.typeId, exData.refExData.exData.arrayExData);
			return Type(exData.refExData.typeId, exData.refExData.exData.basicExData);
		}

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

		inline Type &operator=(const Type &rhs) noexcept = default;
		inline Type &operator=(Type &&rhs) noexcept = default;

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
