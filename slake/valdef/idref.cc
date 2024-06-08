#include <slake/runtime.h>

using namespace slake;

slake::IdRefObject::IdRefObject(Runtime *rt)
	: Object(rt) {
}

IdRefObject::~IdRefObject() {
}

Object *IdRefObject::duplicate() const {
	HostObjectRef<IdRefObject> v = IdRefObject::alloc(_rt);
	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<IdRefObject> slake::IdRefObject::alloc(Runtime *rt) {
	std::pmr::polymorphic_allocator<IdRefObject> allocator(&rt->globalHeapPoolResource);

	IdRefObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt);

	return ptr;
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
