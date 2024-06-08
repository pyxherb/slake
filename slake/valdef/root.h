#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "object.h"
#include "member.h"

namespace slake {
	class RootObject final : public Object {
	public:
		inline RootObject(Runtime *rt)
			: Object(rt) {
			scope = new Scope(this);
		}
		virtual inline ~RootObject() {
		}

		virtual inline Type getType() const override { return TypeId::RootObject; }

		static HostObjectRef<RootObject> alloc(Runtime *rt);
		virtual void dealloc() override;

		RootObject &operator=(const RootObject &) = delete;
		RootObject &operator=(RootObject &&) = delete;
	};
}

#endif
