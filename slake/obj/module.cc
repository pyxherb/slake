#include <slake/runtime.h>

using namespace slake;

SLAKE_API void FieldRecord::replaceAllocator(peff::Alloc* allocator) noexcept {
	name.replaceAllocator(allocator);
}

SLAKE_API ModuleObject::ModuleObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind)
	: MemberObject(rt, selfAllocator, objectKind), members(selfAllocator), localFieldStorage(selfAllocator), fieldRecords(selfAllocator), fieldRecordIndices(selfAllocator), unnamedImports(selfAllocator) {
}

SLAKE_API ModuleObject::ModuleObject(Duplicator *duplicator, const ModuleObject &x, peff::Alloc *allocator, bool &succeededOut) : MemberObject(x, allocator, succeededOut), members(allocator), fieldRecords(allocator), localFieldStorage(allocator), fieldRecordIndices(allocator), unnamedImports(allocator) {
	if (succeededOut) {
		if (!fieldRecords.resizeUninitialized(x.fieldRecords.size())) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < fieldRecords.size(); ++i) {
			peff::constructAt<FieldRecord>(&fieldRecords.at(i), allocator);
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

		if (!unnamedImports.resize(x.unnamedImports.size())) {
			succeededOut = false;
			return;
		}
		memcpy(unnamedImports.data(), x.unnamedImports.data(), unnamedImports.size() * sizeof(void *));
		if (!name.build(x.name)) {
			succeededOut = false;
			return;
		}
		parent = x.parent;
		for (auto i = x.members.begin(); i != x.members.end(); ++i) {
			if (!duplicator->insertTask(DuplicationTask::makeModuleMember(this, i.value()))) {
				succeededOut = false;
				return;
			}
		}
	}
}

SLAKE_API ModuleObject::~ModuleObject() {
}

SLAKE_API Object *ModuleObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API Reference ModuleObject::getMember(const std::string_view &name) const {
	if (auto it = fieldRecordIndices.find(name); it != fieldRecordIndices.endConst()) {
		return Reference::makeStaticFieldRef((ModuleObject *)this, it.value());
	}
	if (auto it = members.find(name); it != members.end()) {
		return Reference::makeObjectRef(it.value());
	}
	return Reference::makeInvalidRef();
}

SLAKE_API bool ModuleObject::addMember(MemberObject *member) {
	if (!members.insert(member->name, +member))
		return false;
	member->setParent(this);
	return true;
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

	associatedRuntime->writeVarUnsafe(Reference::makeStaticFieldRef(this, fieldRecords.size() - 1), associatedRuntime->defaultValueOf(fr.type));
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

SLAKE_API char *ModuleObject::appendTypedFieldSpace(const TypeRef &type) {
	return appendFieldSpace(associatedRuntime->sizeofType(type), associatedRuntime->alignofType(type));
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<ModuleObject, peff::DeallocableDeleter<ModuleObject>> ptr(
		peff::allocAndConstruct<ModuleObject>(curGenerationAllocator.get(), sizeof(std::max_align_t), rt, curGenerationAllocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Duplicator *duplicator, const ModuleObject *other) {
	return (ModuleObject *)other->duplicate(duplicator);
}

SLAKE_API void slake::ModuleObject::dealloc() {
	peff::destroyAndRelease<ModuleObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void ModuleObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	members.replaceAllocator(allocator);

	localFieldStorage.replaceAllocator(allocator);

	fieldRecords.replaceAllocator(allocator);

	for (auto& i : fieldRecords) {
		i.replaceAllocator(allocator);
	}

	fieldRecordIndices.replaceAllocator(allocator);

	unnamedImports.replaceAllocator(allocator);

	for (auto i : unnamedImports) {
		i->replaceAllocator(allocator);
	}
}
