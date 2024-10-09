#include <slake/runtime.h>

using namespace slake;

ModuleObject::ModuleObject(Runtime *rt, AccessModifier access)
	: MemberObject(rt) {
	scope = Scope::alloc(&rt->globalHeapPoolResource, this);
	this->accessModifier = access;
}

ModuleObject::ModuleObject(const ModuleObject &x) : MemberObject(x) {
	imports = x.imports;
	unnamedImports = x.unnamedImports;
	name = x.name;
	parent = x.parent;
	scope = x.scope->duplicate();
}

ModuleObject::~ModuleObject() {
	scope->dealloc();
}

ObjectKind ModuleObject::getKind() const { return ObjectKind::Module; }

Object *ModuleObject::duplicate() const {
	return (Object *)alloc(this).get();
}

MemberObject* ModuleObject::getMember(
	const std::pmr::string& name,
	VarRefContext* varRefContextOut) const {
	return scope->getMember(name);
}

const char* ModuleObject::getName() const {
	return name.c_str();
}

void ModuleObject::setName(const char *name) {
	this->name = name;
}

Object *ModuleObject::getParent() const {
	return parent;
}

void ModuleObject::setParent(Object *parent) {
	this->parent = parent;
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt, AccessModifier access) {
	using Alloc = std::pmr::polymorphic_allocator<ModuleObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<ModuleObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt, access);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<ModuleObject> slake::ModuleObject::alloc(const ModuleObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<ModuleObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<ModuleObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::ModuleObject::dealloc() {
	std::pmr::polymorphic_allocator<ModuleObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
