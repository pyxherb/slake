#include <slake/runtime.h>

using namespace slake;

SLAKE_API ModuleObject::ModuleObject(Runtime *rt)
	: MemberObject(rt), members(&rt->globalHeapPoolAlloc), fieldRecords(&rt->globalHeapPoolAlloc), fieldRecordIndices(&rt->globalHeapPoolAlloc), unnamedImports(&rt->globalHeapPoolAlloc) {
}

SLAKE_API ModuleObject::ModuleObject(const ModuleObject &x, bool &succeededOut) : MemberObject(x, succeededOut), members(&x.associatedRuntime->globalHeapPoolAlloc), fieldRecords(&x.associatedRuntime->globalHeapPoolAlloc), fieldRecordIndices(&x.associatedRuntime->globalHeapPoolAlloc), unnamedImports(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (succeededOut) {
		if (!peff::copyAssign(fieldRecords, x.fieldRecords)) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < fieldRecords.size(); ++i) {
			if (!fieldRecordIndices.insert(fieldRecords.at(i).name, +i)) {
				succeededOut = false;
				return;
			}
		}
		if (!(this->localFieldStorage = (char *)x.associatedRuntime->globalHeapPoolAlloc.alloc(x.szLocalFieldStorage, sizeof(std::max_align_t)))) {
			succeededOut = false;
			return;
		}
		memcpy(this->localFieldStorage, x.localFieldStorage, x.szLocalFieldStorage);
		szLocalFieldStorage = x.szLocalFieldStorage;

		if (!peff::copyAssign(unnamedImports, x.unnamedImports)) {
			succeededOut = false;
			return;
		}
		if (!peff::copyAssign(name, x.name)) {
			succeededOut = false;
			return;
		}
		parent = x.parent;
		for (auto i = x.members.begin(); i != x.members.end(); ++i) {
			MemberObject *duplicatedMember = (MemberObject *)i.value()->duplicate();
			if (!duplicatedMember) {
				succeededOut = false;
				return;
			}
			if (!members.insert(duplicatedMember->name, +duplicatedMember)) {
				succeededOut = false;
				return;
			}
		}
	}
}

SLAKE_API ModuleObject::~ModuleObject() {
	if (this->localFieldStorage)
		associatedRuntime->globalHeapPoolAlloc.release(this->localFieldStorage, szLocalFieldStorage, sizeof(std::max_align_t));
}

SLAKE_API ObjectKind ModuleObject::getKind() const { return ObjectKind::Module; }

SLAKE_API Object *ModuleObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API EntityRef ModuleObject::getMember(const std::string_view &name) const {
	if (auto it = fieldRecordIndices.find(name); it != fieldRecordIndices.endConst()) {
		return EntityRef::makeFieldRef((ModuleObject *)this, it.value());
	}
	if (auto it = members.find(name); it != members.end()) {
		return EntityRef::makeObjectRef(it.value());
	}
	return EntityRef::makeObjectRef(nullptr);
}

SLAKE_API bool ModuleObject::addMember(MemberObject *member) {
	return members.insert(member->name, +member);
}

SLAKE_API void ModuleObject::removeMember(const std::string_view &name) {
	members.remove(name);
}

SLAKE_API bool ModuleObject::removeMemberAndTrim(const std::string_view& name) {
	return members.removeAndResizeBuckets(name);
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt) {
	std::unique_ptr<ModuleObject, util::DeallocableDeleter<ModuleObject>> ptr(
		peff::allocAndConstruct<ModuleObject>(&rt->globalHeapPoolAlloc, sizeof(std::max_align_t), rt));

	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(const ModuleObject *other) {
	return (ModuleObject *)other->duplicate();
}

SLAKE_API void slake::ModuleObject::dealloc() {
	peff::destroyAndRelease<ModuleObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
