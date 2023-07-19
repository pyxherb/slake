#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>

#include "base.h"
#include "generic.h"

namespace slake {
	class MemberValue : public Value, public AccessModified {
	protected:
		Value *_parent = nullptr;
		std::string _name;
		GenericArgList _genericArgs;

		friend bool slake::isConvertible(Type a, Type b);

	public:
		MemberValue(Runtime *rt, AccessModifier access);
		virtual ~MemberValue();

#ifndef NDEBUG
		virtual inline void onRefZero() override {
			if(_parent)
				throw std::logic_error("References became zero when the member object is still bound to a parent object");
			Value::onRefZero();
		}
#endif

		virtual std::string getName() const;

		const Value *getParent() const;
		Value *getParent();

		virtual void bind(Value *parent, std::string name);
		virtual void unbind();

		inline MemberValue& operator=(const MemberValue& x) {
			((Value&)*this) = (Value&)x;

			setAccess(x.getAccess());
			_parent = x._parent;
			_name = x._name;

			return *this;
		}
	};
}

#endif
