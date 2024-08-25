#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "object.h"
#include "var.h"
#include <deque>

namespace slake {
	class ArrayObject final : public Object {
	public:
		ArrayObject(Runtime *rt, const Type &type);
		inline ArrayObject(const ArrayObject &x) : Object(x) {
			values.resize(x.values.size());
			for (size_t i = 0; i < x.values.size(); ++i) {
				values[i] = (BasicVarObject *)x.values[i]->duplicate();
			}

			type = x.type;
		}
		virtual ~ArrayObject();

		std::deque<BasicVarObject *> values;
		Type type;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Array; }

		Object *duplicate() const override;

		static HostObjectRef<ArrayObject> alloc(Runtime *rt, const Type &type);
		static HostObjectRef<ArrayObject> alloc(const ArrayObject *other);
		virtual void dealloc() override;
	};
}

#endif
