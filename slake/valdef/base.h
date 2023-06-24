#ifndef _SLAKE_VALDEF_BASE_H_
#define _SLAKE_VALDEF_BASE_H_

#include <slake/type.h>

#include <atomic>
#include <stdexcept>
#include <string>

namespace Slake {
	class Runtime;
	class MemberValue;

	template <typename T = Value, bool isHostRef = true>
	class ValueRef final {
	public:
		T *_value;

		inline void release() {
			if (_value) {
				if constexpr (isHostRef) {
					_value->decHostRefCount();
				} else {
					_value->decRefCount();
				}
			}
			_value = nullptr;
		}

		inline void discard() noexcept { _value = nullptr; }

		inline ValueRef(const ValueRef<T, isHostRef> &x) : _value(x._value) {
			if (x._value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
		}
		inline ValueRef(const ValueRef<T, isHostRef> &&x) noexcept : _value(x._value) {
			if (x._value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
		}
		inline ValueRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
		}
		inline ~ValueRef() { release(); }
		inline T *operator*() { return _value; }
		inline const T *operator*() const { return _value; }
		inline const T *operator->() const { return _value; }
		inline T *operator->() { return _value; }

		inline ValueRef &operator=(const ValueRef<T, isHostRef> &x) {
			return *this = std::move(x);
		}
		inline ValueRef &operator=(const ValueRef<T, isHostRef> &&x) noexcept {
			if (_value) {
				if constexpr (isHostRef) {
					_value->decHostRefCount();
				} else {
					_value->decRefCount();
				}
			}
			if ((_value = x._value)) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
			return *this;
		}
		template <typename T1, bool isHostRef1>
		inline ValueRef &operator=(const ValueRef<T1, isHostRef1> &&x) {
			if (_value) {
				if constexpr (isHostRef) {
					_value->decHostRefCount();
				} else {
					_value->decRefCount();
				}
			}
			if ((_value = x._value)) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
			}
			return *this;
		}

		template <typename T1, bool isHostRef1>
		inline operator ValueRef<T1, isHostRef1>() {
			return ValueRef<T1, isHostRef1>((T1 *)_value);
		}

		inline operator bool() const {
			return _value;
		}
	};

	using ValueFlags = uint8_t;
	constexpr static ValueFlags VF_WALKED = 0x01;

	class Value {
	protected:
		std::atomic_uint32_t _refCount = 0;
		// The garbage collector will never release it if its host reference count is not 0.
		std::atomic_uint32_t _hostRefCount = 0;
		Runtime *_rt;
		ValueFlags flags = 0;

		friend class Runtime;

	protected:
		void reportSizeToRuntime(long size);

	public:
		Value(Runtime *rt);
		virtual ~Value();
		virtual Type getType() const = 0;

		virtual MemberValue *getMember(std::string name) { return nullptr; };
		virtual const MemberValue *getMember(std::string name) const { return nullptr; }

		inline MemberValue *operator[](std::string name) { return getMember(name); }
		inline const MemberValue *operator[](std::string name) const { return getMember(name); }

		virtual ValueRef<> call(uint8_t nArgs, ValueRef<> *args) { return nullptr; }

		virtual inline void whenRefBecomeZero() { delete this; }

		inline void incRefCount() { _refCount++; }
		inline void decRefCount() {
			if ((!--_refCount) && (!_hostRefCount))
				whenRefBecomeZero();
		}
		inline void incHostRefCount() { _hostRefCount++; }
		inline void decHostRefCount() {
			if ((!--_hostRefCount) && (!_refCount))
				whenRefBecomeZero();
		}
		inline uint32_t getRefCount() { return _refCount; }
		inline uint32_t getHostRefCount() { return _hostRefCount; }
		inline Runtime *getRuntime() const noexcept { return _rt; }

		virtual inline std::string toString() const {
			return "\"refCount\":" + std::to_string(_refCount) +
				   ",\"hostRefCount\":" + std::to_string(_hostRefCount) +
				   ",\"flags\":" + std::to_string(flags) +
				   ",\"rt\":" + std::to_string((uintptr_t)_rt);
		}

		virtual inline Value *copy() const { throw std::logic_error("Not implemented yet"); }

		Value &operator=(const Value &) = delete;
		Value &operator=(const Value &&) = delete;
	};
}

namespace std {
	inline std::string to_string(const Slake::Value &&value) {
		return "{\"type\":" + std::to_string(value.getType()) + "," + value.toString() + "}";
	}
	inline std::string to_string(const Slake::Value &value) {
		return to_string(move(value));
	}
	inline std::string to_string(const Slake::Value *value) {
		return to_string(*value);
	}
}

#endif
