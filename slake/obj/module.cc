#include <slake/runtime.h>

using namespace slake;

SLAKE_API ModuleObject::ModuleObject(Runtime *rt, AccessModifier access)
	: MemberObject(rt) {
	scope = Scope::alloc(&rt->globalHeapPoolResource, this);
	this->accessModifier = access;
}

SLAKE_API ModuleObject::ModuleObject(const ModuleObject &x) : MemberObject(x) {
	imports = x.imports;
	unnamedImports = x.unnamedImports;
	name = x.name;
	parent = x.parent;
	scope = x.scope->duplicate();
}

SLAKE_API ModuleObject::~ModuleObject() {
	scope->dealloc();
}

SLAKE_API ObjectKind ModuleObject::getKind() const { return ObjectKind::Module; }

SLAKE_API Object *ModuleObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API MemberObject *ModuleObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return scope->getMember(name);
}

SLAKE_API const char *ModuleObject::getName() const {
	return name.c_str();
}

SLAKE_API void ModuleObject::setName(const char *name) {
	this->name = name;
}

SLAKE_API Object *ModuleObject::getParent() const {
	return parent;
}

SLAKE_API void ModuleObject::setParent(Object *parent) {
	this->parent = parent;
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt, AccessModifier access) {
	using Alloc = std::pmr::polymorphic_allocator<ModuleObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ModuleObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(const ModuleObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ModuleObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<ModuleObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.insert(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::ModuleObject::dealloc() {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
