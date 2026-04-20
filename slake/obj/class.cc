#include <slake/runtime.h>
#include <variant>

using namespace slake;

SLAKE_API void ObjectFieldRecord::replace_allocator(peff::Alloc *allocator) noexcept {
	name.replace_allocator(allocator);
}

SLAKE_API void ObjectLayout::replace_allocator(peff::Alloc *allocator) noexcept {
	peff::verify_replaceable(self_allocator.get(), allocator);

	self_allocator = allocator;

	field_records.replace_allocator(allocator);

	for (auto &i : field_records) {
		i.replace_allocator(allocator);
	}

	field_name_map.replace_allocator(allocator);

	field_record_init_module_fields_number.replace_allocator(allocator);
}

SLAKE_API ObjectLayout::ObjectLayout(peff::Alloc *self_allocator)
	: self_allocator(self_allocator),
	  field_records(self_allocator),
	  field_name_map(self_allocator),
	  field_record_init_module_fields_number(self_allocator) {
}

SLAKE_API ObjectLayout *ObjectLayout::duplicate(peff::Alloc *allocator) const {
	std::unique_ptr<ObjectLayout, peff::DeallocableDeleter<ObjectLayout>> ptr(alloc(allocator));
	if (!ptr)
		return nullptr;

	if (!ptr->field_records.resize_uninit(field_records.size())) {
		return nullptr;
	}
	if (!ptr->field_record_init_module_fields_number.resize_uninit(field_record_init_module_fields_number.size())) {
		return nullptr;
	}
	for (size_t i = 0; i < field_record_init_module_fields_number.size(); ++i)
		ptr->field_record_init_module_fields_number.at(i) = field_record_init_module_fields_number.at(i);
	for (size_t i = 0; i < field_records.size(); ++i) {
		peff::construct_at<ObjectFieldRecord>(&ptr->field_records.at(i), self_allocator.get());
	}
	for (size_t i = 0; i < field_records.size(); ++i) {
		ObjectFieldRecord &fr = ptr->field_records.at(i);

		if (!fr.name.build(field_records.at(i).name)) {
			return nullptr;
		}
		fr.offset = field_records.at(i).offset;
		fr.idx_init_field_record = field_records.at(i).idx_init_field_record;
		fr.type = field_records.at(i).type;

		if (!ptr->field_name_map.insert(fr.name, +i)) {
			return nullptr;
		}
	}
	ptr->total_size = total_size;

	return ptr.release();
}

SLAKE_API ObjectLayout *ObjectLayout::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<ObjectLayout>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API void ObjectLayout::dealloc() {
	peff::destroy_and_release<ObjectLayout>(self_allocator.get(), this, alignof(ObjectLayout));
}

SLAKE_API MethodTable::MethodTable(peff::Alloc *self_allocator)
	: self_allocator(self_allocator),
	  methods(self_allocator),
	  destructors(self_allocator) {
}

SLAKE_API FnObject *MethodTable::get_method(const std::string_view &name) {
	if (auto it = methods.find(name); it != methods.end())
		return it.value();
	return nullptr;
}

SLAKE_API MethodTable *MethodTable::alloc(peff::Alloc *self_allocator) {
	return peff::alloc_and_construct<MethodTable>(self_allocator, sizeof(std::max_align_t), self_allocator);
}

SLAKE_API void MethodTable::dealloc() {
	peff::destroy_and_release<MethodTable>(self_allocator.get(), this, alignof(MethodTable));
}

SLAKE_API void MethodTable::replace_allocator(peff::Alloc *allocator) noexcept {
	peff::verify_replaceable(self_allocator.get(), allocator);

	self_allocator = allocator;

	methods.replace_allocator(allocator);

	destructors.replace_allocator(allocator);
}

SLAKE_API MethodTable *MethodTable::duplicate(peff::Alloc *allocator) {
	std::unique_ptr<MethodTable, peff::DeallocableDeleter<MethodTable>> new_method_table(alloc(allocator));
	if (!new_method_table)
		return nullptr;

	for (auto [k, v] : methods) {
		if (!new_method_table->methods.insert(std::string_view(v->get_name()), +v))
			return nullptr;
	}

	for (auto i : destructors) {
		if (!new_method_table->destructors.push_back(+i))
			return nullptr;
	}

	return new_method_table.release();
}

