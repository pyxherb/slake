#ifndef _SLAKE_VALDEF_VALUE_H_
#define _SLAKE_VALDEF_VALUE_H_

#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>
#include <cassert>
#include "object.h"
#include <slake/type.h>

namespace slake {
	struct Type;
	class Object;

	// Value type definitions are defined in <slake/type.h>.

	struct ObjectRefValueExData {
		Object *objectPtr;
		bool isHostRef;
	};

	struct Value {
	private:
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
			ObjectRefValueExData asObjectRef;
			char asType[sizeof(Type)];
		} data;
		void _setObjectRef(Object *objectPtr, bool isHostRef);
		void _reset();

	public:
		ValueType valueType = ValueType::Undefined;

		inline Value(const Value &other) {
			*this = other;
		}
		inline Value(Value &&other) {
			*this = std::move(other);
		}
		Value() = default;
		inline Value(int8_t data) {
			this->data.asI8 = data;
			valueType = ValueType::I8;
		}
		inline Value(int16_t data) {
			this->data.asI16 = data;
			valueType = ValueType::I16;
		}
		inline Value(int32_t data) {
			this->data.asI32 = data;
			valueType = ValueType::I32;
		}
		inline Value(int64_t data) {
			this->data.asI64 = data;
			valueType = ValueType::I64;
		}
		inline Value(uint8_t data) {
			this->data.asU8 = data;
			valueType = ValueType::U8;
		}
		inline Value(uint16_t data) {
			this->data.asU16 = data;
			valueType = ValueType::U16;
		}
		inline Value(uint32_t data) {
			this->data.asU32 = data;
			valueType = ValueType::U32;
		}
		inline Value(uint64_t data) {
			this->data.asU64 = data;
			valueType = ValueType::U64;
		}
		inline Value(float data) {
			this->data.asF32 = data;
			valueType = ValueType::F32;
		}
		inline Value(double data) {
			this->data.asF64 = data;
			valueType = ValueType::F64;
		}
		inline Value(bool data) {
			this->data.asBool = data;
			valueType = ValueType::Bool;
		}
		inline Value(Object *objectPtr, bool isHostRef = false) {
			_setObjectRef(objectPtr, isHostRef);
			valueType = ValueType::ObjectRef;
		}
		inline Value(ValueType vt, uint32_t index) {
			this->data.asU32 = index;
			valueType = vt;
		}
		Value(const Type &type);
		~Value();

		inline int8_t getI8() const {
			assert(valueType == ValueType::I8);
			return data.asI8;
		}

		inline int16_t getI16() const {
			assert(valueType == ValueType::I16);
			return data.asI16;
		}

		inline int32_t getI32() const {
			assert(valueType == ValueType::I32);
			return data.asI32;
		}

		inline int64_t getI64() const {
			assert(valueType == ValueType::I64);
			return data.asI64;
		}

		inline uint8_t getU8() const {
			assert(valueType == ValueType::U8);
			return data.asU8;
		}

		inline uint16_t getU16() const {
			assert(valueType == ValueType::U16);
			return data.asU16;
		}

		inline uint32_t getU32() const {
			assert(valueType == ValueType::U32);
			return data.asU32;
		}

		inline uint64_t getU64() const {
			assert(valueType == ValueType::U64);
			return data.asU64;
		}

		inline float getF32() const {
			assert(valueType == ValueType::F32);
			return data.asF32;
		}

		inline double getF64() const {
			assert(valueType == ValueType::F64);
			return data.asF64;
		}

		inline bool getBool() const {
			assert(valueType == ValueType::Bool);
			return data.asBool;
		}

		inline uint32_t getRegIndex() const {
			assert(valueType == ValueType::RegRef);
			return data.asU32;
		}

		Type &getTypeName();
		const Type &getTypeName() const;

		const ObjectRefValueExData &getObjectRef() const;

		Value &operator=(const Value &other);
		inline Value &operator=(Value &&other) noexcept {
			(*this) = other;
			return *this;
		}

		bool operator==(const Value &rhs) const;

		inline bool operator!=(const Value &rhs) const {
			return !(*this == rhs);
		}

		bool operator<(const Value &rhs) const;
	};

	bool isCompatible(const Type &type, const Value &value);
}

#include <slake/type.h>

#endif
