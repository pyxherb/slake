#include <slake/runtime.h>

using namespace slake;

Object *Runtime::resolveIdRef(
	IdRefObject *ref,
	VarRefContext *varRefContextOut,
	Object *scopeObject) const {
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
				switch (curObject->getKind()) {
					case ObjectKind::Module:
					case ObjectKind::Class:
						scopeObject = (MemberObject *)curObject->getParent();
						break;
					default:
						goto fail;
				}
			} else if (!(scopeObject = scopeObject->getMember(i.name, varRefContextOut))) {
				break;
			}

			if (i.genericArgs.size()) {
				for (auto &j : i.genericArgs)
					j.loadDeferredType(this);

				genericInstantiationContext.genericArgs = &i.genericArgs;
				scopeObject = instantiateGenericObject(scopeObject, genericInstantiationContext);
			}

			if (i.hasParamTypes) {
				switch (scopeObject->getKind()) {
					case ObjectKind::Fn: {
						FnObject *fnObject = ((FnObject *)scopeObject);

						for (auto& j : i.paramTypes) {
							j.loadDeferredType(this);
						}

						scopeObject = fnObject->getOverloading(i.paramTypes);
						break;
					}
					default:
						return nullptr;
				}
			}
		}

		if (scopeObject)
			return scopeObject;

	fail:
		switch (curObject->getKind()) {
			case ObjectKind::Module:
			case ObjectKind::Class:
				if (!curObject->getParent())
					return nullptr;
				scopeObject = (MemberObject *)curObject->getParent();
				break;
			case ObjectKind::Instance: {
				scopeObject = ((InstanceObject *)curObject)->_class;
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
		switch (v->getKind()) {
			case ObjectKind::Instance:
				v = (const MemberObject *)((InstanceObject *)v)->_class;
				break;
		}
		entries.push_front({ v->getName(), v->getGenericArgs() });
	} while ((Object *)(v = (const MemberObject *)v->getParent()) != _rootObject);
	return entries;
}
