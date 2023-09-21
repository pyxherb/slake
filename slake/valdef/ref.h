#ifndef _SLAKE_VALDEF_REF_H_
#define _SLAKE_VALDEF_REF_H_

#include "base.h"
#include "generic.h"
#include <deque>

namespace slake {
	struct RefEntry final {
		std::string name;
		GenericArgList genericArgs;

		inline RefEntry(std::string name, GenericArgList genericArgs = {})
			: name(name), genericArgs(genericArgs) {}
	};

	class RefValue final : public Value {
	public:
		std::deque<RefEntry> entries;

		inline RefValue(Runtime *rt) : Value(rt) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}
		virtual ~RefValue() = default;
		virtual inline Type getType() const override { return TypeId::REF; }

		virtual Value *duplicate() const override;

		RefValue &operator=(const RefValue &x) {
			((Value&)*this) = (Value&)x;

			entries = x.entries;

			return *this;
		}
		RefValue &operator=(RefValue &&) = delete;
	};
}

namespace std {
	string to_string(const slake::RefValue *ref);
}

#endif
