#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>

#include "base.h"

namespace Slake {
	class MemberValue : public Value, public AccessModified {
	protected:
		Value *_parent;
		std::string _name;

		friend bool Slake::isConvertible(Type a, Type b);

	public:
		inline MemberValue(Runtime *rt, AccessModifier access, Value *parent = nullptr, std::string name = "")
			: Value(rt), AccessModified(access), _parent(parent), _name(name) {
			if (_parent)
				_parent->incRefCount();
		}
		virtual inline ~MemberValue() {
			if (!getRefCount() && _parent)
				_parent->decRefCount();
		}

		inline std::string getName() const { return _name; }
		inline const Value *getParent() const { return _parent; }

		inline void bind(Value *parent, std::string name) {
			_parent = parent, _name = name;
		}
		inline void unbind() {
			assert(_parent);
			_parent = nullptr;
			_name.clear();
		}

		virtual inline std::string toString() const override {
			return Value::toString() + ",\"parent\":" + std::to_string((uintptr_t)_parent);
		}
	};
}

#endif
