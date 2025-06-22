#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::resolveIdRef(
	IdRefObject *ref,
	EntityRef &objectRefOut,
	MemberObject *scopeObject) {
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
				// TODO: Check if the instance object is a member object.
				scopeObject = (MemberObject *)objectRefOut.asObject.instanceObject;

				if (curName.genericArgs.size()) {
					for (auto &j : curName.genericArgs) {
						SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
					}

					peff::NullAlloc nullAlloc;
					GenericInstantiationContext genericInstantiationContext(&nullAlloc, getFixedAlloc());

					genericInstantiationContext.genericArgs = &curName.genericArgs;
					SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject(scopeObject, scopeObject, &genericInstantiationContext));
					objectRefOut = EntityRef::makeObjectRef(scopeObject);
				}
			} else {
				if (i + 1 != ref->entries.size())
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
			}
		}

		if (ref->paramTypes.has_value() || ref->hasVarArgs) {
			switch (scopeObject->objectKind) {
				case ObjectKind::Fn: {
					FnObject *fnObject = ((FnObject *)scopeObject);

					if (!ref->paramTypes) {
						std::terminate();
					}

					for (auto &j : *ref->paramTypes) {
						SLAKE_RETURN_IF_EXCEPT(j.loadDeferredType(this));
					}

					objectRefOut = EntityRef::makeObjectRef(fnObject->getOverloading(*ref->paramTypes));

					break;
				}
				default:
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
			}
		}

		return {};

	fail:
		switch (curObject->objectKind) {
			case ObjectKind::Module:
				if (!curObject->parent)
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
				scopeObject = (MemberObject *)curObject->parent;
				break;
			case ObjectKind::Class: {
				ClassObject *cls = (ClassObject *)curObject;
				if (cls->baseType.typeId == TypeId::Instance) {
					SLAKE_RETURN_IF_EXCEPT(cls->baseType.loadDeferredType(this));
					scopeObject = (MemberObject *)cls->baseType.getCustomTypeExData();
				} else {
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
				}
				break;
			}
			default:
				return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
		}
	}

	return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
}

SLAKE_API bool Runtime::getFullRef(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const {
	while (v != _rootObject) {
		switch (v->objectKind) {
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

		v = (MemberObject *)v->parent;
	};
	return true;
}
