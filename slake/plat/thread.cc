#include "../runtime.h"

using namespace slake;

SLAKE_API ManagedThread::ManagedThread(Runtime *associatedRuntime, ThreadKind threadKind) : associatedRuntime(associatedRuntime), threadKind(threadKind) {
}

SLAKE_API ManagedThread::~ManagedThread() {
}

void ExecutionThread::dealloc() {
	std::pmr::polymorphic_allocator<ExecutionThread> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

ExecutionThread *ExecutionThread::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<ExecutionThread>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<ExecutionThread, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}

void AttachedExecutionThread::dealloc() {
	std::pmr::polymorphic_allocator<AttachedExecutionThread> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

AttachedExecutionThread *AttachedExecutionThread::alloc(Runtime *associatedRuntime) {
	using Alloc = std::pmr::polymorphic_allocator<AttachedExecutionThread>;
	Alloc allocator(&associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<AttachedExecutionThread, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), associatedRuntime);

	return ptr.release();
}
