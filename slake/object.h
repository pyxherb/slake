#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include <memory>
#include <mutex>
#include <unordered_set>

namespace Slake {
	enum class ValueType : std::uint8_t {
		U8 = 0,
		U16,
		U32,
		U64,
		I8,
		I16,
		I32,
		I64,
		FLOAT,
		DOUBLE,
		STRING,
		FN,
		OBJECT,
		SCOPE_REF  // For instructions only
	};

	class Object {
	protected:
		std::atomic_uint32_t _refCount;

	public:
		virtual inline ~Object() {}
		virtual ValueType getType() const = 0;

		void incRefCount();
		void decRefCount();
		std::uint32_t getRefCount();
	};

	template <typename T, int VT>
	class LiteralObject : public Object {
	protected:
		T _value;

	public:
		inline LiteralObject(T value) { _value = value; }
		virtual inline ~LiteralObject() {}
		virtual inline ValueType getType() const override { return (ValueType)VT; }
		virtual inline const T& getValue() const { return _value; }
		virtual inline void setValue(T& value) { _value = value; }
	};

	using I8Object = LiteralObject<std::int8_t, (int)ValueType::I8>;
	using I16Object = LiteralObject<std::int16_t, (int)ValueType::I16>;
	using I32Object = LiteralObject<std::int32_t, (int)ValueType::I32>;
	using I64Object = LiteralObject<std::int64_t, (int)ValueType::I64>;
	using U8Object = LiteralObject<std::uint8_t, (int)ValueType::U8>;
	using U16Object = LiteralObject<std::uint16_t, (int)ValueType::U16>;
	using U32Object = LiteralObject<std::uint32_t, (int)ValueType::U32>;
	using U64Object = LiteralObject<std::uint64_t, (int)ValueType::U64>;
	using StringObject = LiteralObject<std::string, (int)ValueType::STRING>;

	class ObjectValue : public Object {
	public:
		virtual inline ~ObjectValue() {}

		virtual inline ValueType getType() const override { return ValueType::OBJECT; }
		virtual Object* getMember(std::string name) = 0;
	};

	class RefValue final : public Object {
	public:
		std::string name;
		RefValue* next;

		inline RefValue(std::string name, RefValue* next) : name(name), next(next) {
		}
		virtual inline ~RefValue() {
		}
		virtual inline ValueType getType() const {
			return ValueType::SCOPE_REF;
		}
	};

	template <typename T = Object>
	class ObjectRef final {
	protected:
		T* _value;

	public:
		inline ObjectRef(ObjectRef& x) : _value(x->_value) {
			_value->incRefCount();
		}
		inline ObjectRef(T* value) : _value(value) {
			_value->incRefCount();
		}
		inline ~ObjectRef() {
			_value->decRefCount();
			if (!_value->getRefCount())
				delete _value;
		}
		inline T* operator*() { return _value; }
		inline const T* operator*() const { return _value; }
		inline const T* operator->() const { return _value; }
		inline T* operator->() { return _value; }

		inline ObjectRef& operator=(const ObjectRef& x) {
			this->_value = x._value;
			_value->incRefCount();
		}
		inline ObjectRef& operator=(const ObjectRef&& x) {
			this->_value = x._value;
			_value->incRefCount();
		}
	};
}

#endif
