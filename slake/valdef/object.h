#ifndef _SLAKE_VALDEF_OBJECT_H_
#define _SLAKE_VALDEF_OBJECT_H_

#include "member.h"
#include <unordered_map>

namespace Slake {
	class ObjectValue final : public Value {
	protected:
		std::unordered_map<std::string, MemberValue *> _members;
		Value *const _type;

		inline void addMember(std::string name, MemberValue *value) {
			value->incRefCount();
			if (_members.count(name))
				_members[name]->decRefCount();
			_members[name] = value;
		}

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(_members)::iterator it;

		public:
			inline MyValueIterator(decltype(it) &&it) : it(it) {}
			inline MyValueIterator(decltype(it) &it) : it(it) {}
			inline MyValueIterator(MyValueIterator &&x) noexcept : it(x.it) {}
			inline MyValueIterator(MyValueIterator &x) noexcept : it(x.it) {}
			virtual inline ValueIterator &operator++() override {
				++it;
				return *this;
			}
			virtual inline ValueIterator &&operator++(int) override {
				auto o = *this;
				++it;
				return std::move(o);
			}
			virtual inline ValueIterator &operator--() override {
				--it;
				return *this;
			}
			virtual inline ValueIterator &&operator--(int) override {
				auto o = *this;
				--it;
				return std::move(o);
			}
			virtual inline Value *operator*() override {
				return it->second;
			}

			virtual inline bool operator==(const ValueIterator &&) const override { return true; }
			virtual inline bool operator!=(const ValueIterator &&) const override { return false; }

			virtual inline ValueIterator &operator=(const ValueIterator &&x) noexcept override {
				return *this = (const MyValueIterator &&)x;
			}

			inline MyValueIterator &operator=(const MyValueIterator &&x) noexcept {
				it = x.it;
				return *this;
			}
			inline MyValueIterator &operator=(const MyValueIterator &x) noexcept {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline ObjectValue(Runtime *rt, Value *type) : Value(rt), _type(type) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~ObjectValue() {
			if (!getRefCount())
				for (auto i : _members)
					i.second->decRefCount();
		}

		virtual inline Type getType() const override { return Type(ValueType::OBJECT, _type); }

		virtual inline Value *getMember(std::string name) override {
			return _members.count(name) ? _members.at(name) : nullptr;
		}
		virtual inline const Value *getMember(std::string name) const override { return _members.at(name); }

		virtual inline ValueIterator begin() override { return MyValueIterator(_members.begin()); }
		virtual inline ValueIterator end() override { return MyValueIterator(_members.end()); }

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
