#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>

namespace Slake {
	class ModuleValue : public MemberValue {
	protected:
		std::unordered_map<std::string, Value*> _members;

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(_members)::iterator it;

		public:
			inline MyValueIterator(decltype(it) &&it) noexcept : it(it) {}
			inline MyValueIterator(decltype(it) &it) noexcept : it(it) {}
			inline MyValueIterator(MyValueIterator &&x) noexcept : it(x.it) {}
			inline MyValueIterator(MyValueIterator &x) noexcept : it(x.it) {}
			virtual inline ValueIterator &operator++() override {
				++it;
				return *this;
			}
			virtual inline ValueIterator &&operator++(int) override {
				auto &o = *this;
				++it;
				return std::move(o);
			}
			virtual inline ValueIterator &operator--() override {
				--it;
				return *this;
			}
			virtual inline ValueIterator &&operator--(int) override {
				auto &o = *this;
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
			inline MyValueIterator &operator=(const MyValueIterator &x) {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline ModuleValue(Runtime *rt, AccessModifier access, Value *parent, std::string name) : MemberValue(rt, access, parent, name) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~ModuleValue() {
			if (!getRefCount())
				for (auto &i : _members)
					i.second->decRefCount();
		}

		virtual inline Value *getMember(std::string name) override { return _members.count(name) ? _members.at(name) : nullptr; }
		virtual inline const Value *getMember(std::string name) const override { return _members.count(name) ? _members.at(name) : nullptr; }
		virtual inline Type getType() const override { return ValueType::MOD; }

		virtual inline void addMember(std::string name, MemberValue *value) {
			if (_members.count(name))
				_members.at(name)->decRefCount();
			_members[name] = value;
			value->incRefCount();
		}

		virtual inline ValueIterator begin() override { return MyValueIterator(_members.begin()); }
		virtual inline ValueIterator end() override { return MyValueIterator(_members.end()); }

		virtual inline std::string toString() const override {
			std::string s = MemberValue::toString() + ",\"members\":{";

			for (auto i = _members.begin(); i != _members.end(); ++i) {
				s += (i != _members.begin() ? ",\"" : "\"") + i->first + "\":" + std::to_string((uintptr_t)i->second);
			}

			s += "}";

			return s;
		}

		ModuleValue &operator=(const ModuleValue &) = delete;
		ModuleValue &operator=(const ModuleValue &&) = delete;
	};
}

#endif
