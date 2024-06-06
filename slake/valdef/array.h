#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "object.h"
#include "var.h"
#include <deque>

namespace slake {
	class ArrayObject final : public Object {
	public:
		std::deque<VarObject *> values;
		Type type;

		ArrayObject(Runtime *rt, Type type);
		virtual ~ArrayObject();

		virtual inline Type getType() const override { return Type(TypeId::Array, type); }

		Object *duplicate() const override;

		inline ArrayObject &operator=(const ArrayObject &x) {
			((Object &)*this) = (Object &)x;

			values.resize(x.values.size());
			for (size_t i = 0; i < x.values.size(); ++i) {
				values[i] = (VarObject *)x.values[i]->duplicate();
			}

			type = x.type;
			return *this;
		}
		VarObject &operator=(VarObject &&) = delete;
		ArrayObject &operator=(ArrayObject &&) = delete;
	};
}

#endif
