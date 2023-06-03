#ifndef _SLAKE_VALDEF_ARRAY_H_
#define _SLAKE_VALDEF_ARRAY_H_

#include "base.h"
#include <deque>

namespace Slake {
	class ArrayValue final : public Value {
	protected:
		std::deque<ValueRef<Value, false>> values;
		const Type type;

		class MyValueIterator : public ValueIterator {
		protected:
			decltype(values)::iterator it;

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
				return **it;
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
		inline ArrayValue(Runtime *rt, Type type)
			: Value(rt), type(type) {
			reportSizeToRuntime(sizeof(*this));
		}

		virtual inline ~ArrayValue() {
			for (auto i : values)
				delete *i;
		}
		virtual inline Type getType() const override { return ValueType::ARRAY; }
		inline Type getVarType() const { return type; }

		Value *operator[](uint32_t i) {
			if (i >= values.size())
				throw std::out_of_range("Out of array range");
			return *(values[i]);
		}

		size_t getSize() { return values.size(); }

		virtual inline ValueIterator begin() override { return MyValueIterator(values.begin()); }
		virtual inline ValueIterator end() override { return MyValueIterator(values.end()); }

		virtual inline std::string toString() const override {
			std::string s = Value::toString() + ",\"values\":[";

			for (size_t i = 0; i != values.size(); ++i) {
				s += (i ? "," : "") + values[i];
			}

			s += "[";
		}

		ArrayValue &operator=(const ArrayValue &) = delete;
		ArrayValue &operator=(const ArrayValue &&) = delete;
	};
}

#endif
