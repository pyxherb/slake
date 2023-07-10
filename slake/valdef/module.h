#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>

namespace slake {
	class ModuleValue : public MemberValue {
	protected:
		std::unordered_map<std::string, MemberValue *> _members;

		friend class Runtime;

	public:
		ModuleValue(Runtime *rt, AccessModifier access);
		virtual ~ModuleValue();

		virtual MemberValue *getMember(std::string name) override;
		virtual const MemberValue *getMember(std::string name) const override;

		virtual Type getType() const override;

		/// @brief Add member into the module.
		/// @param name Name of the member.
		/// @param value Member to be added.
		virtual void addMember(std::string name, MemberValue *value);

		inline decltype(_members)::iterator begin() noexcept { return _members.begin(); }
		inline decltype(_members)::iterator end() noexcept { return _members.end(); }
		inline decltype(_members)::const_iterator begin() const noexcept { return _members.begin(); }
		inline decltype(_members)::const_iterator end() const noexcept { return _members.end(); }

		ModuleValue &operator=(const ModuleValue &) = delete;
		ModuleValue &operator=(const ModuleValue &&) = delete;
	};
}

#endif
