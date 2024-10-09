#ifndef _SLAKE_VALDEF_GENERIC_H_
#define _SLAKE_VALDEF_GENERIC_H_

#include <slake/type.h>
#include <cstdint>
#include <deque>

namespace slake {
	struct GenericParam final {
		std::pmr::string name;
		Type baseType = Type(TypeId::Any);
		std::pmr::vector<Type> interfaces;

		GenericParam();
		GenericParam(std::pmr::memory_resource *memoryResource);
	};


	using GenericArgList = std::pmr::vector<Type>;
	using GenericParamList = std::pmr::vector<GenericParam>;

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListComparator {
		bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept;
	};

	/// @brief Equal ("<") comparator for containers.
	struct GenericArgListEqComparator {
		bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept;
	};

	size_t getGenericParamIndex(const GenericParamList &genericParamList, const std::pmr::string &name);

	GenericParam *getGenericParam(Object *object, const std::pmr::string &name, Object **ownerOut = nullptr);
}

#endif
