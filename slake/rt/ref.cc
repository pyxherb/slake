#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::resolveIdRef(
	IdRefObject *ref,
	EntityRef &objectRefOut,
	Object *scopeObject) {
	if (!ref)
		return nullptr;

	if ((!scopeObject))
		if (!(scopeObject = _rootObject))
			return nullptr;

	MemberObject *curObject;

	while ((curObject = (MemberObject *)scopeObject)) {
		for (size_t i = 0; i < ref->entries.size(); ++i) {
			auto &curName = ref->entries.at(i);

			if (!scopeObject)
				goto fail;

			objectRefOut = scopeObject->getMember(curName.name);

			if (!objectRefOut) {
				goto fail;
			}

			if (objectRefOut.kind == ObjectRefKind::ObjectRef) {
				scopeObject = objectRefOut.asObject.instanceObject;

				if (curName.genericArgs.size()) {
					for (auto &j : curName.genericArgs) {
						SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
					}

					GenericInstantiationContext genericInstantiationContext(&globalHeapPoolAlloc);

					genericInstantiationContext.genericArgs = &curName.genericArgs;
					SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject(scopeObject, scopeObject, genericInstantiationContext));
					objectRefOut = EntityRef::makeObjectRef(scopeObject);
				}

				if (curName.hasParamTypes) {
					switch (scopeObject->getKind()) {
					case ObjectKind::Fn: {
						FnObject *fnObject = ((FnObject *)scopeObject);

						for (auto &j : curName.paramTypes) {
							SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
						}

						objectRefOut = EntityRef::makeObjectRef(scopeObject = fnObject->getOverloading(curName.paramTypes));
						break;
					}
					default:
						return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
					}
				}
			} else {
				if (i + 1 != ref->entries.size())
					return ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref);
			}
		}

		return {};

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
	if (!getFullRef(peff::getDefaultAlloc(), v, fullIdRef))
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

SLAKE_API bool Runtime::getFullRef(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const {
	do {
		switch (v->getKind()) {
		case ObjectKind::Instance:
			v = (const MemberObject *)((InstanceObject *)v)->_class;
			break;
		}

		const char *name = v->getName();
		size_t szName = strlen(name);
		peff::String copiedName(allocator);
		if (!copiedName.resize(szName)) {
			return false;
		}
		memcpy(copiedName.data(), name, szName);
		GenericArgList copiedGenericArgs(allocator);
		if (v->getGenericArgs()) {
			if (!peff::copyAssign(copiedGenericArgs, *v->getGenericArgs()))
				return false;
		}

		peff::DynArray<Type> paramList(allocator);
		if (!idRefOut.pushFront(IdRefEntry(std::move(copiedName), std::move(copiedGenericArgs), false, std::move(paramList), false))) {
			return false;
		}
	} while ((Object *)(v = (const MemberObject *)v->getParent()) != _rootObject);
	return true;
}
