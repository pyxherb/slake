#include <slake/runtime.h>

using namespace slake;

SLAKE_API InternalExceptionPointer Runtime::resolveIdRef(
	IdRefObject *ref,
	Reference &objectRefOut,
	Object *scopeObject) {
	assert(ref);

	Object *curObject = scopeObject;

	if ((!curObject))
		if (!(curObject = _rootObject))
			std::terminate();

	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &curName = ref->entries.at(i);

		if (!curObject)
			goto fail;

		objectRefOut = curObject->getMember(curName.name);

		if (!objectRefOut) {
			goto fail;
		}

		if (objectRefOut.kind == ReferenceKind::ObjectRef) {
			// TODO: Check if the instance object is a member object.
			curObject = (MemberObject *)objectRefOut.asObject;

			switch (curObject->getObjectKind()) {
				case ObjectKind::Class:
				case ObjectKind::Interface:
				case ObjectKind::Fn:
					if (curName.genericArgs.size()) {
						peff::NullAlloc nullAlloc;
						GenericInstantiationContext genericInstantiationContext(&nullAlloc, getFixedAlloc());

						genericInstantiationContext.genericArgs = &curName.genericArgs;
						MemberObject *m;
						SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject((MemberObject *)curObject, m, &genericInstantiationContext));
						curObject = m;
						objectRefOut = Reference::makeObjectRef(curObject);
					}
					break;
			}
		} else {
			if (i + 1 != ref->entries.size())
				return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
		}
	}

	if (ref->paramTypes.hasValue() || ref->hasVarArgs) {
		switch (curObject->getObjectKind()) {
			case ObjectKind::Fn: {
				FnObject *fnObject = ((FnObject *)curObject);

				const ParamTypeList &paramTypes = *ref->paramTypes;

				auto it = fnObject->overloadings.find(FnSignature{ paramTypes, ref->hasVarArgs, ref->entries.back().genericArgs.size(), ref->overridenType });

				if (it != fnObject->overloadings.end())
					objectRefOut = Reference::makeObjectRef(it.value());
				else {
					it = fnObject->overloadings.find(FnSignature{ paramTypes, ref->hasVarArgs, ref->entries.back().genericArgs.size(), TypeId::Void });

					if (it == fnObject->overloadings.end())
						return allocOutOfMemoryErrorIfAllocFailed(ReferencedMemberNotFoundError::alloc(const_cast<Runtime *>(this)->getFixedAlloc(), ref));
				}

				objectRefOut = Reference::makeObjectRef(it.value());

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
		ParamTypeList copiedGenericArgs(allocator);
		if (auto p = v->getGenericArgs(); p) {
			if (!copiedGenericArgs.resize(p->size()))
				return false;
			for (size_t i = 0; i < copiedGenericArgs.size(); ++i) {
				copiedGenericArgs.at(i) = p->at(i);
			}
		}

		if (!idRefOut.pushFront(IdRefEntry(std::move(copiedName), std::move(copiedGenericArgs)))) {
			return false;
		}

		v = (MemberObject *)v->parent;
	};
	return true;
}
