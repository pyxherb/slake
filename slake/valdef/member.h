#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>

#include "base.h"

namespace Slake {
	class MemberValue : public Value, public AccessModified {
	protected:
		Value *_parent;
		std::string _name;

		friend bool Slake::isConvertible(Type a, Type b);

	public:
		inline MemberValue(Runtime *rt, AccessModifier access, Value *parent, std::string name)
			: Value(rt), AccessModified(access), _parent(parent), _name(name) {
			if (_parent)
				_parent->incRefCount();
		}
		virtual inline ~MemberValue() {
			if (!getRefCount() && _parent)
				_parent->decRefCount();
		}

		virtual inline std::string getName() const { return _name; }
		virtual inline const Value *getParent() const { return _parent; }

		virtual inline std::string toString() const override {
			return Value::toString() + ",\"parent\":" + std::to_string((uintptr_t)_parent);
		}
	};
}

#endif
