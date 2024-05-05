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

		Value *duplicate() const override;

		inline ArrayValue &operator=(const ArrayValue &x) {
			((Value &)*this) = (Value &)x;

			values.resize(x.values.size());
			for (size_t i = 0; i < x.values.size(); ++i) {
				values[i] = (VarValue *)x.values[i]->duplicate();
			}

			type = x.type;
			return *this;
		}
		VarValue &operator=(VarValue &&) = delete;
		ArrayValue &operator=(ArrayValue &&) = delete;
	};
}

#endif
