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
	class FnObject;
	class FnOverloadingObject;

	class Scope {
	public:
		Object *owner;
		std::unordered_map<std::string, MemberObject *> members;

		inline Scope(Object *owner) : owner(owner) {}

		inline MemberObject *getMember(const std::string &name) {
			if (auto it = members.find(name); it != members.end())
				return it->second;
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
	};

	class MethodTable {
	public:
		std::unordered_map<std::string, FnObject *> methods;
		std::deque<FnOverloadingObject*> destructors;

		inline FnObject* getMethod(const std::string& name) {
			if (auto it = methods.find(name); it != methods.end())
				return it->second;
			return nullptr;
		}

		inline MethodTable* duplicate() const {
			return new MethodTable(*this);
		}
	};
}

#endif
