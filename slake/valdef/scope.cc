#include <slake/runtime.h>

using namespace slake;

SLAKE_API Scope::Scope(std::pmr::memory_resource *memoryResource,
	Object *owner) : memoryResource(memoryResource),
					 owner(owner),
					 members(memoryResource) {}

SLAKE_API MemberObject *Scope::getMember(const std::pmr::string &name) {
	if (auto it = members.find(name); it != members.end())
		return it->second;
	return nullptr;
}

SLAKE_API void Scope::addMember(const std::pmr::string &name, MemberObject *value) {
	if (members.find(name) != members.end())
		throw std::logic_error("The member is already exists");

	putMember(name, value);
}

SLAKE_API void Scope::putMember(const std::pmr::string &name, MemberObject *value) {
	members[name] = value;
	value->setParent(owner);
	value->setName(name.c_str());
}

SLAKE_API void Scope::removeMember(const std::pmr::string &name) {
	if (auto it = members.find(name); it != members.end()) {
		it->second->setParent(nullptr);
		it->second->setName("");
		members.erase(it);
	}

	throw std::logic_error("No such member");
}

SLAKE_API Scope *Scope::alloc(std::pmr::memory_resource *memoryResource, Object *owner) {
	using Alloc = std::pmr::polymorphic_allocator<Scope>;
	Alloc allocator(memoryResource);

	std::unique_ptr<Scope, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), memoryResource, owner);

	return ptr.release();
}

SLAKE_API void Scope::dealloc() {
	std::pmr::polymorphic_allocator<Scope> allocator(memoryResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API Scope *Scope::duplicate() {
	std::unique_ptr<Scope, util::DeallocableDeleter<Scope>> newScope(alloc(memoryResource, owner));

	for (auto &i : members) {
		newScope->putMember(i.first, (MemberObject *)i.second->duplicate());
	}

	return newScope.release();
}

SLAKE_API MethodTable::MethodTable(std::pmr::memory_resource *memoryResource)
	: memoryResource(memoryResource),
	  methods(memoryResource),
	  destructors(memoryResource) {
}

SLAKE_API FnObject *MethodTable::getMethod(const std::pmr::string &name) {
	if (auto it = methods.find(name); it != methods.end())
		return it->second;
	return nullptr;
}

SLAKE_API MethodTable *MethodTable::alloc(std::pmr::memory_resource *memoryResource) {
	using Alloc = std::pmr::polymorphic_allocator<MethodTable>;
	Alloc allocator(memoryResource);

	std::unique_ptr<MethodTable, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), memoryResource);

	return ptr.release();
}

SLAKE_API void MethodTable::dealloc() {
	std::pmr::polymorphic_allocator<MethodTable> allocator(memoryResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API MethodTable *MethodTable::duplicate() {
	using Alloc = std::pmr::polymorphic_allocator<MethodTable>;
	Alloc allocator(memoryResource);

	std::unique_ptr<MethodTable, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *this);

	return ptr.release();
}
