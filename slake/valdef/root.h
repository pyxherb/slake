#ifndef _SLAKE_VALDEF_ROOT_H_
#define _SLAKE_VALDEF_ROOT_H_

#include "base.h"
#include <unordered_map>

namespace Slake {
	class RootValue final : public Value {
	protected:
		std::unordered_map<std::string, ValueRef<Value, false>> _members;

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
				return *(it->second);
			}

			virtual inline bool operator==(const ValueIterator &&) const override { return true; }
			virtual inline bool operator!=(const ValueIterator &&) const override { return false; }

			virtual inline ValueIterator &operator=(const ValueIterator &&x) noexcept override {
				return *this = (const MyValueIterator &&)x;
			}

			virtual inline MyValueIterator &operator=(const MyValueIterator &&x) {
				it = x.it;
				return *this;
			}
			inline MyValueIterator &operator=(const MyValueIterator &x) {
				return *this = std::move(x);
			}
		};

		friend class Runtime;

	public:
		inline RootValue(Runtime *rt)
			: Value(rt) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~RootValue() {
		}
		virtual inline Type getType() const override { return ValueType::ROOT; }

		virtual inline Value *getMember(std::string name) override {
			return _members.count(name) ? *(_members.at(name)) : nullptr;
		}
		virtual inline const Value *getMember(std::string name) const override {
			return _members.count(name) ? *(_members.at(name)) : nullptr;
		}

		virtual inline void addMember(std::string name, Value *value) {
			_members[name] = value;
		}

		virtual inline ValueIterator begin() override { return MyValueIterator(_members.begin()); }
		virtual inline ValueIterator end() override { return MyValueIterator(_members.end()); }

		virtual inline std::string toString() const override {
			std::string s = Value::toString() + ",\"members\":{";

			for (auto i = _members.begin(); i != _members.end(); ++i) {
				s += (i != _members.begin() ? ",\"" : "\"") + i->first + "\":" + std::to_string((uintptr_t)i->second);
			}

			s += "}";

			return s;
		}

		RootValue &operator=(const RootValue &) = delete;
		RootValue &operator=(const RootValue &&) = delete;
	};
}

#endif
