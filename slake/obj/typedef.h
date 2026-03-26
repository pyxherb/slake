#ifndef _SLAKE_OBJ_TYPEDEF_H_
#define _SLAKE_OBJ_TYPEDEF_H_

#include "object.h"
#include <peff/containers/dynarray.h>

namespace slake {
	enum class TypeDefKind : uint8_t {
		CustomTypeDef,		   // Custom type definition
		ArrayTypeDef,		   // Array type definition
		RefTypeDef,			   // Reference type definition
		GenericArgTypeDef,	   // Generic argument type definition
		FnTypeDef,			   // Function type definition
		ParamTypeListTypeDef,  // Parameter type list type definition
		TupleTypeDef,		   // Tuple type definition
		SIMDTypeDef,		   // SIMD type definition
		UnpackingTypeDef,	   // Unpacking type definition
	};

	class HeapTypeObject final : public Object {
	public:
		TypeRef type_ref;

		SLAKE_API HeapTypeObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API HeapTypeObject(Duplicator *duplicator, const HeapTypeObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~HeapTypeObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<HeapTypeObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<HeapTypeObject> alloc(Duplicator *duplicator, const HeapTypeObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class TypeDefObject : public Object {
	private:
		TypeDefKind _type_def_kind;

	public:
		SLAKE_API TypeDefObject(Runtime *rt, peff::Alloc *self_allocator, TypeDefKind type_def_kind);
		SLAKE_API TypeDefObject(Duplicator *duplicator, const TypeDefObject &x, peff::Alloc *allocator);
		SLAKE_API virtual ~TypeDefObject();

		SLAKE_FORCEINLINE TypeDefKind get_type_def_kind() const noexcept {
			return _type_def_kind;
		}
	};

	class CustomTypeDefObject final : public TypeDefObject {
	public:
		Object *type_object;

		SLAKE_API CustomTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API CustomTypeDefObject(Duplicator *duplicator, const CustomTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~CustomTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<CustomTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<CustomTypeDefObject> alloc(Duplicator *duplicator, const CustomTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE bool is_loading_deferred() const noexcept {
			return type_object->get_object_kind() == ObjectKind::IdRef;
		}
	};

	class ArrayTypeDefObject final : public TypeDefObject {
	public:
		HeapTypeObject *element_type;

		SLAKE_API ArrayTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API ArrayTypeDefObject(Duplicator *duplicator, const ArrayTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~ArrayTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ArrayTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ArrayTypeDefObject> alloc(Duplicator *duplicator, const ArrayTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class RefTypeDefObject final : public TypeDefObject {
	public:
		HeapTypeObject *referenced_type;

		SLAKE_API RefTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API RefTypeDefObject(Duplicator *duplicator, const RefTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~RefTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<RefTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<RefTypeDefObject> alloc(Duplicator *duplicator, const RefTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class GenericArgTypeDefObject final : public TypeDefObject {
	public:
		Object *owner_object;
		StringObject *name_object;

		SLAKE_API GenericArgTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API GenericArgTypeDefObject(Duplicator *duplicator, const GenericArgTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~GenericArgTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<GenericArgTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<GenericArgTypeDefObject> alloc(Duplicator *duplicator, const GenericArgTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class FnTypeDefObject final : public TypeDefObject {
	public:
		HeapTypeObject *return_type = nullptr;
		peff::DynArray<HeapTypeObject *> param_types;
		bool has_var_arg = false;

		SLAKE_API FnTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API FnTypeDefObject(Duplicator *duplicator, const FnTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~FnTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<FnTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<FnTypeDefObject> alloc(Duplicator *duplicator, const FnTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class ParamTypeListTypeDefObject final : public TypeDefObject {
	public:
		peff::DynArray<HeapTypeObject *> param_types;
		bool has_var_arg = false;

		SLAKE_API ParamTypeListTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API ParamTypeListTypeDefObject(Duplicator *duplicator, const ParamTypeListTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~ParamTypeListTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<ParamTypeListTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<ParamTypeListTypeDefObject> alloc(Duplicator *duplicator, const ParamTypeListTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class TupleTypeDefObject final : public TypeDefObject {
	public:
		peff::DynArray<HeapTypeObject *> element_types;

		SLAKE_API TupleTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API TupleTypeDefObject(Duplicator *duplicator, const TupleTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~TupleTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<TupleTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<TupleTypeDefObject> alloc(Duplicator *duplicator, const TupleTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};

	class SIMDTypeDefObject final : public TypeDefObject {
	public:
		HeapTypeObject *type;
		uint32_t width = 0;

		SLAKE_API SIMDTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API SIMDTypeDefObject(Duplicator *duplicator, const SIMDTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~SIMDTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<SIMDTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<SIMDTypeDefObject> alloc(Duplicator *duplicator, const SIMDTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	class UnpackingTypeDefObject final : public TypeDefObject {
	public:
		HeapTypeObject *type;

		SLAKE_API UnpackingTypeDefObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API UnpackingTypeDefObject(Duplicator *duplicator, const UnpackingTypeDefObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~UnpackingTypeDefObject();

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<UnpackingTypeDefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<UnpackingTypeDefObject> alloc(Duplicator *duplicator, const UnpackingTypeDefObject *other);
		SLAKE_API virtual void dealloc() override;
	};

	SLAKE_FORCEINLINE HeapTypeObject create_detached_heap_type_object() noexcept {
		return HeapTypeObject(nullptr, &peff::g_null_alloc);
	}

	template <typename T>
	SLAKE_FORCEINLINE T create_detached_type_def_object() noexcept {
		static_assert(std::is_base_of_v<TypeDefObject, T>);
		return T(nullptr, &peff::g_null_alloc);
	}

	struct TypeDefComparator {
		SLAKE_API int operator()(const TypeDefObject *lhs, const TypeDefObject *rhs) const noexcept;
	};

	struct TypeDefLtComparator {
		TypeDefComparator inner_comparator;

		SLAKE_FORCEINLINE bool operator()(const TypeDefObject *lhs, const TypeDefObject *rhs) const noexcept {
			return inner_comparator(lhs, rhs) < 0;
		}
	};

	SLAKE_FORCEINLINE CustomTypeDefObject *TypeRef::get_custom_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::CustomTypeDef);
		return static_cast<CustomTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE ArrayTypeDefObject *TypeRef::get_array_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::ArrayTypeDef);
		return static_cast<ArrayTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE RefTypeDefObject *TypeRef::get_ref_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::RefTypeDef);
		return static_cast<RefTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE GenericArgTypeDefObject *TypeRef::get_generic_arg_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::GenericArgTypeDef);
		return static_cast<GenericArgTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE FnTypeDefObject *TypeRef::get_fn_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::FnTypeDef);
		return static_cast<FnTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE ParamTypeListTypeDefObject *TypeRef::get_param_type_list_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::ParamTypeListTypeDef);
		return static_cast<ParamTypeListTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE TupleTypeDefObject *TypeRef::get_tuple_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::TupleTypeDef);
		return static_cast<TupleTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE SIMDTypeDefObject *TypeRef::get_simdtype_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::SIMDTypeDef);
		return static_cast<SIMDTypeDefObject *>(type_def);
	}

	SLAKE_FORCEINLINE UnpackingTypeDefObject *TypeRef::get_unpacking_type_def() const {
		assert(type_def->get_type_def_kind() == TypeDefKind::UnpackingTypeDef);
		return static_cast<UnpackingTypeDefObject *>(type_def);
	}
}

#endif
