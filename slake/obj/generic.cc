#include <slake/runtime.h>

using namespace slake;

SLAKE_API GenericParam::GenericParam(peff::Alloc *selfAllocator) : name(selfAllocator), interfaces(selfAllocator) {
}

SLAKE_API GenericParam::GenericParam(GenericParam &&rhs) : name(std::move(rhs.name)), baseType(std::move(rhs.baseType)), interfaces(std::move(rhs.interfaces)) {
}

SLAKE_API void GenericParam::replaceAllocator(peff::Alloc *allocator) noexcept {
	name.replaceAllocator(allocator);
	interfaces.replaceAllocator(allocator);
}

SLAKE_API peff::Option<int> GenericArgListComparator::operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
	if (lhs.size() < rhs.size())
		return -1;
	if (lhs.size() > rhs.size())
		return 1;

	int result;
	for (size_t j = 0; j < lhs.size(); ++j) {
		if ((exceptPtr = Runtime::compareTypes(allocator.get(), lhs.at(j), rhs.at(j), result))) {
			return peff::NULL_OPTION;
		}

		if (result)
			return +result;
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
