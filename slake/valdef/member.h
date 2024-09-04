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

		MemberObject(Runtime *rt);
		inline MemberObject(const MemberObject &x) : Object(x) {
			accessModifier = x.accessModifier;
		}
		virtual ~MemberObject();

		virtual const char *getName() const;
		virtual void setName(const char *name);
		virtual Object *getParent() const;
		virtual void setParent(Object *parent);
		virtual GenericArgList getGenericArgs() const;
	};
}

#endif
