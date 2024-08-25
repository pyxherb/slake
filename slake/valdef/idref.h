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
		IdRefObject(const IdRefObject &x) : Object(x) {
			entries = x.entries;
		}
		virtual ~IdRefObject();

		std::deque<IdRefEntry> entries;

		virtual inline ObjectKind getKind() const override { return ObjectKind::IdRef; }

		virtual Object *duplicate() const override;

		static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		virtual void dealloc() override;
	};
}

namespace std {
	string to_string(std::deque<slake::IdRefEntry> &idRefEntries);
	string to_string(const slake::IdRefObject *ref);
}

#endif
