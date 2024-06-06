#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasObject final : public MemberObject {
	public:
		mutable Object *src;

		AliasObject(Runtime *rt, AccessModifier access, Object *src);
		virtual ~AliasObject();

		virtual inline Type getType() const override { return TypeId::Alias; }

		virtual Object *duplicate() const override;

		inline AliasObject &operator=(const AliasObject &x) {
			((Object &)*this) = (Object &)x;

			src = x.src;

			return *this;
		}
		AliasObject &operator=(AliasObject &&) = delete;
	};

	inline Object *unwrapAlias(Object *value) noexcept {
		if (value->getType() != TypeId::Alias)
			return value;
		return ((AliasObject *)value)->src;
	}

	inline const Object *unwrapAlias(const Object *value) noexcept {
		if (value->getType() != TypeId::Alias)
			return value;
		return ((AliasObject *)value)->src;
	}
}

#endif
