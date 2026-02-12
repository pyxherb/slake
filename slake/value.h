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

	enum class ReferenceKind : uint8_t {
		Invalid = 0,

		StaticFieldRef,
		LocalVarRef,
		CoroutineLocalVarRef,
		ObjectFieldRef,
		ArrayElementRef,
		ArgRef,
		CoroutineArgRef,
		StructFieldRef,

		ObjectRef,
		ArgPackRef,
		AotPtrRef,
		StructRef,
	};

	class StructObject;

	struct StaticFieldRef {
		BasicModuleObject *moduleObject;
		uint32_t index;

		StaticFieldRef() = default;
		SLAKE_FORCEINLINE StaticFieldRef(BasicModuleObject *moduleObject, uint32_t index) : moduleObject(moduleObject), index(index) {}
	};

	struct ArrayElementRef {
		ArrayObject *arrayObject;
		size_t index;

		ArrayElementRef() = default;
		SLAKE_FORCEINLINE ArrayElementRef(ArrayObject *arrayObject, uint32_t index) : arrayObject(arrayObject), index(index) {}
	};

	struct ObjectFieldRef {
		InstanceObject *instanceObject;
		uint32_t fieldIndex;

		ObjectFieldRef() = default;
		SLAKE_FORCEINLINE ObjectFieldRef(InstanceObject *instanceObject, uint32_t fieldIndex) : instanceObject(instanceObject), fieldIndex(fieldIndex) {}
	};

	struct LocalVarRef {
		Context *context;
		size_t stackOff;

		LocalVarRef() = default;
		SLAKE_FORCEINLINE LocalVarRef(Context *context, size_t stackOff) : context(context), stackOff(stackOff) {}
	};

	struct CoroutineLocalVarRef {
		CoroutineObject *coroutine;
		size_t stackOff;

		CoroutineLocalVarRef() = default;
		SLAKE_FORCEINLINE CoroutineLocalVarRef(CoroutineObject *coroutine, size_t stackOff) : coroutine(coroutine), stackOff(stackOff) {}
	};

	struct LocalVarStructRef {
		void *ptr;
		StructObject *structObject;

		LocalVarStructRef() = default;
		SLAKE_FORCEINLINE LocalVarStructRef(void *ptr, StructObject *structObject) : ptr(ptr), structObject(structObject) {}
	};

	struct ArgRef {
		const MajorFrame *majorFrame;
		char *dataStack;
		size_t stackSize;
		uint32_t argIndex;

		ArgRef() = default;
		SLAKE_FORCEINLINE ArgRef(const MajorFrame *majorFrame, char *dataStack, size_t stackSize, uint32_t argIndex) : majorFrame(majorFrame), dataStack(dataStack), stackSize(stackSize), argIndex(argIndex) {}
	};

	struct ArgPackRef {
		MajorFrame *majorFrame;
		uint32_t begin;
		uint32_t end;

		ArgPackRef() = default;
	};

	struct CoroutineArgRef {
		CoroutineObject *coroutine;
		uint32_t argIndex;

		CoroutineArgRef() = default;
		SLAKE_FORCEINLINE CoroutineArgRef(CoroutineObject *coroutine, uint32_t argIndex) : coroutine(coroutine), argIndex(argIndex) {}
	};

	// Because struct is a value type,
	// destroying the parent container will make the entire struct
	// inaccessible, thus any reference to it should be a temporary reference.
	// Passing temporary reference out of current scope is invalid, so we don't
	// care about the structure's parent scope, just walking the structure
	// itself.
	struct StructRefData {
		union {
			StaticFieldRef asStaticField;
			ArrayElementRef asArrayElement;
			ObjectFieldRef asObjectField;
			LocalVarRef asLocalVar;
			CoroutineLocalVarRef asCoroutineLocalVar;
			ArgRef asArg;
			CoroutineArgRef asCoroutineArg;
		};

		StructRefData() = default;
		SLAKE_FORCEINLINE StructRefData(const StaticFieldRef &innerReference) : asStaticField(innerReference) {}
		SLAKE_FORCEINLINE StructRefData(const ArrayElementRef &innerReference) : asArrayElement(innerReference) {}
		SLAKE_FORCEINLINE StructRefData(const ObjectFieldRef &innerReference) : asObjectField(innerReference) {}
		SLAKE_FORCEINLINE StructRefData(const LocalVarRef &innerReference) : asLocalVar(innerReference) {}
		SLAKE_FORCEINLINE StructRefData(const CoroutineLocalVarRef &innerReference) : asCoroutineLocalVar(innerReference) {}
		SLAKE_FORCEINLINE StructRefData(const ArgRef &innerReference) : asArg(innerReference) {}
		SLAKE_FORCEINLINE StructRefData(const CoroutineArgRef &innerReference) : asCoroutineArg(innerReference) {}
	};

	struct StructFieldRef {
		StructRefData structRef;
		uint32_t idxField;
		ReferenceKind innerReferenceKind;

		StructFieldRef() = default;
		StructFieldRef(const StructRefData &structRef,
			uint32_t idxField,
			ReferenceKind innerReferenceKind)
			: structRef(structRef),
			  idxField(idxField),
			  innerReferenceKind(innerReferenceKind) {}
	};

	struct StructRef {
		StructRefData structRef;
		ReferenceKind innerReferenceKind;

		StructRef() = default;
		StructRef(const StructRefData &structRef,
			ReferenceKind innerReferenceKind)
			: structRef(structRef),
			  innerReferenceKind(innerReferenceKind) {}
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
			void *asAotPtr;
		};
		ReferenceKind kind;

		SLAKE_FORCEINLINE Reference() noexcept = default;
		SLAKE_FORCEINLINE Reference(const Reference &) noexcept = default;

		SLAKE_FORCEINLINE Reference(ReferenceKind referenceKind) noexcept : kind(referenceKind) {}
		SLAKE_FORCEINLINE Reference(std::nullptr_t) noexcept : kind(ReferenceKind::ObjectRef), asObject(nullptr) {}
		SLAKE_FORCEINLINE Reference(Object *object) noexcept : kind(ReferenceKind::ObjectRef), asObject(object) {}
		SLAKE_FORCEINLINE Reference(const StaticFieldRef &ref) noexcept : kind(ReferenceKind::StaticFieldRef), asStaticField(ref) {}
		SLAKE_FORCEINLINE Reference(const ArrayElementRef &ref) noexcept : kind(ReferenceKind::ArrayElementRef), asArrayElement(ref) {}
		SLAKE_FORCEINLINE Reference(const ObjectFieldRef &ref) noexcept : kind(ReferenceKind::ObjectFieldRef), asObjectField(ref) {}
		SLAKE_FORCEINLINE Reference(const LocalVarRef &ref) noexcept : kind(ReferenceKind::LocalVarRef), asLocalVar(ref) {}
		SLAKE_FORCEINLINE Reference(const CoroutineLocalVarRef &ref) noexcept : kind(ReferenceKind::CoroutineLocalVarRef), asCoroutineLocalVar(ref) {}
		SLAKE_FORCEINLINE Reference(const ArgRef &ref) noexcept : kind(ReferenceKind::ArgRef), asArg(ref) {}
		SLAKE_FORCEINLINE Reference(const ArgPackRef &ref) noexcept : kind(ReferenceKind::ArgPackRef), asArgPack(ref) {}
		SLAKE_FORCEINLINE Reference(const CoroutineArgRef &ref) noexcept : kind(ReferenceKind::CoroutineArgRef), asCoroutineArg(ref) {}
		SLAKE_FORCEINLINE Reference(const StructRef &ref) noexcept : kind(ReferenceKind::StructRef), asStruct(ref) {}
		SLAKE_FORCEINLINE Reference(const StructFieldRef &ref) noexcept : kind(ReferenceKind::StructFieldRef), asStructField(ref) {}

		Reference &operator=(const Reference &) noexcept = default;

		SLAKE_FORCEINLINE Reference &operator=(ReferenceKind referenceKind) noexcept {
			this->kind = referenceKind;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(std::nullptr_t) noexcept {
			this->kind = ReferenceKind::ObjectRef;
			this->asObject = nullptr;
			return *this;
		}
		SLAKE_FORCEINLINE Reference &operator=(const StaticFieldRef &ref) noexcept {
			kind = ReferenceKind::StaticFieldRef;
			asStaticField = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ArrayElementRef &ref) noexcept {
			kind = ReferenceKind::ArrayElementRef;
			asArrayElement = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ObjectFieldRef &ref) noexcept {
			kind = ReferenceKind::ObjectFieldRef;
			asObjectField = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const LocalVarRef &ref) noexcept {
			kind = ReferenceKind::LocalVarRef;
			asLocalVar = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const CoroutineLocalVarRef &ref) noexcept {
			kind = ReferenceKind::CoroutineLocalVarRef;
			asCoroutineLocalVar = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ArgRef &ref) noexcept {
			kind = ReferenceKind::ArgRef;
			asArg = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const ArgPackRef &ref) noexcept {
			kind = ReferenceKind::ArgPackRef;
			asArgPack = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const CoroutineArgRef &ref) noexcept {
			kind = ReferenceKind::CoroutineArgRef;
			asCoroutineArg = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const StructRef &ref) noexcept {
			kind = ReferenceKind::StructRef;
			asStruct = ref;
			return *this;
		}

		SLAKE_FORCEINLINE Reference &operator=(const StructFieldRef &ref) noexcept {
			kind = ReferenceKind::StructFieldRef;
			asStructField = ref;
			return *this;
		}

		SLAKE_FORCEINLINE bool isInvalid() const noexcept {
			return kind == ReferenceKind::Invalid;
		}

		SLAKE_FORCEINLINE bool isObjectRef() const noexcept {
			return kind == ReferenceKind::ObjectRef;
		}

		SLAKE_FORCEINLINE bool isStaticFieldRef() const noexcept {
			return kind == ReferenceKind::StaticFieldRef;
		}

		SLAKE_FORCEINLINE bool isLocalVarRef() const noexcept {
			return kind == ReferenceKind::LocalVarRef;
		}

		SLAKE_FORCEINLINE bool isCoroutineLocalVarRef() const noexcept {
			return kind == ReferenceKind::CoroutineLocalVarRef;
		}

		SLAKE_FORCEINLINE bool isObjectFieldRef() const noexcept {
			return kind == ReferenceKind::ObjectFieldRef;
		}

		SLAKE_FORCEINLINE bool isArrayElementRef() const noexcept {
			return kind == ReferenceKind::ArrayElementRef;
		}

		SLAKE_FORCEINLINE bool isArgRef() const noexcept {
			return kind == ReferenceKind::ArgRef;
		}

		SLAKE_FORCEINLINE bool isCoroutineArgRef() const noexcept {
			return kind == ReferenceKind::CoroutineArgRef;
		}

		SLAKE_FORCEINLINE bool isStructFieldRef() const noexcept {
			return kind == ReferenceKind::StructFieldRef;
		}

		SLAKE_FORCEINLINE bool isArgPackRef() const noexcept {
			return kind == ReferenceKind::ArgPackRef;
		}

		SLAKE_FORCEINLINE bool isAotPtrRef() const noexcept {
			return kind == ReferenceKind::AotPtrRef;
		}

		SLAKE_FORCEINLINE bool isStructRef() const noexcept {
			return kind == ReferenceKind::StructRef;
		}

		SLAKE_FORCEINLINE const StaticFieldRef &getStaticFieldRef() const noexcept {
			assert(kind == ReferenceKind::StaticFieldRef);
			return asStaticField;
		}

		SLAKE_FORCEINLINE Object *getObjectRef() const noexcept {
			assert(kind == ReferenceKind::ObjectRef);
			return asObject;
		}

		SLAKE_FORCEINLINE Object *&getObjectRef() noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return asObject;
		}

		SLAKE_FORCEINLINE StaticFieldRef &getStaticFieldRef() noexcept {
			assert(kind == ReferenceKind::StaticFieldRef);
			return asStaticField;
		}

		SLAKE_FORCEINLINE const LocalVarRef &getLocalVarRef() const noexcept {
			assert(kind == ReferenceKind::LocalVarRef);
			return asLocalVar;
		}

		SLAKE_FORCEINLINE LocalVarRef &getLocalVarRef() noexcept {
			assert(kind == ReferenceKind::LocalVarRef);
			return asLocalVar;
		}

		SLAKE_FORCEINLINE const CoroutineLocalVarRef &getCoroutineLocalVarRef() const noexcept {
			assert(kind == ReferenceKind::CoroutineLocalVarRef);
			return asCoroutineLocalVar;
		}

		SLAKE_FORCEINLINE CoroutineLocalVarRef &getCoroutineLocalVarRef() noexcept {
			assert(kind == ReferenceKind::CoroutineLocalVarRef);
			return asCoroutineLocalVar;
		}

		SLAKE_FORCEINLINE const ObjectFieldRef &getObjectFieldRef() const noexcept {
			assert(kind == ReferenceKind::ObjectFieldRef);
			return asObjectField;
		}

		SLAKE_FORCEINLINE ObjectFieldRef &getObjectFieldRef() noexcept {
			assert(kind == ReferenceKind::ObjectFieldRef);
			return asObjectField;
		}

		SLAKE_FORCEINLINE const ArrayElementRef &getArrayElementRef() const noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return asArrayElement;
		}

		SLAKE_FORCEINLINE ArrayElementRef &getArrayElementRef() noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return asArrayElement;
		}

		SLAKE_FORCEINLINE const ArgRef &getArgRef() const noexcept {
			assert(kind == ReferenceKind::ArgRef);
			return asArg;
		}

		SLAKE_FORCEINLINE ArgRef &getArgRef() noexcept {
			assert(kind == ReferenceKind::ArgRef);
			return asArg;
		}

		SLAKE_FORCEINLINE const CoroutineArgRef &getCoroutineArgRef() const noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return asCoroutineArg;
		}

		SLAKE_FORCEINLINE CoroutineArgRef &getCoroutineArgRef() noexcept {
			assert(kind == ReferenceKind::ArrayElementRef);
			return asCoroutineArg;
		}

		SLAKE_FORCEINLINE const ArgPackRef &getArgPackRef() const noexcept {
			assert(kind == ReferenceKind::ArgPackRef);
			return asArgPack;
		}

		SLAKE_FORCEINLINE ArgPackRef &getArgPackRef() noexcept {
			assert(kind == ReferenceKind::ArgPackRef);
			return asArgPack;
		}

		SLAKE_FORCEINLINE const void *getAotPtrRef() const noexcept {
			assert(kind == ReferenceKind::AotPtrRef);
			return asAotPtr;
		}

		SLAKE_FORCEINLINE void *&getAotPtrRef() noexcept {
			assert(kind == ReferenceKind::AotPtrRef);
			return asAotPtr;
		}

		SLAKE_FORCEINLINE const StructRef &getStructRef() const noexcept {
			assert(kind == ReferenceKind::StructRef);
			return asStruct;
		}

		SLAKE_FORCEINLINE StructRef &getStructRef() noexcept {
			assert(kind == ReferenceKind::StructRef);
			return asStruct;
		}

		SLAKE_FORCEINLINE const StructFieldRef &getStructFieldRef() const noexcept {
			assert(kind == ReferenceKind::StructRef);
			return asStructField;
		}

		SLAKE_FORCEINLINE StructFieldRef &getStructFieldRef() noexcept {
			assert(kind == ReferenceKind::StructRef);
			return asStructField;
		}

		SLAKE_FORCEINLINE bool isValid() const noexcept {
			return kind != ReferenceKind::Invalid;
		}
		explicit SLAKE_FORCEINLINE operator bool() const noexcept {
			return isValid();
		}

		SLAKE_API bool operator==(const Reference &rhs) const;
		SLAKE_API bool operator<(const Reference &rhs) const;
		SLAKE_API bool operator>(const Reference &rhs) const;
	};

