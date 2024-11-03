#include <slake/runtime.h>

using namespace slake;

SLAKE_API IdRefEntry::IdRefEntry(std::pmr::memory_resource *memoryResource)
	: name(memoryResource), genericArgs(memoryResource), paramTypes(memoryResource) {
	// For resize() methods.
}

SLAKE_API IdRefEntry::IdRefEntry(std::pmr::string &&name,
	GenericArgList &&genericArgs,
	bool hasParamTypes,
	std::pmr::vector<Type> &&paramTypes,
	bool hasVarArg)
	: name(name),
	  genericArgs(genericArgs),
	  hasParamTypes(hasParamTypes),
	  paramTypes(paramTypes),
	  hasVarArg(hasVarArg) {}

SLAKE_API slake::IdRefObject::IdRefObject(Runtime *rt)
	: Object(rt) {
}

SLAKE_API IdRefObject::IdRefObject(const IdRefObject &x) : Object(x) {
	entries = x.entries;
}

SLAKE_API IdRefObject::~IdRefObject() {
}

SLAKE_API ObjectKind IdRefObject::getKind() const { return ObjectKind::IdRef; }

SLAKE_API Object *IdRefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<IdRefObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<IdRefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(const IdRefObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<IdRefObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<IdRefObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::IdRefObject::dealloc() {
	std::pmr::polymorphic_allocator<IdRefObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API std::string std::to_string(const slake::IdRefObject *ref) {
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
