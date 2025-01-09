#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::resolveIdRef(
	IdRefObject *ref,
	VarRefContext *varRefContextOut,
	Object *&objectOut,
	Object *scopeObject) {
	if (!ref)
		return nullptr;

	if ((!scopeObject))
		if (!(scopeObject = _rootObject))
			return nullptr;

	MemberObject *curObject;

	while ((curObject = (MemberObject *)scopeObject)) {
		for (auto &i : ref->entries) {
			if (!scopeObject)
				goto fail;

			if (!(scopeObject = scopeObject->getMember(i.name, varRefContextOut))) {
				break;
			}

			if (i.genericArgs.size()) {
				for (auto &j : i.genericArgs) {
					SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
				}

				GenericInstantiationContext genericInstantiationContext(&globalHeapPoolResource);

				genericInstantiationContext.genericArgs = &i.genericArgs;
				SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject(scopeObject, scopeObject, genericInstantiationContext));
			}

			if (i.hasParamTypes) {
				switch (scopeObject->getKind()) {
					case ObjectKind::Fn: {
						FnObject *fnObject = ((FnObject *)scopeObject);

						for (auto &j : i.paramTypes) {
							SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
						}

						scopeObject = fnObject->getOverloading(i.paramTypes);
						break;
					}
					default:
						return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
				}
			}
		}

		if (scopeObject) {
			objectOut = scopeObject;
			return {};
		}

	fail:
		switch (curObject->getKind()) {
			case ObjectKind::Module:
				if (!curObject->getParent())
					return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
				scopeObject = (MemberObject *)curObject->getParent();
				break;
			case ObjectKind::Class: {
				ClassObject *cls = (ClassObject *)curObject;
				if (cls->parentClass.typeId == TypeId::Instance) {
					SLAKE_RETURN_IF_EXCEPT(cls->parentClass.loadDeferredType(this));
					scopeObject = cls->parentClass.getCustomTypeExData();
				} else {
					return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
				}
				break;
			}
			case ObjectKind::Instance: {
				scopeObject = ((InstanceObject *)curObject)->_class;
				break;
			}
			default:
				return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
		}
	}

	return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
}

SLAKE_API std::string Runtime::getFullName(const MemberObject *v) const {
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

SLAKE_API std::string Runtime::getFullName(const IdRefObject *v) const {
	return std::to_string(v);
}

SLAKE_API std::deque<IdRefEntry> Runtime::getFullRef(const MemberObject *v) const {
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
