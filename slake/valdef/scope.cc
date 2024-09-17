#include <slake/runtime.h>

using namespace slake;

void Scope::putMember(const std::pmr::string &name, MemberObject *value) {
	members[name] = value;
	value->setParent(owner);
	value->setName(name.c_str());
}

void Scope::removeMember(const std::pmr::string &name) {
	if (auto it = members.find(name); it != members.end()) {
		it->second->setParent(nullptr);
		it->second->setName("");
		members.erase(it);
	}

	throw std::logic_error("No such member");
}

Scope *Scope::alloc(std::pmr::memory_resource *memoryResource, Object *owner) {
	using Alloc = std::pmr::polymorphic_allocator<Scope>;
	Alloc allocator(memoryResource);

	std::unique_ptr<Scope, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), memoryResource, owner);

	return ptr.release();
}

void Scope::dealloc() {
	std::pmr::polymorphic_allocator<Scope> allocator(memoryResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

Scope *Scope::duplicate() {
	std::unique_ptr<Scope, util::DeallocableDeleter<Scope>> newScope(alloc(memoryResource, owner));

	for (auto &i : members) {
		newScope->putMember(i.first, (MemberObject *)i.second->duplicate());
	}

	return newScope.release();
}

MethodTable *MethodTable::alloc(std::pmr::memory_resource *memoryResource) {
	using Alloc = std::pmr::polymorphic_allocator<MethodTable>;
	Alloc allocator(memoryResource);

	std::unique_ptr<MethodTable, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), memoryResource);

	return ptr.release();
}

void MethodTable::dealloc() {
	std::pmr::polymorphic_allocator<MethodTable> allocator(memoryResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

MethodTable *MethodTable::duplicate() {
	using Alloc = std::pmr::polymorphic_allocator<MethodTable>;
	Alloc allocator(memoryResource);

	std::unique_ptr<MethodTable, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *this);

	return ptr.release();
}
