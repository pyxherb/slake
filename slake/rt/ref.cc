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

			switch (scopeObject->getKind()) {
				case ObjectKind::Module: {
					ModuleObject *obj = (ModuleObject *)scopeObject;

					if (auto it = obj->fieldRecordIndices.find(i.name); it != obj->fieldRecordIndices.end()) {
						scopeObject = obj->fieldAccessor;
						if (varRefContextOut)
							*varRefContextOut = VarRefContext::makeFieldContext(it.value());
						break;
					}

					if (!(scopeObject = scopeObject->getMember(i.name, varRefContextOut))) {
						goto fail;
					}

					break;
				}
				case ObjectKind::Class: {
					ClassObject *obj = (ClassObject *)scopeObject;

					if (auto it = obj->fieldRecordIndices.find(i.name); it != obj->fieldRecordIndices.end()) {
						scopeObject = obj->fieldAccessor;
						if (varRefContextOut)
							*varRefContextOut = VarRefContext::makeFieldContext(it.value());
						break;
					}

					if (!(scopeObject = scopeObject->getMember(i.name, varRefContextOut))) {
						goto fail;
					}

					break;
				}
				default: {
					if (!(scopeObject = scopeObject->getMember(i.name, varRefContextOut))) {
						goto fail;
					}
				}
			}

			if (i.genericArgs.size()) {
				for (auto &j : i.genericArgs) {
					SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
				}

				GenericInstantiationContext genericInstantiationContext(&globalHeapPoolAlloc);

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

	peff::DynArray<IdRefEntry> fullIdRef;
	if (!getFullRef(v, fullIdRef))
		throw std::bad_alloc();

	for (size_t i = 0; i < fullIdRef.size(); ++i) {
		auto &scope = fullIdRef.at(i);

		if (i)
			s += ".";
		s += scope.name;

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += std::to_string(scope.genericArgs.at(j), this);
			}
			s += ">";
		}
	}

	return s;
}

SLAKE_API std::string Runtime::getFullName(const IdRefObject *v) const {
	return std::to_string(v);
}

SLAKE_API bool Runtime::getFullRef(const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const {
	do {
		switch (v->getKind()) {
			case ObjectKind::Instance:
				v = (const MemberObject *)((InstanceObject *)v)->_class;
				break;
		}

		const char *name = v->getName();
		size_t szName = strlen(name);
		peff::String copiedName(&globalHeapPoolAlloc);
		if (!copiedName.resize(szName)) {
			return false;
		}
		memcpy(copiedName.data(), name, szName);
		GenericArgList copiedGenericArgs(&globalHeapPoolAlloc);
		if (v->getGenericArgs()) {
			if (!peff::copyAssign(copiedGenericArgs, *v->getGenericArgs()))
				return false;
		}

		peff::DynArray<Type> paramList(&globalHeapPoolAlloc);
		if (!idRefOut.pushFront(IdRefEntry(std::move(copiedName), std::move(copiedGenericArgs), false, std::move(paramList), false))) {
			return false;
		}
	} while ((Object *)(v = (const MemberObject *)v->getParent()) != _rootObject);
	return true;
}
