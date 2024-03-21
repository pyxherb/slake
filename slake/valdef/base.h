#ifndef _SLAKE_VALDEF_BASE_H_
#define _SLAKE_VALDEF_BASE_H_

#include "scope.h"
#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>

namespace slake {
	class Runtime;
	class MemberValue;
	class Value;

	bool _isRuntimeInDestruction(Runtime *runtime);

	template <typename T = Value>
	class ValueRef final {
	public:
		T *_value = nullptr;
		Runtime *_rt = nullptr;

		inline void reset() {
			if (_value) {
				--_value->hostRefCount;
				_value = nullptr;
			}
		}

		inline void discard() noexcept { _value = nullptr; }

		inline ValueRef(const ValueRef<T> &x) : _value(x._value) {
			if (x._value) {
				++_value->hostRefCount;
				_rt = x->_rt;
			}
		}
		inline ValueRef(ValueRef<T> &&x) noexcept : _value(x._value) {
			if (x._value) {
				_rt = x->_rt;
				x._value = nullptr;
			}
		}
		inline ValueRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				++_value->hostRefCount;
				_rt = value->_rt;
			}
		}
		inline ~ValueRef() {
			reset();
		}

		inline const T *get() const { return _value; }
		inline T *get() { return _value; }
		inline const T *operator->() const { return _value; }
		inline T *operator->() { return _value; }

		inline ValueRef<T> &operator=(const ValueRef<T> &x) {
			reset();

			if ((_value = x._value)) {
				++_value->hostRefCount;
				_rt = _value->_rt;
			}

			return *this;
		}
		inline ValueRef<T> &operator=(ValueRef<T> &&x) noexcept {
			reset();

			if ((_value = x._value)) {
				_rt = _value->_rt;
				x._value = nullptr;
			}

			return *this;
		}

		inline ValueRef<T> &operator=(T *other) {
			reset();

			if ((_value = other)) {
				++_value->hostRefCount;
				_rt = _value->_rt;
			}

			return *this;
		}

		inline bool operator<(const ValueRef<T> &rhs) const noexcept {
			return _value < rhs._value;
		}
		inline bool operator>(const ValueRef<T> &rhs) const noexcept {
			return _value > rhs._value;
		}
		inline bool operator==(const ValueRef<T> &rhs) const noexcept {
			return _value == rhs._value;
		}

		inline operator bool() const {
			return _value;
		}
	};

	using ValueFlags = uint8_t;
	constexpr static ValueFlags
		VF_WALKED = 0x01,  // The value has been walked by the garbage collector.
		VF_ALIAS = 0x02	   // The value is an alias thus the scope should not be deleted.
		;

	struct Type;
	class Scope;

	class Value {
	protected:
		void reportSizeAllocatedToRuntime(size_t size);
		void reportSizeFreedToRuntime(size_t size);

		friend class Runtime;

	public:
		// The value will never be freed if its host reference count is not 0.
		mutable std::atomic_uint32_t hostRefCount = 0;

		Runtime *_rt;
		ValueFlags _flags = 0;

		Scope *scope = nullptr;

		/// @brief The basic constructor.
		/// @param rt Runtime which the value belongs to.
		Value(Runtime *rt);
		virtual ~Value();

		/// @brief Get type of the value.
		/// @return Type of the value.
		virtual Type getType() const = 0;

		/// @brief Call the value as callable.
		/// @param nArgs Number of arguments.
		/// @param args Pointer to linear-arranged arguments.
		/// @return Result of the calling.
		virtual ValueRef<> call(Value *thisObject, std::deque<Value *> args) const;

		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		virtual Value *duplicate() const;

		inline Runtime *getRuntime() const noexcept { return _rt; }

		Value *getMember(const std::string &name);
		std::deque<std::pair<Scope *, MemberValue *>> getMemberChain(const std::string &name);

		Value &operator=(const Value &x);
		Value &operator=(Value &&) = delete;
	};
}

#include <slake/type.h>

#endif
