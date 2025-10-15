#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>
#include <cassert>
#include "type.h"
#include <slake/generated/config.h>

namespace slake {
	class CoroutineObject;
	class Object;
	struct Context;

	// Value type definitions are defined in <slake/type.h>.

	enum class EntityRefKind : uint8_t {
		Invalid = 0,
		StaticFieldRef,
		ArrayElementRef,
		ObjectRef,
		InstanceFieldRef,
		LocalVarRef,
		CoroutineLocalVarRef,
		ArgRef,
		ArgPackRef,
		CoroutineArgRef,
		AotPtrRef,
		StructRef,
		StructFieldRef,
	};

	class StructObject;

	struct StaticFieldRef {
		ModuleObject *moduleObject;
		uint32_t index;
	};

	struct ArrayElementRef {
		ArrayObject *arrayObject;
		size_t index;
	};

	struct ObjectFieldRef {
		InstanceObject *instanceObject;
		uint32_t fieldIndex;
	};

	struct LocalVarRef {
		Context *context;
		size_t stackOff;
	};

	struct CoroutineLocalVarRef {
		CoroutineObject *coroutine;
		size_t stackOff;
	};

	struct LocalVarStructRef {
		void *ptr;
		StructObject *structObject;
	};

	struct ArgRef {
		MajorFrame *majorFrame;
		uint32_t argIndex;
	};

	struct ArgPackRef {
		MajorFrame *majorFrame;
		uint32_t begin;
		uint32_t end;
	};

	struct CoroutineArgRef {
		CoroutineObject *coroutine;
		uint32_t argIndex;
	};

	// Because struct is a value type,
	// destroying the parent container will make the entire struct
	// inaccessible, thus any reference to it should be a temporary reference.
	// Passing temporary reference out of current scope is invalid, so we don't
	// care about the structure's parent scope, just walking the structure
	// itself.
	struct StructRef {
		StructObject *structObject;
		void *basePtr;
	};

	struct StructFieldRef {
		StructRef structRef;
		uint32_t idxField;
	};

	struct Reference {
		union {
			StaticFieldRef asStaticField;
			ArrayElementRef asArrayElement;
			Object *asObject;
			ObjectFieldRef asObjectField;
			LocalVarRef asLocalVar;
			CoroutineLocalVarRef asCoroutineLocalVar;
			ArgRef asArg;
			ArgPackRef asArgPack;
			CoroutineArgRef asCoroutineArg;
			StructRef asStruct;
			StructFieldRef asStructField;
			struct {
				void *ptr;
			} asAotPtr;
		};
		EntityRefKind kind;

