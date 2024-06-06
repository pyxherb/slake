#ifndef _SLAKE_VALDEF_REF_H_
#define _SLAKE_VALDEF_REF_H_

#include "object.h"
#include "generic.h"
#include <deque>

namespace slake {
	struct IdRefEntry final {
		std::string name;
		GenericArgList genericArgs;

		inline IdRefEntry(std::string name, GenericArgList genericArgs = {})
			: name(name), genericArgs(genericArgs) {}
	};

	class IdRefObject final : public Object {
	public:
		std::deque<IdRefEntry> entries;

		IdRefObject(Runtime *rt);
		virtual ~IdRefObject();

		virtual inline Type getType() const override { return TypeId::IdRef; }

		virtual Object *duplicate() const override;

		inline IdRefObject &operator=(const IdRefObject &x) {
			((Object&)*this) = (Object&)x;

			entries = x.entries;

			return *this;
		}
		IdRefObject &operator=(IdRefObject &&) = delete;
	};
}

namespace std {
	string to_string(std::deque<slake::IdRefEntry> &idRefEntries);
	string to_string(const slake::IdRefObject *ref);
}

#endif
