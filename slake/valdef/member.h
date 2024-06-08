#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>
#include <unordered_map>
#include <memory>

#include "object.h"
#include "generic.h"

namespace slake {
	class MemberObject : public Object, public AccessModified {
	public:
		MemberObject(Runtime *rt, AccessModifier access);
		virtual ~MemberObject();

		Object *_parent = nullptr;
		std::string _name;

		GenericArgList _genericArgs;

		virtual std::string getName() const;

		const Object *getParent() const;
		Object *getParent();

		virtual void bind(Object *parent, std::string name);
		virtual void unbind();

		inline MemberObject &operator=(const MemberObject &x) {
			((Object &)*this) = (Object &)x;

			setAccess(x.getAccess());
			_parent = x._parent;
			_name = x._name;

			return *this;
		}
	};
}

#endif
