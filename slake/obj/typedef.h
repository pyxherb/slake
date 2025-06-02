#ifndef _SLAKE_OBJ_TYPEDEF_H_
#define _SLAKE_OBJ_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class TypeDefObject final : public Object {
	public:
		Type type = TypeId::None;

		SLAKE_API TypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API TypeDefObject(Duplicator *duplicator, const TypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~TypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<TypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<TypeDefObject> alloc(Duplicator *duplicator, const TypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class FnTypeDefObject final : public Object {
	public:
		Type returnType = TypeId::None;
		peff::DynArray<Type> paramTypes;
		bool hasVarArg = false;

		SLAKE_API FnTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API FnTypeDefObject(Duplicator *duplicator, const FnTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~FnTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<FnTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<FnTypeDefObject> alloc(Duplicator *duplicator, const FnTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
