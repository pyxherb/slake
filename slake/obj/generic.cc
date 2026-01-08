#include <slake/runtime.h>

using namespace slake;

SLAKE_API GenericParam::GenericParam(peff::Alloc *selfAllocator) : name(selfAllocator), interfaces(selfAllocator) {
}

SLAKE_API GenericParam::GenericParam(GenericParam &&rhs) : name(std::move(rhs.name)), inputType(std::move(rhs.inputType)), baseType(std::move(rhs.baseType)), interfaces(std::move(rhs.interfaces)) {
}

SLAKE_API void GenericParam::replaceAllocator(peff::Alloc *allocator) noexcept {
	name.replaceAllocator(allocator);
	interfaces.replaceAllocator(allocator);
}

SLAKE_API int ParamListComparator::operator()(const ParamTypeList &lhs, const ParamTypeList &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;

	for (size_t j = 0; j < lhs.size(); ++j) {
		TypeRef l = lhs.at(j), r = rhs.at(j);

		if (l.typeId == TypeId::GenericArg && r.typeId == TypeId::GenericArg) {
			GenericArgTypeDefObject *ltd = (GenericArgTypeDefObject *)l.typeDef,
									*rtd = (GenericArgTypeDefObject *)r.typeDef;

			auto findDefObject = [](Object *member, std::string_view name) -> std::pair<Object *, size_t> {
				for (Object *i = member; i;) {
					switch (i->getObjectKind()) {
						case ObjectKind::Class: {
							ClassObject *j = (ClassObject *)i;

							if (auto it = j->mappedGenericParams.find(name); it != j->mappedGenericParams.end()) {
								return { j, it.value() };
							}
							i = j->parent;
							break;
						}
						case ObjectKind::Interface: {
							InterfaceObject *j = (InterfaceObject *)i;

							if (auto it = j->mappedGenericParams.find(name); it != j->mappedGenericParams.end()) {
								return { j, it.value() };
							}
							i = j->parent;
							break;
						}
						case ObjectKind::Struct: {
							StructObject *j = (StructObject *)i;

							if (auto it = j->mappedGenericParams.find(name); it != j->mappedGenericParams.end()) {
								return { j, it.value() };
							}
							i = j->parent;
							break;
						}
						case ObjectKind::FnOverloading: {
							FnOverloadingObject *j = (FnOverloadingObject *)i;

							if (auto it = j->mappedGenericParams.find(name); it != j->mappedGenericParams.end()) {
								return { j, it.value() };
							}
							i = j->fnObject;
							break;
						}
						default:
							return { nullptr, SIZE_MAX };
					}
				}
				return { nullptr, SIZE_MAX };
			};

			auto ld = findDefObject(ltd->ownerObject, ltd->nameObject->data),
				 rd = findDefObject(rtd->ownerObject, rtd->nameObject->data);

			if (ld.first && rd.first) {
				if (ld.first->getObjectKind() < ld.first->getObjectKind())
					return -1;
				if (rd.first->getObjectKind() > rd.first->getObjectKind())
					return 1;

				switch (ld.first->getObjectKind()) {
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

SLAKE_API size_t slake::getGenericParamIndex(const GenericParamList &genericParamList, const std::string_view &name) {
	for (size_t i = 0; i < genericParamList.size(); ++i) {
		if (genericParamList.at(i).name == name)
			return i;
	}

	return SIZE_MAX;
}

SLAKE_API GenericParam *slake::getGenericParam(Object *object, const std::string_view &name, Object **ownerOut) {
	while (true) {
		size_t idxGenericParam;

		switch (object->getObjectKind()) {
			case ObjectKind::Class: {
				ClassObject *classObject = (ClassObject *)object;
				idxGenericParam = getGenericParamIndex(classObject->genericParams, name);
				if (idxGenericParam != SIZE_MAX) {
					if (ownerOut)
						*ownerOut = object;
					return &classObject->genericParams.at(idxGenericParam);
				}

				if (!classObject->parent)
					return nullptr;

				object = classObject->parent;

				break;
			}
			case ObjectKind::Interface: {
				InterfaceObject *interfaceObject = (InterfaceObject *)object;
				idxGenericParam = getGenericParamIndex(interfaceObject->genericParams, name);
				if (idxGenericParam != SIZE_MAX) {
					if (ownerOut)
						*ownerOut = object;
					return &interfaceObject->genericParams.at(idxGenericParam);
				}

				if (!interfaceObject->parent)
					return nullptr;

				object = interfaceObject->parent;

				break;
			}
			case ObjectKind::FnOverloading: {
				FnOverloadingObject *fnOverloadingObject = (FnOverloadingObject *)object;
				idxGenericParam = getGenericParamIndex(fnOverloadingObject->genericParams, name);
				if (idxGenericParam != SIZE_MAX) {
					if (ownerOut)
						*ownerOut = object;
					return &fnOverloadingObject->genericParams.at(idxGenericParam);
				}

				if (!fnOverloadingObject->fnObject->parent)
					return nullptr;

				object = fnOverloadingObject->fnObject->parent;

				break;
			}
			default:
				return nullptr;
		}
	}
}
