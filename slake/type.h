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
	class MemberObject;

	union TypeExData {
		ValueType valueType;
		Object *ptr;
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

		inline bool operator<(const Type &rhs) const {
			if (typeId < rhs.typeId)
				return true;
			else if (typeId > rhs.typeId)
				return false;
			else {
				switch (rhs.typeId) {
					case TypeId::Instance: {
						auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();
						assert(lhsType->getKind() != ObjectKind::IdRef &&
							   rhsType->getKind() != ObjectKind::IdRef);

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
				case TypeId::Instance: {
					auto lhsType = getCustomTypeExData(), rhsType = rhs.getCustomTypeExData();
					assert(lhsType->getKind() != ObjectKind::IdRef &&
						   rhsType->getKind() != ObjectKind::IdRef);

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
