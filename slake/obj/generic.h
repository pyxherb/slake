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

			if (!dest.interfaces.resize(interfaces.size()))
				return false;
			memcpy(dest.interfaces.data(), interfaces.data(), interfaces.size() * sizeof(TypeRef));

			return true;
		}

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	using ParamTypeList = peff::DynArray<TypeRef>;
	using GenericParamList = peff::DynArray<GenericParam>;

	/// @brief Three-way comparator for containers.
	struct ParamListComparator {
		SLAKE_API int operator()(const ParamTypeList &lhs, const ParamTypeList &rhs) const noexcept;
	};

	struct GenericArgListComparator {
		SLAKE_API int operator()(const peff::DynArray<Value> &lhs, const peff::DynArray<Value> &rhs) const noexcept;
	};

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListLtComparator {
		ParamListComparator innerComparator;

		SLAKE_FORCEINLINE bool operator()(const ParamTypeList& lhs, const ParamTypeList& rhs) const noexcept {
			return innerComparator(lhs, rhs) < 0;
		}
	};

	/// @brief Equality ("==") comparator for containers.
	struct GenericArgListEqComparator {
		ParamListComparator innerComparator;

		SLAKE_FORCEINLINE GenericArgListEqComparator(peff::Alloc *allocator) {}
		SLAKE_API bool operator()(const ParamTypeList& lhs, const ParamTypeList& rhs) const noexcept {
			return innerComparator(lhs, rhs) == 0;
		}
	};

	SLAKE_API size_t getGenericParamIndex(const GenericParamList &genericParamList, const std::string_view &name);

	SLAKE_API GenericParam *getGenericParam(Object *object, const std::string_view &name, Object **ownerOut = nullptr);
}

#endif
