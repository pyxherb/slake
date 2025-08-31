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
		Invalid = 0,

		Void,  // Void

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

	class Runtime;
	class Object;
	class IdRefObject;
	class StringObject;
	class ModuleObject;
	class ArrayObject;
	class InstanceObject;
	class TypeDefObject;
	struct MajorFrame;

	enum class ObjectKind : uint8_t {
		Invalid = 0,  // Invalid

		String,	 // String

		TypeDef,			   // Type definition
		FnTypeDef,			   // Function type definition
		ParamTypeListTypeDef,  // Parameter type list type definition
		TupleTypeDef,		   // Parameter type list type definition
		SIMDTypeDef,		   // Parameter type list type definition

		Fn,				// Function
		FnOverloading,	// Function overloading
		Module,			// Module
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
		Context,	 // Context
		Resumable,	 // Resumable
		Coroutine,	 // Coroutine
	};

	struct GenericArgTypeExData {
		StringObject *nameObject;
		// For type comparison.
		Object *ownerObject;
	};

	union TypeExData {
		Object *object;
		TypeDefObject *typeDef;
		GenericArgTypeExData genericArg;
	};

	class Duplicator;

	struct Type;

	SLAKE_FORCEINLINE bool verifyType(const Type &type);

	SLAKE_FORCEINLINE bool verifyObjectKind(const Object *object);
	SLAKE_FORCEINLINE bool verifyObjectKind(const Object *object, ObjectKind objectKind);

	struct Type final {
		TypeExData exData;

		TypeId typeId;	// Type ID

		SLAKE_FORCEINLINE Type() = default;
		SLAKE_FORCEINLINE Type(const Type &x) = default;
		SLAKE_FORCEINLINE Type(Type &&x) = default;
		SLAKE_FORCEINLINE Type(TypeId type) noexcept : typeId(type), exData({}) {
			assert(verifyType(*this));
		}
		SLAKE_FORCEINLINE Type(TypeId type, Object *destObject) noexcept : typeId(type) {
			assert(destObject && verifyObjectKind((Object *)destObject));
			exData.object = destObject;
			assert(verifyType(*this));
		}
		SLAKE_FORCEINLINE Type(StringObject *nameObject, Object *ownerObject) noexcept : typeId(TypeId::GenericArg) {
			assert(verifyObjectKind((Object *)nameObject, ObjectKind::String));
			assert(verifyObjectKind(ownerObject));
			exData.genericArg.nameObject = nameObject;
			exData.genericArg.ownerObject = ownerObject;
		}

		SLAKE_API Type duplicate(bool &succeededOut) const;

		SLAKE_FORCEINLINE Object *getCustomTypeExData() const {
			assert((typeId == TypeId::Instance) ||
				   (typeId == TypeId::Array) ||
				   (typeId == TypeId::Ref) ||
				   (typeId == TypeId::SIMD));
			assert(exData.object && verifyObjectKind(exData.object));
			return exData.object;
		}

		SLAKE_FORCEINLINE StringObject *getGenericArgNameObject() const {
			assert(typeId == TypeId::GenericArg);
			assert(exData.genericArg.nameObject && verifyObjectKind((Object *)exData.genericArg.nameObject, ObjectKind::String));
			assert(exData.genericArg.ownerObject && verifyObjectKind(exData.genericArg.ownerObject));

			return exData.genericArg.nameObject;
		}
		SLAKE_FORCEINLINE Object *getGenericArgOwnerObject() const {
			assert(typeId == TypeId::GenericArg);
			assert(exData.genericArg.nameObject && verifyObjectKind((Object *)exData.genericArg.nameObject, ObjectKind::String));
			assert(exData.genericArg.ownerObject && verifyObjectKind(exData.genericArg.ownerObject));
			return exData.genericArg.ownerObject;
		}

		SLAKE_API Type &getArrayExData() const;
		SLAKE_API Type &getRefExData() const;
		SLAKE_API Type &getUnpackingExData() const;

		SLAKE_API bool isLoadingDeferred() const noexcept;
		[[nodiscard]] SLAKE_API InternalExceptionPointer loadDeferredType(Runtime *rt);

		SLAKE_FORCEINLINE explicit operator bool() const noexcept {
			return typeId != TypeId::Void;
		}

		SLAKE_FORCEINLINE bool operator==(TypeId typeId) noexcept {
			return this->typeId == typeId;
		}

		SLAKE_FORCEINLINE bool operator!=(TypeId typeId) noexcept {
			return this->typeId != typeId;
		}

		Type &operator=(const Type &rhs) noexcept = default;
		Type &operator=(Type &&rhs) noexcept = default;

		SLAKE_FORCEINLINE Object *resolveCustomType() const {
			if (typeId == TypeId::Instance)
				return (Object *)getCustomTypeExData();
			return nullptr;
		}
	};

	SLAKE_FORCEINLINE bool verifyType(const Type &type) {
		if ((type.typeId < TypeId::Void) || (type.typeId > TypeId::Unknown))
			return false;

		switch (type.typeId) {
			case TypeId::Invalid:
				return false;
			case TypeId::Instance:
			case TypeId::Array:
			case TypeId::Ref:
				type.getCustomTypeExData();
				break;
			case TypeId::GenericArg:
				type.getGenericArgNameObject();
				type.getGenericArgOwnerObject();
				break;
			case TypeId::Tuple:
				/* TODO: Implement it. */
				break;
			case TypeId::SIMD:
				/* TODO: Implement it. */
				break;
			case TypeId::Fn:
				/* TODO: Implement it. */
				break;
			case TypeId::ParamTypeList:
				/* TODO: Implement it. */
				break;
			case TypeId::Unpacking:
				/* TODO: Implement it. */
				break;
			default:
				break;
		}

		return true;
	}

	class ClassObject;
	class InterfaceObject;

	class Runtime;
}

namespace std {
	SLAKE_API string to_string(const slake::Type &type, const slake::Runtime *rt);
}

#endif
