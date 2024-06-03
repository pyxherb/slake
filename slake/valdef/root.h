#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "value.h"
#include "member.h"

namespace slake {
	class RootValue final : public Value {
	public:
		inline RootValue(Runtime *rt)
			: Value(rt) {
			scope = new Scope(this);
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual inline ~RootValue() {
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
		}
		virtual inline Type getType() const override { return TypeId::RootValue; }

		RootValue &operator=(const RootValue &) = delete;
		RootValue &operator=(RootValue &&) = delete;
	};
}

#endif
