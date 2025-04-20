#include "idref.h"

using namespace slkc;

SLKC_API IdRef::IdRef(
	peff::Alloc *selfAllocator,
	const peff::WeakPtr<Document> &document)
	: selfAllocator(selfAllocator),
	  entries(selfAllocator),
	  document(document) {
}

SLKC_API IdRef::~IdRef() {
}

SLKC_API void IdRef::dealloc() noexcept {
	peff::destroyAndRelease<IdRef>(selfAllocator.get(), this, ASTNODE_ALIGNMENT);
}

SLKC_API std::optional<IdRefEntry> slkc::duplicateIdRefEntry(peff::Alloc *selfAllocator, const IdRefEntry &rhs) {
	IdRefEntry newIdRefEntry(selfAllocator);

	if (!newIdRefEntry.genericArgs.build(rhs.genericArgs)) {
		return {};
	}

	if (!newIdRefEntry.name.build(rhs.name)) {
		return {};
	}

	newIdRefEntry.nameTokenIndex = rhs.nameTokenIndex;
	newIdRefEntry.leftAngleBracketTokenIndex = rhs.leftAngleBracketTokenIndex;
	newIdRefEntry.rightAngleBracketTokenIndex = rhs.rightAngleBracketTokenIndex;
	newIdRefEntry.accessOpTokenIndex = rhs.accessOpTokenIndex;

	if (!newIdRefEntry.commaTokenIndices.build(rhs.commaTokenIndices)) {
		return {};
	}

	return std::move(newIdRefEntry);
}

SLKC_API IdRefPtr slkc::duplicateIdRef(peff::Alloc *selfAllocator, IdRef *rhs) {
	IdRefPtr newIdRefPtr = IdRefPtr(
		peff::allocAndConstruct<IdRef>(
			selfAllocator,
			ASTNODE_ALIGNMENT,
			selfAllocator,
			rhs->document));

	if (!newIdRefPtr->entries.resizeUninitialized(rhs->entries.size())) {
		return {};
	}

	for (size_t i = 0; i < rhs->entries.size(); ++i) {
		std::optional<IdRefEntry> duplicatedEntry = duplicateIdRefEntry(selfAllocator, rhs->entries.at(i));

		if (!duplicatedEntry.has_value())
			return {};

		peff::constructAt<IdRefEntry>(&newIdRefPtr->entries.at(i), std::move(*duplicatedEntry));
		duplicatedEntry.reset();
	}

	newIdRefPtr->tokenRange = rhs->tokenRange;
	newIdRefPtr->document = rhs->document;

	return newIdRefPtr;
}
