#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasValue final : public MemberValue {
	public:
		mutable Value *src;

		AliasValue(Runtime *rt, AccessModifier access, Value *src);
		virtual ~AliasValue();

		virtual inline Type getType() const override { return TypeId::ALIAS; }

		virtual ValueRef<> call(std::deque<Value *> args) const override;

		virtual Value *duplicate() const override;

		inline AliasValue &operator=(const AliasValue &x) {
			((Value &)*this) = (Value &)x;

			src = x.src;

			return *this;
		}
		AliasValue &operator=(AliasValue &&) = delete;
	};

	inline Value *unwrapAlias(Value *value) noexcept {
		if (value->getType() != TypeId::ALIAS)
			return value;
		return ((AliasValue *)value)->src;
	}

	inline const Value *unwrapAlias(const Value *value) noexcept {
		if (value->getType() != TypeId::ALIAS)
			return value;
		return ((AliasValue *)value)->src;
	}
}

#endif
