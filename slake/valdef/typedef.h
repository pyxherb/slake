#ifndef _SLAKE_VALDEF_TYPEDEF_H_
#define _SLAKE_VALDEF_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class TypeDefObject final : public Object {
	public:
		Type type;

		TypeDefObject(Runtime *rt, const Type &type);
		TypeDefObject(const TypeDefObject &x);
		virtual ~TypeDefObject();

		virtual ObjectKind getKind() const override;

		virtual Object *duplicate() const override;

		static HostObjectRef<TypeDefObject> alloc(Runtime *rt, const Type &type);
		static HostObjectRef<TypeDefObject> alloc(const TypeDefObject *other);
		virtual void dealloc() override;
	};
}

#endif
