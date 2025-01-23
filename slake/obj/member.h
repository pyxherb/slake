#ifndef _SLAKE_OBJ_MEMBER_H_
#define _SLAKE_OBJ_MEMBER_H_

#include <slake/access.h>
#include <cassert>
#include <unordered_map>
#include <memory>

#include "object.h"
#include "generic.h"

namespace slake {
	class MemberObject : public Object {
	public:
		peff::String name;

		AccessModifier accessModifier = 0;

		SLAKE_API MemberObject(Runtime *rt);
		SLAKE_API MemberObject(const MemberObject &x, bool &succeededOut);
		SLAKE_API virtual ~MemberObject();

		SLAKE_API const char *getName() const;
		SLAKE_API bool setName(const char *name);
		SLAKE_API virtual Object *getParent() const;
		SLAKE_API virtual void setParent(Object *parent);
		SLAKE_API virtual const GenericArgList *getGenericArgs() const;
	};
}

#endif
