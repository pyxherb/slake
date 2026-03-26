#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::resolve_id_ref(
	IdRefObject *ref,
	Reference &object_ref_out,
	Object *scope_object) {
	assert(ref);

	Object *cur_object = scope_object;

	if ((!cur_object))
		if (!(cur_object = _root_object))
			std::terminate();

	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &cur_name = ref->entries.at(i);

		if (!cur_object)
			goto fail;

		object_ref_out = cur_object->get_member(cur_name.name);

		if (!object_ref_out) {
			goto fail;
		}

		if (object_ref_out.kind == ReferenceKind::ObjectRef) {
			// TODO: Check if the instance object is a member object.
			cur_object = (MemberObject *)object_ref_out.as_object;

			switch (cur_object->get_object_kind()) {
				case ObjectKind::Class:
				case ObjectKind::Interface:
				case ObjectKind::Fn:
				case ObjectKind::ScopedEnum:
				case ObjectKind::UnionEnum:
					if (cur_name.generic_args.size()) {
						peff::NullAlloc null_alloc;
						GenericInstantiationContext generic_instantiation_context(&null_alloc, get_fixed_alloc());

						generic_instantiation_context.generic_args = &cur_name.generic_args;
						MemberObject *m;
						SLAKE_RETURN_IF_EXCEPT(instantiate_generic_object((MemberObject *)cur_object, m, &generic_instantiation_context));
						cur_object = m;
						object_ref_out = Reference(cur_object);
					}
					break;
			}
		} else {
			if (i + 1 != ref->entries.size()) {
				object_ref_out = ReferenceKind::Invalid;
				return {};
			}
		}
	}

	if (ref->param_types.has_value() || ref->has_var_args) {
		switch (cur_object->get_object_kind()) {
			case ObjectKind::Fn: {
				FnObject *fn_object = ((FnObject *)cur_object);

				const ParamTypeList &param_types = *ref->param_types;

				auto it = fn_object->overloadings.find(FnSignature{ param_types, ref->has_var_args, ref->entries.back().generic_args.size(), ref->overriden_type });

				if (it != fn_object->overloadings.end())
					object_ref_out = Reference(it.value());
				else {
					it = fn_object->overloadings.find(FnSignature{ param_types, ref->has_var_args, ref->entries.back().generic_args.size(), TypeId::Void });

					if (it == fn_object->overloadings.end()) {
						object_ref_out = ReferenceKind::Invalid;
						return {};
					}
				}

				object_ref_out = Reference(it.value());

				break;
			}
			default:
				object_ref_out = ReferenceKind::Invalid;
				return {};
		}
	}

	return {};

fail:;
	object_ref_out = ReferenceKind::Invalid;
	return {};
}

SLAKE_API bool Runtime::get_full_ref(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &id_ref_out) const {
	while (v != _root_object) {
		if (!v) {
			peff::String name(allocator);

			if (!name.build("(Orphaned)"))
				return false;

			if (!id_ref_out.push_front(IdRefEntry(std::move(name), { allocator }))) {
				return false;
			}

			return true;
		}

		switch (v->get_object_kind()) {
			case ObjectKind::Instance:
				v = (const MemberObject *)((InstanceObject *)v)->_class;
				break;
		}

		std::string_view name = v->get_name();
		peff::String copied_name(allocator);
		if (!copied_name.build(name)) {
			return false;
		}
		peff::DynArray<Value> copied_generic_args(allocator);
		if (auto p = v->get_generic_args(); p) {
			if (!copied_generic_args.resize(p->size()))
				return false;
			for (size_t i = 0; i < copied_generic_args.size(); ++i) {
				copied_generic_args.at(i) = p->at(i);
			}
		}

		if (!id_ref_out.push_front(IdRefEntry(std::move(copied_name), std::move(copied_generic_args)))) {
			return false;
		}

		v = (MemberObject *)v->get_parent();
	};
	return true;
}
