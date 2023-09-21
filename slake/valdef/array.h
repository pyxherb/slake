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

		inline Value *at(uint32_t i) {
			if (i >= values.size())
				throw std::out_of_range("Out of array range");
			return *(values[i]);
		}

		size_t getSize() { return values.size(); }

		inline decltype(values)::iterator begin() { return values.begin(); }
		inline decltype(values)::iterator end() { return values.end(); }
		inline decltype(values)::const_iterator begin() const { return values.begin(); }
		inline decltype(values)::const_iterator end() const { return values.end(); }

		ArrayValue &operator=(const ArrayValue &) = delete;
		ArrayValue &operator=(ArrayValue &&) = delete;
	};
}

#endif
