#include <slake/runtime.h>

using namespace slake;

SLAKE_API MajorFrame::MajorFrame(Runtime *rt) noexcept
	: associated_runtime(rt) {
}

SLAKE_API void MajorFrame::dealloc() noexcept {
	peff::destroy_and_release<MajorFrame>(associated_runtime->get_fixed_alloc(), this, alignof(MajorFrame));
}

SLAKE_API char *Context::stack_alloc(size_t size) noexcept {
	if (size_t new_stack_top = stack_top + size;
		new_stack_top > stack_size) {
		return nullptr;
	} else
		stack_top = new_stack_top;

	return data_stack + stack_size - stack_top;
}

SLAKE_API char *Context::align_stack(size_t alignment) noexcept {
	const size_t addr_diff = (uintptr_t)(calc_stack_addr(data_stack, stack_size, stack_top)) % alignment;
	if (addr_diff)
		return stack_alloc(addr_diff);
	return data_stack + stack_size - stack_top;
}

SLAKE_API char *Context::aligned_stack_alloc(size_t size, size_t alignment) noexcept {
	const size_t addr_diff = (uintptr_t)(calc_stack_addr(data_stack, stack_size, stack_top)) % alignment;
	if (addr_diff)
		return stack_alloc(size + (addr_diff));
	return stack_alloc(size);
}

SLAKE_API Context::Context(Runtime *runtime, peff::Alloc *self_allocator) : runtime(runtime), self_allocator(self_allocator) {
}

SLAKE_API Context::~Context() {
	{
		MajorFrame *cur_major_frame;
		size_t off_cur_major_frame = this->off_cur_major_frame;

		if (off_cur_major_frame != SIZE_MAX)
			do {
				cur_major_frame = this->runtime->_fetch_major_frame(this, off_cur_major_frame);
				off_cur_major_frame = cur_major_frame->off_prev_frame;
				std::destroy_at(cur_major_frame);
			} while (off_cur_major_frame != SIZE_MAX);
	}

	if (data_stack) {
		self_allocator->release(data_stack, stack_size, sizeof(std::max_align_t));
	}
}

SLAKE_API void Context::replace_allocator(peff::Alloc *allocator) noexcept {
	peff::verify_replaceable(self_allocator.get(), allocator);

	self_allocator = allocator;

	MajorFrame *cur_major_frame;
	size_t off_cur_major_frame = this->off_cur_major_frame;

	do {
		cur_major_frame = this->runtime->_fetch_major_frame(this, off_cur_major_frame);
		cur_major_frame->replace_allocator(allocator);
		off_cur_major_frame = cur_major_frame->off_prev_frame;
	} while (cur_major_frame->off_prev_frame != SIZE_MAX);
}

SLAKE_API void Context::for_each_major_frame(MajorFrameWalker walker, void *user_data) {
	MajorFrame *cur_major_frame;
	size_t off_cur_major_frame = this->off_cur_major_frame;

	do {
		cur_major_frame = this->runtime->_fetch_major_frame(this, off_cur_major_frame);
		if (!walker(cur_major_frame, user_data))
			break;
		off_cur_major_frame = cur_major_frame->off_prev_frame;
	} while (cur_major_frame->off_prev_frame != SIZE_MAX);
}

SLAKE_API ContextObject::ContextObject(
	Runtime *rt,
	peff::Alloc *self_allocator)
	: Object(rt, self_allocator, ObjectKind::Context), _context(rt, self_allocator) {
}

SLAKE_API ContextObject::~ContextObject() {
}

SLAKE_API HostObjectRef<ContextObject> slake::ContextObject::alloc(Runtime *rt, size_t stack_size) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<ContextObject, peff::DeallocableDeleter<ContextObject>> ptr(
		peff::alloc_and_construct<ContextObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!(ptr->_context.data_stack = (char *)cur_generation_allocator->alloc(stack_size, sizeof(std::max_align_t))))
		return nullptr;

	ptr->_context.stack_size = stack_size;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API MajorFrame::~MajorFrame() {
}

SLAKE_API void slake::ContextObject::dealloc() {
	peff::destroy_and_release<ContextObject>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void MajorFrame::replace_allocator(peff::Alloc *allocator) noexcept {
}

SLAKE_API InternalExceptionPointer ContextObject::resume(HostRefHolder *host_ref_holder) {
	_context.flags &= ~CTX_YIELDED;
	return associated_runtime->exec_context(this);
}

SLAKE_API bool ContextObject::is_done() {
	return _context.flags & CTX_DONE;
}

SLAKE_API void ContextObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->Object::replace_allocator(allocator);

	_context.replace_allocator(allocator);
}
