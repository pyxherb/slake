#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "base.h"
#include <deque>

namespace slake {
	class ArrayValue final : public Value {
	public:
		std::deque<Value*> values;
		Type type;

		inline ArrayValue(Runtime *rt, Type type);
		virtual ~ArrayValue();

		virtual inline Type getType() const override { return TypeId::Array; }

		ArrayValue &operator=(const ArrayValue &) = delete;
		ArrayValue &operator=(ArrayValue &&) = delete;
	};
}

#endif
