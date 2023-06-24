#ifndef _SLAKE_VALDEF_OBJECT_H_
#define _SLAKE_VALDEF_OBJECT_H_

#include <unordered_map>

#include "member.h"

namespace Slake {
	class ObjectValue final : public Value {
	protected:
		std::unordered_map<std::string, MemberValue *> _members;
		Value *const _type;
		ObjectValue *_parent = nullptr;

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
			if (!getRefCount())
				for (auto i : _members) {
					i.second->unbind();
					i.second->decRefCount();
				}
		}

		friend class Runtime;

	public:
		inline ObjectValue(Runtime *rt, Value *type, ObjectValue *parent = nullptr)
			: Value(rt), _type(type), _parent(parent) {
			reportSizeToRuntime(sizeof(*this));
		}

		/// @brief Delete the object and execute its destructor (if exists).
		///
		/// @note Do not delete objects directly.
		virtual inline ~ObjectValue() {
			_releaseMembers();
		}

		virtual inline Type getType() const override { return Type(ValueType::OBJECT, _type); }

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

		virtual void whenRefBecomeZero() override;

		ObjectValue(ObjectValue &) = delete;
		ObjectValue(ObjectValue &&) = delete;
		ObjectValue &operator=(const ObjectValue &) = delete;
		ObjectValue &operator=(const ObjectValue &&) = delete;

		virtual inline std::string toString() const override {
			std::string s = Value::toString() + ",\"classtype\":" + std::to_string((uintptr_t)_type) + ",\"members\":{";

			for (auto i = _members.begin(); i != _members.end(); ++i) {
				s += (i != _members.begin() ? ",\"" : "\"") + i->first + "\":" + std::to_string((uintptr_t)i->second);
			}

			s += "}";

			return s;
		}
	};
}

#endif
