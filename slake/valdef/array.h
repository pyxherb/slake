#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "base.h"
#include <deque>

namespace slake {
	class ArrayValue final : public Value {
	protected:
		std::deque<ValueRef<Value, false>> values;
		Type type;

		friend class Runtime;

	public:
		inline ArrayValue(Runtime *rt, Type type)
			: Value(rt), type(type) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual inline ~ArrayValue() {
			for (auto i : values)
				delete *i;
		}
		virtual inline Type getType() const override { return TypeId::ARRAY; }
		inline Type getVarType() const { return type; }

		Value *operator[](uint32_t i) {
			if (i >= values.size())
				throw std::out_of_range("Out of array range");
			return *(values[i]);
		}

		size_t getSize() { return values.size(); }

		ArrayValue &operator=(const ArrayValue &) = delete;
		ArrayValue &operator=(const ArrayValue &&) = delete;
	};
}

#endif
