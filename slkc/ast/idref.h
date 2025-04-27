#ifndef _SLKC_AST_IDREF_H_
#define _SLKC_AST_IDREF_H_

#include "typename_base.h"
#include <peff/containers/dynarray.h>

namespace slkc {
	struct IdRefEntry {
		peff::String name;
		peff::DynArray<peff::SharedPtr<TypeNameNode>> genericArgs;
		size_t accessOpTokenIndex = SIZE_MAX, nameTokenIndex = SIZE_MAX, leftAngleBracketTokenIndex = SIZE_MAX, rightAngleBracketTokenIndex = SIZE_MAX;
		peff::DynArray<size_t> commaTokenIndices;

		SLAKE_FORCEINLINE IdRefEntry(peff::Alloc *selfAllocator): name(selfAllocator), genericArgs(selfAllocator), commaTokenIndices(selfAllocator) {}
		SLAKE_FORCEINLINE IdRefEntry(IdRefEntry&& rhs): name(std::move(rhs.name)), genericArgs(std::move(rhs.genericArgs)), accessOpTokenIndex(rhs.accessOpTokenIndex), nameTokenIndex(rhs.nameTokenIndex), leftAngleBracketTokenIndex(rhs.leftAngleBracketTokenIndex), rightAngleBracketTokenIndex(rhs.rightAngleBracketTokenIndex), commaTokenIndices(std::move(rhs.commaTokenIndices)) {
		}
	};

	SLKC_API std::optional<IdRefEntry> duplicateIdRefEntry(peff::Alloc *selfAllocator, const IdRefEntry &rhs);

	class IdRef final {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::DynArray<IdRefEntry> entries;
		TokenRange tokenRange;

		SLKC_API IdRef(peff::Alloc *selfAllocator);
		SLKC_API virtual ~IdRef();

		SLKC_API void dealloc() noexcept;
	};

	using IdRefPtr = std::unique_ptr<IdRef, peff::DeallocableDeleter<IdRef>>;

	SLKC_API IdRefPtr duplicateIdRef(peff::Alloc *selfAllocator, IdRef *rhs);
}

#endif
