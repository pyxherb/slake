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
	struct Type;
	class CoroutineObject;
	class Object;
	struct Context;

	// Value type definitions are defined in <slake/type.h>.

	enum class ObjectRefKind : uint8_t {
		FieldRef,
		ArrayElementRef,
		ObjectRef,
		InstanceFieldRef,
		LocalVarRef,
		CoroutineLocalVarRef,
		Alloca,
		ArgRef,
		ArgPackRef,
		CoroutineArgRef,
		AotPtrRef,
	};

	struct EntityRef {
		union {
			struct {
				ModuleObject *moduleObject;
				size_t index;
			} asField;
			struct {
				ArrayObject *arrayObject;
				size_t index;
			} asArray;
			struct {
				Object *instanceObject;
			} asObject;
			struct {
				InstanceObject *instanceObject;
				size_t fieldIndex;
			} asObjectField;
			struct {
				Context *context;
				size_t stackOff;
			} asLocalVar;
			struct {
				CoroutineObject *coroutine;
				size_t stackOff;
			} asCoroutineLocalVar;
			struct {
				Context *context;
				size_t stackOff;
			} asAlloca;
			struct {
				MajorFrame *majorFrame;
				size_t argIndex;
			} asArg;
			struct {
				MajorFrame *majorFrame;
				size_t begin;
				size_t end;
			} asArgPack;
			struct {
				CoroutineObject *coroutine;
				uint32_t argIndex;
			} asCoroutineArg;
			struct {
				void *ptr;
			} asAotPtr;
		};
		ObjectRefKind kind;

		static SLAKE_FORCEINLINE EntityRef makeFieldRef(ModuleObject *moduleObject, size_t index) {
			EntityRef ref = {};

			ref.asField.moduleObject = moduleObject;
			ref.asField.index = index;
			ref.kind = ObjectRefKind::FieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeArrayElementRef(ArrayObject *arrayObject, size_t index) {
			EntityRef ref = {};

			ref.asArray.arrayObject = arrayObject;
			ref.asArray.index = index;
			ref.kind = ObjectRefKind::ArrayElementRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeObjectRef(Object *instanceObject) {
			EntityRef ref = {};

			ref.asObject.instanceObject = instanceObject;
			ref.kind = ObjectRefKind::ObjectRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeInstanceFieldRef(InstanceObject *instanceObject, size_t fieldIndex) {
			EntityRef ref = {};

			ref.asObjectField.instanceObject = instanceObject;
			ref.asObjectField.fieldIndex = fieldIndex;
			ref.kind = ObjectRefKind::InstanceFieldRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeLocalVarRef(Context *context, size_t offset) {
			EntityRef ref = {};

			ref.asLocalVar.context = context;
			ref.asLocalVar.stackOff = offset;
			ref.kind = ObjectRefKind::LocalVarRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeCoroutineLocalVarRef(CoroutineObject *coroutine, size_t offset) {
			EntityRef ref = {};

			ref.asCoroutineLocalVar.coroutine = coroutine;
			ref.asCoroutineLocalVar.stackOff = offset;
			ref.kind = ObjectRefKind::CoroutineLocalVarRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeArgRef(MajorFrame *majorFrame, size_t argIndex) {
			EntityRef ref = {};

			ref.asArg.majorFrame = majorFrame;
			ref.asArg.argIndex = argIndex;
			ref.kind = ObjectRefKind::ArgRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeArgPackRef(MajorFrame *majorFrame, size_t begin, size_t end) {
			EntityRef ref = {};

			ref.asArgPack.majorFrame = majorFrame;
			ref.asArgPack.begin = begin;
			ref.asArgPack.end = end;
			ref.kind = ObjectRefKind::ArgPackRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeCoroutineArgRef(CoroutineObject *coroutine, size_t argIndex) {
			EntityRef ref = {};

			ref.asCoroutineArg.coroutine = coroutine;
			ref.asCoroutineArg.argIndex = argIndex;
			ref.kind = ObjectRefKind::CoroutineArgRef;

			return ref;
		}

		static SLAKE_FORCEINLINE EntityRef makeAotPtrRef(void *ptr) {
			EntityRef ref = {};

			ref.asAotPtr.ptr = ptr;
			ref.kind = ObjectRefKind::ArgRef;

			return ref;
		}

		explicit SLAKE_FORCEINLINE operator bool() const {
			if (kind != ObjectRefKind::ObjectRef)
				return true;
			return asObject.instanceObject;
		}

		SLAKE_API bool operator==(const EntityRef &rhs) const;
		SLAKE_API bool operator<(const EntityRef &rhs) const;
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
		Type asType;
		EntityRef asObjectRef;

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
		SLAKE_FORCEINLINE Value(const EntityRef &entityRef) noexcept : valueType(ValueType::EntityRef) {
			this->data.asObjectRef = entityRef;
		}
		SLAKE_FORCEINLINE Value(ValueType vt) noexcept : valueType(vt) {
		}
		SLAKE_FORCEINLINE constexpr Value(ValueType vt, uint32_t index) noexcept : valueType(vt), data(index) {
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

		SLAKE_FORCEINLINE Type &getTypeName() noexcept {
			assert(valueType == ValueType::TypeName);
			return data.asType;
		}
		SLAKE_FORCEINLINE const Type &getTypeName() const noexcept {
			assert(valueType == ValueType::TypeName);
			return data.asType;
		}

		SLAKE_FORCEINLINE EntityRef &getEntityRef() noexcept {
			assert(valueType == ValueType::EntityRef);
			return data.asObjectRef;
		}
		SLAKE_FORCEINLINE const EntityRef &getEntityRef() const noexcept {
			assert(valueType == ValueType::EntityRef);
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

	SLAKE_API InternalExceptionPointer isCompatible(peff::Alloc *allocator, const Type &type, const Value &value, bool &resultOut);
}

#endif
