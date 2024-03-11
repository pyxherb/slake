#ifndef _SLAKE_VALDEF_MEMBER_H_
#define _SLAKE_VALDEF_MEMBER_H_

#include <slake/access.h>
#include <cassert>
#include <unordered_map>
#include <memory>

#include "base.h"
#include "generic.h"

namespace slake {
	class MemberValue : public Value, public AccessModified {
	public:
		Value *_parent = nullptr;
		std::string _name;

		friend bool slake::isConvertible(Type a, Type b);

		GenericArgList _genericArgs;

		MemberValue(Runtime *rt, AccessModifier access);
		virtual ~MemberValue();

		virtual std::string getName() const;

		const Value *getParent() const;
		Value *getParent();

		virtual void bind(Value *parent, std::string name);
		virtual void unbind();

		inline MemberValue &operator=(const MemberValue &x) {
			((Value &)*this) = (Value &)x;

			setAccess(x.getAccess());
			_parent = x._parent;
			_name = x._name;

			return *this;
		}
	};

	class Scope {
	private:
		void _getMemberChain(const std::string &name, std::deque<std::pair<Scope*, MemberValue*>> &membersOut);

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

		inline void addMember(const std::string &name, MemberValue *value) {
			if (members.find(name) != members.end())
				throw std::logic_error("The member is already exists");

			putMember(name, value);
		}

		inline void putMember(const std::string &name, MemberValue *value) {
			members[name] = value;
			value->bind(owner, name);
		}

		inline void removeMember(const std::string &name) {
			if (auto it = members.find(name); it != members.end()) {
				it->second->unbind();
				members.erase(it);
			}

			throw std::logic_error("No such member");
		}

		inline Scope *duplicate() {
			std::unique_ptr<Scope> newScope = std::make_unique<Scope>(owner, parent);

			for (auto i : members) {
				newScope->putMember(i.first, (MemberValue *)i.second->duplicate());
			}

			return newScope.get();
		}

		inline std::deque<std::pair<Scope*, MemberValue*>> getMemberChain(const std::string &name) {
			std::deque<std::pair<Scope*, MemberValue*>> members;

			_getMemberChain(name, members);

			return members;
		}
	};

	Value *memberOf(Value *value, const std::string &name);
	std::deque<std::pair<Scope*, MemberValue*>> memberChainOf(Value *value, const std::string &name);
}

#endif
