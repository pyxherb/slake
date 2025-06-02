#include <slake/runtime.h>

using namespace slake;

SLAKE_API DuplicationTask DuplicationTask::makeNormal(Object **dest, Object *src) {
	DuplicationTask task;

	task.taskType = DuplicationTaskType::Normal;

	task.asNormal.src = src;
	task.asNormal.dest = dest;

	return task;
}

SLAKE_API DuplicationTask DuplicationTask::makeModuleMember(ModuleObject *mod, MemberObject *src) {
	DuplicationTask task;

	task.taskType = DuplicationTaskType::ModuleMember;

	task.asModuleMember.mod = mod;
	task.asModuleMember.src = src;

	return task;
}

SLAKE_API DuplicationTask DuplicationTask::makeType(Type *type, const Type &src) {
	DuplicationTask task;

	task.taskType = DuplicationTaskType::Type;

	task.asType.type = type;
	task.asType.src = src;

	return task;
}

SLAKE_API Duplicator::Duplicator(Runtime *runtime, peff::Alloc *allocator) : runtime(runtime), tasks(allocator) {
}

SLAKE_API bool Duplicator::insertTask(DuplicationTask &&task) {
	return tasks.pushBack(std::move(task));
}

SLAKE_API bool Duplicator::exec() {
	peff::List<DuplicationTask> tasks = std::move(this->tasks);

	this->tasks = peff::List<DuplicationTask>(tasks.allocator());

	for (auto &i : tasks) {
		switch (i.taskType) {
			case DuplicationTaskType::Normal:
				if (!(*i.asNormal.dest = i.asNormal.src->duplicate(this))) {
					return false;
				}
				break;
			case DuplicationTaskType::ModuleMember: {
				MemberObject *object = (MemberObject *)i.asModuleMember.src->duplicate(this);
				if (!object) {
					return false;
				}
				if (!(i.asModuleMember.mod->members.insert(object->name, +object))) {
					return false;
				}
				break;
			}
			case DuplicationTaskType::Type: {
				bool succeeded;
				(*i.asType.type) = i.asType.src.duplicate(succeeded);

				if (!succeeded) {
					return false;
				}
				break;
			}
		}
	}

	return true;
}

SLAKE_API Object::Object(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind) : associatedRuntime(rt), selfAllocator(selfAllocator), objectKind(objectKind) {
}

SLAKE_API Object::Object(const Object &x, peff::Alloc *allocator) {
	associatedRuntime = x.associatedRuntime;
	selfAllocator = allocator;
	objectKind = x.objectKind;
	_flags = x._flags & ~VF_WALKED;
}

SLAKE_API Object::~Object() {
	associatedRuntime->invalidateGenericCache(this);
}

SLAKE_API Object *Object::duplicate(Duplicator *duplicator) const {
	throw std::logic_error("duplicate() method is not supported");
}

SLAKE_API void Object::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(selfAllocator.get(), allocator);

	selfAllocator = allocator;
}

SLAKE_API EntityRef Object::getMember(const std::string_view &name) const {
	return EntityRef::makeObjectRef(nullptr);
}

SLAKE_API HostRefHolder::HostRefHolder(peff::Alloc *selfAllocator)
	: holdedObjects(selfAllocator) {
}

SLAKE_API HostRefHolder::~HostRefHolder() {
	for (auto i : holdedObjects)
		--i->hostRefCount;
}

SLAKE_API bool HostRefHolder::addObject(Object *object) {
	if (!holdedObjects.contains(object)) {
		if (!holdedObjects.insert(+object))
			return false;
		++object->hostRefCount;
	}
	return true;
}

SLAKE_API void HostRefHolder::removeObject(Object *object) noexcept {
	assert(holdedObjects.contains(object));
	holdedObjects.remove(object);
	--object->hostRefCount;
}
