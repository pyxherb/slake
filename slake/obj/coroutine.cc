#include <slake/runtime.h>
#include <peff/base/scope_guard.h>

using namespace slake;

SLAKE_API CoroutineObject::CoroutineObject(Runtime *rt, peff::Alloc *self_allocator) : Object(rt, self_allocator, ObjectKind::Coroutine), cur_context(nullptr), bound_major_frame(nullptr), overloading(nullptr), stack_data(nullptr), len_stack_data(0), off_stack_top(0), off_regs(0) {
}

SLAKE_API CoroutineObject::~CoroutineObject() {
	release_stack_data();
}

SLAKE_API HostObjectRef<CoroutineObject> slake::CoroutineObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<CoroutineObject, peff::DeallocableDeleter<CoroutineObject>> ptr(
		peff::alloc_and_construct<CoroutineObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::CoroutineObject::dealloc() {
	peff::destroy_and_release<CoroutineObject>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API char *slake::CoroutineObject::alloc_stack_data(size_t size) {
	assert(!stack_data);
	if (size) {
		if (!(stack_data = (char *)self_allocator->alloc(size, 1))) {
			return nullptr;
		}
		len_stack_data = size;
		return stack_data;
	}
	return nullptr;
}

SLAKE_API void slake::CoroutineObject::release_stack_data() {
	if (stack_data) {
		self_allocator->release(stack_data, len_stack_data, 1);
		stack_data = nullptr;
		len_stack_data = 0;
	}
}

SLAKE_API void CoroutineObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);
}
