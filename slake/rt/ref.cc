#include <slake/runtime.h>

using namespace slake;

Value *Runtime::resolveRef(ValueRef<RefValue> ref, Value *scopeValue) const {
	if (!ref)
		return nullptr;

	if ((!scopeValue))
		if (!(scopeValue = _rootValue))
			return nullptr;

	MemberValue *value = (MemberValue *)scopeValue;

	while (value && value->getParent()) {
		value = (MemberValue *)scopeValue;

		for (auto &i : ref->scopes) {
			if (!scopeValue)
				goto fail;

			if (i == "base") {
				switch (value->getType().valueType) {
					case ValueType::MOD:
					case ValueType::CLASS:
					case ValueType::STRUCT:
						scopeValue = (MemberValue *)value->getParent();
						break;
					case ValueType::OBJECT:
						scopeValue = *((ObjectValue *)value)->_parent;
						break;
					default:
						goto fail;
				}
			} else if (!(scopeValue = scopeValue->getMember(i)))
				break;
		}

		if (scopeValue)
			return scopeValue;

	fail:
		switch (value->getType().valueType) {
			case ValueType::MOD:
			case ValueType::CLASS:
			case ValueType::STRUCT:
				scopeValue = (MemberValue *)value->getParent();
				break;
			case ValueType::OBJECT: {
				auto t = ((ObjectValue *)value)->getType();
				scopeValue = *t.getCustomTypeExData();
				break;
			}
			default:
				return nullptr;
		}
	}

	return nullptr;
}
