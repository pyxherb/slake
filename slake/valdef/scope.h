#pragma once

#include <unordered_map>
#include <deque>
#include <stdexcept>
#include <memory>

namespace slake {
	class Value;
	class MemberValue;

	class Scope {
	private:
		void _getMemberChain(const std::string &name, std::deque<std::pair<Scope *, MemberValue *>> &membersOut);

	public:
		Scope *parent;
		Value *owner;
		std::unordered_map<std::string, MemberValue *> members;

		inline Scope(Value *owner, Scope *parent = nullptr) : owner(owner), parent(parent) {}

		inline MemberValue *getMember(const std::string &name) {
			if (auto it = members.find(name); it != members.end())
				return it->second;
			if (parent)
				return parent->getMember(name);
			return nullptr;
		}

		void putMember(const std::string &name, MemberValue *value);

		inline void addMember(const std::string &name, MemberValue *value) {
			if (members.find(name) != members.end())
				throw std::logic_error("The member is already exists");

			putMember(name, value);
		}

		void removeMember(const std::string &name);

		Scope *duplicate();

		inline std::deque<std::pair<Scope *, MemberValue *>> getMemberChain(const std::string &name) {
			std::deque<std::pair<Scope *, MemberValue *>> members;

			_getMemberChain(name, members);

			return members;
		}
	};
}
