#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>
#include <unordered_map>
#include <memory>

#include "object.h"
#include "generic.h"

namespace slake {
	class MemberObject : public Object {
	public:
		AccessModifier accessModifier = 0;

		SLAKE_API MemberObject(Runtime *rt);
		SLAKE_API MemberObject(const MemberObject &x);
		SLAKE_API virtual ~MemberObject();

		SLAKE_API virtual const char *getName() const;
		SLAKE_API virtual void setName(const char *name);
		SLAKE_API virtual Object *getParent() const;
		SLAKE_API virtual void setParent(Object *parent);
		SLAKE_API virtual GenericArgList getGenericArgs() const;
	};
}

#endif
