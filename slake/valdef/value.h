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

		static inline VarRefContext makeArrayContext(uint32_t index) {
			VarRefContext context = {};

			context.asArray.index = index;

			return context;
		}

		static inline VarRefContext makeInstanceContext(size_t fieldIndex) {
			VarRefContext context = {};

			context.asInstance.fieldIndex = fieldIndex;

			return context;
		}

		static inline VarRefContext makeLocalVarContext(uint32_t localVarIndex) {
			VarRefContext context = {};

			context.asLocalVar.localVarIndex = localVarIndex;

			return context;
		}
	};

	struct VarRef {
		VarObject *varPtr;
		VarRefContext context;

		VarRef() = default;
		inline VarRef(VarObject *varPtr) : varPtr(varPtr) {}
		inline VarRef(
			VarObject *varPtr,
			const VarRefContext &context)
			: varPtr(varPtr),
			  context(context) {}
		bool operator<(const VarRef &rhs) const;
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
			Object *asObjectRef;
			char asType[sizeof(Type)];
			VarRef asVarRef;
		} data;

	public:
		ValueType valueType = ValueType::Undefined;

		Value() = default;
		Value(const Value &other) = default;
		Value(Value &&other) = default;
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
		inline Value(Object *objectPtr) {
			this->data.asObjectRef = objectPtr;
			valueType = ValueType::ObjectRef;
		}
		inline Value(ValueType vt, uint32_t index) {
			this->data.asU32 = index;
			valueType = vt;
		}
		inline Value(const VarRef &varRef) {
			this->data.asVarRef = varRef;
			valueType = ValueType::VarRef;
		}
		Value(const Type &type);

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

		inline VarRef &getVarRef() {
			assert(valueType == ValueType::VarRef);
			return data.asVarRef;
		}

		inline const VarRef &getVarRef() const {
			assert(valueType == ValueType::VarRef);
			return data.asVarRef;
		}

		Type &getTypeName();
		const Type &getTypeName() const;

		inline Object* getObjectRef() const {
			return data.asObjectRef;
		}

		Value &operator=(const Value &other) = default;
		inline Value &operator=(Value &&other) noexcept = default;

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
