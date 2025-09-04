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

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	using GenericArgList = peff::DynArray<Type>;
	using GenericParamList = peff::DynArray<GenericParam>;

	/// @brief Three-way comparator for containers.
	struct GenericArgListComparator {
		peff::RcObjectPtr<peff::Alloc> allocator;
		mutable InternalExceptionPointer exceptPtr;

		SLAKE_FORCEINLINE GenericArgListComparator(peff::Alloc *allocator) : allocator(allocator) {}
		SLAKE_API peff::Option<int> operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept;

		SLAKE_FORCEINLINE InternalExceptionPointer getExceptionPtr() {
			assert(exceptPtr);
			return std::move(exceptPtr);
		}
	};

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListLtComparator {
		GenericArgListComparator innerComparator;
		mutable InternalExceptionPointer exceptPtr;

		SLAKE_FORCEINLINE GenericArgListLtComparator(peff::Alloc *allocator) : innerComparator(allocator) {}
		SLAKE_FORCEINLINE peff::Option<bool> operator()(const GenericArgList& lhs, const GenericArgList& rhs) const noexcept {
			auto result = innerComparator(lhs, rhs);
			if (result.hasValue())
				return result.value() < 0;
			return {};
		}

		SLAKE_FORCEINLINE InternalExceptionPointer getExceptionPtr() {
			return innerComparator.getExceptionPtr();
		}
	};

	/// @brief Equality ("==") comparator for containers.
	struct GenericArgListEqComparator {
		GenericArgListComparator innerComparator;
		mutable InternalExceptionPointer exceptPtr;

		SLAKE_FORCEINLINE GenericArgListEqComparator(peff::Alloc *allocator) : innerComparator(allocator) {}
		SLAKE_API peff::Option<bool> operator()(const GenericArgList& lhs, const GenericArgList& rhs) const noexcept {
			auto result = innerComparator(lhs, rhs);
			if (result.hasValue())
				return result.value() == 0;
			return {};
		}

		SLAKE_FORCEINLINE InternalExceptionPointer getExceptionPtr() {
			return innerComparator.getExceptionPtr();
		}
	};

	SLAKE_API size_t getGenericParamIndex(const GenericParamList &genericParamList, const std::string_view &name);

	SLAKE_API GenericParam *getGenericParam(Object *object, const std::string_view &name, Object **ownerOut = nullptr);
}

#endif
