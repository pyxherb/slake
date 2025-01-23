#include <slake/runtime.h>

using namespace slake;

SLAKE_API ModuleObject::ModuleObject(Runtime *rt, ScopeUniquePtr &&scope, AccessModifier access)
	: MemberObject(rt), scope(scope.release()) {
	this->scope->owner = this;
	this->accessModifier = access;
}

SLAKE_API ModuleObject::ModuleObject(const ModuleObject &x, bool &succeededOut) : MemberObject(x, succeededOut) {
	if (succeededOut) {
		if (!peff::copyAssign(imports, x.imports)) {
			succeededOut = false;
			return;
		}
		if (!peff::copyAssign(unnamedImports, x.unnamedImports)) {
			succeededOut = false;
			return;
		}
		if (!peff::copyAssign(name, x.name)) {
			succeededOut = false;
			return;
		}
		parent = x.parent;
		if (!(scope = x.scope->duplicate())) {
			succeededOut = false;
			return;
		}
		scope->owner = this;
	}
}

SLAKE_API ModuleObject::~ModuleObject() {
	scope->dealloc();
}

SLAKE_API ObjectKind ModuleObject::getKind() const { return ObjectKind::Module; }

SLAKE_API Object *ModuleObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API MemberObject *ModuleObject::getMember(
	const std::string_view &name,
	VarRefContext *varRefContextOut) const {
	return scope->getMember(name);
}

SLAKE_API Object *ModuleObject::getParent() const {
	return parent;
}

SLAKE_API void ModuleObject::setParent(Object *parent) {
	this->parent = parent;
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Runtime *rt, ScopeUniquePtr &&scope, AccessModifier access) {
	std::unique_ptr<ModuleObject, util::DeallocableDeleter<ModuleObject>> ptr(
		peff::allocAndConstruct<ModuleObject>(&rt->globalHeapPoolAlloc, sizeof(std::max_align_t), rt, std::move(scope), access));

	if (!ptr)
		return nullptr;

	if (!rt->createdObjects.pushBack(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(const ModuleObject *other) {
	return (ModuleObject *)other->duplicate();
}

SLAKE_API void slake::ModuleObject::dealloc() {
	peff::destroyAndRelease<ModuleObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}
