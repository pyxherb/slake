#ifndef _SLAKE_VALDEF_LITERAL_H_
#define _SLAKE_VALDEF_LITERAL_H_

#include "base.h"

namespace Slake {
	template <typename T, ValueType VT>
	class LiteralValue final : public Value {
	protected:
		T _value;

	public:
		inline LiteralValue(Runtime *rt, T value) : Value(rt), _value(value) {
			reportSizeToRuntime(sizeof(*this));
			if constexpr (std::is_same<T, std::string>::value) {
				reportSizeToRuntime((long)value.size());
			}
		}
		virtual inline ~LiteralValue() {}

		virtual inline Type getType() const override { return VT; }

		virtual inline const T &getValue() const { return _value; }
		virtual inline void setValue(T &value) {
			if constexpr (std::is_same<T, std::string>::value) {
				reportSizeToRuntime(((long)value.size()) - (long)_value.size());
			}
			_value = value;
		}

		virtual inline Value *copy() const override {
			decltype(this) v = new LiteralValue<T, VT>(getRuntime(), _value);
			return (Value *)v;
		}

		LiteralValue &operator=(const LiteralValue &) = delete;
		LiteralValue &operator=(const LiteralValue &&) = delete;

		virtual inline std::string toString() const override {
			if constexpr (std::is_same<T, std::string>::value)
				return Value::toString() + ",\"value\":\"" + _value + "\"";
			else
				return Value::toString() + ",\"value\":" + std::to_string(_value);
		}
	};
	

	using I8Value = LiteralValue<std::int8_t, ValueType::I8>;
	using I16Value = LiteralValue<std::int16_t, ValueType::I16>;
	using I32Value = LiteralValue<std::int32_t, ValueType::I32>;
	using I64Value = LiteralValue<std::int64_t, ValueType::I64>;
	using U8Value = LiteralValue<uint8_t, ValueType::U8>;
	using U16Value = LiteralValue<uint16_t, ValueType::U16>;
	using U32Value = LiteralValue<uint32_t, ValueType::U32>;
	using U64Value = LiteralValue<uint64_t, ValueType::U64>;
	using F32Value = LiteralValue<float, ValueType::F32>;
	using F64Value = LiteralValue<double, ValueType::F64>;
	using BoolValue = LiteralValue<bool, ValueType::BOOL>;
	using StringValue = LiteralValue<std::string, ValueType::STRING>;
}

#endif
