#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasObject final : public MemberObject {
	public:
		AliasObject(Runtime *rt, AccessModifier access, Object *src);
		virtual ~AliasObject();

		mutable Object *src;

		virtual inline Type getType() const override { return TypeId::Alias; }

		virtual Object *duplicate() const override;

		static HostObjectRef<AliasObject> alloc(Runtime *rt, Object *src);
		virtual void dealloc() override;

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
