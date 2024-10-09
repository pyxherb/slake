#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasObject final : public MemberObject {
	public:
		mutable Object *src;
		std::string name;
		Object *parent;

		AliasObject(Runtime *rt, AccessModifier access, Object *src);
		AliasObject(const AliasObject &other);
		virtual ~AliasObject();

		virtual const char *getName() const;
		virtual void setName(const char *name);
		virtual Object *getParent() const;
		virtual void setParent(Object *parent);

		virtual inline ObjectKind getKind() const override { return ObjectKind::Alias; }

		virtual Object *duplicate() const override;

		virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		static HostObjectRef<AliasObject> alloc(Runtime *rt, Object *src);
		static HostObjectRef<AliasObject> alloc(const AliasObject *other);
		virtual void dealloc() override;
	};
}

#endif
