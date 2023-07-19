#ifndef _SLAKE_VALDEF_OBJECT_H_
#define _SLAKE_VALDEF_OBJECT_H_

#include <unordered_map>
#include <deque>

#include "member.h"
#include "generic.h"

namespace slake {
	class ObjectValue final : public Value {
	protected:
		GenericArgList _genericArgs;
		std::unordered_map<std::string, MemberValue *> _members;
		ClassValue* _class;
		ValueRef<ObjectValue> _parent;

		inline void addMember(std::string name, MemberValue *value) {
			if (_members.count(name)) {
				_members.at(name)->unbind();
				_members.at(name)->decRefCount();
			}
			_members[name] = value;
			value->incRefCount();
			value->bind(this, name);
		}

		inline void _releaseMembers() {
			if (!refCount)
				for (auto i : _members) {
					i.second->unbind();
					i.second->decRefCount();
				}
		}

		friend class Runtime;
		friend void walkForInstantiation(Value* v);

	public:
		inline ObjectValue(Runtime *rt, ClassValue* cls, ObjectValue *parent = nullptr)
			: Value(rt), _class(cls), _parent(parent) {
			reportSizeToRuntime(sizeof(*this) - sizeof(Value));
		}

		/// @brief Delete the object and execute its destructor (if exists).
		///
		/// @note Never delete objects directly.
		virtual inline ~ObjectValue() {
			_releaseMembers();
		}

		virtual inline Type getType() const override { return Type(TypeId::OBJECT, (Value*)_class); }

		virtual inline MemberValue *getMember(std::string name) override {
			if (_members.count(name))
				return _members.at(name);
			return _parent ? _parent->getMember(name) : nullptr;
		}
		virtual inline const MemberValue *getMember(std::string name) const override {
			if (_members.count(name))
				return _members.at(name);
			return _parent ? _parent->getMember(name) : nullptr;
		}

		virtual void onRefZero() override;

		virtual Value *duplicate() const override;

		ObjectValue(ObjectValue &) = delete;
		ObjectValue(ObjectValue &&) = delete;
		inline ObjectValue &operator=(const ObjectValue &x) {
			(Value&)*this = (const Value&)x;

			_genericArgs = x._genericArgs;
			for(auto i: x._members)
				addMember(i.first, (MemberValue*)i.second->duplicate());
			_class = x._class;

			return *this;
		}
		ObjectValue &operator=(const ObjectValue &&) = delete;
	};
}

#endif
