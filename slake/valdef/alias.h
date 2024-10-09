#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasObject final : public MemberObject {
	public:
		mutable Object *src;
		std::string name;
		Object *parent;

		SLAKE_API AliasObject(Runtime *rt, AccessModifier access, Object *src);
		SLAKE_API AliasObject(const AliasObject &other);
		SLAKE_API virtual ~AliasObject();

		SLAKE_API virtual const char *getName() const;
		SLAKE_API virtual void setName(const char *name);
		SLAKE_API virtual Object *getParent() const;
		SLAKE_API virtual void setParent(Object *parent);

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		SLAKE_API static HostObjectRef<AliasObject> alloc(Runtime *rt, Object *src);
		SLAKE_API static HostObjectRef<AliasObject> alloc(const AliasObject *other);
		SLAKE_API virtual void dealloc() override;
	};
}

#endif
