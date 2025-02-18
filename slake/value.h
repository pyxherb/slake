#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>
#include <cassert>
#include "type.h"

namespace slake {
	struct Type;
	class Object;

	// Value type definitions are defined in <slake/type.h>.

	enum class ObjectRefKind : uint8_t {
		FieldRef,
		ArrayElementRef,
		InstanceRef,
		InstanceFieldRef,
		LocalVarRef,
		ArgRef,
		AotPtrRef,
	};

	struct ObjectRef {
		union {
			struct {
				ModuleObject *moduleObject;
				uint32_t index;
			} asField;
			struct {
				ArrayObject *arrayObject;
				uint32_t index;
			} asArray;
			struct {
				Object *instanceObject;
			} asInstance;
			struct {
				InstanceObject *instanceObject;
				size_t fieldIndex;
			} asInstanceField;
			struct {
				MajorFrame *majorFrame;
				uint32_t localVarIndex;
			} asLocalVar;
			struct {
				MajorFrame *majorFrame;
				uint32_t argIndex;
			} asArg;
			struct {
				void *ptr;
			} asAotPtr;
		};
		ObjectRefKind kind;

		static SLAKE_FORCEINLINE ObjectRef makeFieldRef(ModuleObject *moduleObject, uint32_t index) {
			ObjectRef ref = {};

			ref.asField.moduleObject = moduleObject;
			ref.asField.index = index;
			ref.kind = ObjectRefKind::FieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE ObjectRef makeArrayElementRef(ArrayObject *arrayObject, uint32_t index) {
			ObjectRef ref = {};

			ref.asArray.arrayObject = arrayObject;
			ref.asArray.index = index;
			ref.kind = ObjectRefKind::ArrayElementRef;

			return ref;
		}

		static SLAKE_FORCEINLINE ObjectRef makeInstanceRef(Object *instanceObject) {
			ObjectRef ref = {};

			ref.asInstance.instanceObject = instanceObject;
			ref.kind = ObjectRefKind::InstanceRef;

			return ref;
		}

		static SLAKE_FORCEINLINE ObjectRef makeInstanceFieldRef(InstanceObject *instanceObject, size_t fieldIndex) {
			ObjectRef ref = {};

			ref.asInstanceField.instanceObject = instanceObject;
			ref.asInstanceField.fieldIndex = fieldIndex;
			ref.kind = ObjectRefKind::InstanceFieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE ObjectRef makeLocalVarRef(MajorFrame *majorFrame, uint32_t localVarIndex) {
			ObjectRef ref = {};

			ref.asLocalVar.majorFrame = majorFrame;
			ref.asLocalVar.localVarIndex = localVarIndex;
			ref.kind = ObjectRefKind::LocalVarRef;

			return ref;
		}

		static SLAKE_FORCEINLINE ObjectRef makeArgRef(MajorFrame *majorFrame, uint32_t argIndex) {
			ObjectRef ref = {};

			ref.asArg.majorFrame = majorFrame;
			ref.asArg.argIndex = argIndex;
			ref.kind = ObjectRefKind::ArgRef;

			return ref;
		}

		static SLAKE_FORCEINLINE ObjectRef makeAotPtrRef(void *ptr) {
			ObjectRef ref = {};

			ref.asAotPtr.ptr = ptr;
			ref.kind = ObjectRefKind::ArgRef;

			return ref;
		}

		SLAKE_FORCEINLINE operator bool() const {
			if (kind != ObjectRefKind::InstanceRef)
				return true;
			return asInstance.instanceObject;
		}

		SLAKE_API bool operator==(const ObjectRef &rhs) const;
		SLAKE_API bool operator<(const ObjectRef &rhs) const;
	};

	struct Value {
		union {
			int8_t asI8;
			int16_t asI16;
			int32_t asI32;
			int64_t asI64;
			uint8_t asU8;
			uint16_t asU16;
			uint32_t asU32;
			uint64_t asU64;
			float asF32;
			double asF64;
			bool asBool;
			Type asType;
			ObjectRef asObjectRef;
		} data;

		ValueType valueType;

		Value() = default;
		Value(const Value &other) = default;
		Value(Value &&other) = default;
		SLAKE_FORCEINLINE Value(int8_t data) noexcept : valueType(ValueType::I8) {
			this->data.asI8 = data;
		}
		SLAKE_FORCEINLINE Value(int16_t data) noexcept : valueType(ValueType::I16) {
			this->data.asI16 = data;
		}
		SLAKE_FORCEINLINE Value(int32_t data) noexcept : valueType(ValueType::I32) {
			this->data.asI32 = data;
		}
		SLAKE_FORCEINLINE Value(int64_t data) noexcept : valueType(ValueType::I64) {
			this->data.asI64 = data;
		}
		SLAKE_FORCEINLINE Value(uint8_t data) noexcept : valueType(ValueType::U8) {
			this->data.asU8 = data;
		}
		SLAKE_FORCEINLINE Value(uint16_t data) noexcept : valueType(ValueType::U16) {
			this->data.asU16 = data;
		}
		SLAKE_FORCEINLINE Value(uint32_t data) noexcept : valueType(ValueType::U32) {
			this->data.asU32 = data;
		}
		SLAKE_FORCEINLINE Value(uint64_t data) noexcept : valueType(ValueType::U64) {
			this->data.asU64 = data;
		}
		SLAKE_FORCEINLINE Value(float data) noexcept : valueType(ValueType::F32) {
			this->data.asF32 = data;
		}
		SLAKE_FORCEINLINE Value(double data) noexcept : valueType(ValueType::F64) {
			this->data.asF64 = data;
		}
		SLAKE_FORCEINLINE Value(bool data) noexcept : valueType(ValueType::Bool) {
			this->data.asBool = data;
		}
		SLAKE_FORCEINLINE Value(const ObjectRef &objectRef) noexcept : valueType(ValueType::ObjectRef) {
			this->data.asObjectRef = objectRef;
		}
		SLAKE_FORCEINLINE Value(ValueType vt) noexcept : valueType(vt) {
		}
		SLAKE_FORCEINLINE Value(ValueType vt, uint32_t index) noexcept : valueType(vt) {
			this->data.asU32 = index;
		}
		SLAKE_FORCEINLINE Value(const Type &type) noexcept : valueType(ValueType::TypeName) {
			data.asType = type;
		}

		SLAKE_FORCEINLINE int8_t getI8() const noexcept {
			assert(valueType == ValueType::I8);
			return data.asI8;
		}

		SLAKE_FORCEINLINE int16_t getI16() const noexcept {
			assert(valueType == ValueType::I16);
			return data.asI16;
		}

		SLAKE_FORCEINLINE int32_t getI32() const noexcept {
			assert(valueType == ValueType::I32);
			return data.asI32;
		}

		SLAKE_FORCEINLINE int64_t getI64() const noexcept {
			assert(valueType == ValueType::I64);
			return data.asI64;
		}

		SLAKE_FORCEINLINE uint8_t getU8() const noexcept {
			assert(valueType == ValueType::U8);
			return data.asU8;
		}

		SLAKE_FORCEINLINE uint16_t getU16() const noexcept {
			assert(valueType == ValueType::U16);
			return data.asU16;
		}

		SLAKE_FORCEINLINE uint32_t getU32() const noexcept {
			assert(valueType == ValueType::U32);
			return data.asU32;
		}

		SLAKE_FORCEINLINE uint64_t getU64() const noexcept {
			assert(valueType == ValueType::U64);
			return data.asU64;
		}

		SLAKE_FORCEINLINE float getF32() const noexcept {
			assert(valueType == ValueType::F32);
			return data.asF32;
		}

		SLAKE_FORCEINLINE double getF64() const noexcept {
			assert(valueType == ValueType::F64);
			return data.asF64;
		}

		SLAKE_FORCEINLINE bool getBool() const noexcept {
			assert(valueType == ValueType::Bool);
			return data.asBool;
		}

		SLAKE_FORCEINLINE uint32_t getRegIndex() const noexcept {
			assert(valueType == ValueType::RegRef);
			return data.asU32;
		}

		SLAKE_FORCEINLINE Type &getTypeName() noexcept {
			assert(valueType == ValueType::TypeName);
			return data.asType;
		}
		SLAKE_FORCEINLINE const Type &getTypeName() const noexcept {
			assert(valueType == ValueType::TypeName);
			return data.asType;
		}

		SLAKE_FORCEINLINE ObjectRef &getObjectRef() noexcept {
			assert(valueType == ValueType::ObjectRef);
			return data.asObjectRef;
		}
		SLAKE_FORCEINLINE const ObjectRef &getObjectRef() const noexcept {
			assert(valueType == ValueType::ObjectRef);
			return data.asObjectRef;
		}

		Value &operator=(const Value &other) noexcept = default;
		Value &operator=(Value &&other) noexcept = default;

		SLAKE_API bool operator==(const Value &rhs) const;

		SLAKE_FORCEINLINE bool operator!=(const Value &rhs) const {
			return !(*this == rhs);
		}

		SLAKE_API bool operator<(const Value &rhs) const;
	};

	SLAKE_API bool isCompatible(const Type &type, const Value &value);
}

#endif
