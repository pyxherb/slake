#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>
#include <unordered_map>
#include <memory>

#include "value.h"
#include "generic.h"

namespace slake {
	class MemberValue : public Value, public AccessModified {
	public:
		Value *_parent = nullptr;
		std::string _name;

		GenericArgList _genericArgs;

		MemberValue(Runtime *rt, AccessModifier access);
		virtual ~MemberValue();

		virtual std::string getName() const;

		const Value *getParent() const;
		Value *getParent();

		virtual void bind(Value *parent, std::string name);
		virtual void unbind();

		inline MemberValue &operator=(const MemberValue &x) {
			((Value &)*this) = (Value &)x;

			setAccess(x.getAccess());
			_parent = x._parent;
			_name = x._name;

			return *this;
		}
	};
}

#endif
