#include <slake/runtime.h>

using namespace slake;

SLAKE_API void FieldRecord::replaceAllocator(peff::Alloc *allocator) noexcept {
	name.replaceAllocator(allocator);
}

SLAKE_API BasicModuleObject::BasicModuleObject(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind)
	: MemberObject(rt, selfAllocator, objectKind),
	  members(selfAllocator),
	  localFieldStorage(selfAllocator),
	  fieldRecords(selfAllocator),
	  fieldRecordIndices(selfAllocator) {
}

SLAKE_API BasicModuleObject::BasicModuleObject(Duplicator *duplicator, const BasicModuleObject &x, peff::Alloc *allocator, bool &succeededOut)
	: MemberObject(x, allocator, succeededOut),
	  members(allocator),
	  fieldRecords(allocator),
	  localFieldStorage(allocator),
	  fieldRecordIndices(allocator) {
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
		for (auto i = x.members.begin(); i != x.members.end(); ++i) {
			if (!duplicator->insertTask(DuplicationTask::makeModuleMember(this, i.value()))) {
				succeededOut = false;
				return;
			}
		}
	}
}

SLAKE_API BasicModuleObject::~BasicModuleObject() {
}

SLAKE_API Reference BasicModuleObject::getMember(const std::string_view &name) const {
	if (auto it = fieldRecordIndices.find(name); it != fieldRecordIndices.endConst()) {
		return Reference::makeStaticFieldRef((BasicModuleObject *)this, it.value());
	}
	if (auto it = members.find(name); it != members.end()) {
		return Reference::makeObjectRef(it.value());
	}
	return Reference::makeInvalidRef();
}

SLAKE_API bool BasicModuleObject::addMember(MemberObject *member) {
	if (!members.insert(member->getName(), +member))
		return false;
	member->setParent(this);
	return true;
}

SLAKE_API bool BasicModuleObject::removeMember(const std::string_view &name) {
	return members.remove(name);
}

SLAKE_API bool BasicModuleObject::appendFieldRecord(FieldRecord &&fieldRecord) {
	if (!fieldRecords.pushBack(std::move(fieldRecord))) {
		return false;
	}
	FieldRecord &fr = fieldRecords.back();
	if (!fieldRecordIndices.insert(fr.name, fieldRecordIndices.size())) {
		if (!fieldRecords.popBack())
			fieldRecords.popBackWithoutShrink();
		return false;
	}

	if (char *p = appendTypedFieldSpace(fr.type); p) {
		fr.offset = p - localFieldStorage.data();
	} else {
		if (!fieldRecords.popBack())
			fieldRecords.popBackWithoutShrink();
		return false;
	}

	associatedRuntime->writeVarUnsafe(Reference::makeStaticFieldRef(this, fieldRecords.size() - 1), associatedRuntime->defaultValueOf(fr.type));
	return true;
}

SLAKE_API bool BasicModuleObject::appendFieldRecordWithoutAlloc(FieldRecord &&fieldRecord) {
	if (!fieldRecords.pushBack(std::move(fieldRecord))) {
		return false;
	}
	FieldRecord &fr = fieldRecords.back();
	if (!fieldRecordIndices.insert(fr.name, fieldRecordIndices.size())) {
		if (!fieldRecords.popBack())
			fieldRecords.popBackWithoutShrink();
		return false;
	}
	fr.offset = SIZE_MAX;

	return true;
}

SLAKE_API char *BasicModuleObject::appendFieldSpace(size_t size, size_t alignment) {
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

SLAKE_API char *BasicModuleObject::appendTypedFieldSpace(const TypeRef &type) {
	return appendFieldSpace(associatedRuntime->sizeofType(type), associatedRuntime->alignofType(type));
}

SLAKE_API bool BasicModuleObject::reallocFieldSpaces() noexcept {
	localFieldStorage.clear();
	for (auto &i : fieldRecords) {
		if (char *p = appendTypedFieldSpace(i.type); p) {
			i.offset = p - localFieldStorage.data();
		} else {
			localFieldStorage.clear();
			return false;
		}
	}
	return true;
}

SLAKE_API void BasicModuleObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	members.replaceAllocator(allocator);

	localFieldStorage.replaceAllocator(allocator);

	fieldRecords.replaceAllocator(allocator);

	for (auto &i : fieldRecords) {
		i.replaceAllocator(allocator);
	}

	fieldRecordIndices.replaceAllocator(allocator);
}

SLAKE_API ModuleObject::ModuleObject(Runtime *rt, peff::Alloc *selfAllocator)
	: BasicModuleObject(rt, selfAllocator, ObjectKind::Module), unnamedImports(selfAllocator) {
}

SLAKE_API ModuleObject::ModuleObject(Duplicator *duplicator, const ModuleObject &x, peff::Alloc *allocator, bool &succeededOut)
	: BasicModuleObject(duplicator, x, allocator, succeededOut), unnamedImports(allocator) {
	if (succeededOut) {
		if (!unnamedImports.resize(x.unnamedImports.size())) {
			succeededOut = false;
			return;
		}
		memcpy(unnamedImports.data(), x.unnamedImports.data(), unnamedImports.size() * sizeof(void *));
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
	this->BasicModuleObject::replaceAllocator(allocator);

	unnamedImports.replaceAllocator(allocator);

	for (auto i : unnamedImports) {
		i->replaceAllocator(allocator);
	}
}
