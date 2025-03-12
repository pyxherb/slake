#include <slake/runtime.h>

using namespace slake;

SLAKE_API GenericParam::GenericParam(): name(nullptr), interfaces(nullptr) { std::terminate(); }
SLAKE_API GenericParam::GenericParam(peff::Alloc *selfAllocator) : name(selfAllocator), interfaces(selfAllocator) {
}

SLAKE_API GenericParam::GenericParam(GenericParam &&rhs) : name(std::move(rhs.name)), baseType(std::move(rhs.baseType)), interfaces(std::move(rhs.interfaces)) {
}

SLAKE_API bool GenericArgListComparator::operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return true;
	if (lhs.size() > rhs.size())
		return false;

	for (size_t i = 0; i < lhs.size(); ++i) {
		if (lhs.at(i) < rhs.at(i))
			return true;
		if (lhs.at(i) > rhs.at(i))
			return false;
	}

	return false;
}

SLAKE_API bool GenericArgListEqComparator::operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
	if (lhs.size() != rhs.size())
		return false;

	for (size_t i = 0; i < lhs.size(); ++i) {
		if (lhs.at(i) != rhs.at(i))
			return false;
	}

	return true;
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

		switch (object->getKind()) {
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
