#ifndef _SLAKE_OBJ_GENERIC_H_
#define _SLAKE_OBJ_GENERIC_H_

#include <slake/type.h>
#include <cstdint>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>

namespace slake {
	struct GenericParam final {
		peff::String name;
		TypeRef baseType = TypeId::Any;
		peff::DynArray<TypeRef> interfaces;

		SLAKE_API GenericParam(peff::Alloc *selfAllocator);
		SLAKE_API GenericParam(GenericParam &&rhs);

		SLAKE_FORCEINLINE bool copy(GenericParam &dest) const {
			peff::constructAt<GenericParam>(&dest, interfaces.allocator());

			if (!dest.name.build(name))
				return false;

			dest.baseType = baseType;

			if (!peff::copyAssign(dest.interfaces, interfaces))
				return false;

			return true;
		}

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	using GenericArgList = peff::DynArray<TypeRef>;
	using GenericParamList = peff::DynArray<GenericParam>;

	/// @brief Three-way comparator for containers.
	struct GenericArgListComparator {
		SLAKE_FORCEINLINE GenericArgListComparator() {}
		SLAKE_API int operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept;
	};

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListLtComparator {
		GenericArgListComparator innerComparator;

		SLAKE_FORCEINLINE GenericArgListLtComparator(peff::Alloc *allocator) {}
		SLAKE_FORCEINLINE bool operator()(const GenericArgList& lhs, const GenericArgList& rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};

	/// @brief Equality ("==") comparator for containers.
	struct GenericArgListEqComparator {
		GenericArgListComparator innerComparator;

		SLAKE_FORCEINLINE GenericArgListEqComparator(peff::Alloc *allocator) {}
		SLAKE_API bool operator()(const GenericArgList& lhs, const GenericArgList& rhs) const noexcept {
			return innerComparator(lhs, rhs) == 0;
		}
	};

	SLAKE_API size_t getGenericParamIndex(const GenericParamList &genericParamList, const std::string_view &name);

	SLAKE_API GenericParam *getGenericParam(Object *object, const std::string_view &name, Object **ownerOut = nullptr);
}

#endif
