#include <slake/runtime.h>

using namespace slake;

SLAKE_API GenericParam::GenericParam(peff::Alloc *self_allocator) : name(self_allocator), interfaces(self_allocator) {
}

SLAKE_API GenericParam::GenericParam(GenericParam &&rhs) : name(std::move(rhs.name)), input_type(std::move(rhs.input_type)), base_type(std::move(rhs.base_type)), interfaces(std::move(rhs.interfaces)) {
}

SLAKE_API void GenericParam::replace_allocator(peff::Alloc *allocator) noexcept {
	name.replace_allocator(allocator);
	interfaces.replace_allocator(allocator);
}

SLAKE_API int ParamListComparator::operator()(const ParamTypeList &lhs, const ParamTypeList &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;

	for (size_t j = 0; j < lhs.size(); ++j) {
		TypeRef l = lhs.at(j), r = rhs.at(j);

		if (l.type_id == TypeId::GenericArg && r.type_id == TypeId::GenericArg) {
			GenericArgTypeDefObject *ltd = (GenericArgTypeDefObject *)l.type_def,
									*rtd = (GenericArgTypeDefObject *)r.type_def;

			auto find_def_object = [](Object *member, std::string_view name) -> std::pair<Object *, size_t> {
				for (Object *i = member; i;) {
					switch (i->get_object_kind()) {
						case ObjectKind::Class: {
							ClassObject *j = (ClassObject *)i;

							if (auto it = j->mapped_generic_params.find(name); it != j->mapped_generic_params.end()) {
								return { j, it.value() };
							}
							i = j->get_parent();
							break;
						}
						case ObjectKind::Interface: {
							InterfaceObject *j = (InterfaceObject *)i;

							if (auto it = j->mapped_generic_params.find(name); it != j->mapped_generic_params.end()) {
								return { j, it.value() };
							}
							i = j->get_parent();
							break;
						}
						case ObjectKind::Struct: {
							StructObject *j = (StructObject *)i;

							if (auto it = j->mapped_generic_params.find(name); it != j->mapped_generic_params.end()) {
								return { j, it.value() };
							}
							i = j->get_parent();
							break;
						}
						case ObjectKind::FnOverloading: {
							FnOverloadingObject *j = (FnOverloadingObject *)i;

							if (auto it = j->mapped_generic_params.find(name); it != j->mapped_generic_params.end()) {
								return { j, it.value() };
							}
							i = j->fn_object;
							break;
						}
						default:
							return { nullptr, SIZE_MAX };
					}
				}
				return { nullptr, SIZE_MAX };
			};

			auto ld = find_def_object(ltd->owner_object, ltd->name_object->data),
				 rd = find_def_object(rtd->owner_object, rtd->name_object->data);

			if (ld.first && rd.first) {
				if (ld.first->get_object_kind() < ld.first->get_object_kind())
					return -1;
				if (rd.first->get_object_kind() > rd.first->get_object_kind())
					return 1;

				switch (ld.first->get_object_kind()) {
					case ObjectKind::FnOverloading: {
						if (ld.second < rd.second)
							return -1;
						if (ld.second > rd.second)
							return 1;
						return 0;
					}
					default:
						break;
				}
			}
		}

		if (l < r)
			return -1;
		if (l > r)
			return 1;
	}

	return 0;
}

SLAKE_API int GenericArgListComparator::operator()(const peff::DynArray<Value>& lhs, const peff::DynArray<Value>& rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;

	for (size_t i = 0; i < lhs.size(); ++i) {
		if (lhs.at(i) < rhs.at(i))
			return -1;
		if (lhs.at(i) > rhs.at(i))
			return 1;
	}

	return 0;
}

SLAKE_API size_t slake::get_generic_param_index(const GenericParamList &generic_param_list, const std::string_view &name) {
	for (size_t i = 0; i < generic_param_list.size(); ++i) {
		if (generic_param_list.at(i).name == name)
			return i;
	}

	return SIZE_MAX;
}

SLAKE_API GenericParam *slake::get_generic_param(Object *object, const std::string_view &name, Object **owner_out) {
	while (true) {
		size_t idx_generic_param;

		switch (object->get_object_kind()) {
			case ObjectKind::Class: {
				ClassObject *class_object = (ClassObject *)object;
				idx_generic_param = get_generic_param_index(class_object->generic_params, name);
				if (idx_generic_param != SIZE_MAX) {
					if (owner_out)
						*owner_out = object;
					return &class_object->generic_params.at(idx_generic_param);
				}

				if (!class_object->get_parent())
					return nullptr;

				object = class_object->get_parent();

				break;
			}
			case ObjectKind::Interface: {
				InterfaceObject *interface_object = (InterfaceObject *)object;
				idx_generic_param = get_generic_param_index(interface_object->generic_params, name);
				if (idx_generic_param != SIZE_MAX) {
					if (owner_out)
						*owner_out = object;
					return &interface_object->generic_params.at(idx_generic_param);
				}

				if (!interface_object->get_parent())
					return nullptr;

				object = interface_object->get_parent();

				break;
			}
			case ObjectKind::FnOverloading: {
				FnOverloadingObject *fn_overloading_object = (FnOverloadingObject *)object;
				idx_generic_param = get_generic_param_index(fn_overloading_object->generic_params, name);
				if (idx_generic_param != SIZE_MAX) {
					if (owner_out)
						*owner_out = object;
					return &fn_overloading_object->generic_params.at(idx_generic_param);
				}

				if (!fn_overloading_object->fn_object->get_parent())
					return nullptr;

				object = fn_overloading_object->fn_object->get_parent();

				break;
			}
			default:
				return nullptr;
		}
	}
}
