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

		ObjectRef,
		ArgPackRef,
		AotPtrRef,

		StaticFieldStructFieldRef = StaticFieldRef | 0x80,
		LocalVarStructFieldRef = LocalVarRef | 0x80,
		CoroutineLocalVarStructFieldRef = CoroutineLocalVarRef | 0x80,
		ObjectFieldStructFieldRef = ObjectFieldRef | 0x80,
		ArrayElementStructFieldRef = ArrayElementRef | 0x80,
		ArgStructFieldRef = ArgRef | 0x80,
		CoroutineArgStructFieldRef = CoroutineArgRef | 0x80,
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
		uint32_t argIndex;

		ArgRef() = default;
		SLAKE_FORCEINLINE ArgRef(const MajorFrame *majorFrame, uint32_t argIndex) : majorFrame(majorFrame), argIndex(argIndex) {}
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
			void *asAotPtr;
		};
		uint32_t structFieldIndex;
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
			return (((uint8_t)kind) >= (uint8_t)ReferenceKind::StaticFieldStructFieldRef) &&
				   (((uint8_t)kind) <= (uint8_t)ReferenceKind::CoroutineArgStructFieldRef);
		}

		SLAKE_FORCEINLINE bool isArgPackRef() const noexcept {
			return kind == ReferenceKind::ArgPackRef;
		}

		SLAKE_FORCEINLINE bool isAotPtrRef() const noexcept {
			return kind == ReferenceKind::AotPtrRef;
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
			assert(kind == ReferenceKind::ObjectRef);
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

	using ValueFlags = uint8_t;
	constexpr ValueFlags VALUE_LOCAL = 0x01;
	struct Value {
		union {
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
		};

		ValueType valueType;
		ValueFlags valueFlags;

		SLAKE_FORCEINLINE Value() noexcept = default;
		SLAKE_API Value(const Value &other) noexcept = default;
		SLAKE_FORCEINLINE Value(Value &&other) noexcept = default;
		SLAKE_FORCEINLINE constexpr Value(int8_t data) noexcept : valueType(ValueType::I8), asI8(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int16_t data) noexcept : valueType(ValueType::I16), asI16(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int32_t data) noexcept : valueType(ValueType::I32), asI32(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(int64_t data) noexcept : valueType(ValueType::I64), asI64(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, ssize_t data) noexcept : valueType(ValueType::ISize), asISize(data), valueFlags(0) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(uint8_t data) noexcept : valueType(ValueType::U8), asU8(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint16_t data) noexcept : valueType(ValueType::U16), asU16(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint32_t data) noexcept : valueType(ValueType::U32), asU32(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(uint64_t data) noexcept : valueType(ValueType::U64), asU64(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(SizeTypeMarker marker, size_t data) noexcept : valueType(ValueType::USize), asUSize(data), valueFlags(0) {
			SLAKE_REFERENCED_PARAM(marker);
		}
		SLAKE_FORCEINLINE constexpr Value(float data) noexcept : valueType(ValueType::F32), asF32(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(double data) noexcept : valueType(ValueType::F64), asF64(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr explicit Value(bool data) noexcept : valueType(ValueType::Bool), asBool(data), valueFlags(0) {
		}
		SLAKE_FORCEINLINE Value(const Reference &reference) noexcept : valueType(ValueType::Reference), asReference(reference), valueFlags(0) {
			if(reference.kind == ReferenceKind::Invalid)
			std::terminate();
		}
		SLAKE_FORCEINLINE Value(std::nullptr_t) noexcept : valueType(ValueType::Reference), asReference(nullptr), valueFlags(0) {
		}
		SLAKE_FORCEINLINE Value(const TypelessScopedEnumValue &v) noexcept : valueType(ValueType::TypelessScopedEnum), asTypelessScopedEnum(v), valueFlags(0) {
		}
		SLAKE_FORCEINLINE Value(ValueType vt) noexcept : valueType(vt), valueFlags(0) {
		}
		SLAKE_FORCEINLINE constexpr Value(ValueType vt, uint32_t index) noexcept : valueType(vt), asU32(index), valueFlags(0) {
		}
		SLAKE_FORCEINLINE Value(const TypeRef &type) noexcept : valueType(ValueType::TypeName), asType(type), valueFlags(0) {
		}

		SLAKE_FORCEINLINE constexpr Value &operator=(ValueType data) noexcept {
			valueType = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int8_t data) noexcept {
			valueType = ValueType::I8;
			this->asI8 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int16_t data) noexcept {
			valueType = ValueType::I16;
			this->asI16 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int32_t data) noexcept {
			valueType = ValueType::I32;
			this->asI32 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(int64_t data) noexcept {
			valueType = ValueType::I64;
			this->asI64 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint8_t data) noexcept {
			valueType = ValueType::U8;
			this->asU8 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint16_t data) noexcept {
			valueType = ValueType::U16;
			this->asU16 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint32_t data) noexcept {
			valueType = ValueType::U32;
			this->asU32 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(uint64_t data) noexcept {
			valueType = ValueType::U64;
			this->asU64 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(float data) noexcept {
			valueType = ValueType::F32;
			this->asF32 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(double data) noexcept {
			valueType = ValueType::F64;
			this->asF64 = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE constexpr Value &operator=(bool data) noexcept {
			valueType = ValueType::Bool;
			this->asBool = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const Reference &entityRef) noexcept {
			valueType = ValueType::Reference;
			this->asReference = entityRef;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(std::nullptr_t entityRef) noexcept {
			valueType = ValueType::Reference;
			this->asReference = entityRef;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const TypelessScopedEnumValue &data) noexcept {
			valueType = ValueType::TypelessScopedEnum;
			this->asTypelessScopedEnum = data;
			this->valueFlags = 0;
			return *this;
		}
		SLAKE_FORCEINLINE Value &operator=(const TypeRef &type) noexcept {
			valueType = ValueType::TypeName;
			asType = type;
			this->valueFlags = 0;
			return *this;
		}

		SLAKE_FORCEINLINE int8_t getI8() const noexcept {
			assert(valueType == ValueType::I8);
			return asI8;
		}

		SLAKE_FORCEINLINE int16_t getI16() const noexcept {
			assert(valueType == ValueType::I16);
			return asI16;
		}

		SLAKE_FORCEINLINE int32_t getI32() const noexcept {
			assert(valueType == ValueType::I32);
			return asI32;
		}

		SLAKE_FORCEINLINE int64_t getI64() const noexcept {
			assert(valueType == ValueType::I64);
			return asI64;
		}

		SLAKE_FORCEINLINE ssize_t getISize() const noexcept {
			assert(valueType == ValueType::ISize);
			return asISize;
		}

		SLAKE_FORCEINLINE uint8_t getU8() const noexcept {
			assert(valueType == ValueType::U8);
			return asU8;
		}

		SLAKE_FORCEINLINE uint16_t getU16() const noexcept {
			assert(valueType == ValueType::U16);
			return asU16;
		}

		SLAKE_FORCEINLINE uint32_t getU32() const noexcept {
			assert(valueType == ValueType::U32);
			return asU32;
		}

		SLAKE_FORCEINLINE uint64_t getU64() const noexcept {
			assert(valueType == ValueType::U64);
			return asU64;
		}

		SLAKE_FORCEINLINE size_t getUSize() const noexcept {
			assert(valueType == ValueType::USize);
			return asUSize;
		}

		SLAKE_FORCEINLINE float getF32() const noexcept {
			assert(valueType == ValueType::F32);
			return asF32;
		}

		SLAKE_FORCEINLINE double getF64() const noexcept {
			assert(valueType == ValueType::F64);
			return asF64;
		}

		SLAKE_FORCEINLINE bool getBool() const noexcept {
			assert(valueType == ValueType::Bool);
			return asBool;
		}

		SLAKE_FORCEINLINE uint32_t getRegIndex() const noexcept {
			assert(valueType == ValueType::RegIndex);
			return asU32;
		}

		SLAKE_FORCEINLINE uint32_t getLabel() const noexcept {
			assert(valueType == ValueType::Label);
			return asU32;
		}

		SLAKE_FORCEINLINE TypeRef &getTypeName() noexcept {
			assert(valueType == ValueType::TypeName);
			return asType;
		}
		SLAKE_FORCEINLINE const TypeRef &getTypeName() const noexcept {
			assert(valueType == ValueType::TypeName);
			return asType;
		}

		SLAKE_FORCEINLINE Reference &getReference() noexcept {
			assert(valueType == ValueType::Reference);
			return asReference;
		}
		SLAKE_FORCEINLINE const Reference &getReference() const noexcept {
			assert(valueType == ValueType::Reference);
			return asReference;
		}
		SLAKE_FORCEINLINE TypelessScopedEnumValue &getTypelessScopedEnum() noexcept {
			assert(valueType == ValueType::TypelessScopedEnum);
			return asTypelessScopedEnum;
		}
		SLAKE_FORCEINLINE const TypelessScopedEnumValue &getTypelessScopedEnum() const noexcept {
			assert(valueType == ValueType::TypelessScopedEnum);
			return asTypelessScopedEnum;
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
			return (valueType == ValueType::Reference) && (asReference.kind == ReferenceKind::ObjectRef) && (!asReference.asObject);
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

	SLAKE_API bool isCompatible(const TypeRef &type, const Value &value) noexcept;
}

#endif
