#ifndef _SLAKE_VALDEF_REF_H_
#define _SLAKE_VALDEF_REF_H_

#include "base.h"
#include <vector>

namespace Slake {
	class RefValue final : public Value {
	public:
		std::vector<std::string> scopes;

		inline RefValue(Runtime *rt) : Value(rt) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~RefValue() {}
		virtual inline Type getType() const override { return ValueType::REF; }

		RefValue &operator=(const RefValue &) = delete;
		RefValue &operator=(const RefValue &&) = delete;

		virtual inline std::string toString() const override {
			std::string s = Value::toString() + ",\"scopes\":[";

			for (size_t i = 0; i != scopes.size(); ++i) {
				s += (i ? ",\"" : "\"") + scopes[i] + "\"";
			}

			s += "]";

			return s;
		}
	};
}

#endif
