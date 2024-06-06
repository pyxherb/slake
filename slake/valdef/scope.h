#ifndef _SLAKE_VALDEF_SCOPE_H_
#define _SLAKE_VALDEF_SCOPE_H_

#include <unordered_map>
#include <deque>
#include <stdexcept>
#include <memory>
#include <string>

namespace slake {
	class Object;
	class MemberObject;

	class Scope {
	private:
		void _getMemberChain(const std::string &name, std::deque<std::pair<Scope *, MemberObject *>> &membersOut);

	public:
		Scope *parent;
		Object *owner;
		std::unordered_map<std::string, MemberObject *> members;

		inline Scope(Object *owner, Scope *parent = nullptr) : owner(owner), parent(parent) {}

		inline MemberObject *getMember(const std::string &name) {
			if (auto it = members.find(name); it != members.end())
				return it->second;
			if (parent)
				return parent->getMember(name);
			return nullptr;
		}

		void putMember(const std::string &name, MemberObject *value);

		inline void addMember(const std::string &name, MemberObject *value) {
			if (members.find(name) != members.end())
				throw std::logic_error("The member is already exists");

			putMember(name, value);
		}

		void removeMember(const std::string &name);

		Scope *duplicate();

		inline std::deque<std::pair<Scope *, MemberObject *>> getMemberChain(const std::string &name) {
			std::deque<std::pair<Scope *, MemberObject *>> members;

			_getMemberChain(name, members);

			return members;
		}
	};
}

#endif
