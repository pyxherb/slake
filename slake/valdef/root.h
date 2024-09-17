#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "object.h"
#include "member.h"

namespace slake {
	class RootObject final : public Object {
	public:
		Scope *scope;

		RootObject(Runtime *rt);
		virtual inline ~RootObject() {
			scope->dealloc();
		}

		virtual inline ObjectKind getKind() const override { return ObjectKind::RootObject; }

		virtual MemberObject *getMember(
			const std::pmr::string &name,
			VarRefContext *varRefContextOut) const;

		static HostObjectRef<RootObject> alloc(Runtime *rt);
		virtual void dealloc() override;
	};
}

#endif