		static SLAKE_FORCEINLINE Reference makeInvalidRef() {
			Reference ref = {};

			ref.kind = EntityRefKind::Invalid;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeStaticFieldRef(ModuleObject *moduleObject, size_t index) {
			Reference ref = {};

			ref.asStaticField.moduleObject = moduleObject;
			ref.asStaticField.index = index;
			ref.kind = EntityRefKind::StaticFieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeArrayElementRef(ArrayObject *arrayObject, size_t index) {
			Reference ref = {};

			ref.asArrayElement.arrayObject = arrayObject;
			ref.asArrayElement.index = index;
			ref.kind = EntityRefKind::ArrayElementRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeObjectRef(Object *instanceObject) {
			Reference ref = {};

			ref.asObject = instanceObject;
			ref.kind = EntityRefKind::ObjectRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeInstanceFieldRef(InstanceObject *instanceObject, size_t fieldIndex) {
			Reference ref = {};

			ref.asObjectField.instanceObject = instanceObject;
			ref.asObjectField.fieldIndex = fieldIndex;
			ref.kind = EntityRefKind::InstanceFieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeLocalVarRef(Context *context, size_t offset) {
			Reference ref = {};

			ref.asLocalVar.context = context;
			ref.asLocalVar.stackOff = offset;
			ref.kind = EntityRefKind::LocalVarRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeCoroutineLocalVarRef(CoroutineObject *coroutine, size_t offset) {
			Reference ref = {};

			ref.asCoroutineLocalVar.coroutine = coroutine;
			ref.asCoroutineLocalVar.stackOff = offset;
			ref.kind = EntityRefKind::CoroutineLocalVarRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeArgRef(MajorFrame *majorFrame, size_t argIndex) {
			Reference ref = {};

			ref.asArg.majorFrame = majorFrame;
			ref.asArg.argIndex = argIndex;
			ref.kind = EntityRefKind::ArgRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeArgPackRef(MajorFrame *majorFrame, size_t begin, size_t end) {
			Reference ref = {};

			ref.asArgPack.majorFrame = majorFrame;
			ref.asArgPack.begin = begin;
			ref.asArgPack.end = end;
			ref.kind = EntityRefKind::ArgPackRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeCoroutineArgRef(CoroutineObject *coroutine, size_t argIndex) {
			Reference ref = {};

			ref.asCoroutineArg.coroutine = coroutine;
			ref.asCoroutineArg.argIndex = argIndex;
			ref.kind = EntityRefKind::CoroutineArgRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeStructRef(const StructRef &structRef) {
			Reference ref = {};

			ref.asStruct = structRef;
			ref.kind = EntityRefKind::StructRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeStructFieldRef(const StructRef &structRef, uint32_t fieldIndex) {
			Reference ref = {};

			ref.asStructField.structRef = structRef;
			ref.asStructField.idxField = fieldIndex;
			ref.kind = EntityRefKind::StructFieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE Reference makeAotPtrRef(void *ptr) {
			Reference ref = {};

			ref.asAotPtr.ptr = ptr;
			ref.kind = EntityRefKind::ArgRef;

			return ref;
		}

		explicit SLAKE_FORCEINLINE operator bool() const {
			return kind != EntityRefKind::Invalid;
		}

		SLAKE_API bool operator==(const Reference &rhs) const;
		SLAKE_API bool operator<(const Reference &rhs) const;
	};

#ifndef ssize_t
	using ssize_t = std::make_signed_t<size_t>;
#endif

	using SizeTypeMarker = bool;
	constexpr static SizeTypeMarker SIZETYPE_MARKER = true;

	union ValueData {
		int8_t asI8;
		int16_t asI16;
		int32_t asI32;
		int64_t asI64;
		ssize_t asISize;
		uint8_t asU8;
		uint16_t asU16;
		uint32_t asU32;
		uint64_t asU64;
		size_t asUSize;
		float asF32;
		double asF64;
		bool asBool;
		TypeRef asType;
		Reference asReference;

		ValueData() noexcept = default;
		SLAKE_FORCEINLINE constexpr ValueData(const ValueData &other) noexcept = default;
		SLAKE_FORCEINLINE constexpr ValueData(ValueData &&other) noexcept = default;

		SLAKE_FORCEINLINE constexpr ValueData &operator=(const ValueData &other) noexcept = default;
		SLAKE_FORCEINLINE constexpr ValueData &operator=(ValueData &&other) noexcept = default;

		SLAKE_FORCEINLINE constexpr ValueData(int8_t data) noexcept : asI8(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(int16_t data) noexcept : asI16(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(int32_t data) noexcept : asI32(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(int64_t data) noexcept : asI64(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(SizeTypeMarker marker, ssize_t data) noexcept : asISize(data) {
			SLAKE_REFERENCED_PARAM(marker);
		}

		SLAKE_FORCEINLINE constexpr ValueData(uint8_t data) noexcept : asU8(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(uint16_t data) noexcept : asU16(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(uint32_t data) noexcept : asU32(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(uint64_t data) noexcept : asU64(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(SizeTypeMarker marker, size_t data) noexcept : asUSize(data) {
			SLAKE_REFERENCED_PARAM(marker);
		}

		SLAKE_FORCEINLINE constexpr ValueData(float data) noexcept : asF32(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(double data) noexcept : asF64(data) {
		}

		SLAKE_FORCEINLINE constexpr ValueData(bool data) noexcept : asBool(data) {
		}
	};

	struct Value {
		ValueData data;

		ValueType valueType;

		SLAKE_FORCEINLINE Value() = default;
		SLAKE_FORCEINLINE constexpr Value(const Value &other) = default;
		SLAKE_FORCEINLINE constexpr Value(Value &&other) = default;
		SLAKE_FORCEINLINE constexpr Value(int8_t data) noexcept : valueType(ValueType::I8), data((int8_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(int16_t data) noexcept : valueType(ValueType::I16), data((int16_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(int32_t data) noexcept : valueType(ValueType::I32), data((int32_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(int64_t data) noexcept : valueType(ValueType::I64), data((int64_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, ssize_t data) noexcept : valueType(ValueType::ISize), data(data) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(uint8_t data) noexcept : valueType(ValueType::U8), data((uint8_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint16_t data) noexcept : valueType(ValueType::U16), data((uint16_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint32_t data) noexcept : valueType(ValueType::U32), data((uint32_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint64_t data) noexcept : valueType(ValueType::U64), data((uint64_t)data) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, size_t data) noexcept : valueType(ValueType::USize), data(data) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(float data) noexcept : valueType(ValueType::F32), data(data) {
		}
		SLAKE_FORCEINLINE constexpr Value(double data) noexcept : valueType(ValueType::F64), data(data) {
		}
		SLAKE_FORCEINLINE constexpr Value(bool data) noexcept : valueType(ValueType::Bool), data(data) {
		}
		SLAKE_FORCEINLINE Value(const Reference &entityRef) noexcept : valueType(ValueType::Reference) {
			this->data.asReference = entityRef;
		}
		SLAKE_FORCEINLINE Value(ValueType vt) noexcept : valueType(vt), data(/*Uninitialized*/) {
		}
		SLAKE_FORCEINLINE constexpr Value(ValueType vt, uint32_t index) noexcept : valueType(vt), data(index) {
		}
		SLAKE_FORCEINLINE Value(const TypeRef &type) noexcept : valueType(ValueType::TypeName) {
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

		SLAKE_FORCEINLINE ssize_t getISize() const noexcept {
			assert(valueType == ValueType::ISize);
			return data.asISize;
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

		SLAKE_FORCEINLINE size_t getUSize() const noexcept {
			assert(valueType == ValueType::USize);
			return data.asUSize;
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

		SLAKE_FORCEINLINE uint32_t getLabel() const noexcept {
			assert(valueType == ValueType::Label);
			return data.asU32;
		}

		SLAKE_FORCEINLINE TypeRef &getTypeName() noexcept {
			assert(valueType == ValueType::TypeName);
			return data.asType;
		}
		SLAKE_FORCEINLINE const TypeRef &getTypeName() const noexcept {
			assert(valueType == ValueType::TypeName);
			return data.asType;
		}

		SLAKE_FORCEINLINE Reference &getReference() noexcept {
			assert(valueType == ValueType::Reference);
			return data.asReference;
		}
		SLAKE_FORCEINLINE const Reference &getReference() const noexcept {
			assert(valueType == ValueType::Reference);
			return data.asReference;
		}

		Value &operator=(const Value &other) noexcept = default;
		Value &operator=(Value &&other) noexcept = default;

		SLAKE_API bool operator==(const Value &rhs) const;

		SLAKE_FORCEINLINE bool operator!=(const Value &rhs) const {
			return !(*this == rhs);
		}

		SLAKE_API bool operator<(const Value &rhs) const;
	};

	SLAKE_API InternalExceptionPointer isCompatible(peff::Alloc *allocator, const TypeRef &type, const Value &value, bool &resultOut);
}

#endif
