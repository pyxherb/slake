#ifndef _SLAKE_OBJ_TYPEDEF_H_
#define _SLAKE_OBJ_TYPEDEF_H_

#include <unordered_map>
#include <deque>

#include "object.h"

namespace slake {
	class HeapTypeObject final : public Object {
	public:
		TypeRef typeRef;

		SLAKE_API HeapTypeObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API HeapTypeObject(Duplicator *duplicator, const HeapTypeObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~HeapTypeObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<HeapTypeObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<HeapTypeObject> alloc(Duplicator *duplicator, const HeapTypeObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class CustomTypeDefObject final : public Object {
	public:
		Object *typeObject;

		SLAKE_API CustomTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API CustomTypeDefObject(Duplicator *duplicator, const CustomTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~CustomTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<CustomTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<CustomTypeDefObject> alloc(Duplicator *duplicator, const CustomTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE bool isLoadingDeferred() const noexcept {
			return typeObject->getObjectKind() == ObjectKind::IdRef;
		}
	};

	class ArrayTypeDefObject final : public Object {
	public:
		HeapTypeObject *elementType;

		SLAKE_API ArrayTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API ArrayTypeDefObject(Duplicator *duplicator, const ArrayTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~ArrayTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ArrayTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ArrayTypeDefObject> alloc(Duplicator *duplicator, const ArrayTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class RefTypeDefObject final : public Object {
	public:
		HeapTypeObject *referencedType;

		SLAKE_API RefTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API RefTypeDefObject(Duplicator *duplicator, const RefTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~RefTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<RefTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<RefTypeDefObject> alloc(Duplicator *duplicator, const RefTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class GenericArgTypeDefObject final : public Object {
	public:
		Object *ownerObject;
		StringObject *nameObject;

		SLAKE_API GenericArgTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API GenericArgTypeDefObject(Duplicator *duplicator, const GenericArgTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~GenericArgTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<GenericArgTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<GenericArgTypeDefObject> alloc(Duplicator *duplicator, const GenericArgTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class FnTypeDefObject final : public Object {
	public:
		HeapTypeObject *returnType = nullptr;
		peff::DynArray<HeapTypeObject *> paramTypes;
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
		peff::DynArray<HeapTypeObject *> paramTypes;
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
		peff::DynArray<HeapTypeObject *> elementTypes;

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
		HeapTypeObject *type;
		uint32_t width = 0;

		SLAKE_API SIMDTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API SIMDTypeDefObject(Duplicator *duplicator, const SIMDTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~SIMDTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<SIMDTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<SIMDTypeDefObject> alloc(Duplicator *duplicator, const SIMDTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class UnpackingTypeDefObject final : public Object {
	public:
		HeapTypeObject *type;

		SLAKE_API UnpackingTypeDefObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API UnpackingTypeDefObject(Duplicator *duplicator, const UnpackingTypeDefObject &x, peff::Alloc *allocator, bool &succeededOut);
		SLAKE_API virtual ~UnpackingTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<UnpackingTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<UnpackingTypeDefObject> alloc(Duplicator *duplicator, const UnpackingTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	struct TypeDefComparator {
		SLAKE_API int operator()(const Object *lhs, const Object *rhs) const noexcept;
	};

	struct TypeDefLtComparator {
		TypeDefComparator innerComparator;

		SLAKE_FORCEINLINE bool operator()(const Object* lhs, const Object* rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};
}

#endif
