#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "base.h"
#include "var.h"
#include <deque>

namespace slake {
	class ArrayValue final : public Value {
	public:
		std::deque<VarValue *> values;
		Type type;

		ArrayValue(Runtime *rt, Type type);
		virtual ~ArrayValue();

		virtual inline Type getType() const override { return Type(TypeId::Array, type); }

		ArrayValue &operator=(const ArrayValue &) = delete;
		ArrayValue &operator=(ArrayValue &&) = delete;
	};
}

#endif
