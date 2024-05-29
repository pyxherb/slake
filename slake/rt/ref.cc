#include <slake/runtime.h>

using namespace slake;

Value *Runtime::resolveIdRef(IdRefValue* ref, Value *scopeValue) const {
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

std::string Runtime::getFullName(const MemberValue *v) const {
	std::string s;
	do {
		switch (v->getType().typeId) {
			case TypeId::Object:
				v = (const MemberValue *)((ObjectValue *)v)->getType().getCustomTypeExData();
				break;
		}
		s = v->getName() + (s.empty() ? "" : "." + s);
	} while ((Value *)(v = (const MemberValue *)v->getParent()) != _rootValue);
	return s;
}

std::string Runtime::getFullName(const IdRefValue *v) const {
	return std::to_string(v);
}

std::deque<IdRefEntry> Runtime::getFullRef(const MemberValue *v) const {
	std::deque<IdRefEntry> entries;
	do {
		switch (v->getType().typeId) {
			case TypeId::Object:
				v = (const MemberValue *)((ObjectValue *)v)->getType().getCustomTypeExData();
				break;
		}
		entries.push_front({ v->getName(), v->_genericArgs });
	} while ((Value *)(v = (const MemberValue *)v->getParent()) != _rootValue);
	return entries;
}
