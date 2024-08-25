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
		AccessModifier accessModifier;

		MemberObject(Runtime *rt, AccessModifier access);
		inline MemberObject(const MemberObject &x) : Object(x) {
			accessModifier = x.accessModifier;
			_parent = x._parent;
			_name = x._name;
		}
		virtual ~MemberObject();

		Object *_parent = nullptr;
		std::string _name;

		GenericArgList _genericArgs;

		virtual std::string getName() const;

		const Object *getParent() const;
		Object *getParent();

		virtual void bind(Object *parent, std::string name);
		virtual void unbind();
	};
}

#endif
