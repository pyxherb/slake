#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include <memory>
#include <mutex>
#include <unordered_set>

namespace Slake {
	class Object;
	extern std::unordered_set<Object*> _objectPool;
	extern std::mutex _objectPoolMutex;

	class Object {
	protected:
		// For object manager.
		std::size_t _refCount = 0;
		std::mutex _refCountMutex;

	public:
		inline Object() {}
		virtual inline ~Object() {
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
		virtual inline ValueType getType() const {
			return ValueType::SCOPE_REF;
		}
	};
}

#endif
