#include <slake/runtime.h>

using namespace slake;

Value *Runtime::resolveRef(RefValue* ref, Value *scopeValue) const {
	if (!ref)
		return nullptr;

	if ((!scopeValue))
		if (!(scopeValue = _rootValue))
			return nullptr;

	MemberValue *curValue;

	GenericInstantiationContext genericInstantiationContext = { nullptr, {} };

	while ((curValue = (MemberValue *)scopeValue)) {
		for (auto &i : ref->entries) {
			if (!scopeValue)
				goto fail;

			if (i.name == "base") {
				switch (curValue->getType().typeId) {
					case TypeId::Module:
					case TypeId::Class:
						scopeValue = (MemberValue *)curValue->getParent();
						break;
					case TypeId::Object:
						scopeValue = ((ObjectValue *)curValue)->_parent;
						break;
					default:
						goto fail;
				}
			} else if (!(scopeValue = scopeValue->getMember(i.name))) {
				break;
			}

			if (i.genericArgs.size()) {
				for (auto &j : i.genericArgs)
					j.loadDeferredType(this);

				genericInstantiationContext.genericArgs = &i.genericArgs;
				scopeValue = instantiateGenericValue(scopeValue, genericInstantiationContext);
			}
		}

		if (scopeValue)
			return scopeValue;

	fail:
		switch (curValue->getType().typeId) {
			case TypeId::Module:
			case TypeId::Class:
				if(!curValue->getParent())
					return nullptr;
				scopeValue = (MemberValue *)curValue->getParent();
				break;
			case TypeId::Object: {
				auto t = ((ObjectValue *)curValue)->getType();
				scopeValue = t.getCustomTypeExData();
				break;
			}
			default:
				return nullptr;
		}
	}

	return nullptr;
}
