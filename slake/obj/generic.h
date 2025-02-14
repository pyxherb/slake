#ifndef _SLAKE_OBJ_GENERIC_H_
#define _SLAKE_OBJ_GENERIC_H_

#include <slake/type.h>
#include <cstdint>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>

namespace slake {
	struct GenericParam final {
		peff::String name;
		Type baseType = Type(TypeId::Any);
		peff::DynArray<Type> interfaces;

		SLAKE_API GenericParam();
		SLAKE_API GenericParam(peff::Alloc *selfAllocator);
		SLAKE_API GenericParam(GenericParam &&rhs);

		SLAKE_FORCEINLINE bool copy(GenericParam &dest) const {
			peff::constructAt<GenericParam>(&dest, interfaces.allocator());

			if (!peff::copyAssign(dest.name, name))
				return false;

			dest.baseType = baseType;

			if (!peff::copyAssign(dest.interfaces, interfaces))
				return false;

			return true;
		}
	};

	using GenericArgList = peff::DynArray<Type>;
	using GenericParamList = peff::DynArray<GenericParam>;

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListComparator {
		SLAKE_API bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept;
	};

	/// @brief Equal ("<") comparator for containers.
	struct GenericArgListEqComparator {
		SLAKE_API bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept;
	};

	SLAKE_API size_t getGenericParamIndex(const GenericParamList &genericParamList, const std::string_view &name);

	SLAKE_API GenericParam *getGenericParam(Object *object, const std::string_view &name, Object **ownerOut = nullptr);
}

#endif
