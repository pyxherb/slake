#ifndef _SLAKE_VALDEF_GENERIC_H_
#define _SLAKE_VALDEF_GENERIC_H_

#include <slake/type.h>
#include <cstdint>
#include <deque>

namespace slake {
	struct GenericParam final {
		std::pmr::string name;
		Type baseType = Type(TypeId::Any);
		std::deque<Type> interfaces;

		inline GenericParam(std::pmr::memory_resource *memoryResource) : name(memoryResource) {
		}
	};

	using GenericArgList = std::deque<Type>;
	using GenericParamList = std::deque<GenericParam>;

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
}

#endif
