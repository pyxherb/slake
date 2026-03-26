#include <slake/runtime.h>
#include <algorithm>

using namespace slake;

SLAKE_API ArrayObject::ArrayObject(Runtime *rt, peff::Alloc *self_allocator, const TypeRef &element_type, size_t element_size)
	: Object(rt, self_allocator, ObjectKind::Array),
	  element_type(element_type),
	  element_size(element_size) {
}

SLAKE_API ArrayObject::~ArrayObject() {
	if (data) {
		self_allocator->release(data, element_size * length, element_alignment);
	}
}

SLAKE_API ArrayObject *ArrayObject::alloc(Runtime *rt, const TypeRef &element_type, size_t element_size) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();
	std::unique_ptr<ArrayObject, peff::DeallocableDeleter<ArrayObject>> ptr(
		peff::alloc_and_construct<ArrayObject>(
			cur_generation_allocator.get(),
			alignof(ArrayObject),
			rt, cur_generation_allocator.get(), element_type, element_size));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void ArrayObject::dealloc() {
	peff::destroy_and_release<ArrayObject>(self_allocator.get(), this, alignof(ArrayObject));
}

InternalExceptionPointer slake::raise_invalid_array_index_error(Runtime *rt, size_t index) {
	return alloc_out_of_memory_error_if_alloc_failed(InvalidArrayIndexError::alloc(rt->get_fixed_alloc(), index));
}
