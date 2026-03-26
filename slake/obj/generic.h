#ifndef _SLAKE_OBJ_GENERIC_H_
#define _SLAKE_OBJ_GENERIC_H_

#include <slake/type.h>
#include <cstdint>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>

namespace slake {
	struct GenericParam final {
		peff::String name;
		TypeRef input_type = slake::TypeId::Invalid;
		TypeRef base_type = TypeId::Invalid;
		peff::DynArray<TypeRef> interfaces;

		SLAKE_API GenericParam(peff::Alloc *self_allocator);
		SLAKE_API GenericParam(GenericParam &&rhs);

		SLAKE_FORCEINLINE bool copy(GenericParam &dest) const {
			peff::construct_at<GenericParam>(&dest, interfaces.allocator());

			if (!dest.name.build(name))
				return false;

			dest.base_type = base_type;

			if (!dest.interfaces.resize(interfaces.size()))
				return false;
			memcpy(dest.interfaces.data(), interfaces.data(), interfaces.size() * sizeof(TypeRef));

			return true;
		}

		SLAKE_API void replace_allocator(peff::Alloc *allocator) noexcept;
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
		ParamListComparator inner_comparator;

		SLAKE_FORCEINLINE bool operator()(const ParamTypeList& lhs, const ParamTypeList& rhs) const noexcept {
			return inner_comparator(lhs, rhs) < 0;
		}
	};

	/// @brief Equality ("==") comparator for containers.
	struct GenericArgListEqComparator {
		ParamListComparator inner_comparator;

		SLAKE_FORCEINLINE GenericArgListEqComparator(peff::Alloc *allocator) {}
		SLAKE_API bool operator()(const ParamTypeList& lhs, const ParamTypeList& rhs) const noexcept {
			return inner_comparator(lhs, rhs) == 0;
		}
	};

	SLAKE_API size_t get_generic_param_index(const GenericParamList &generic_param_list, const std::string_view &name);

	SLAKE_API GenericParam *get_generic_param(Object *object, const std::string_view &name, Object **owner_out = nullptr);
}

#endif
