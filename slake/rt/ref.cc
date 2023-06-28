#include <slake/runtime.h>

using namespace Slake;

Value *Runtime::resolveRef(ValueRef<RefValue> ref, Value *v) {
	if (!ref)
		return nullptr;

	if ((!v))
		if (!(v = _rootValue))
			return nullptr;

	MemberValue *value = (MemberValue *)v;

	while (value && value->getParent()) {
		value = (MemberValue *)v;

		for (auto &i : ref->scopes) {
			if (!v)
				goto fail;

			if (i == "base") {
				switch (value->getType().valueType) {
					case ValueType::MOD:
					case ValueType::CLASS:
					case ValueType::STRUCT:
						v = (MemberValue *)value->getParent();
						break;
					case ValueType::OBJECT:
						v = *((ObjectValue *)value)->_parent;
						break;
					default:
						goto fail;
				}
			} else if (!(v = v->getMember(i)))
				break;
		}

		if (v)
			return v;

	fail:
		switch (value->getType().valueType) {
			case ValueType::MOD:
			case ValueType::CLASS:
			case ValueType::STRUCT:
				v = (MemberValue *)value->getParent();
				break;
			case ValueType::OBJECT: {
				auto t = ((ObjectValue *)value)->getType();
				v = t.exData.customType;
				break;
			}
			default:
				return nullptr;
		}
	}

	return nullptr;
}
