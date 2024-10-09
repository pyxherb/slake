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

		SLAKE_API IdRefEntry(std::pmr::memory_resource *memoryResource);
		SLAKE_API IdRefEntry(std::pmr::string &&name,
			GenericArgList &&genericArgs = {},
			bool hasParamTypes = false,
			std::pmr::vector<Type> &&paramTypes = {},
			bool hasVarArg = false);
	};

	class IdRefObject final : public Object {
	public:
		std::pmr::deque<IdRefEntry> entries;

		SLAKE_API IdRefObject(Runtime *rt);
		SLAKE_API IdRefObject(const IdRefObject &x);
		SLAKE_API virtual ~IdRefObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<IdRefObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<IdRefObject> alloc(const IdRefObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

namespace std {
	SLAKE_API string to_string(std::vector<slake::IdRefEntry> &idRefEntries);
	SLAKE_API string to_string(const slake::IdRefObject *ref);
}

#endif
