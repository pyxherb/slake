#include <slake/runtime.h>

using namespace slake;

SLAKE_API void FieldRecord::replace_allocator(peff::Alloc *allocator) noexcept {
	name.replace_allocator(allocator);
}

SLAKE_API BasicModuleObject::BasicModuleObject(Runtime *rt, peff::Alloc *self_allocator, ObjectKind object_kind)
	: MemberObject(rt, self_allocator, object_kind),
	  members(self_allocator),
	  local_field_storage(self_allocator),
	  field_records(self_allocator),
	  field_record_indices(self_allocator) {
}

SLAKE_API BasicModuleObject::BasicModuleObject(Duplicator *duplicator, const BasicModuleObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: MemberObject(x, allocator, succeeded_out),
	  members(allocator),
	  field_records(allocator),
	  local_field_storage(allocator),
	  field_record_indices(allocator) {
	if (succeeded_out) {
		if (!field_records.resize_uninit(x.field_records.size())) {
			succeeded_out = false;
			return;
		}
		for (size_t i = 0; i < field_records.size(); ++i) {
			peff::construct_at<FieldRecord>(&field_records.at(i), allocator);
		}
		for (size_t i = 0; i < field_records.size(); ++i) {
			FieldRecord &fr = field_records.at(i);

			fr.access_modifier = x.field_records.at(i).access_modifier;
			fr.offset = x.field_records.at(i).offset;
			fr.type = x.field_records.at(i).type;

			if (!fr.name.build(x.field_records.at(i).name)) {
				succeeded_out = false;
				return;
			}

			if (!field_record_indices.insert(fr.name, +i)) {
				succeeded_out = false;
				return;
			}
		}

		if (!local_field_storage.resize(x.local_field_storage.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(local_field_storage.data(), x.local_field_storage.data(), local_field_storage.size());
		for (auto i = x.members.begin(); i != x.members.end(); ++i) {
			if (!duplicator->insert_task(DuplicationTask::make_module_member(this, i.value()))) {
				succeeded_out = false;
				return;
			}
		}
	}
}

SLAKE_API BasicModuleObject::~BasicModuleObject() {
}

SLAKE_API Reference BasicModuleObject::get_member(const std::string_view &name) const {
	if (auto it = field_record_indices.find(name); it != field_record_indices.end_const()) {
		return StaticFieldRef((BasicModuleObject *)this, it.value());
	}
	if (auto it = members.find(name); it != members.end()) {
		return Reference(it.value());
	}
	return ReferenceKind::Invalid;
}

SLAKE_API bool BasicModuleObject::add_member(MemberObject *member) {
	if (!members.shrink_buckets())
		return false;
	if (!members.insert(member->get_name(), +member))
		return false;
	member->set_parent(this);
	return true;
}

[[nodiscard]] SLAKE_API bool BasicModuleObject::shrink_member_storage() {
	return members.shrink_buckets();
}

SLAKE_API void BasicModuleObject::remove_member(const std::string_view &name) {
	members.remove(name);
}

SLAKE_API bool BasicModuleObject::append_field_record(FieldRecord &&field_record) {
	if (!field_records.push_back(std::move(field_record))) {
		return false;
	}
	FieldRecord &fr = field_records.back();
	if (!field_record_indices.insert(fr.name, field_record_indices.size())) {
		field_records.pop_back();
		return false;
	}

	if (char *p = append_typed_field_space(fr.type); p) {
		fr.offset = p - local_field_storage.data();
	} else {
		field_records.pop_back();
		return false;
	}

	Runtime::write_var(StaticFieldRef(this, field_records.size() - 1), associated_runtime->default_value_of(fr.type));
	return true;
}

SLAKE_API InternalExceptionPointer BasicModuleObject::append_field_record_with_value(FieldRecord &&field_record, const Value &value) {
	if (!field_records.push_back(std::move(field_record))) {
		return OutOfMemoryError::alloc();
	}
	FieldRecord &fr = field_records.back();
	if (!field_record_indices.insert(fr.name, field_record_indices.size())) {
		field_records.pop_back();
		return OutOfMemoryError::alloc();
	}

	if (char *p = append_typed_field_space(fr.type); p) {
		fr.offset = p - local_field_storage.data();
	} else {
		field_records.pop_back();
		return OutOfMemoryError::alloc();
	}

	SLAKE_RETURN_IF_EXCEPT(associated_runtime->write_var_checked(StaticFieldRef(this, field_records.size() - 1), value));
	return {};
}

SLAKE_API bool BasicModuleObject::append_field_record_without_alloc(FieldRecord &&field_record) {
	if (!field_records.push_back(std::move(field_record))) {
		return false;
	}
	FieldRecord &fr = field_records.back();
	if (!field_record_indices.insert(fr.name, field_record_indices.size())) {
		field_records.pop_back();
		return false;
	}
	fr.offset = SIZE_MAX;

	return true;
}

SLAKE_API char *BasicModuleObject::append_field_space(size_t size, size_t alignment) {
	size_t original_size = local_field_storage.size();
	size_t begin_off, size_increment = 0;

	if (alignment > 1) {
		if (size_t diff = original_size % alignment; diff) {
			begin_off = original_size + (alignment - diff);
			size_increment += size + (alignment - diff);
		}
	}

	begin_off = original_size;
	size_increment += size;

	if (!local_field_storage.resize(original_size + size_increment)) {
		return nullptr;
	}

	return local_field_storage.data() + begin_off;
}

SLAKE_API char *BasicModuleObject::append_typed_field_space(const TypeRef &type) {
	return append_field_space(Runtime::sizeof_type(type), Runtime::alignof_type(type));
}

SLAKE_API bool BasicModuleObject::realloc_field_spaces() noexcept {
	local_field_storage.clear();
	for (auto &i : field_records) {
		if (char *p = append_typed_field_space(i.type); p) {
			i.offset = p - local_field_storage.data();
		} else {
			local_field_storage.clear_and_shrink();
			return false;
		}
	}
	if (!local_field_storage.shrink_to_fit()) {
		local_field_storage.clear_and_shrink();
		return false;
	}
	return true;
}

SLAKE_API void BasicModuleObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replace_allocator(allocator);

	members.replace_allocator(allocator);

	local_field_storage.replace_allocator(allocator);

	field_records.replace_allocator(allocator);

	for (auto &i : field_records) {
		i.replace_allocator(allocator);
	}

	field_record_indices.replace_allocator(allocator);
}

SLAKE_API peff::Option<FieldRecord &> BasicModuleObject::get_field_record(const std::string_view &name) {
	if (auto it = field_record_indices.find(name); it != field_record_indices.end())
		return field_records.at(it.value());
	return peff::NULL_OPTION;
}

SLAKE_API peff::Option<const FieldRecord &> BasicModuleObject::get_field_record(const std::string_view &name) const {
	if (auto it = field_record_indices.find(name); it != field_record_indices.end())
		return field_records.at(it.value());
	return peff::NULL_OPTION;
}

SLAKE_API ModuleObject::ModuleObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::Module), unnamed_imports(self_allocator) {
}

SLAKE_API ModuleObject::ModuleObject(Duplicator *duplicator, const ModuleObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out), unnamed_imports(allocator) {
	if (succeeded_out) {
		if (!unnamed_imports.resize(x.unnamed_imports.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(unnamed_imports.data(), x.unnamed_imports.data(), unnamed_imports.size() * sizeof(void *));
		for (auto i = x.members.begin(); i != x.members.end(); ++i) {
			if (!duplicator->insert_task(DuplicationTask::make_module_member(this, i.value()))) {
				succeeded_out = false;
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
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<ModuleObject, peff::DeallocableDeleter<ModuleObject>> ptr(
		peff::alloc_and_construct<ModuleObject>(cur_generation_allocator.get(), sizeof(std::max_align_t), rt, cur_generation_allocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ModuleObject> slake::ModuleObject::alloc(Duplicator *duplicator, const ModuleObject *other) {
	return (ModuleObject *)other->duplicate(duplicator);
}

SLAKE_API void slake::ModuleObject::dealloc() {
	peff::destroy_and_release<ModuleObject>(self_allocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void ModuleObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);

	unnamed_imports.replace_allocator(allocator);

	for (auto i : unnamed_imports) {
		i->replace_allocator(allocator);
	}
}
