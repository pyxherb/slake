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

		inline GenericParam() {}
		inline GenericParam(std::pmr::memory_resource *memoryResource) : name(memoryResource), interfaces(memoryResource) {
		}
	};

	using GenericArgList = std::pmr::vector<Type>;
	using GenericParamList = std::pmr::vector<GenericParam>;

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListComparator {
		inline bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
			if (lhs.size() < rhs.size())
				return true;
			if (lhs.size() > rhs.size())
				return false;

			for (size_t i = 0; i < lhs.size(); ++i) {
				if (lhs[i] < rhs[i])
					return true;
			}

			return false;
		}
	};

	/// @brief Equal ("<") comparator for containers.
	struct GenericArgListEqComparator {
		inline bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
			if (lhs.size() != rhs.size())
				return false;

			for (size_t i = 0; i < lhs.size(); ++i) {
				if (lhs[i] != rhs[i])
					return false;
			}

			return true;
		}
	};

	inline size_t getGenericParamIndex(const GenericParamList &genericParamList, const std::pmr::string &name) {
		for (size_t i = 0; i < genericParamList.size(); ++i) {
			if (genericParamList[i].name == name)
				return i;
		}

		return SIZE_MAX;
	}
}

#endif
