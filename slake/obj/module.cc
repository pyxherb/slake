#include <slake/runtime.h>

using namespace slake;

SLAKE_API ModuleObject::ModuleObject(Runtime *rt)
	: MemberObject(rt), members(&rt->globalHeapPoolAlloc), localFieldStorage(&rt->globalHeapPoolAlloc), fieldRecords(&rt->globalHeapPoolAlloc), fieldRecordIndices(&rt->globalHeapPoolAlloc), unnamedImports(&rt->globalHeapPoolAlloc) {
}

SLAKE_API ModuleObject::ModuleObject(const ModuleObject &x, bool &succeededOut) : MemberObject(x, succeededOut), members(&x.associatedRuntime->globalHeapPoolAlloc), fieldRecords(&x.associatedRuntime->globalHeapPoolAlloc), localFieldStorage(&x.associatedRuntime->globalHeapPoolAlloc), fieldRecordIndices(&x.associatedRuntime->globalHeapPoolAlloc), unnamedImports(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (succeededOut) {
		if (!fieldRecords.resizeUninitialized(x.fieldRecords.size())) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < fieldRecords.size(); ++i) {
			peff::constructAt<FieldRecord>(&fieldRecords.at(i), &x.associatedRuntime->globalHeapPoolAlloc);
		}
		for (size_t i = 0; i < fieldRecords.size(); ++i) {
			FieldRecord &fr = fieldRecords.at(i);

			fr.accessModifier = x.fieldRecords.at(i).accessModifier;
			fr.offset = x.fieldRecords.at(i).offset;
			fr.type = x.fieldRecords.at(i).type;

			if (!fr.name.build(x.fieldRecords.at(i).name)) {
				succeededOut = false;
				return;
			}

			if (!fieldRecordIndices.insert(fr.name, +i)) {
				succeededOut = false;
				return;
			}
		}

		if (!localFieldStorage.resize(x.localFieldStorage.size())) {
			succeededOut = false;
			return;
		}
		memcpy(localFieldStorage.data(), x.localFieldStorage.data(), localFieldStorage.size());

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

SLAKE_API bool ModuleObject::removeMemberAndTrim(const std::string_view &name) {
	return members.removeAndResizeBuckets(name);
}

SLAKE_API bool ModuleObject::appendFieldRecord(FieldRecord &&fieldRecord) {
	if (!fieldRecords.pushBack(std::move(fieldRecord))) {
		return false;
	}
	FieldRecord &fr = fieldRecords.back();
	if (!fieldRecordIndices.insert(fr.name, fieldRecordIndices.size())) {
		fieldRecords.popBack();
		return false;
	}

	if (char *p = appendTypedFieldSpace(fr.type); p) {
		fr.offset = p - localFieldStorage.data();
	} else {
		fieldRecords.popBack();
		return false;
	}

	associatedRuntime->writeVar(EntityRef::makeFieldRef(this, fieldRecords.size() - 1), associatedRuntime->defaultValueOf(fr.type));
	return true;
}

SLAKE_API char *ModuleObject::appendFieldSpace(size_t size, size_t alignment) {
	size_t originalSize = localFieldStorage.size();
	size_t beginOff, sizeIncrement = 0;

	if (alignment > 1) {
		if (size_t diff = originalSize % alignment; diff) {
			beginOff = originalSize + (alignment - diff);
			sizeIncrement += size + (alignment - diff);
		}
	}

	beginOff = originalSize;
	sizeIncrement += size;

	if (!localFieldStorage.resize(originalSize + sizeIncrement)) {
		return nullptr;
	}

	return localFieldStorage.data() + beginOff;
}

SLAKE_API char *ModuleObject::appendTypedFieldSpace(const Type &type) {
	return appendFieldSpace(associatedRuntime->sizeofType(type), associatedRuntime->alignofType(type));
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
