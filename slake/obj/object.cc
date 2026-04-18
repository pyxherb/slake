#include <slake/runtime.h>

using namespace slake;

SLAKE_API DuplicationTask DuplicationTask::make_normal(Object **dest, Object *src) {
	DuplicationTask task;

	task.task_type = DuplicationTaskType::Normal;

	task.as_normal.src = src;
	task.as_normal.dest = dest;

	return task;
}

SLAKE_API DuplicationTask DuplicationTask::make_module_member(BasicModuleObject *mod, MemberObject *src) {
	DuplicationTask task;

	task.task_type = DuplicationTaskType::ModuleMember;

	task.as_module_member.mod = mod;
	task.as_module_member.src = src;

	return task;
}

SLAKE_API DuplicationTask DuplicationTask::make_type(TypeRef *type, const TypeRef &src) {
	DuplicationTask task;

	task.task_type = DuplicationTaskType::Type;

	task.as_type.type = type;
	task.as_type.src = src;

	return task;
}

SLAKE_API Duplicator::Duplicator(Runtime *runtime, peff::Alloc *allocator) : runtime(runtime), tasks(allocator) {
}

SLAKE_API bool Duplicator::insert_task(DuplicationTask &&task) {
	return tasks.push_back(std::move(task));
}

SLAKE_API bool Duplicator::exec() {
	peff::List<DuplicationTask> tasks = std::move(this->tasks);

	this->tasks = peff::List<DuplicationTask>(tasks.allocator());

	for (auto &i : tasks) {
		switch (i.task_type) {
			case DuplicationTaskType::Normal:
				if (!(*i.as_normal.dest = i.as_normal.src->duplicate(this))) {
					return false;
				}
				break;
			case DuplicationTaskType::ModuleMember: {
				MemberObject *object = (MemberObject *)i.as_module_member.src->duplicate(this);
				if (!object) {
					return false;
				}
				if (!(i.as_module_member.mod->add_member(object))) {
					return false;
				}
				break;
			}
			case DuplicationTaskType::Type: {
				bool succeeded;
				(*i.as_type.type) = i.as_type.src.duplicate(succeeded);

				if (!succeeded) {
					return false;
				}
				break;
			}
		}
	}

	return true;
}

SLAKE_API Object::Object(Runtime *rt, peff::Alloc *self_allocator, ObjectKind object_kind) : associated_runtime(rt), _object_kind(object_kind) {
}

SLAKE_API Object::Object(const Object &x, peff::Alloc *allocator) {
	associated_runtime = x.associated_runtime;
	_object_kind = x._object_kind;
	object_flags = x.object_flags & ~VF_WALKED;
}

SLAKE_API Object::~Object() {
}

SLAKE_API Object *Object::duplicate(Duplicator *duplicator) const {
	SLAKE_REFERENCED_PARAM(duplicator);

	std::terminate();
}

SLAKE_API void Object::replace_allocator(peff::Alloc *allocator) noexcept {
}

SLAKE_API peff::Alloc *Object::get_allocator() const noexcept {
	return associated_runtime->get_generational_alloc(object_generation);
}

SLAKE_API Reference Object::get_member(const std::string_view &name) const {
	return ReferenceKind::Invalid;
}

SLAKE_API HostRefHolder::HostRefHolder(peff::Alloc *self_allocator)
	: holded_objects(self_allocator) {
}

SLAKE_API HostRefHolder::~HostRefHolder() {
	for (auto i : holded_objects)
		--i->host_ref_count;
}

SLAKE_API bool HostRefHolder::add_object(Object *object) {
	if (!holded_objects.contains(object)) {
		if (!holded_objects.insert(+object))
			return false;
		++object->host_ref_count;
	}
	return true;
}

SLAKE_API void HostRefHolder::remove_object(Object *object) noexcept {
	assert(holded_objects.contains(object));
	holded_objects.remove(object);
	--object->host_ref_count;
}
