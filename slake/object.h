#ifndef _SLAKE_OBJECT_H_
#define _SLAKE_OBJECT_H_

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
		BOOL,
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

	template <typename T, ValueType VT>
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

	using I8Object = LiteralObject<std::int8_t, ValueType::I8>;
	using I16Object = LiteralObject<std::int16_t, ValueType::I16>;
	using I32Object = LiteralObject<std::int32_t, ValueType::I32>;
	using I64Object = LiteralObject<std::int64_t, ValueType::I64>;
	using U8Object = LiteralObject<std::uint8_t, ValueType::U8>;
	using U16Object = LiteralObject<std::uint16_t, ValueType::U16>;
	using U32Object = LiteralObject<std::uint32_t, ValueType::U32>;
	using U64Object = LiteralObject<std::uint64_t, ValueType::U64>;
	using FloatObject = LiteralObject<float, ValueType::FLOAT>;
	using DoubleObject = LiteralObject<double, ValueType::FLOAT>;
	using BoolObject = LiteralObject<bool, ValueType::BOOL>;
	using StringObject = LiteralObject<std::string, ValueType::STRING>;

	class ClassObject : public Object {
	public:
		virtual inline ~ClassObject() {}

		virtual inline ValueType getType() const override { return ValueType::OBJECT; }
		virtual Object* getMember(std::string name) = 0;
	};

	class RefObject final : public Object {
	public:
		std::string name;
		RefObject* next;

		RefObject(std::string name, RefObject* next);
		virtual ~RefObject();
		virtual inline ValueType getType() const override { return ValueType::SCOPE_REF; }
	};

	template <typename T = Object>
	class ObjectRef final {
	public:
		T* _value;

		inline ObjectRef(const ObjectRef<T>& x) : _value(x._value) {
			if (_value)
				_value->incRefCount();
		}
		inline ObjectRef(const ObjectRef<T>&& x) : _value(x._value) {
			if (_value)
				_value->incRefCount();
		}
		inline ObjectRef(T* value = nullptr) : _value(value) {
			if (_value)
				_value->incRefCount();
		}
		inline ~ObjectRef() {
			if (_value)
				_value->decRefCount();
		}
		inline T* operator*() { return _value; }
		inline const T* operator*() const { return _value; }
		inline const T* operator->() const { return _value; }
		inline T* operator->() { return _value; }

		template <typename T1 = T>
		T1* get() { return (T1*)_value; }
		template <typename T1 = T>
		const T1* get() const { return (T1*)_value; }

		inline ObjectRef& operator=(const ObjectRef& x) {
			if (_value)
				_value->decRefCount();
			this->_value = x._value;
			_value->incRefCount();
		}
		inline ObjectRef& operator=(const ObjectRef&& x) {
			if (_value)
				_value->decRefCount();
			this->_value = x._value;
			_value->incRefCount();
		}
		template <typename T1>
		inline ObjectRef& operator=(const ObjectRef<T1>&& x) {
			if (_value)
				_value->decRefCount();
			this->_value = x._value;
			_value->incRefCount();
		}
	};
}

#endif
