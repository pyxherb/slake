#ifndef _SLAKE_VALDEF_REF_H_
#define _SLAKE_VALDEF_REF_H_

#include "base.h"
#include <deque>

namespace slake {
	class RefValue final : public Value {
	public:
		std::deque<std::string> scopes;
		std::deque<Type> genericArgs;

		inline RefValue(Runtime *rt) : Value(rt) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}
		virtual ~RefValue() = default;
		virtual inline Type getType() const override { return ValueType::REF; }

		RefValue &operator=(const RefValue &) = delete;
		RefValue &operator=(const RefValue &&) = delete;
	};
}

namespace std {
	inline string to_string(slake::RefValue *ref) {
		string s;
		for (size_t i = 0; i < ref->scopes.size(); ++i) {
			if (i)
				s += ".";
			s += ref->scopes[i];
		}
		return s;
	}
}

#endif
