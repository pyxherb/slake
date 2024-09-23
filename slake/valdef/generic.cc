#include <slake/runtime.h>

using namespace slake;

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
