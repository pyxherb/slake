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
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
		}

		virtual inline ~RootObject() {
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
		}
		virtual inline Type getType() const override { return TypeId::RootObject; }

		RootObject &operator=(const RootObject &) = delete;
		RootObject &operator=(RootObject &&) = delete;
	};
}

#endif
