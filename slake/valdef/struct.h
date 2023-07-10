#ifndef _SLAKE_VALDEF_STRUCT_H_
#define _SLAKE_VALDEF_STRUCT_H_

#include "object.h"

namespace slake {
	class StructValue : public MemberValue {
	protected:
		std::unordered_map<std::string, ValueRef<MemberValue, false>> _members;

	public:
		inline StructValue(Runtime *rt, AccessModifier access) : MemberValue(rt, access) {
			reportSizeToRuntime(sizeof(*this) - sizeof(MemberValue));
		}
		virtual ~StructValue() = default;

		virtual inline Type getType() const override { return ValueType::STRUCT; }

		virtual inline MemberValue *getMember(std::string name) override {
			return *(_members.at(name));
		}
		virtual inline const MemberValue *getMember(std::string name) const override { return *(_members.at(name)); }

		ObjectValue &operator=(const ObjectValue &) = delete;
		ObjectValue &operator=(const ObjectValue &&) = delete;
	};
}

#endif
