#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "base.h"
#include <deque>

namespace Slake {
	class ArrayValue final : public Value {
	protected:
		std::deque<ValueRef<Value, false>> values;
		const Type type;

		friend class Runtime;

	public:
		inline ArrayValue(Runtime *rt, Type type)
			: Value(rt), type(type) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~ArrayValue() {
			for (auto i : values)
				delete *i;
		}
		virtual inline Type getType() const override { return ValueType::ARRAY; }
		inline Type getVarType() const { return type; }

		Value *operator[](uint32_t i) {
			if (i >= values.size())
				throw std::out_of_range("Out of array range");
			return *(values[i]);
		}

		size_t getSize() { return values.size(); }

		virtual inline std::string toString() const override {
			std::string s = Value::toString() + ",\"values\":[";

			for (size_t i = 0; i != values.size(); ++i) {
				s += (i ? "," : "") + values[i];
			}

			s += "[";
		}

		ArrayValue &operator=(const ArrayValue &) = delete;
		ArrayValue &operator=(const ArrayValue &&) = delete;
	};
}

#endif
