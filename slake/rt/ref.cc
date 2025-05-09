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
			} else {
				if (i + 1 != ref->entries.size())
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref));
			}
		}

		if (ref->paramTypes.size() || ref->hasVarArgs) {
			switch (scopeObject->getKind()) {
				case ObjectKind::Fn: {
					FnObject *fnObject = ((FnObject *)scopeObject);

					for (auto &j : ref->paramTypes) {
						SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
					}

					objectRefOut = EntityRef::makeObjectRef(scopeObject = fnObject->getOverloading(ref->paramTypes));
					break;
				}
				default:
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref));
			}
		}

		return {};

	fail:
		switch (curObject->getKind()) {
			case ObjectKind::Module:
				if (!curObject->parent)
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref));
				scopeObject = (MemberObject *)curObject->parent;
				break;
			case ObjectKind::Class: {
				ClassObject *cls = (ClassObject *)curObject;
				if (cls->baseType.typeId == TypeId::Instance) {
					SLAKE_RETURN_IF_EXCEPT(cls->baseType.loadDeferredType(this));
					scopeObject = cls->baseType.getCustomTypeExData();
				} else {
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref));
				}
				break;
			}
			case ObjectKind::Instance: {
				scopeObject = ((InstanceObject *)curObject)->_class;
				break;
			}
			default:
				return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref));
		}
	}

	return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this), ref));
}

SLAKE_API bool Runtime::getFullRef(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const {
	do {
		switch (v->getKind()) {
			case ObjectKind::Instance:
				v = (const MemberObject *)((InstanceObject *)v)->_class;
				break;
		}

		std::string_view name = v->name;
		peff::String copiedName(allocator);
		if (!copiedName.build(name)) {
			return false;
		}
		GenericArgList copiedGenericArgs(allocator);
		if (auto p = v->getGenericArgs(); p) {
			copiedGenericArgs.resize(p->size());
			for (size_t i = 0; i < copiedGenericArgs.size(); ++i) {
				if (!peff::copyAssign(copiedGenericArgs.at(i), p->at(i)))
					return false;
			}
		}

		if (!idRefOut.pushFront(IdRefEntry(std::move(copiedName), std::move(copiedGenericArgs)))) {
			return false;
		}
	} while ((Object *)(v = (const MemberObject *)v->parent) != _rootObject);
	return true;
}
