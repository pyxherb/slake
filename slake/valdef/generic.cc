#include <slake/runtime.h>

using namespace slake;

GenericParam::GenericParam() {}
GenericParam::GenericParam(std::pmr::memory_resource *memoryResource) : name(memoryResource), interfaces(memoryResource) {
}

bool GenericArgListComparator::operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return true;
	if (lhs.size() > rhs.size())
		return false;

	for (size_t i = 0; i < lhs.size(); ++i) {
		if (lhs[i] < rhs[i])
			return true;
	}

	return false;
}

bool GenericArgListEqComparator::operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
	if (lhs.size() != rhs.size())
		return false;

	for (size_t i = 0; i < lhs.size(); ++i) {
		if (lhs[i] != rhs[i])
			return false;
	}

	return true;
}

size_t slake::getGenericParamIndex(const GenericParamList &genericParamList, const std::pmr::string &name) {
	for (size_t i = 0; i < genericParamList.size(); ++i) {
		if (genericParamList[i].name == name)
			return i;
	}

	return SIZE_MAX;
}

GenericParam *slake::getGenericParam(Object *object, const std::pmr::string &name, Object **ownerOut) {
	while (true) {
		size_t idxGenericParam;

		switch (object->getKind()) {
			case ObjectKind::Class: {
				ClassObject *classObject = (ClassObject *)object;
				idxGenericParam = getGenericParamIndex(classObject->genericParams, name);
				if (idxGenericParam != SIZE_MAX) {
					if (ownerOut)
						*ownerOut = object;
					return &classObject->genericParams[idxGenericParam];
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
					return &interfaceObject->genericParams[idxGenericParam];
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
					return &fnOverloadingObject->genericParams[idxGenericParam];
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
