#include "runtime.h"

using namespace slake;

SLAKE_API CountablePoolAlloc::CountablePoolAlloc(Runtime *runtime, peff::Alloc *upstream) : runtime(runtime), upstream(upstream) {}

SLAKE_API peff::UUID CountablePoolAlloc::getTypeId() const noexcept {
	return PEFF_UUID(1a4b6c8d, 0e2f, 4a6b, 8c1d, 2e4f6a8b0c2e);
}

SLAKE_API size_t CountablePoolAlloc::incRef(size_t globalRc) noexcept {
	return ++refCount;
}

SLAKE_API size_t CountablePoolAlloc::decRef(size_t globalRc) noexcept {
	if (!--refCount) {
		onRefZero();
		return 0;
	}

	return refCount;
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

SLAKE_API bool CountablePoolAlloc::isReplaceable(const peff::Alloc *rhs) const noexcept {
	if (getTypeId() != rhs->getTypeId())
		return false;

	CountablePoolAlloc *r = (CountablePoolAlloc *)rhs;

	if (runtime != r->runtime)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

SLAKE_API size_t GenerationalPoolAlloc::incRef(size_t globalRc) noexcept {
	++refCount;
#ifndef _NDEBUG
	if (!recordedRefPoints.insert(+globalRc, nullptr)) {
		puts("Error: error adding reference point!");
	}
#endif
	return refCount;
}

SLAKE_API size_t GenerationalPoolAlloc::decRef(size_t globalRc) noexcept {
	--refCount;
#ifndef _NDEBUG
	if (auto it = recordedRefPoints.find(+globalRc); it != recordedRefPoints.end()) {
		recordedRefPoints.remove(+globalRc);
	} else {
		std::terminate();
	}
#endif
	if (!refCount) {
		onRefZero();
		return 0;
	}
	return refCount;
}

SLAKE_API GenerationalPoolAlloc::GenerationalPoolAlloc(Runtime *runtime, peff::Alloc *upstream) : runtime(runtime), upstream(upstream)
#ifndef _NDEBUG
																								  ,
																								  recordedRefPoints(upstream)
#endif
{
}

SLAKE_API peff::UUID GenerationalPoolAlloc::getTypeId() const noexcept {
	return PEFF_UUID(3c2d4e6f, 8a0b, 2c4e, 6a8b, 0d2e4f6a8c1d);
}

SLAKE_API void GenerationalPoolAlloc::onRefZero() noexcept {
}

SLAKE_API void *GenerationalPoolAlloc::alloc(size_t size, size_t alignment) noexcept {
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	szAllocated += size;

	return p;
}

SLAKE_API void GenerationalPoolAlloc::release(void *p, size_t size, size_t alignment) noexcept {
	assert(size <= szAllocated);

	upstream->release(p, size, alignment);

	szAllocated -= size;
}

SLAKE_API bool GenerationalPoolAlloc::isReplaceable(const peff::Alloc *rhs) const noexcept {
	if (getTypeId() != rhs->getTypeId())
		return false;

	GenerationalPoolAlloc *r = (GenerationalPoolAlloc *)rhs;

	if (runtime != r->runtime)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

SLAKE_API peff::Alloc *Runtime::getCurGenAlloc() {
	return &youngAlloc;
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

SLAKE_API InternalExceptionPointer Runtime::_doCompareType(CompareTypeContext &context, int &resultOut) {
	while (context.frames.size() > 1) {
		CompareTypeFrame &frame = context.frames.back();

		switch (frame.kind) {
			case CompareTypeFrameKind::Normal: {
				NormalCompareTypeFrameExData &exData = std::get<NormalCompareTypeFrameExData>(frame.exData);

				if (exData.lhs.typeId < exData.rhs.typeId) {
					resultOut = -1;
					return {};
				}
				if (exData.lhs.typeId > exData.rhs.typeId) {
					resultOut = 1;
					return {};
				}
				switch (exData.rhs.typeId) {
					case TypeId::Instance: {
						auto lhsType = exData.lhs.getCustomTypeExData(), rhsType = exData.rhs.getCustomTypeExData();

						// TODO: Use comparison instead of the simple assert.
						assert(lhsType->objectKind == rhsType->objectKind);
						switch (lhsType->objectKind) {
							case ObjectKind::IdRef: {
								// Comparison between deferred resolving types are not allowed
								std::terminate();
							}
							case ObjectKind::Class:
							case ObjectKind::Interface: {
								if (lhsType < rhsType) {
									resultOut = -1;
									return {};
								}
								if (lhsType > rhsType) {
									resultOut = 1;
									return {};
								}
							}
							default:
								std::terminate();
						}

						context.frames.popBack();

						resultOut = 0;

						break;
					}
					case TypeId::Array: {
						NormalCompareTypeFrameExData newExData = { exData.lhs.getArrayExData(), exData.rhs.getArrayExData() };

						context.frames.popBack();

						if (!context.frames.pushBack(CompareTypeFrame(std::move(newExData)))) {
							context.frames.back().exceptionPtr = OutOfMemoryError::alloc();
						}

						break;
					}
					case TypeId::Ref: {
						NormalCompareTypeFrameExData newExData = { exData.lhs.getRefExData(), exData.rhs.getRefExData() };

						context.frames.popBack();

						if (!context.frames.pushBack(CompareTypeFrame(std::move(newExData)))) {
							context.frames.back().exceptionPtr = OutOfMemoryError::alloc();
						}

						break;
					}
					default:
						context.frames.popBack();
						resultOut = 0;
						break;
				}
			}
		}
	}
}

SLAKE_API InternalExceptionPointer Runtime::compareType(peff::Alloc *allocator, const Type &lhs, const Type &rhs, int &resultOut) {
	CompareTypeContext context(allocator);

	if (!context.frames.pushBack(CompareTypeFrame())) {
		context.frames.back().exceptionPtr = OutOfMemoryError::alloc();
	}

	SLAKE_RETURN_IF_EXCEPT(_doCompareType(context, resultOut));

	resultOut = context.frames.back().result;

	return {};
}

SLAKE_API Runtime::Runtime(peff::Alloc *selfAllocator, peff::Alloc *upstream, RuntimeFlags flags)
	: selfAllocator(selfAllocator),
	  fixedAlloc(this, upstream),
	  _flags(flags | _RT_INITING),
	  _genericCacheLookupTable(&fixedAlloc),
	  _genericCacheDir(&fixedAlloc),
	  managedThreadRunnables(&fixedAlloc),
	  parallelGcThreads(&fixedAlloc),
	  parallelGcThreadRunnables(&fixedAlloc),
	  youngAlloc(this, &fixedAlloc),
	  persistentAlloc(this, &fixedAlloc) {
	_flags &= ~_RT_INITING;
}

SLAKE_API Runtime::~Runtime() {
	_genericCacheDir.clear();
	_genericCacheLookupTable.clear();

	activeContexts.clear();
	managedThreadRunnables.clear();

	_flags |= _RT_DEINITING;

	gc();

	_releaseParallelGcResources();

	_rootObject = nullptr;

	// No need to delete the root object explicitly.

	assert(!youngObjectList);
	assert(!persistentObjectList);
	// Self allocator should be moved out in the dealloc() method, or the runtime has been destructed prematurely.
	assert(!selfAllocator);
}

SLAKE_API void Runtime::addSameKindObjectToList(Object **list, Object *object) {
	if (*list) {
		assert(!(*list)->prevSameKindObject);
		(*list)->prevSameKindObject = object;
	}

	object->nextSameKindObject = (*list);

	object->sameKindObjectList = list;

	*list = object;
}

SLAKE_API void Runtime::removeSameKindObjectToList(Object **list, Object *object) {
	if (object->nextSameKindObject) {
		object->nextSameKindObject->prevSameKindObject = object->prevSameKindObject;
	}

	if (object->prevSameKindObject) {
		object->prevSameKindObject->nextSameKindObject = object->nextSameKindObject;
	} else {
		assert(object == *list);

		*list = object->nextSameKindObject;
	}

	object->nextSameKindObject = nullptr;

	object->prevSameKindObject = nullptr;
}

SLAKE_API bool Runtime::addObject(Object *object) {
	if (youngObjectList) {
		assert(!youngObjectList->prevSameGenObject);
		youngObjectList->prevSameGenObject = object;
	}

	object->nextSameGenObject = youngObjectList;
	youngObjectList = object;

	++nYoungObjects;

	return true;
}

SLAKE_API bool Runtime::constructAt(Runtime *dest, peff::Alloc *upstream, RuntimeFlags flags) {
	peff::constructAt<Runtime>(dest, nullptr, upstream, flags);

	peff::ScopeGuard destroyGuard([dest]() noexcept {
		std::destroy_at<Runtime>(dest);
	});

	if (!(dest->_rootObject = ModuleObject::alloc(dest).get())) {
		return false;
	}

	if (!(dest->_allocParallelGcResources())) {
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
