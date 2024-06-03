#ifndef _SLAKE_VALDEF_REF_H_
#define _SLAKE_VALDEF_REF_H_

#include "value.h"
#include "generic.h"
#include <deque>

namespace slake {
	struct IdRefEntry final {
		std::string name;
		GenericArgList genericArgs;

		inline IdRefEntry(std::string name, GenericArgList genericArgs = {})
			: name(name), genericArgs(genericArgs) {}
	};

	class IdRefValue final : public Value {
	public:
		std::deque<IdRefEntry> entries;

		IdRefValue(Runtime *rt);
		virtual ~IdRefValue();

		virtual inline Type getType() const override { return TypeId::IdRef; }

		virtual Value *duplicate() const override;

		inline IdRefValue &operator=(const IdRefValue &x) {
			((Value&)*this) = (Value&)x;

			entries = x.entries;

			return *this;
		}
		IdRefValue &operator=(IdRefValue &&) = delete;
	};
}

namespace std {
	string to_string(std::deque<slake::IdRefEntry> &idRefEntries);
	string to_string(const slake::IdRefValue *ref);
}

#endif
