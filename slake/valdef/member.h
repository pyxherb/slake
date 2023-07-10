#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>

#include "base.h"

namespace slake {
	class MemberValue : public Value, public AccessModified {
	protected:
		Value *_parent = nullptr;
		std::string _name;

		friend bool slake::isConvertible(Type a, Type b);

	public:
		MemberValue(Runtime *rt, AccessModifier access);
		virtual ~MemberValue();

		std::string getName() const;

		const Value *getParent() const;
		Value *getParent();

		void bind(Value *parent, std::string name);
		void unbind();
	};
}

#endif
