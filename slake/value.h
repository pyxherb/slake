#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include <memory>
#include <mutex>
#include <unordered_set>

#include "types.h"

namespace Slake {
	class Object;
	extern std::unordered_set<Object*> _objectPool;
	extern std::mutex _objectPoolMutex;

	class Object {
	protected:
		std::uint32_t _uniqueId;
		static std::uint32_t _allocCounter;	   // Used for unique IDs.
		static std::mutex _allocCounterMutex;  // Used for allocation counter.

		// For object manager.
		std::size_t _refCount = 0;
		std::mutex _refCountMutex;

	public:
		inline Object() {
			_allocCounterMutex.lock();
			_uniqueId = _allocCounter++;
			_allocCounterMutex.unlock();

			_objectPoolMutex.lock();
			_objectPool.insert(this);
			_objectPoolMutex.unlock();
		}
		virtual inline ~Object() {
			_allocCounterMutex.lock();
			_objectPool.insert(this);
			_allocCounterMutex.unlock();
		}

		inline std::uint32_t getUniqueId() const { return _uniqueId; }
		virtual inline bool operator==(std::uint32_t uniqueId) const {
			return uniqueId == _uniqueId;
		}
		std::size_t getRefCount() const { return _refCount; }
		inline void incRef() {
			_refCountMutex.lock();
			_refCount++;
			_refCountMutex.unlock();
		}
		inline void decRef() {
			_refCountMutex.lock();
			_refCount--;
			_refCountMutex.unlock();
			if (!_refCount)
				delete this;
		}
	};

	class ObjectRef final {
	protected:
		Object* _object;
		std::uint32_t _uniqueId;

	public:
		inline ObjectRef(Object* object) {
			_object = object;
			_uniqueId = object->getUniqueId();
		}
		inline ~ObjectRef() {}
		std::uint32_t getUniqueId() const { return _uniqueId; }
		bool isValid() const {
			return (_objectPool.count(_object)) && (_object->getUniqueId() == _uniqueId);
		}
		const Object* operator->() const {
			if (isValid())
				return _object;
			throw std::logic_error("Dangling reference detected, the object may have been released");
		}
		Object* operator->() {
			if (isValid())
				return _object;
			throw std::logic_error("Dangling reference detected, the object may have been released");
		}
	};

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
		SCOPE_REF // For instructions only
	};

	class Value {
	public:
		virtual inline ~Value() {}
		virtual ValueType getType() const = 0;
	};

	template <typename T, int VT>
	class LiteralValue : public Value {
	protected:
		T _value;

	public:
		inline LiteralValue(T value) { _value = value; }
		virtual inline ~LiteralValue() {}
		virtual inline ValueType getType() const override { return (ValueType)VT; }
		virtual inline const T& getValue() const { return _value; }
		virtual inline void setValue(T& value) { _value = value; }
	};

	using I8Value = LiteralValue<std::int8_t, (int)ValueType::I8>;
	using I16Value = LiteralValue<std::int16_t, (int)ValueType::I16>;
	using I32Value = LiteralValue<std::int32_t, (int)ValueType::I32>;
	using I64Value = LiteralValue<std::int64_t, (int)ValueType::I64>;
	using U8Value = LiteralValue<std::uint8_t, (int)ValueType::U8>;
	using U16Value = LiteralValue<std::uint16_t, (int)ValueType::U16>;
	using U32Value = LiteralValue<std::uint32_t, (int)ValueType::U32>;
	using U64Value = LiteralValue<std::uint64_t, (int)ValueType::U64>;
	using StringValue = LiteralValue<std::string, (int)ValueType::STRING>;

	class ObjectValue : public Value {
	public:
		virtual inline ~ObjectValue() {}

		virtual inline ValueType getType() const override { return ValueType::OBJECT; }
		virtual std::shared_ptr<Value> getMember(std::string name) = 0;
	};

	class ScopeRefValue final : public Value {
	public:
		std::string name;
		std::shared_ptr<ScopeRefValue> next;

		virtual inline ~ScopeRefValue() {
		}
		virtual ValueType getType() const {
			return ValueType::SCOPE_REF;
		}
	};
}

#endif
