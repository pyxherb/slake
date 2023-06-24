#ifndef _SLAKE_VALDEF_LVAR_H_
#define _SLAKE_VALDEF_LVAR_H_

#include "base.h"

namespace Slake {
	template <ValueType VT>
	class LocalVarRefValue : public Value {
	public:
		const uint32_t off;

		inline LocalVarRefValue(Runtime *rt, uint32_t off) : off(off), Value(rt) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~LocalVarRefValue() {}

		virtual inline Type getType() const override { return VT; }

		virtual inline Value *copy() const override {
			return (Value *)new LocalVarRefValue<VT>(getRuntime(), off);
		}

		LocalVarRefValue &operator=(const LocalVarRefValue &) = delete;
		LocalVarRefValue &operator=(const LocalVarRefValue &&) = delete;
	};
}

#endif
