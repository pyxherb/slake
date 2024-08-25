#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasObject final : public MemberObject {
	public:
		AliasObject(Runtime *rt, AccessModifier access, Object *src);
		inline AliasObject(const AliasObject &other) : MemberObject(other) {
			src = other.src;
		}
		virtual ~AliasObject();

		mutable Object *src;

		virtual inline ObjectKind getKind() const override { return ObjectKind::Alias; }

		virtual Object *duplicate() const override;

		static HostObjectRef<AliasObject> alloc(Runtime *rt, Object *src);
		static HostObjectRef<AliasObject> alloc(const AliasObject *other);
		virtual void dealloc() override;
	};
}

#endif
