#include <slake/runtime.h>

using namespace slake;

Value *Runtime::resolveRef(RefValue* ref, Value *scopeValue) const {
	if (!ref)
		return nullptr;

	if ((!scopeValue))
		if (!(scopeValue = _rootValue))
			return nullptr;

	MemberValue *value = (MemberValue *)scopeValue;

	while (value) {
		value = (MemberValue *)scopeValue;

		for (auto &i : ref->entries) {
			if (!scopeValue)
				goto fail;

			if (i.name == "base") {
				switch (value->getType().typeId) {
					case TypeId::MOD:
					case TypeId::CLASS:
						scopeValue = (MemberValue *)value->getParent();
						break;
					case TypeId::OBJECT:
						scopeValue = ((ObjectValue *)value)->_parent.get();
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

				scopeValue = instantiateGenericValue(scopeValue, i.genericArgs);
			}
		}

		if (scopeValue)
			return scopeValue;

	fail:
		switch (value->getType().typeId) {
			case TypeId::MOD:
			case TypeId::CLASS:
				if(!value->getParent())
					return nullptr;
				scopeValue = (MemberValue *)value->getParent();
				break;
			case TypeId::OBJECT: {
				auto t = ((ObjectValue *)value)->getType();
				scopeValue = t.getCustomTypeExData().get();
				break;
			}
			default:
				return nullptr;
		}
	}

	return nullptr;
}
