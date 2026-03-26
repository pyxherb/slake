#include "runtime.h"

using namespace slake;

SLAKE_API CountablePoolAlloc::CountablePoolAlloc(Runtime *runtime, peff::Alloc *upstream) : runtime(runtime), upstream(upstream) {}

SLAKE_API peff::UUID CountablePoolAlloc::type_identity() const noexcept {
	return PEFF_UUID(1a4b6c8d, 0e2f, 4a6b, 8c1d, 2e4f6a8b0c2e);
}

SLAKE_API size_t CountablePoolAlloc::inc_ref(size_t global_rc) noexcept {
	SLAKE_REFERENCED_PARAM(global_rc);

	return ++ref_count;
}

SLAKE_API size_t CountablePoolAlloc::dec_ref(size_t global_rc) noexcept {
	SLAKE_REFERENCED_PARAM(global_rc);

	if (!--ref_count) {
		on_ref_zero();
		return 0;
	}

	return ref_count;
}

SLAKE_API void CountablePoolAlloc::on_ref_zero() noexcept {
}

SLAKE_API void *CountablePoolAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	sz_allocated += size;

	return p;
}

SLAKE_API void *CountablePoolAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	void *p = upstream->realloc(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

SLAKE_API void *CountablePoolAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	void *p = upstream->realloc_in_place(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

SLAKE_API void CountablePoolAlloc::release(void *p, size_t size, size_t alignment) noexcept {
	assert(size <= sz_allocated);

	upstream->release(p, size, alignment);

	sz_allocated -= size;
}

SLAKE_API bool CountablePoolAlloc::is_replaceable(const peff::Alloc *rhs) const noexcept {
	if (type_identity() != rhs->type_identity())
		return false;

	CountablePoolAlloc *r = (CountablePoolAlloc *)rhs;

	if (runtime != r->runtime)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

SLAKE_API size_t GenerationalPoolAlloc::inc_ref(size_t global_rc) noexcept {
	++ref_count;

	// if (global_rc == 1868)
	//	puts("");
#ifndef NDEBUG
	#if SLAKE_DEBUG_ALLOCATOR
	if (!recorded_ref_points.insert(+global_rc)) {
		puts("Error: error adding reference point!");
	}
	#endif
#endif
	return ref_count;
}

SLAKE_API size_t GenerationalPoolAlloc::dec_ref(size_t global_rc) noexcept {
	--ref_count;
#ifndef NDEBUG
	#if SLAKE_DEBUG_ALLOCATOR
	if (auto it = recorded_ref_points.find(+global_rc); it != recorded_ref_points.end()) {
		recorded_ref_points.remove(+global_rc);
	} else {
		std::terminate();
	}
	#endif
#endif
	if (!ref_count) {
		on_ref_zero();
		return 0;
	}
	return ref_count;
}

SLAKE_API GenerationalPoolAlloc::GenerationalPoolAlloc(Runtime *runtime, peff::Alloc *upstream) : runtime(runtime), upstream(upstream)
#ifndef _NDEBUG
																								  ,
																								  recorded_ref_points(upstream)
#endif
{
}

SLAKE_API peff::UUID GenerationalPoolAlloc::type_identity() const noexcept {
	return PEFF_UUID(3c2d4e6f, 8a0b, 2c4e, 6a8b, 0d2e4f6a8c1d);
}

SLAKE_API void GenerationalPoolAlloc::on_ref_zero() noexcept {
}

SLAKE_API void *GenerationalPoolAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	sz_allocated += size;

	return p;
}

SLAKE_API void *GenerationalPoolAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	void *p = upstream->realloc(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

SLAKE_API void *GenerationalPoolAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	void *p = upstream->realloc_in_place(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

SLAKE_API void GenerationalPoolAlloc::release(void *p, size_t size, size_t alignment) noexcept {
	assert(size <= sz_allocated);

	upstream->release(p, size, alignment);

	sz_allocated -= size;
}

SLAKE_API bool GenerationalPoolAlloc::is_replaceable(const peff::Alloc *rhs) const noexcept {
	if (type_identity() != rhs->type_identity())
		return false;

	GenerationalPoolAlloc *r = (GenerationalPoolAlloc *)rhs;

	if (runtime != r->runtime)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

SLAKE_API peff::Alloc *Runtime::get_cur_gen_alloc() {
	return &young_alloc;
}

SLAKE_API size_t Runtime::sizeof_type(const TypeRef &type) {
	switch (type.type_id) {
		case TypeId::I8:
			return sizeof(int8_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::I16:
			return sizeof(int16_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::I32:
			return sizeof(int32_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::I64:
			return sizeof(int64_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::U8:
			return sizeof(uint8_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::U16:
			return sizeof(uint16_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::U32:
			return sizeof(uint32_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::U64:
			return sizeof(uint64_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::F32:
			return sizeof(float) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::F64:
			return sizeof(double) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::Bool:
			return sizeof(bool) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::String:
			return sizeof(void *);
		case TypeId::Instance:
			return sizeof(void *);
		case TypeId::StructInstance: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::Struct);
			auto so = static_cast<StructObject *>(type.get_custom_type_def()->type_object);

			if (!so->cached_object_layout)
				std::terminate();

			return so->cached_object_layout->total_size + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		}
		case TypeId::ScopedEnum:{
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::ScopedEnum);
			auto so = static_cast<ScopedEnumObject *>(type.get_custom_type_def()->type_object);

			return sizeof_type(so->base_type) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		}
		case TypeId::TypelessScopedEnum:
			return sizeof(uint32_t) + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		case TypeId::UnionEnum: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnum);
			auto so = static_cast<UnionEnumObject *>(type.get_custom_type_def()->type_object);

			if (!so->cached_max_size)
				std::terminate();

			return so->cached_max_size + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		}
		case TypeId::UnionEnumItem: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnumItem);
			auto so = static_cast<UnionEnumItemObject *>(type.get_custom_type_def()->type_object);

			if (!so->cached_object_layout)
				std::terminate();

			return so->cached_object_layout->total_size + (type.type_modifier & TYPE_NULLABLE ? 1 : 0);
		}
		case TypeId::Array:
			return sizeof(void *);
		case TypeId::Any:
			return sizeof(Value);
		default:
			break;
	}
	std::terminate();
}

SLAKE_API size_t Runtime::alignof_type(const TypeRef &type) {
	switch (type.type_id) {
		case TypeId::I8:
			return alignof(int8_t);
		case TypeId::I16:
			return alignof(int16_t);
		case TypeId::I32:
			return alignof(int32_t);
		case TypeId::I64:
			return alignof(int64_t);
		case TypeId::U8:
			return alignof(uint8_t);
		case TypeId::U16:
			return alignof(uint16_t);
		case TypeId::U32:
			return alignof(uint32_t);
		case TypeId::U64:
			return alignof(uint64_t);
		case TypeId::F32:
			return alignof(float);
		case TypeId::F64:
			return alignof(double);
		case TypeId::Bool:
			return alignof(bool);
		case TypeId::String:
			return alignof(void *);
		case TypeId::Instance:
			return alignof(void *);
		case TypeId::StructInstance: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::Struct);
			auto so = static_cast<StructObject *>(type.get_custom_type_def()->type_object);

			if (!so->cached_object_layout)
				std::terminate();

			return so->cached_object_layout->alignment;
		}
		case TypeId::ScopedEnum:{
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::ScopedEnum);
			auto so = static_cast<ScopedEnumObject *>(type.get_custom_type_def()->type_object);

			return alignof_type(so->base_type);
		}
		case TypeId::TypelessScopedEnum:
			return alignof(uint32_t);
		case TypeId::UnionEnum: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnum);
			auto so = static_cast<UnionEnumObject *>(type.get_custom_type_def()->type_object);

			if (!so->cached_max_align)
				std::terminate();

			return so->cached_max_align;
		}
		case TypeId::UnionEnumItem: {
			assert(type.get_custom_type_def()->type_object->get_object_kind() == ObjectKind::UnionEnumItem);
			auto so = static_cast<UnionEnumItemObject *>(type.get_custom_type_def()->type_object);

			if (!so->cached_object_layout)
				std::terminate();

			return so->cached_object_layout->alignment;
		}
		case TypeId::Array:
			return alignof(void *);
		case TypeId::TempRef:
			return alignof(void *);
		default:
			break;
	}
	std::terminate();
}

SLAKE_API Value Runtime::default_value_of(const TypeRef &type) const {
	switch (type.type_id) {
		case TypeId::I8:
			return Value((int8_t)0);
		case TypeId::I16:
			return Value((int16_t)0);
		case TypeId::I32:
			return Value((int32_t)0);
		case TypeId::I64:
			return Value((int64_t)0);
		case TypeId::U8:
			return Value((uint8_t)0);
		case TypeId::U16:
			return Value((uint16_t)0);
		case TypeId::U32:
			return Value((uint32_t)0);
		case TypeId::U64:
			return Value((uint64_t)0);
		case TypeId::F32:
			return Value((float)0);
		case TypeId::F64:
			return Value((double)0);
		case TypeId::Bool:
			return Value(false);
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return Value(slake::Reference(nullptr));
		case TypeId::Ref:
			return Value(slake::ObjectFieldRef(nullptr, UINT32_MAX));
			break;
		case TypeId::TempRef:
			// TODO: Replace it with TempRef version.
			return Value(slake::ObjectFieldRef(nullptr, UINT32_MAX));
			break;
		default:
			break;
	}
	std::terminate();
}

SLAKE_API InternalExceptionPointer Runtime::load_deferred_custom_type_def(CustomTypeDefObject *custom_type_def) {
	IdRefObject *id_ref_object = (IdRefObject *)custom_type_def->type_object;

	slake::Reference entity_ref;
	SLAKE_RETURN_IF_EXCEPT(resolve_id_ref(id_ref_object, entity_ref));

	if (!entity_ref)
		return ReferencedMemberNotFoundError::alloc(get_fixed_alloc(), id_ref_object);

	if (entity_ref.kind != ReferenceKind::ObjectRef)
		std::terminate();

	custom_type_def->type_object = entity_ref.as_object;

	return {};
}

SLAKE_API Runtime::Runtime(peff::Alloc *self_allocator, peff::Alloc *upstream, RuntimeFlags flags)
	: self_allocator(self_allocator),
	  fixed_alloc(this, upstream),
	  runtime_flags(flags | _RT_INITING),
	  _generic_cache_lookup_table(&fixed_alloc),
	  _generic_cache_dir(&fixed_alloc),
	  managed_thread_runnables(&fixed_alloc),
	  young_alloc(this, &fixed_alloc),
	  persistent_alloc(this, &fixed_alloc),
	  type_defs(&fixed_alloc) {
	runtime_flags &= ~_RT_INITING;
}

SLAKE_API Runtime::~Runtime() {
	_generic_cache_dir.clear();
	_generic_cache_lookup_table.clear();

	active_contexts.clear();
	managed_thread_runnables.clear();

	runtime_flags |= _RT_DEINITING;

	_root_object = nullptr;

	_sz_computed_gc_limit = 0;
	_sz_computed_persistent_gc_limit = 0;
	gc();

	_release_parallel_gc_resources();

	// No need to delete the root object explicitly.

	assert(!fixed_alloc.sz_allocated);
	assert(!young_object_list);
	assert(!persistent_object_list);
	// Self allocator should be moved out in the dealloc() method, or the runtime has been destructed prematurely.
	assert(!self_allocator);
}

SLAKE_API TypeDefObject *Runtime::get_equal_type_def(TypeDefObject *type_def) const noexcept {
	if (auto it = type_defs.find(type_def); it != type_defs.end()) {
		return *it;
	}

	return nullptr;
}

SLAKE_API void Runtime::unregister_type_def(TypeDefObject *type_def) noexcept {
	type_defs.remove(type_def);
}

SLAKE_API InternalExceptionPointer Runtime::register_type_def(TypeDefObject *type_def) noexcept {
	if (type_defs.contains(+type_def))
		std::terminate();

	if (!type_defs.insert(+type_def))
		return OutOfMemoryError::alloc();

	return {};
}

SLAKE_API bool Runtime::add_object(Object *object) noexcept {
	if (young_object_list) {
		assert(!young_object_list->prev_same_gen_object);
		young_object_list->prev_same_gen_object = object;
	}

	object->gc_status = ObjectGCStatus::Unwalked;

	object->next_same_gen_object = young_object_list;
	young_object_list = object;

	++num_young_objects;

	return true;
}

SLAKE_API void Runtime::remove_object(Object *object) noexcept {
	if (young_object_list == object) {
		young_object_list = object->prev_same_gen_object ? object->prev_same_gen_object : object->next_same_gen_object;
	}
	if (persistent_object_list == object) {
		persistent_object_list = object->prev_same_gen_object ? object->prev_same_gen_object : object->next_same_gen_object;
	}

	if (object->prev_same_gen_object)
		object->prev_same_gen_object->next_same_gen_object = object->next_same_gen_object;
	if (object->next_same_gen_object)
		object->next_same_gen_object->prev_same_gen_object = object->prev_same_gen_object;

	switch (object->object_generation) {
		case ObjectGeneration::Young:
			--num_young_objects;
			break;
		case ObjectGeneration::Persistent:
			--num_persistent_objects;
			break;
		default:
			std::terminate();
	}
}

SLAKE_API bool Runtime::construct_at(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags) {
	peff::construct_at<Runtime>(dest, nullptr, upstream, flags);

	peff::ScopeGuard destroy_guard([dest]() noexcept {
		std::destroy_at<Runtime>(dest);
	});

	if (!(dest->_root_object = ModuleObject::alloc(dest).get())) {
		return false;
	}

	if (!(dest->_alloc_parallel_gc_resources())) {
		return false;
	}

	dest->_root_object->set_access(ACCESS_STATIC);

	destroy_guard.release();

	return true;
}

SLAKE_API Runtime *Runtime::alloc(peff::Alloc *self_allocator, peff::Alloc *upstream, RuntimeFlags flags) {
	Runtime *runtime = nullptr;

	if (!(runtime = (Runtime *)self_allocator->alloc(sizeof(Runtime), alignof(Runtime)))) {
		return nullptr;
	}

	peff::ScopeGuard release_guard([runtime, self_allocator]() noexcept {
		self_allocator->release(runtime, sizeof(Runtime), alignof(Runtime));
	});

	if (!construct_at(runtime, upstream, flags)) {
		return nullptr;
	}
	runtime->self_allocator = self_allocator;

	release_guard.release();
	return runtime;
}

SLAKE_API void Runtime::dealloc() noexcept {
	peff::RcObjectPtr<peff::Alloc> self_allocator = std::move(this->self_allocator);
	std::destroy_at<Runtime>(this);
	if (self_allocator) {
		self_allocator->release(this, sizeof(Runtime), alignof(Runtime));
	}
}
