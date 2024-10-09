#ifndef _SLAKE_VALDEF_TYPEDEF_H_
#define _SLAKE_VALDEF_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class TypeDefObject final : public Object {
	public:
		Type type;

		SLAKE_API TypeDefObject(Runtime *rt, const Type &type);
		SLAKE_API TypeDefObject(const TypeDefObject &x);
		SLAKE_API virtual ~TypeDefObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<TypeDefObject> alloc(Runtime *rt, const Type &type);
		SLAKE_API static HostObjectRef<TypeDefObject> alloc(const TypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
