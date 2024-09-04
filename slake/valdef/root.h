#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "object.h"
#include "member.h"

namespace slake {
	class RootObject final : public Object {
	public:
		Scope *scope;

		inline RootObject(Runtime *rt)
			: Object(rt) {
			scope = new Scope(this);
		}
		virtual inline ~RootObject() {
			delete scope;
		}

		virtual inline ObjectKind getKind() const override { return ObjectKind::RootObject; }

		virtual MemberObject *getMember(
			const std::string &name,
			VarRefContext *varRefContextOut) const;

		static HostObjectRef<RootObject> alloc(Runtime *rt);
		virtual void dealloc() override;
	};
}

#endif
