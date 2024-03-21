#ifndef _SLAKE_VALDEF_GENERIC_H_
#define _SLAKE_VALDEF_GENERIC_H_

#include <slake/type.h>
#include <cstdint>
#include <deque>

namespace slake {
	struct GenericParam final {
		std::string name;
		Type baseType = Type(TypeId::Any);
		std::deque<Type> interfaces, traits;
	};

	using GenericArgList = std::deque<Type>;
	using GenericParamList = std::deque<GenericParam>;

	/// @brief Less than ("<") comparator for containers.
	struct GenericArgListComparator {
		inline bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
			if (lhs.size() < rhs.size())
				return true;

			for (size_t i = 0; i < lhs.size(); ++i) {
				if (lhs[i] < rhs[i])
					return true;
			}

			return false;
		}
	};
}

#endif
