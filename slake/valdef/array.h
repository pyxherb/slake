#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "object.h"
#include "var.h"
#include <deque>

namespace slake {
	class ArrayObject final : public Object {
	public:
		ArrayObject(Runtime *rt, const Type &type);
		virtual ~ArrayObject();

		std::deque<VarObject *> values;
		Type type;

		virtual inline Type getType() const override { return Type::makeArrayTypeName(type, 1); }

		Object *duplicate() const override;

		static HostObjectRef<ArrayObject> alloc(Runtime *rt, const Type &type);
		virtual void dealloc() override;

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
