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
		IdRefObject(Runtime *rt);
		virtual ~IdRefObject();

		std::deque<IdRefEntry> entries;

		virtual inline Type getType() const override { return TypeId::IdRef; }

		virtual Object *duplicate() const override;

		static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		virtual void dealloc() override;

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
