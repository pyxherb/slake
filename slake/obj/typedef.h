#ifndef _SLAKE_OBJ_TYPEDEF_H_
#define _SLAKE_OBJ_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class TypeDefObject final : public Object {
	public:
		Type type = TypeId::Void;

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
		Type returnType = TypeId::Void;
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

	class ParamTypeListTypeDefObject final : public Object {
	public:
		peff::DynArray<Type> paramTypes;
		bool hasVarArg = false;

		SLAKE_API ParamTypeListTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API ParamTypeListTypeDefObject(Duplicator *duplicator, const ParamTypeListTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ParamTypeListTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ParamTypeListTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ParamTypeListTypeDefObject> alloc(Duplicator *duplicator, const ParamTypeListTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class TupleTypeDefObject final : public Object {
	public:
		peff::DynArray<Type> elementTypes;

		SLAKE_API TupleTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API TupleTypeDefObject(Duplicator *duplicator, const TupleTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~TupleTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<TupleTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<TupleTypeDefObject> alloc(Duplicator *duplicator, const TupleTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	class SIMDTypeDefObject final : public Object {
	public:
		Type type = TypeId::Void;
		uint32_t width = 0;

		SLAKE_API SIMDTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API SIMDTypeDefObject(Duplicator *duplicator, const SIMDTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~SIMDTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<SIMDTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<SIMDTypeDefObject> alloc(Duplicator *duplicator, const SIMDTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
