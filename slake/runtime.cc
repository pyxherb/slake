#include "runtime.h"

using namespace slake;

SLAKE_API CountablePoolAlloc slake::g_countablePoolDefaultAlloc(nullptr);

SLAKE_API CountablePoolAlloc::CountablePoolAlloc(peff::Alloc *upstream) : upstream(upstream) {}

SLAKE_API peff::Alloc *CountablePoolAlloc::getDefaultAlloc() const noexcept {
	return &g_countablePoolDefaultAlloc;
}

SLAKE_API void CountablePoolAlloc::onRefZero() noexcept {
}

SLAKE_API void *CountablePoolAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	szAllocated += size;

	return p;
}

SLAKE_API void CountablePoolAlloc::release(void *p, size_t size, size_t alignment) noexcept {
	assert(size <= szAllocated);
	upstream->release(p, size, alignment);

	szAllocated -= size;
}

SLAKE_API size_t Runtime::sizeofType(const Type &type) {
	switch (type.typeId) {
		case TypeId::I8:
			return sizeof(int8_t);
		case TypeId::I16:
			return sizeof(int16_t);
		case TypeId::I32:
			return sizeof(int32_t);
		case TypeId::I64:
			return sizeof(int64_t);
		case TypeId::U8:
			return sizeof(uint8_t);
		case TypeId::U16:
			return sizeof(uint16_t);
		case TypeId::U32:
			return sizeof(uint32_t);
		case TypeId::U64:
			return sizeof(uint64_t);
		case TypeId::F32:
			return sizeof(float);
		case TypeId::F64:
			return sizeof(double);
		case TypeId::Bool:
			return sizeof(bool);
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
		case TypeId::Ref:
			return sizeof(EntityRef);
		case TypeId::Any:
			return sizeof(Value);
		default:
			break;
	}
	std::terminate();
}

SLAKE_API size_t Runtime::alignofType(const Type &type) {
	switch (type.typeId) {
		case TypeId::I8:
			return sizeof(int8_t);
		case TypeId::I16:
			return sizeof(int16_t);
		case TypeId::I32:
			return sizeof(int32_t);
		case TypeId::I64:
			return sizeof(int64_t);
		case TypeId::U8:
			return sizeof(uint8_t);
		case TypeId::U16:
			return sizeof(uint16_t);
		case TypeId::U32:
			return sizeof(uint32_t);
		case TypeId::U64:
			return sizeof(uint64_t);
		case TypeId::F32:
			return sizeof(float);
		case TypeId::F64:
			return sizeof(double);
		case TypeId::Bool:
			return sizeof(bool);
		case TypeId::String:
		case TypeId::Instance:
		case TypeId::Array:
			return sizeof(void *);
		default:
			break;
	}
	std::terminate();
}

SLAKE_API Value Runtime::defaultValueOf(const Type &type) {
	switch (type.typeId) {
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
			return Value(slake::EntityRef::makeObjectRef(nullptr));
		default:
			break;
	}
	std::terminate();
}

SLAKE_API Runtime::Runtime(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags)
	: selfAllocator(selfAllocator),
	  globalHeapPoolAlloc(upstream),
	  _flags(flags | _RT_INITING),
	  _genericCacheLookupTable(&globalHeapPoolAlloc),
	  _genericCacheDir(&globalHeapPoolAlloc),
	  createdObjects(&globalHeapPoolAlloc),
	  managedThreadRunnables(&globalHeapPoolAlloc) {
	_flags &= ~_RT_INITING;
}

SLAKE_API Runtime::~Runtime() {
	_genericCacheDir.clear();
	_genericCacheLookupTable.clear();

	activeContexts.clear();
	managedThreadRunnables.clear();

	_flags |= _RT_DEINITING;

	gc();

	_rootObject = nullptr;

	// No need to delete the root object explicitly.

	assert(!createdObjects.size());
	assert(!globalHeapPoolAlloc.szAllocated);
	// Self allocator should be moved out in the dealloc() method, or the runtime has been destructed prematurely.
	assert(!selfAllocator);
}

SLAKE_API bool Runtime::constructAt(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags) {
	peff::constructAt<Runtime>(dest, nullptr, upstream, flags);
	peff::ScopeGuard destroyGuard([dest]() noexcept {
		std::destroy_at<Runtime>(dest);
	});
	if (!(dest->_rootObject = ModuleObject::alloc(dest).get())) {
		return false;
	}
	dest->_rootObject->setAccess(ACCESS_STATIC);
	destroyGuard.release();
	return true;
}

SLAKE_API Runtime *Runtime::alloc(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags) {
	Runtime *runtime = nullptr;

	if (!(runtime = (Runtime *)selfAllocator->alloc(sizeof(Runtime), alignof(Runtime)))) {
		return nullptr;
	}

	peff::ScopeGuard releaseGuard([runtime, selfAllocator]() noexcept {
		selfAllocator->release(runtime, sizeof(Runtime), alignof(Runtime));
	});

	if (!constructAt(runtime, upstream, flags)) {
		return nullptr;
	}
	runtime->selfAllocator = selfAllocator;

	releaseGuard.release();
	return runtime;
}

SLAKE_API void Runtime::dealloc() noexcept {
	peff::RcObjectPtr<peff::Alloc> selfAllocator = std::move(this->selfAllocator);
	std::destroy_at<Runtime>(this);
	if (selfAllocator) {
		selfAllocator->release(this, sizeof(Runtime), alignof(Runtime));
	}
}
