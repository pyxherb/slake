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
	class VarObject;

	// Value type definitions are defined in <slake/type.h>.

	union VarRefContext {
		struct {
			uint32_t index;
		} asArray;
		struct {
			size_t fieldIndex;
		} asInstance;
		struct {
			uint32_t localVarIndex;
		} asLocalVar;

		static SLAKE_FORCEINLINE VarRefContext makeArrayContext(uint32_t index) {
			VarRefContext context = {};

			context.asArray.index = index;

			return context;
		}

		static SLAKE_FORCEINLINE VarRefContext makeInstanceContext(size_t fieldIndex) {
			VarRefContext context = {};

			context.asInstance.fieldIndex = fieldIndex;

			return context;
		}

		static SLAKE_FORCEINLINE VarRefContext makeLocalVarContext(uint32_t localVarIndex) {
			VarRefContext context = {};

			context.asLocalVar.localVarIndex = localVarIndex;

			return context;
		}
	};

	struct VarRef {
		VarObject *varPtr;
		VarRefContext context;

		VarRef() = default;
		SLAKE_FORCEINLINE VarRef(VarObject *varPtr) : varPtr(varPtr) {}
		SLAKE_FORCEINLINE VarRef(
			VarObject *varPtr,
			const VarRefContext &context)
			: varPtr(varPtr),
			  context(context) {}
		SLAKE_API bool operator<(const VarRef &rhs) const;
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
			Object *asObjectRef;
			char asType[sizeof(Type)];
			VarRef asVarRef;
		} data;

		ValueType valueType;

		Value() = default;
		Value(const Value &other) = default;
		Value(Value &&other) = default;
		SLAKE_FORCEINLINE Value(int8_t data) : valueType(ValueType::I8) {
			this->data.asI8 = data;
		}
		SLAKE_FORCEINLINE Value(int16_t data) : valueType(ValueType::I16) {
			this->data.asI16 = data;
		}
		SLAKE_FORCEINLINE Value(int32_t data) : valueType(ValueType::I32) {
			this->data.asI32 = data;
		}
		SLAKE_FORCEINLINE Value(int64_t data) : valueType(ValueType::I64) {
			this->data.asI64 = data;
		}
		SLAKE_FORCEINLINE Value(uint8_t data) : valueType(ValueType::U8) {
			this->data.asU8 = data;
		}
		SLAKE_FORCEINLINE Value(uint16_t data) : valueType(ValueType::U16) {
			this->data.asU16 = data;
		}
		SLAKE_FORCEINLINE Value(uint32_t data) : valueType(ValueType::U32) {
			this->data.asU32 = data;
		}
		SLAKE_FORCEINLINE Value(uint64_t data) : valueType(ValueType::U64) {
			this->data.asU64 = data;
		}
		SLAKE_FORCEINLINE Value(float data) : valueType(ValueType::F32) {
			this->data.asF32 = data;
		}
		SLAKE_FORCEINLINE Value(double data) : valueType(ValueType::F64) {
			this->data.asF64 = data;
		}
		SLAKE_FORCEINLINE Value(bool data) : valueType(ValueType::Bool) {
			this->data.asBool = data;
		}
		SLAKE_FORCEINLINE Value(Object *objectPtr) : valueType(ValueType::ObjectRef) {
			this->data.asObjectRef = objectPtr;
		}
		SLAKE_FORCEINLINE Value(ValueType vt) : valueType(vt) {
		}
		SLAKE_FORCEINLINE Value(ValueType vt, uint32_t index) : valueType(vt) {
			this->data.asU32 = index;
		}
		SLAKE_FORCEINLINE Value(const VarRef &varRef) : valueType(ValueType::VarRef) {
			this->data.asVarRef = varRef;
		}
		SLAKE_API Value(const Type &type);

		SLAKE_FORCEINLINE int8_t getI8() const {
			assert(valueType == ValueType::I8);
			return data.asI8;
		}

		SLAKE_FORCEINLINE int16_t getI16() const {
			assert(valueType == ValueType::I16);
			return data.asI16;
		}

		SLAKE_FORCEINLINE int32_t getI32() const {
			assert(valueType == ValueType::I32);
			return data.asI32;
		}

		SLAKE_FORCEINLINE int64_t getI64() const {
			assert(valueType == ValueType::I64);
			return data.asI64;
		}

		SLAKE_FORCEINLINE uint8_t getU8() const {
			assert(valueType == ValueType::U8);
			return data.asU8;
		}

		SLAKE_FORCEINLINE uint16_t getU16() const {
			assert(valueType == ValueType::U16);
			return data.asU16;
		}

		SLAKE_FORCEINLINE uint32_t getU32() const {
			assert(valueType == ValueType::U32);
			return data.asU32;
		}

		SLAKE_FORCEINLINE uint64_t getU64() const {
			assert(valueType == ValueType::U64);
			return data.asU64;
		}

		SLAKE_FORCEINLINE float getF32() const {
			assert(valueType == ValueType::F32);
			return data.asF32;
		}

		SLAKE_FORCEINLINE double getF64() const {
			assert(valueType == ValueType::F64);
			return data.asF64;
		}

		SLAKE_FORCEINLINE bool getBool() const {
			assert(valueType == ValueType::Bool);
			return data.asBool;
		}

		SLAKE_FORCEINLINE uint32_t getRegIndex() const {
			assert(valueType == ValueType::RegRef);
			return data.asU32;
		}

		SLAKE_FORCEINLINE VarRef &getVarRef() {
			assert(valueType == ValueType::VarRef);
			return data.asVarRef;
		}

		SLAKE_FORCEINLINE const VarRef &getVarRef() const {
			assert(valueType == ValueType::VarRef);
			return data.asVarRef;
		}

		SLAKE_API Type &getTypeName();
		SLAKE_API const Type &getTypeName() const;

		SLAKE_FORCEINLINE Object *getObjectRef() const {
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
