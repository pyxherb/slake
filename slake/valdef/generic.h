#pragma once

#include <slake/type.h>
#include <cstdint>
#include <deque>

namespace slake {
	enum class GenericFilter : uint8_t {
		Extends = 0,  // Derived from a class
		Implements,	  // Implements an interace
		HasTrait	  // Has a trait
	};

	struct GenericQualifier final {
		GenericFilter filter;
		Type type;

		inline GenericQualifier(GenericFilter filter, Type type)
			: filter(filter), type(type) {}
	};

	struct GenericParam final {
		std::string name;
		std::deque<GenericQualifier> qualifiers;

		inline GenericParam(std::string name, std::deque<GenericQualifier> qualifiers)
			: name(name), qualifiers(qualifiers) {}
	};

	using GenericArgList = std::deque<Type>;
	using GenericParamList = std::deque<GenericParam>;

	/// @brief Less than ("<") comparator for containers such as map and set.
	struct GenericArgListComparator {
		bool operator()(const GenericArgList &lhs, const GenericArgList &rhs) const noexcept {
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
