#ifndef _SLAKE_OBJ_TYPEDEF_H_
#define _SLAKE_OBJ_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class TypeDefObject final : public Object {
	public:
		Type type;

		SLAKE_API TypeDefObject(Runtime *rt, const Type &type);
		SLAKE_API TypeDefObject(const TypeDefObject &x, bool &succeededOut);
		SLAKE_API virtual ~TypeDefObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<TypeDefObject> alloc(Runtime *rt, const Type &type);
		SLAKE_API static HostObjectRef<TypeDefObject> alloc(const TypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class FnTypeDefObject final : public Object {
	public:
		Type returnType;
		peff::DynArray<Type> paramTypes;
		bool hasVarArg = false;

		SLAKE_API FnTypeDefObject(Runtime *rt, const Type &returnType, peff::DynArray<Type> &&paramTypes);
		SLAKE_API FnTypeDefObject(const FnTypeDefObject &x, bool &succeededOut);
		SLAKE_API virtual ~FnTypeDefObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<FnTypeDefObject> alloc(Runtime *rt, const Type &type, peff::DynArray<Type> &&paramTypes);
		SLAKE_API static HostObjectRef<FnTypeDefObject> alloc(const FnTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
