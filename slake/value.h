#ifndef _SLAKE_VALUE_H_
#define _SLAKE_VALUE_H_

#include "types.h"
#include <memory>

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
		OBJECT
	};

	class IValue {
	public:
		virtual inline ~IValue() {}
		virtual ValueType getType() = 0;
	};

	template <typename T, int VT>
	class LiteralValue : public IValue {
	protected:
		T _value;

	public:
		inline LiteralValue(T value) { _value = value; }
		virtual inline ~LiteralValue() {}
		virtual inline ValueType getType() { return VT; }
		virtual inline std::uint8_t& getValue() { return _value; }
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

	class ObjectValue : public IValue {
	public:
		virtual inline ~ObjectValue() {}

		virtual inline ValueType getType() { return ValueType::OBJECT; }
		virtual std::shared_ptr<IValue> getMember(std::string name) = 0;
	};
}

#endif
