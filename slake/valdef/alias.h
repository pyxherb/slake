#ifndef _SLAKE_VALDEF_ALIAS_H_
#define _SLAKE_VALDEF_ALIAS_H_

#include "member.h"

namespace slake {
	class AliasValue final : public MemberValue {
	private:
		mutable ValueRef<Value, false> _src;
		friend class Runtime;
		friend Value *unwrapAlias(Value *value) noexcept;
		friend const Value *unwrapAlias(const Value *value) noexcept;

	public:
		inline AliasValue(Runtime *rt, AccessModifier access, Value *src)
			: MemberValue(rt, access), _src(src) {
			reportSizeToRuntime(sizeof(*this) - sizeof(MemberValue));
		}
		virtual ~AliasValue() = default;

		virtual inline Type getType() const override { return TypeId::ALIAS; }
		inline ValueRef<> getSource() const { return *_src; }

		virtual MemberValue *getMember(std::string name) override;
		virtual const MemberValue *getMember(std::string name) const override;

		virtual ValueRef<> call(std::deque<ValueRef<>> args) const override;

		virtual Value *duplicate() const override;

		inline AliasValue &operator=(const AliasValue &x) {
			((Value &)*this) = (Value &)x;

			_src = x._src;

			return *this;
		}
		AliasValue &operator=(const AliasValue &&) = delete;
	};

	inline Value *unwrapAlias(Value *value) noexcept {
		if (value->getType() != TypeId::ALIAS)
			return value;
		return *((AliasValue *)value)->_src;
	}

	inline const Value *unwrapAlias(const Value *value) noexcept {
		if (value->getType() != TypeId::ALIAS)
			return value;
		return *((AliasValue *)value)->_src;
	}
}

#endif
