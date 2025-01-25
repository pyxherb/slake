#include <slake/runtime.h>

using namespace slake;

SLAKE_API IdRefEntry::IdRefEntry(peff::Alloc *selfAllocator)
	: name(selfAllocator), genericArgs(selfAllocator), paramTypes(selfAllocator) {
	// For resize() methods.
}

SLAKE_API IdRefEntry::IdRefEntry(peff::String &&name,
	GenericArgList &&genericArgs,
	bool hasParamTypes,
	peff::DynArray<Type> &&paramTypes,
	bool hasVarArg)
	: name(std::move(name)),
	  genericArgs(std::move(genericArgs)),
	  hasParamTypes(hasParamTypes),
	  paramTypes(std::move(paramTypes)),
	  hasVarArg(hasVarArg) {}

SLAKE_API slake::IdRefObject::IdRefObject(Runtime *rt)
	: Object(rt),
	  entries(&rt->globalHeapPoolAlloc) {
}

SLAKE_API IdRefObject::IdRefObject(const IdRefObject &x, bool &succeededOut)
	: Object(x),
	  entries(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (!(peff::copyAssign(entries, x.entries))) {
		succeededOut = false;
		return;
	}
}

SLAKE_API IdRefObject::~IdRefObject() {
}

SLAKE_API ObjectKind IdRefObject::getKind() const { return ObjectKind::IdRef; }

SLAKE_API Object *IdRefObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(Runtime *rt) {
	std::unique_ptr<IdRefObject, util::DeallocableDeleter<IdRefObject>> ptr(
		peff::allocAndConstruct<IdRefObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));
	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(const IdRefObject *other) {
	bool succeeded = true;

	std::unique_ptr<IdRefObject, util::DeallocableDeleter<IdRefObject>> ptr(
		peff::allocAndConstruct<IdRefObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::IdRefObject::dealloc() {
	peff::destroyAndRelease<IdRefObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API std::string std::to_string(const slake::IdRefObject *ref) {
	string s;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &scope = ref->entries.at(i);

		if (i)
			s += ".";
		s += scope.name.data();

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += to_string(scope.genericArgs.at(j), ref->getRuntime());
			}
			s += ">";
		}
	}
	return s;
}
