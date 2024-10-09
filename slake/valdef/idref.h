#ifndef _SLAKE_VALDEF_REF_H_
#define _SLAKE_VALDEF_REF_H_

#include "object.h"
#include "generic.h"
#include <deque>

namespace slake {
	struct IdRefEntry final {
		std::pmr::string name;
		GenericArgList genericArgs;

		bool hasParamTypes = false;
		std::pmr::vector<Type> paramTypes;
		bool hasVarArg;

		IdRefEntry(std::pmr::memory_resource *memoryResource);
		IdRefEntry(std::pmr::string &&name,
			GenericArgList &&genericArgs = {},
			bool hasParamTypes = false,
			std::pmr::vector<Type> &&paramTypes = {},
			bool hasVarArg = false);
	};

	class IdRefObject final : public Object {
	public:
		std::pmr::deque<IdRefEntry> entries;

		IdRefObject(Runtime *rt);
		IdRefObject(const IdRefObject &x);
		virtual ~IdRefObject();

		virtual inline ObjectKind getKind() const override { return ObjectKind::IdRef; }

		virtual Object *duplicate() const override;

		static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		virtual void dealloc() override;
	};
}

namespace std {
	string to_string(std::vector<slake::IdRefEntry> &idRefEntries);
	string to_string(const slake::IdRefObject *ref);
}

#endif
