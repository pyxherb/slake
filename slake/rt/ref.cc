#include <slake/runtime.h>

using namespace slake;

Object *Runtime::resolveIdRef(IdRefObject* ref, Object *scopeObject) const {
	if (!ref)
		return nullptr;

	if ((!scopeObject))
		if (!(scopeObject = _rootObject))
			return nullptr;

	MemberObject *curObject;

	GenericInstantiationContext genericInstantiationContext = { nullptr, {} };

	while ((curObject = (MemberObject *)scopeObject)) {
		for (auto &i : ref->entries) {
			if (!scopeObject)
				goto fail;

			if (i.name == "base") {
				switch (curObject->getType().typeId) {
					case TypeId::Module:
					case TypeId::Class:
						scopeObject = (MemberObject *)curObject->getParent();
						break;
					default:
						goto fail;
				}
			} else if (!(scopeObject = scopeObject->getMember(i.name))) {
				break;
			}

			if (i.genericArgs.size()) {
				for (auto &j : i.genericArgs)
					j.loadDeferredType(this);

				genericInstantiationContext.genericArgs = &i.genericArgs;
				scopeObject = instantiateGenericObject(scopeObject, genericInstantiationContext);
			}
		}

		if (scopeObject)
			return scopeObject;

	fail:
		switch (curObject->getType().typeId) {
			case TypeId::Module:
			case TypeId::Class:
				if(!curObject->getParent())
					return nullptr;
				scopeObject = (MemberObject *)curObject->getParent();
				break;
			case TypeId::Instance: {
				auto t = ((InstanceObject *)curObject)->getType();
				scopeObject = t.getCustomTypeExData();
				break;
			}
			default:
				return nullptr;
		}
	}

	return nullptr;
}

std::string Runtime::getFullName(const MemberObject *v) const {
	std::string s;

	auto fullIdRef = getFullRef(v);

	for (size_t i = 0; i < fullIdRef.size(); ++i) {
		auto &scope = fullIdRef[i];

		if (i)
			s += ".";
		s += scope.name;

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += std::to_string(scope.genericArgs[j], this);
			}
			s += ">";
		}
	}

	return s;
}

std::string Runtime::getFullName(const IdRefObject *v) const {
	return std::to_string(v);
}

std::deque<IdRefEntry> Runtime::getFullRef(const MemberObject *v) const {
	std::deque<IdRefEntry> entries;
	do {
		switch (v->getType().typeId) {
			case TypeId::Instance:
				v = (const MemberObject *)((InstanceObject *)v)->getType().getCustomTypeExData();
				break;
		}
		entries.push_front({ v->getName(), v->_genericArgs });
	} while ((Object *)(v = (const MemberObject *)v->getParent()) != _rootObject);
	return entries;
}
