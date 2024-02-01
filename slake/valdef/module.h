#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
	class ModuleValue : public MemberValue {
	public:
		std::unordered_map<std::string, MemberValue *> _members;

		ModuleValue(Runtime *rt, AccessModifier access);
		virtual ~ModuleValue();

		virtual MemberValue *getMember(std::string name) override;
		virtual const MemberValue *getMember(std::string name) const override;

		virtual Type getType() const override;

		/// @brief Add a member into the module.
		/// @param name Name of the member.
		/// @param value Member to be added.
		virtual void addMember(std::string name, MemberValue *value);

		inline decltype(_members)::iterator begin() noexcept { return _members.begin(); }
		inline decltype(_members)::iterator end() noexcept { return _members.end(); }
		inline decltype(_members)::const_iterator begin() const noexcept { return _members.begin(); }
		inline decltype(_members)::const_iterator end() const noexcept { return _members.end(); }

		virtual Value *duplicate() const override;

		inline ModuleValue &operator=(const ModuleValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;

			for (const auto &i : x._members) {
				addMember(i.first, (MemberValue *)i.second->duplicate());
			}

			return *this;
		}
		ModuleValue &operator=(ModuleValue &&) = delete;
	};
}

#endif