#ifndef ssize_t
	using ssize_t = std::make_signed_t<size_t>;
#endif

	using SizeTypeMarker = bool;
	constexpr static SizeTypeMarker SIZETYPE_MARKER = true;

	struct TypelessScopedEnumValue {
		uint32_t value;
		TypeRef type;
	};

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
		TypelessScopedEnumValue asTypelessScopedEnum;

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

	using ValueFlags = uint8_t;
	constexpr ValueFlags VALUE_LOCAL = 0x01;
	struct Value {
		ValueData data;

		ValueType valueType;
		ValueFlags valueFlags;

		SLAKE_FORCEINLINE Value() noexcept = default;
		SLAKE_API Value(const Value &other) noexcept = default;
		SLAKE_FORCEINLINE Value(Value &&other) noexcept = default;
		SLAKE_FORCEINLINE constexpr Value(int8_t data) noexcept : valueType(ValueType::I8), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int16_t data) noexcept : valueType(ValueType::I16), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int32_t data) noexcept : valueType(ValueType::I32), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int64_t data) noexcept : valueType(ValueType::I64), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, ssize_t data) noexcept : valueType(ValueType::ISize), data(data), valueFlags(0) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(uint8_t data) noexcept : valueType(ValueType::U8), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint16_t data) noexcept : valueType(ValueType::U16), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint32_t data) noexcept : valueType(ValueType::U32), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint64_t data) noexcept : valueType(ValueType::U64), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, size_t data) noexcept : valueType(ValueType::USize), data(data), valueFlags(0) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(float data) noexcept : valueType(ValueType::F32), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(double data) noexcept : valueType(ValueType::F64), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(bool data) noexcept : valueType(ValueType::Bool), data(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE Value(const Reference &entityRef) noexcept : valueType(ValueType::Reference), valueFlags(0) {
			this->data.asReference = entityRef;
		}
		SLAKE_FORCEINLINE Value(std::nullptr_t) noexcept : valueType(ValueType::Reference), valueFlags(0) {
			this->data.asReference = nullptr;
		}
		SLAKE_FORCEINLINE Value(const TypelessScopedEnumValue &v) noexcept : valueType(ValueType::TypelessScopedEnum), valueFlags(0) {
			this->data.asTypelessScopedEnum = v;
		}
		SLAKE_FORCEINLINE Value(ValueType vt) noexcept : valueType(vt), data(/*Uninitialized*/), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(ValueType vt, uint32_t index) noexcept : valueType(vt), data(index), valueFlags(0) {
		}
		SLAKE_FORCEINLINE Value(const TypeRef &type) noexcept : valueType(ValueType::TypeName), valueFlags(0) {
			data.asType = type;
		}

		SLAKE_FORCEINLINE constexpr Value &operator=(ValueType data) noexcept {
			valueType = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int8_t data) noexcept {
			valueType = ValueType::I8;
			this->data.asI8 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int16_t data) noexcept {
			valueType = ValueType::I16;
			this->data.asI16 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int32_t data) noexcept {
			valueType = ValueType::I32;
			this->data.asI32 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int64_t data) noexcept {
			valueType = ValueType::I64;
			this->data.asI64 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint8_t data) noexcept {
			valueType = ValueType::U8;
			this->data.asU8 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint16_t data) noexcept {
			valueType = ValueType::U16;
			this->data.asU16 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint32_t data) noexcept {
			valueType = ValueType::U32;
			this->data.asU32 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint64_t data) noexcept {
			valueType = ValueType::U64;
			this->data.asU64 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(float data) noexcept {
			valueType = ValueType::F32;
			this->data.asF32 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(double data) noexcept {
			valueType = ValueType::F64;
			this->data.asF64 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(bool data) noexcept {
			valueType = ValueType::Bool;
			this->data.asBool = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const Reference &entityRef) noexcept {
			valueType = ValueType::Reference;
			this->data.asReference = entityRef;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(std::nullptr_t entityRef) noexcept {
			valueType = ValueType::Reference;
			this->data.asReference = entityRef;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const TypelessScopedEnumValue &data) noexcept {
			valueType = ValueType::TypelessScopedEnum;
			this->data.asTypelessScopedEnum = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const TypeRef &type) noexcept {
			valueType = ValueType::TypeName;
			data.asType = type;
			this->valueFlags = 0;
			return *this;
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
			assert(valueType == ValueType::RegIndex);
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
		SLAKE_FORCEINLINE TypelessScopedEnumValue &getTypelessScopedEnum() noexcept {
			assert(valueType == ValueType::TypelessScopedEnum);
			return data.asTypelessScopedEnum;
		}
		SLAKE_FORCEINLINE const TypelessScopedEnumValue &getTypelessScopedEnum() const noexcept {
			assert(valueType == ValueType::TypelessScopedEnum);
			return data.asTypelessScopedEnum;
		}

		//
		// Helper APIs.
		//

		SLAKE_FORCEINLINE bool isI8() const noexcept {
			return valueType == ValueType::I8;
		}

		SLAKE_FORCEINLINE bool isI16() const noexcept {
			return valueType == ValueType::I16;
		}

		SLAKE_FORCEINLINE bool isI32() const noexcept {
			return valueType == ValueType::I32;
		}

		SLAKE_FORCEINLINE bool isI64() const noexcept {
			return valueType == ValueType::I64;
		}

		SLAKE_FORCEINLINE bool isISize() const noexcept {
			return valueType == ValueType::ISize;
		}

		SLAKE_FORCEINLINE bool isU8() const noexcept {
			return valueType == ValueType::U8;
		}

		SLAKE_FORCEINLINE bool isU16() const noexcept {
			return valueType == ValueType::U16;
		}

		SLAKE_FORCEINLINE bool isU32() const noexcept {
			return valueType == ValueType::U32;
		}

		SLAKE_FORCEINLINE bool isU64() const noexcept {
			return valueType == ValueType::U64;
		}

		SLAKE_FORCEINLINE bool isUSize() const noexcept {
			return valueType == ValueType::USize;
		}

		SLAKE_FORCEINLINE bool isF32() const noexcept {
			return valueType == ValueType::F32;
		}

		SLAKE_FORCEINLINE bool isF64() const noexcept {
			return valueType == ValueType::F64;
		}

		SLAKE_FORCEINLINE bool isBool() const noexcept {
			return valueType == ValueType::Bool;
		}

		SLAKE_FORCEINLINE bool isRegIndex() const noexcept {
			return valueType == ValueType::RegIndex;
		}

		SLAKE_FORCEINLINE bool isLabel() const noexcept {
			return valueType == ValueType::Label;
		}

		SLAKE_FORCEINLINE bool isTypeName() const noexcept {
			return valueType == ValueType::TypeName;
		}

		SLAKE_FORCEINLINE bool isReference() const noexcept {
			return valueType == ValueType::Reference;
		}
		SLAKE_FORCEINLINE bool isNull() const noexcept {
			return (valueType == ValueType::Reference) && (data.asReference.kind == ReferenceKind::ObjectRef) && (!data.asReference.asObject);
		}
		SLAKE_FORCEINLINE bool isTypelessScopedEnum() const noexcept {
			return valueType == ValueType::TypelessScopedEnum;
		}

		SLAKE_FORCEINLINE bool isLocal() const noexcept {
			return valueFlags & VALUE_LOCAL;
		}
		SLAKE_FORCEINLINE void setLocal() noexcept {
			valueFlags |= VALUE_LOCAL;
		}
		SLAKE_FORCEINLINE void clearLocal() noexcept {
			valueFlags &= ~VALUE_LOCAL;
		}

		Value &operator=(const Value &other) noexcept = default;
		Value &operator=(Value &&other) noexcept = default;

		SLAKE_API bool operator==(const Value &rhs) const noexcept;

		SLAKE_FORCEINLINE bool operator!=(const Value &rhs) const noexcept {
			return !(*this == rhs);
		}

		SLAKE_API int comparesTo(const Value &rhs) const noexcept;
		SLAKE_API bool operator<(const Value &rhs) const noexcept;
		SLAKE_API bool operator>(const Value &rhs) const noexcept;
	};

	SLAKE_API Reference extractStructInnerRef(const StructRefData &structRef, ReferenceKind innerReferenceKind);
	SLAKE_API bool isCompatible(const TypeRef &type, const Value &value) noexcept;
}

#endif
