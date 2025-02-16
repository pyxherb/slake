#include <slake/runtime.h>

using namespace slake;

SLAKE_API Scope::Scope(
	peff::Alloc *selfAllocator,
	Object *owner) : selfAllocator(selfAllocator),
					 owner(owner),
					 members(selfAllocator) {}

SLAKE_API MemberObject *Scope::getMember(const std::string_view &name) {
	if (auto it = members.find(name); it != members.end())
		return it.value();
	return nullptr;
}

SLAKE_API bool Scope::putMember(MemberObject *value) {
	bool result = members.insert(value->name, std::move(value));

	if (!result)
		return false;

	value->setParent(owner);
	return true;
}

SLAKE_API void Scope::removeMember(const std::string_view &name) {
	members.remove(name);
}

SLAKE_API bool Scope::removeMemberConservative(const std::string_view &name) {
	return members.removeAndResizeBuckets(name);
}

SLAKE_API Scope *Scope::alloc(peff::Alloc *selfAllocator, Object *owner) {
	return peff::allocAndConstruct<Scope>(selfAllocator, sizeof(std::max_align_t), selfAllocator, owner);
}

SLAKE_API void Scope::dealloc() {
	peff::destroyAndRelease<Scope>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API Scope *Scope::duplicate() {
	std::unique_ptr<Scope, util::DeallocableDeleter<Scope>> newScope(alloc(selfAllocator.get(), owner));
	if (!newScope)
		return nullptr;

	for (auto i = members.begin(); i != members.end(); ++i) {
		MemberObject *duplicatedMember = (MemberObject *)i.value()->duplicate();
		if (!duplicatedMember) {
			return nullptr;
		}
		if (!newScope->putMember(duplicatedMember))
			return nullptr;
	}

	return newScope.release();
}

SLAKE_API MethodTable::MethodTable(peff::Alloc *selfAllocator)
	: selfAllocator(selfAllocator),
	  methods(selfAllocator) {
}

SLAKE_API FnObject *MethodTable::getMethod(const std::string_view &name) {
	if (auto it = methods.find(name); it != methods.end())
		return it.value();
	return nullptr;
}

SLAKE_API MethodTable *MethodTable::alloc(peff::Alloc *selfAllocator) {
	return peff::allocAndConstruct<MethodTable>(selfAllocator, sizeof(std::max_align_t), selfAllocator);
}

SLAKE_API void MethodTable::dealloc() {
	peff::destroyAndRelease<MethodTable>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API MethodTable *MethodTable::duplicate() {
	std::unique_ptr<MethodTable, util::DeallocableDeleter<MethodTable>> newMethodTable(alloc(selfAllocator.get()));
	if (!newMethodTable)
		return nullptr;

	if (!peff::copyAssign(newMethodTable->methods, methods)) {
		return nullptr;
	}
	if (!peff::copyAssign(newMethodTable->nativeDestructors, nativeDestructors)) {
		return nullptr;
	}

	return newMethodTable.release();
}
