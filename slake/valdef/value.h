#ifndef _SLAKE_VALDEF_VALUE_H_
#define _SLAKE_VALDEF_VALUE_H_

#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>
#include <cassert>

namespace slake {
	struct Type;
	class Object;

	enum class ValueType : uint8_t {
		I8 = 0,		// Signed 8-bit integer
		I16,		// Signed 16-bit integer
		I32,		// Signed 32-bit integer
		I64,		// Signed 64-bit integer
		U8,			// Unsigned 8-bit integer
		U16,		// Unsigned 16-bit integer
		U32,		// Unsigned 32-bit integer
		U64,		// Unsigned 64-bit integer
		F32,		// 32-bit floating point number
		F64,		// 64-bit floating point number
		Bool,		// Boolean
		String,		// String
		ObjectRef,	// Object reference

		RegRef,		  // Register reference
		ArgRef,		  // Argument reference
		LocalVarRef,  // Local variable reference
		TypeName,	  // Type name

		Invalid = UINT8_MAX,
	};

	struct IndexedRefValueExData {
		bool unwrap;
		uint32_t index;
	};

	struct ObjectRefValueExData {
		Object *objectPtr;
		bool isHostRef;
	};

	struct Value {
	private:
		std::variant<
			uint8_t,
			uint16_t,
			uint32_t,
			uint64_t,
			int8_t,
			int16_t,
			int32_t,
			int64_t,
			float,
			double,
			bool,
			std::string,
			ObjectRefValueExData,
			IndexedRefValueExData,
			Type *>
			data;
		void _reset();

	public:
		ValueType valueType = ValueType::Invalid;

		inline Value(const Value &other) {
			*this = other;
		}
		inline Value(Value &&other) {
			*this = std::move(other);
		}
		Value() = default;
		inline Value(int8_t data) {
			this->data = data;
			valueType = ValueType::I8;
		}
		inline Value(int16_t data) {
			this->data = data;
			valueType = ValueType::I16;
		}
		inline Value(int32_t data) {
			this->data = data;
			valueType = ValueType::I32;
		}
		inline Value(int64_t data) {
			this->data = data;
			valueType = ValueType::I64;
		}
		inline Value(uint8_t data) {
			this->data = data;
			valueType = ValueType::U8;
		}
		inline Value(uint16_t data) {
			this->data = data;
			valueType = ValueType::U16;
		}
		inline Value(uint32_t data) {
			this->data = data;
			valueType = ValueType::U32;
		}
		inline Value(uint64_t data) {
			this->data = data;
			valueType = ValueType::U64;
		}
		inline Value(float data) {
			this->data = data;
			valueType = ValueType::F32;
		}
		inline Value(double data) {
			this->data = data;
			valueType = ValueType::F64;
		}
		inline Value(bool data) {
			this->data = data;
			valueType = ValueType::Bool;
		}
		inline Value(const std::string &data) {
			this->data = data;
			valueType = ValueType::String;
		}
		inline Value(std::string &&data) {
			this->data = std::move(data);
			valueType = ValueType::String;
		}
		inline Value(Object *objectPtr, bool isHostRef = false) {
			if (isHostRef) {
				if (objectPtr)
					++objectPtr->hostRefCount;
			}
			this->data = ObjectRefValueExData{ objectPtr, isHostRef };
			valueType = ValueType::ObjectRef;
		}
		inline Value(ValueType vt, uint32_t index, bool unwrap) {
			this->data = IndexedRefValueExData{ unwrap, index };
			valueType = vt;
		}
		Value(const Type &type);
		~Value();

		inline int8_t getI8() const {
			assert(valueType == ValueType::I8);
			return std::get<int8_t>(data);
		}

		inline int16_t getI16() const {
			assert(valueType == ValueType::I16);
			return std::get<int16_t>(data);
		}

		inline int32_t getI32() const {
			assert(valueType == ValueType::I32);
			return std::get<int32_t>(data);
		}

		inline int64_t getI64() const {
			assert(valueType == ValueType::I64);
			return std::get<int64_t>(data);
		}

		inline uint8_t getU8() const {
			assert(valueType == ValueType::U8);
			return std::get<uint8_t>(data);
		}

		inline uint16_t getU16() const {
			assert(valueType == ValueType::U16);
			return std::get<uint16_t>(data);
		}

		inline uint32_t getU32() const {
			assert(valueType == ValueType::U32);
			return std::get<uint32_t>(data);
		}

		inline uint64_t getU64() const {
			assert(valueType == ValueType::U64);
			return std::get<uint64_t>(data);
		}

		inline float getF32() const {
			assert(valueType == ValueType::F32);
			return std::get<float>(data);
		}

		inline double getF64() const {
			assert(valueType == ValueType::F64);
			return std::get<double>(data);
		}

		inline bool getBool() const {
			assert(valueType == ValueType::Bool);
			return std::get<bool>(data);
		}

		inline const std::string &getString() const {
			assert(valueType == ValueType::String);
			return std::get<std::string>(data);
		}

		Type &getTypeName();
		const Type &getTypeName() const;

		const ObjectRefValueExData &getObjectRef() const;

		const IndexedRefValueExData &getIndexedRef() const;

		Value &operator=(const Value &other);
		Value &operator=(Value &&other);
	};
}

#include <slake/type.h>

#endif
