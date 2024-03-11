#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "base.h"
#include "member.h"

namespace slake {
	class RootValue final : public Value {
	public:
		std::unique_ptr<Scope> scope;

		inline RootValue(Runtime *rt)
			: Value(rt), scope(std::make_unique<Scope>(this)) {
			reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual inline ~RootValue() {
			reportSizeFreedToRuntime(sizeof(*this) - sizeof(Value));
		}
		virtual inline Type getType() const override { return TypeId::ROOT; }

		RootValue &operator=(const RootValue &) = delete;
		RootValue &operator=(RootValue &&) = delete;
	};
}

#endif
