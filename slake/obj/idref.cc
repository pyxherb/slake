#include <slake/runtime.h>

using namespace slake;

SLAKE_API IdRefEntry::IdRefEntry(peff::Alloc *selfAllocator)
	: name(selfAllocator), genericArgs(selfAllocator) {
	// For resize() methods.
}

SLAKE_API IdRefEntry::IdRefEntry(peff::String &&name,
	GenericArgList &&genericArgs)
	: name(std::move(name)),
	  genericArgs(std::move(genericArgs)) {}

SLAKE_API void IdRefEntry::replaceAllocator(peff::Alloc *allocator) noexcept {
	name.replaceAllocator(allocator);

	genericArgs.replaceAllocator(allocator);
}

SLAKE_API slake::IdRefObject::IdRefObject(Runtime *rt, peff::Alloc *selfAllocator)
	: Object(rt, selfAllocator, ObjectKind::IdRef),
	  entries(selfAllocator),
	  paramTypes(),
	  hasVarArgs(false) {
}

SLAKE_API IdRefObject::IdRefObject(const IdRefObject &x, peff::Alloc *allocator, bool &succeededOut)
	: Object(x, allocator),
	  entries(allocator) {
	if (!(peff::copyAssign(entries, x.entries))) {
		succeededOut = false;
		return;
	}

	if (x.paramTypes.has_value()) {
		peff::DynArray<Type> copiedParamTypes(allocator);

		if (!copiedParamTypes.resize(x.paramTypes->size())) {
			succeededOut = false;
			return;
		}

		for (size_t i = 0; i < x.paramTypes->size(); ++i) {
			if (!peff::copy(copiedParamTypes.at(i), x.paramTypes->at(i))) {
				succeededOut = false;
				return;
			}
		}

		paramTypes = std::move(copiedParamTypes);
	}

	hasVarArgs = x.hasVarArgs;
}

SLAKE_API IdRefObject::~IdRefObject() {
}

SLAKE_API Object *IdRefObject::duplicate(Duplicator *duplicator) const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<IdRefObject, util::DeallocableDeleter<IdRefObject>> ptr(
		peff::allocAndConstruct<IdRefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt,
			curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<IdRefObject> slake::IdRefObject::alloc(const IdRefObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<IdRefObject, util::DeallocableDeleter<IdRefObject>> ptr(
		peff::allocAndConstruct<IdRefObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::IdRefObject::dealloc() {
	peff::destroyAndRelease<IdRefObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void IdRefObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	entries.replaceAllocator(allocator);

	for (auto& i : entries) {
		i.replaceAllocator(allocator);
	}

	if (paramTypes.has_value())
		paramTypes->replaceAllocator(allocator);
}

SLAKE_API std::string std::to_string(const slake::IdRefObject *ref) {
	string s;
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		auto &scope = ref->entries.at(i);

		if (i)
			s += ".";
		s += scope.name.data();

		if (auto nGenericParams = scope.genericArgs.size(); nGenericParams) {
			s += "<";
			for (size_t j = 0; j < nGenericParams; ++j) {
				if (j)
					s += ",";
				s += to_string(scope.genericArgs.at(j), ref->getRuntime());
			}
			s += ">";
		}
	}
	return s;
}
