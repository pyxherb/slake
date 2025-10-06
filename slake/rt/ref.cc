#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::resolveIdRef(
	IdRefObject *ref,
	EntityRef &objectRefOut,
	Object *scopeObject) {
	assert(ref);

	if ((!scopeObject))
		if (!(scopeObject = _rootObject))
			std::terminate();

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
			scopeObject = (MemberObject *)objectRefOut.asObject;

			switch (scopeObject->getObjectKind()) {
				case ObjectKind::Class:
				case ObjectKind::Interface:
				case ObjectKind::Fn:
					if (curName.genericArgs.size()) {
						peff::NullAlloc nullAlloc;
						GenericInstantiationContext genericInstantiationContext(&nullAlloc, getFixedAlloc());

						genericInstantiationContext.genericArgs = &curName.genericArgs;
						MemberObject *m;
						SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject((MemberObject *)scopeObject, m, &genericInstantiationContext));
						scopeObject = m;
						objectRefOut = EntityRef::makeObjectRef(scopeObject);
					}
					break;
			}
		} else {
			if (i + 1 != ref->entries.size())
				return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
		}
	}

	if (ref->paramTypes.hasValue() || ref->hasVarArgs) {
		switch (scopeObject->getObjectKind()) {
			case ObjectKind::Fn: {
				FnObject *fnObject = ((FnObject *)scopeObject);

				FnOverloadingObject *overloading = findOverloading(fnObject, *ref->paramTypes, 0, ref->hasVarArgs);

				if (!overloading)
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));

				if (!(objectRefOut = EntityRef::makeObjectRef(overloading)))
					return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));

				break;
			}
			default:
				return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
		}
	}

	return {};

fail:;

	return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
}

SLAKE_API bool Runtime::getFullRef(peff::Alloc *allocator, const MemberObject *v, peff::DynArray<IdRefEntry> &idRefOut) const {
	while (v != _rootObject) {
		if (!v) {
			peff::String name(allocator);

			if (!name.build("(Orphaned)"))
				return false;

			if (!idRefOut.pushFront(IdRefEntry(std::move(name), { allocator }))) {
				return false;
			}

			return true;
		}

		switch (v->getObjectKind()) {
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