SLAKE_API Object *ClassObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API slake::ClassObject::ClassObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::Class),
	  generic_args(self_allocator),
	  mapped_generic_args(self_allocator),
	  generic_params(self_allocator),
	  mapped_generic_params(self_allocator),
	  impl_types(self_allocator) {
}

SLAKE_API const peff::DynArray<Value> *ClassObject::get_generic_args() const {
	return &generic_args;
}

SLAKE_API ClassObject::ClassObject(Duplicator *duplicator, const ClassObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out),
	  generic_args(allocator),
	  mapped_generic_args(allocator),  // No need to copy
	  generic_params(allocator),
	  mapped_generic_params(allocator),	 // No need to copy
	  impl_types(allocator) {
	if (succeeded_out) {
		class_flags = x.class_flags;

		if (!generic_args.resize(x.generic_args.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(generic_args.data(), x.generic_args.data(), generic_args.size() * sizeof(Value));
		for (auto [k, v] : x.mapped_generic_args) {
			peff::String name(allocator);

			if (!name.build(k)) {
				succeeded_out = false;
				return;
			}

			if (!(mapped_generic_args.insert(std::move(name), Value(v)))) {
				succeeded_out = false;
				return;
			}
		}
		if (!generic_params.resize_uninit(x.generic_params.size())) {
			succeeded_out = false;
			return;
		}
		for (size_t i = 0; i < x.generic_params.size(); ++i) {
			if (!x.generic_params.at(i).copy(generic_params.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroy_at<GenericParam>(&generic_params.at(j - 1));
				}
				succeeded_out = false;
				return;
			}
		}
		if (!impl_types.resize(x.impl_types.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(impl_types.data(), x.impl_types.data(), impl_types.size() * sizeof(TypeRef));

		base_type = x.base_type;

		// DO NOT copy the cached instantiated method table.
	}
}

SLAKE_API ClassObject::~ClassObject() {
}

SLAKE_API bool ClassObject::has_implemented(InterfaceObject *p_interface) const {
	for (auto &i : impl_types) {
		InterfaceObject *interface_object = (InterfaceObject *)i.get_custom_type_def()->type_object;
		if (interface_object->is_derived_from(p_interface))
			return true;
	}
	return false;
}

SLAKE_API bool ClassObject::is_base_of(const ClassObject *p_class) const {
	const ClassObject *i = p_class;
	while (true) {
		if (i == this)
			return true;

		if (i->base_type.type_id == TypeId::Void)
			break;
		auto parent_class_object = (i->base_type.get_custom_type_def())->type_object;
		assert(parent_class_object->get_object_kind() == ObjectKind::Class);
		i = (ClassObject *)parent_class_object;
	}

	return false;
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Duplicator *duplicator, const ClassObject *other) {
	bool succeeded = true;

	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<ClassObject, peff::DeallocableDeleter<ClassObject>> ptr(
		peff::alloc_and_construct<ClassObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ClassObject> slake::ClassObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<ClassObject, peff::DeallocableDeleter<ClassObject>> ptr(
		peff::alloc_and_construct<ClassObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::ClassObject::dealloc() {
	peff::destroy_and_release<ClassObject>(get_allocator(), this, alignof(ClassObject));
}

SLAKE_API void ClassObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);

	generic_args.replace_allocator(allocator);

	mapped_generic_params.replace_allocator(allocator);
	mapped_generic_args.replace_allocator(allocator);

	generic_params.replace_allocator(allocator);

	for (auto &i : generic_params) {
		i.replace_allocator(allocator);
	}

	impl_types.replace_allocator(allocator);

	if (cached_instantiated_method_table)
		cached_instantiated_method_table->replace_allocator(allocator);

	if (cached_object_layout)
		cached_object_layout->replace_allocator(allocator);
}

SLAKE_API InterfaceObject::InterfaceObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::Interface),
	  generic_args(self_allocator),
	  mapped_generic_args(self_allocator),
	  generic_params(self_allocator),
	  mapped_generic_params(self_allocator),
	  impl_types(self_allocator),
	  impl_interface_indices(self_allocator) {
}

SLAKE_API InterfaceObject::InterfaceObject(Duplicator *duplicator, const InterfaceObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out),
	  generic_args(allocator),
	  mapped_generic_args(allocator),  // No need to copy
	  generic_params(allocator),
	  mapped_generic_params(allocator),	 // No need to copy
	  impl_types(allocator),
	  impl_interface_indices(allocator) {
	if (succeeded_out) {
		if (!generic_params.resize_uninit(x.generic_params.size())) {
			succeeded_out = false;
			return;
		}
		for (size_t i = 0; i < x.generic_params.size(); ++i) {
			if (!x.generic_params.at(i).copy(generic_params.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroy_at<GenericParam>(&generic_params.at(j - 1));
				}
				succeeded_out = false;
				return;
			}
		}

		if (!impl_types.resize(x.impl_types.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(impl_types.data(), x.impl_types.data(), impl_types.size() * sizeof(TypeRef));
	}
}

struct UpdateInterfaceInheritanceRelationshipFrame {
	InterfaceObject *interface_object;
	size_t index;
};

struct UpdateInterfaceInheritanceRelationshipContext {
	peff::List<UpdateInterfaceInheritanceRelationshipFrame> frames;

	SLAKE_FORCEINLINE UpdateInterfaceInheritanceRelationshipContext(peff::Alloc *allocator) : frames(allocator) {}
};

SLAKE_FORCEINLINE static InternalExceptionPointer _update_interface_inheritance_relationship(InterfaceObject *interface_object, UpdateInterfaceInheritanceRelationshipContext &context) noexcept {
	if (!context.frames.push_back({ interface_object, 0 }))
		return OutOfMemoryError::alloc();

	while (context.frames.size()) {
		UpdateInterfaceInheritanceRelationshipFrame &cur_frame = context.frames.back();

		InterfaceObject *interface_object = cur_frame.interface_object;
		// Check if the interface has cyclic inheritance.
		if (!cur_frame.index) {
			for (auto &i : context.frames) {
				if ((&i != &cur_frame) && (i.interface_object == cur_frame.interface_object))
					std::terminate();
			}
		}
		if (cur_frame.index >= interface_object->impl_types.size()) {
			if (!interface_object->impl_interface_indices.insert(+interface_object))
				return OutOfMemoryError::alloc();
			context.frames.pop_back();
			continue;
		}

		TypeRef type_ref = interface_object->impl_types.at(cur_frame.index);

		// TODO: Return a malformed interface exception.
		if ((!type_ref.type_def) || (type_ref.type_def->get_type_def_kind() != TypeDefKind::CustomTypeDef))
			std::terminate();

		CustomTypeDefObject *td = type_ref.get_custom_type_def();

		// TODO: Return a malformed interface exception.
		if (td->type_object->get_object_kind() != ObjectKind::Interface)
			std::terminate();

		if (!context.frames.push_back({ (InterfaceObject *)td->type_object, 0 }))
			return OutOfMemoryError::alloc();

		++cur_frame.index;
	}

	return {};
}

SLAKE_API InternalExceptionPointer InterfaceObject::update_inheritance_relationship(peff::Alloc *allocator) noexcept {
	invalidate_inheritance_relationship_cache();

	UpdateInterfaceInheritanceRelationshipContext context(allocator);

	return _update_interface_inheritance_relationship(this, context);
}

SLAKE_API bool InterfaceObject::is_derived_from(InterfaceObject *p_interface) const {
	if (p_interface == this)
		return true;

	return impl_interface_indices.contains(p_interface);
}

SLAKE_API const peff::DynArray<Value> *InterfaceObject::get_generic_args() const {
	return &generic_args;
}

SLAKE_API InterfaceObject::~InterfaceObject() {
}

SLAKE_API Object *InterfaceObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<InterfaceObject, peff::DeallocableDeleter<InterfaceObject>> ptr(
		peff::alloc_and_construct<InterfaceObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt,
			cur_generation_allocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<InterfaceObject> slake::InterfaceObject::alloc(Duplicator *duplicator, const InterfaceObject *other) {
	bool succeeded = true;

	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<InterfaceObject, peff::DeallocableDeleter<InterfaceObject>> ptr(
		peff::alloc_and_construct<InterfaceObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::InterfaceObject::dealloc() {
	peff::destroy_and_release<InterfaceObject>(get_allocator(), this, alignof(InterfaceObject));
}

SLAKE_API void InterfaceObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);

	generic_args.replace_allocator(allocator);

	mapped_generic_params.replace_allocator(allocator);
	mapped_generic_args.replace_allocator(allocator);

	generic_params.replace_allocator(allocator);

	for (auto &i : generic_params) {
		i.replace_allocator(allocator);
	}

	impl_types.replace_allocator(allocator);

	impl_interface_indices.replace_allocator(allocator);
}

SLAKE_API Object *StructObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

struct IndexedStructRecursionCheckFrameExData {
	size_t index;
};

struct EnumModuleIteratorStructRecursionCheckFrameExData {
	BasicModuleObject::MembersMap::ConstIterator enum_module_iterator;
};

struct StructRecursionCheckFrame {
	Object *struct_object;

	std::variant<IndexedStructRecursionCheckFrameExData,
		EnumModuleIteratorStructRecursionCheckFrameExData>
		ex_data;
};

struct StructRecursionCheckContext {
	peff::List<StructRecursionCheckFrame> frames;
	peff::Set<Object *> walked_objects;

	SLAKE_FORCEINLINE StructRecursionCheckContext(peff::Alloc *allocator) : frames(allocator), walked_objects(allocator) {}
};

static SLAKE_FORCEINLINE InternalExceptionPointer _is_struct_recursed(StructRecursionCheckContext &context) {
	auto check_type_ref = [&context](const TypeRef &type_ref) -> InternalExceptionPointer {
		switch (type_ref.type_id) {
			case TypeId::StructInstance: {
				CustomTypeDefObject *td = type_ref.get_custom_type_def();

				assert(td->type_object->get_object_kind() == ObjectKind::Struct);
				if (!context.frames.push_back({ td->type_object, IndexedStructRecursionCheckFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();
				break;
			}
			case TypeId::UnionEnum: {
				CustomTypeDefObject *td = type_ref.get_custom_type_def();

				assert(td->type_object->get_object_kind() == ObjectKind::UnionEnum);
				if (!context.frames.push_back({ td->type_object, EnumModuleIteratorStructRecursionCheckFrameExData{ ((UnionEnumObject *)td->type_object)->get_members().begin() } }))
					return OutOfMemoryError::alloc();
				break;
			}
			case TypeId::UnionEnumItem: {
				CustomTypeDefObject *td = type_ref.get_custom_type_def();

				assert(td->type_object->get_object_kind() == ObjectKind::UnionEnum);
				if (!context.frames.push_back({ td->type_object, IndexedStructRecursionCheckFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();
				break;
			}
		}
		return {};
	};
	while (context.frames.size()) {
		StructRecursionCheckFrame &cur_frame = context.frames.back();

		switch (cur_frame.struct_object->get_object_kind()) {
			case ObjectKind::Struct: {
				StructObject *struct_object = (StructObject *)cur_frame.struct_object;
				size_t &index = std::get<IndexedStructRecursionCheckFrameExData>(cur_frame.ex_data).index;
				if (!index) {
					if (context.walked_objects.contains(cur_frame.struct_object))
						// Recursed!
						std::terminate();

					if (!context.walked_objects.insert(+cur_frame.struct_object))
						return OutOfMemoryError::alloc();
				} else if (index >= struct_object->get_field_records().size()) {
					context.walked_objects.remove(struct_object);
					context.frames.pop_back();
					continue;
				}

				auto &cur_record = struct_object->get_field_records().at(index);

				TypeRef type_ref = cur_record.type;
				SLAKE_RETURN_IF_EXCEPT(check_type_ref(type_ref));

				++index;
				break;
			}
			case ObjectKind::UnionEnum: {
				UnionEnumObject *struct_object = (UnionEnumObject *)cur_frame.struct_object;
				BasicModuleObject::MembersMap::ConstIterator &iterator = std::get<EnumModuleIteratorStructRecursionCheckFrameExData>(cur_frame.ex_data).enum_module_iterator;
				if (iterator == struct_object->get_members().begin_const()) {
					if (context.walked_objects.contains(cur_frame.struct_object))
						// Recursed!
						std::terminate();

					if (!context.walked_objects.insert(+cur_frame.struct_object))
						return OutOfMemoryError::alloc();
				} else if (iterator == struct_object->get_members().end_const()) {
					context.walked_objects.remove(struct_object);
					context.frames.pop_back();
					continue;
				}

				auto cur_record = *iterator;

				if (!context.frames.push_back({ cur_record.second, IndexedStructRecursionCheckFrameExData{ 0 } }))
					return OutOfMemoryError::alloc();
				if (!context.walked_objects.insert(+struct_object))
					return OutOfMemoryError::alloc();
				break;

				++iterator;
				break;
			}
			case ObjectKind::UnionEnumItem: {
				UnionEnumItemObject *struct_object = (UnionEnumItemObject *)cur_frame.struct_object;
				size_t &index = std::get<IndexedStructRecursionCheckFrameExData>(cur_frame.ex_data).index;
				if (!index) {
					if (context.walked_objects.contains(cur_frame.struct_object))
						// Recursed!
						std::terminate();

					if (!context.walked_objects.insert(+cur_frame.struct_object))
						return OutOfMemoryError::alloc();
				} else if (index >= struct_object->get_field_records().size()) {
					context.walked_objects.remove(struct_object);
					context.frames.pop_back();
					continue;
				}

				auto &cur_record = struct_object->get_field_records().at(index);

				TypeRef type_ref = cur_record.type;
				SLAKE_RETURN_IF_EXCEPT(check_type_ref(type_ref));

				++index;
				break;
			}
			default:
				std::terminate();
		}
	}

	return {};
}

SLAKE_API InternalExceptionPointer StructObject::is_recursed(peff::Alloc *allocator) noexcept {
	StructRecursionCheckContext context(allocator);

	if (!context.frames.push_back({ this, IndexedStructRecursionCheckFrameExData{ 0 } }))
		return OutOfMemoryError::alloc();

	return _is_struct_recursed(context);
}

SLAKE_API slake::StructObject::StructObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::Struct),
	  generic_args(self_allocator),
	  mapped_generic_args(self_allocator),
	  generic_params(self_allocator),
	  mapped_generic_params(self_allocator),
	  impl_types(self_allocator) {
}

SLAKE_API const peff::DynArray<Value> *StructObject::get_generic_args() const {
	return &generic_args;
}

SLAKE_API StructObject::StructObject(Duplicator *duplicator, const StructObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out),
	  generic_args(allocator),
	  mapped_generic_args(allocator),  // No need to copy
	  generic_params(allocator),
	  mapped_generic_params(allocator),	 // No need to copy
	  impl_types(allocator) {
	if (succeeded_out) {
		struct_flags = x.struct_flags;

		if (!generic_args.resize(x.generic_args.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(generic_args.data(), x.generic_args.data(), generic_args.size() * sizeof(Value));
		for (auto [k, v] : x.mapped_generic_args) {
			peff::String name(allocator);

			if (!name.build(k)) {
				succeeded_out = false;
				return;
			}

			if (!(mapped_generic_args.insert(std::move(name), Value(v)))) {
				succeeded_out = false;
				return;
			}
		}
		if (!generic_params.resize_uninit(x.generic_params.size())) {
			succeeded_out = false;
			return;
		}
		for (size_t i = 0; i < x.generic_params.size(); ++i) {
			if (!x.generic_params.at(i).copy(generic_params.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroy_at<GenericParam>(&generic_params.at(j - 1));
				}
				succeeded_out = false;
				return;
			}
		}
		if (!impl_types.resize(x.impl_types.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(impl_types.data(), x.impl_types.data(), impl_types.size() * sizeof(TypeRef));
	}
}

SLAKE_API StructObject::~StructObject() {
}

SLAKE_API HostObjectRef<StructObject> slake::StructObject::alloc(Duplicator *duplicator, const StructObject *other) {
	bool succeeded = true;

	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = other->associated_runtime->get_cur_gen_alloc();

	std::unique_ptr<StructObject, peff::DeallocableDeleter<StructObject>> ptr(
		peff::alloc_and_construct<StructObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			duplicator, *other, cur_generation_allocator.get(), succeeded));

	if (!succeeded)
		return nullptr;

	if (!other->associated_runtime->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<StructObject> slake::StructObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<StructObject, peff::DeallocableDeleter<StructObject>> ptr(
		peff::alloc_and_construct<StructObject>(
			cur_generation_allocator.get(),
			sizeof(std::max_align_t),
			rt, cur_generation_allocator.get()));

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::StructObject::dealloc() {
	peff::destroy_and_release<StructObject>(get_allocator(), this, alignof(StructObject));
}

SLAKE_API void StructObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);

	generic_args.replace_allocator(allocator);

	mapped_generic_params.replace_allocator(allocator);
	mapped_generic_args.replace_allocator(allocator);

	generic_params.replace_allocator(allocator);

	for (auto &i : generic_params) {
		i.replace_allocator(allocator);
	}

	impl_types.replace_allocator(allocator);

	if (cached_object_layout)
		cached_object_layout->replace_allocator(allocator);
}

SLAKE_API ScopedEnumObject::ScopedEnumObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::ScopedEnum) {
}

SLAKE_API ScopedEnumObject::ScopedEnumObject(Duplicator *duplicator, const ScopedEnumObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out) {
	if (succeeded_out) {
		base_type = x.base_type;
	}
}

SLAKE_API ScopedEnumObject::~ScopedEnumObject() {
}

SLAKE_API Object *ScopedEnumObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<ScopedEnumObject> ScopedEnumObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<ScopedEnumObject, peff::DeallocableDeleter<ScopedEnumObject>> ptr(
		peff::alloc_and_construct<ScopedEnumObject>(cur_generation_allocator.get(), sizeof(std::max_align_t), rt, cur_generation_allocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<ScopedEnumObject> ScopedEnumObject::alloc(Duplicator *duplicator, const ScopedEnumObject *other) {
	return (ScopedEnumObject *)other->duplicate(duplicator);
}

SLAKE_API void ScopedEnumObject::dealloc() {
	peff::destroy_and_release<ScopedEnumObject>(get_allocator(), this, alignof(ScopedEnumObject));
}

SLAKE_API void ScopedEnumObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);
}

SLAKE_API UnionEnumItemObject::UnionEnumItemObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::UnionEnumItem) {
}

SLAKE_API UnionEnumItemObject::UnionEnumItemObject(Duplicator *duplicator, const UnionEnumItemObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out) {
}

SLAKE_API UnionEnumItemObject::~UnionEnumItemObject() {
}

SLAKE_API Object *UnionEnumItemObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API HostObjectRef<UnionEnumItemObject> slake::UnionEnumItemObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<UnionEnumItemObject, peff::DeallocableDeleter<UnionEnumItemObject>> ptr(
		peff::alloc_and_construct<UnionEnumItemObject>(cur_generation_allocator.get(), sizeof(std::max_align_t), rt, cur_generation_allocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<UnionEnumItemObject> slake::UnionEnumItemObject::alloc(Duplicator *duplicator, const UnionEnumItemObject *other) {
	return (UnionEnumItemObject *)other->duplicate(duplicator);
}

SLAKE_API void slake::UnionEnumItemObject::dealloc() {
	peff::destroy_and_release<UnionEnumItemObject>(get_allocator(), this, alignof(UnionEnumItemObject));
}

SLAKE_API void UnionEnumItemObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);
}

SLAKE_API UnionEnumObject::UnionEnumObject(Runtime *rt, peff::Alloc *self_allocator)
	: BasicModuleObject(rt, self_allocator, ObjectKind::UnionEnum),
	  generic_args(self_allocator),
	  mapped_generic_args(self_allocator),
	  generic_params(self_allocator),
	  mapped_generic_params(self_allocator) {
}

SLAKE_API UnionEnumObject::UnionEnumObject(Duplicator *duplicator, const UnionEnumObject &x, peff::Alloc *allocator, bool &succeeded_out)
	: BasicModuleObject(duplicator, x, allocator, succeeded_out),
	  generic_args(allocator),
	  mapped_generic_args(allocator),
	  generic_params(allocator),
	  mapped_generic_params(allocator) {
	if (succeeded_out) {
		if (!generic_args.resize(x.generic_args.size())) {
			succeeded_out = false;
			return;
		}
		memcpy(generic_args.data(), x.generic_args.data(), generic_args.size() * sizeof(Value));
		for (auto [k, v] : x.mapped_generic_args) {
			peff::String name(allocator);

			if (!name.build(k)) {
				succeeded_out = false;
				return;
			}

			if (!(mapped_generic_args.insert(std::move(name), Value(v)))) {
				succeeded_out = false;
				return;
			}
		}
		if (!generic_params.resize_uninit(x.generic_params.size())) {
			succeeded_out = false;
			return;
		}
		for (size_t i = 0; i < x.generic_params.size(); ++i) {
			if (!x.generic_params.at(i).copy(generic_params.at(i))) {
				for (size_t j = i; j; --j) {
					peff::destroy_at<GenericParam>(&generic_params.at(j - 1));
				}
				succeeded_out = false;
				return;
			}
		}
	}
}

SLAKE_API UnionEnumObject::~UnionEnumObject() {
}

SLAKE_API Object *UnionEnumObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(duplicator, this).get();
}

SLAKE_API Reference UnionEnumObject::get_member(const std::string_view &name) const {
	if (auto it = members.find(name); it != members.end()) {
		return Reference(it.value());
	}
	return ReferenceKind::Invalid;
}

SLAKE_API bool UnionEnumObject::add_member(MemberObject *member) {
	if (!members.insert(member->get_name(), +member))
		return false;
	member->set_parent(this);
	return true;
}

SLAKE_API void UnionEnumObject::remove_member(const std::string_view &name) {
	members.remove(name);
}

SLAKE_API HostObjectRef<UnionEnumObject> UnionEnumObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> cur_generation_allocator = rt->get_cur_gen_alloc();

	std::unique_ptr<UnionEnumObject, peff::DeallocableDeleter<UnionEnumObject>> ptr(
		peff::alloc_and_construct<UnionEnumObject>(cur_generation_allocator.get(), sizeof(std::max_align_t), rt, cur_generation_allocator.get()));

	if (!ptr)
		return nullptr;

	if (!rt->add_object(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<UnionEnumObject> UnionEnumObject::alloc(Duplicator *duplicator, const UnionEnumObject *other) {
	return (UnionEnumObject *)other->duplicate(duplicator);
}

SLAKE_API void UnionEnumObject::dealloc() {
	peff::destroy_and_release<UnionEnumObject>(get_allocator(), this, alignof(UnionEnumObject));
}

SLAKE_API void UnionEnumObject::replace_allocator(peff::Alloc *allocator) noexcept {
	this->BasicModuleObject::replace_allocator(allocator);

	generic_args.replace_allocator(allocator);
	mapped_generic_args.replace_allocator(allocator);

	generic_params.replace_allocator(allocator);
	mapped_generic_params.replace_allocator(allocator);
}
