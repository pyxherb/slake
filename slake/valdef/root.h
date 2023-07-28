#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "base.h"
#include "member.h"
#include <unordered_map>

namespace slake {
	class RootValue final : public Value {
	protected:
		std::unordered_map<std::string, ValueRef<MemberValue, false>> _members;

		friend class Runtime;

	public:
		inline RootValue(Runtime *rt)
			: Value(rt) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}

		virtual ~RootValue() = default;
		virtual inline Type getType() const override { return TypeId::ROOT; }

		virtual inline MemberValue *getMember(std::string name) override {
			return _members.count(name) ? *(_members.at(name)) : nullptr;
		}
		virtual inline const MemberValue *getMember(std::string name) const override {
			return _members.count(name) ? *(_members.at(name)) : nullptr;
		}

		virtual inline void addMember(std::string name, MemberValue *value) {
			if (_members.count(name))
				_members.at(name)->unbind();
			_members[name] = value;
			value->bind(this, name);
		}

		inline decltype(_members)::iterator begin() { return _members.begin(); }
		inline decltype(_members)::iterator end() { return _members.end(); }
		inline decltype(_members)::const_iterator begin() const { return _members.begin(); }
		inline decltype(_members)::const_iterator end() const { return _members.end(); }

		RootValue &operator=(const RootValue &) = delete;
		RootValue &operator=(const RootValue &&) = delete;
	};
}

#endif
