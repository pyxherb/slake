#ifndef _SLAKE_VALDEF_LITERAL_H_
#define _SLAKE_VALDEF_LITERAL_H_

#include "value.h"

namespace slake {
	template <typename T, TypeId VT>
	class LiteralValue final : public Value {
	protected:
		T _data;
		friend class Runtime;

	public:
		inline LiteralValue(Runtime *rt, T data) : Value(rt), _data(data) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
			if constexpr (std::is_same<T, std::string>::value) {
				reportSizeAllocatedToRuntime(data.size());
			}
		}
		virtual inline ~LiteralValue() {
			if constexpr (std::is_same<T, std::string>::value) {
				reportSizeFreedToRuntime(_data.size());
			}
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual inline Type getType() const override { return VT; }

		virtual inline const T &getData() const { return _data; }
		virtual inline void setData(T &data) {
			if constexpr (std::is_same<T, std::string>::value) {
				if(data.size() < _data.size()) {
					reportSizeFreedToRuntime(_data.size() - data.size());
				} else
					reportSizeAllocatedToRuntime(data.size() - _data.size());
			}
			_data = data;
		}

		virtual inline Value *duplicate() const override {
			decltype(this) v = new LiteralValue<T, VT>(getRuntime(), _data);
			(Value&)*v = (const Value&)*this;
			return (Value *)v;
		}

		LiteralValue &operator=(const LiteralValue &) = delete;
		LiteralValue &operator=(LiteralValue &&) = delete;
	};


	using I8Value = LiteralValue<std::int8_t, TypeId::I8>;
	using I16Value = LiteralValue<std::int16_t, TypeId::I16>;
	using I32Value = LiteralValue<std::int32_t, TypeId::I32>;
	using I64Value = LiteralValue<std::int64_t, TypeId::I64>;
	using U8Value = LiteralValue<uint8_t, TypeId::U8>;
	using U16Value = LiteralValue<uint16_t, TypeId::U16>;
	using U32Value = LiteralValue<uint32_t, TypeId::U32>;
	using U64Value = LiteralValue<uint64_t, TypeId::U64>;
	using F32Value = LiteralValue<float, TypeId::F32>;
	using F64Value = LiteralValue<double, TypeId::F64>;
	using BoolValue = LiteralValue<bool, TypeId::Bool>;
	using StringValue = LiteralValue<std::string, TypeId::String>;
	using WStringValue = LiteralValue<std::u32string, TypeId::WString>;
	using CharValue = LiteralValue<uint8_t, TypeId::Char>;
	using WCharValue = LiteralValue<char32_t, TypeId::WChar>;
	using TypeNameValue = LiteralValue<Type, TypeId::TypeName>;
}

#endif
