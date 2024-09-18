#include <slake/runtime.h>

using namespace slake;

slake::IdRefObject::IdRefObject(Runtime *rt)
	: Object(rt) {
}

IdRefObject::~IdRefObject() {
}

Object *IdRefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<IdRefObject> slake::IdRefObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<IdRefObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<IdRefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<IdRefObject> slake::IdRefObject::alloc(const IdRefObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<IdRefObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<IdRefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::IdRefObject::dealloc() {
	std::pmr::polymorphic_allocator<IdRefObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

std::string std::to_string(const slake::IdRefObject *ref) {
	string s;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &scope = ref->entries[i];

		if (i)
			s += ".";
		s += scope.name;

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += to_string(scope.genericArgs[j], ref->getRuntime());
			}
			s += ">";
		}
	}
	return s;
}
