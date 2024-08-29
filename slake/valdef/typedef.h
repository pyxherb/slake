#ifndef _SLAKE_VALDEF_TYPEDEF_H_
#define _SLAKE_VALDEF_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class TypeDefObject final : public Object {
	public:
		Type type;

		inline TypeDefObject(Runtime *rt, const Type &type)
			: Object(rt), type(type) {
		}
		inline TypeDefObject(const TypeDefObject &x) : Object(x) {
			type = x.type.duplicate();
		}
		virtual inline ~TypeDefObject() {
		}

		virtual inline ObjectKind getKind() const override { return ObjectKind::Instance; }

		virtual Object *duplicate() const override;

		static HostObjectRef<TypeDefObject> alloc(Runtime *rt, const Type &type);
		static HostObjectRef<TypeDefObject> alloc(const TypeDefObject *other);
		virtual void dealloc() override;
	};
}

#endif
