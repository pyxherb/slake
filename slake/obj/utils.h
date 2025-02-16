#ifndef _SLAKE_OBJ_UTILS_H_
#define _SLAKE_OBJ_UTILS_H_

#include <slake/basedefs.h>
#include <peff/containers/set.h>

namespace slake {
	class Object;

	template <typename T = Object>
	class HostObjectRef final {
	public:
		T *_value = nullptr;

		SLAKE_FORCEINLINE void reset() {
			if (_value) {
				--_value->hostRefCount;
				_value = nullptr;
			}
		}

		SLAKE_FORCEINLINE T *release() {
			T *v = _value;
			--_value->hostRefCount;
			_value = nullptr;
			return v;
		}

		SLAKE_FORCEINLINE void discard() noexcept { _value = nullptr; }

		SLAKE_FORCEINLINE HostObjectRef(const HostObjectRef<T> &x) : _value(x._value) {
			if (x._value) {
				++_value->hostRefCount;
			}
		}
		SLAKE_FORCEINLINE HostObjectRef(HostObjectRef<T> &&x) noexcept : _value(x._value) {
			if (x._value) {
				x._value = nullptr;
			}
		}
		SLAKE_FORCEINLINE HostObjectRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				++_value->hostRefCount;
			}
		}
		SLAKE_FORCEINLINE ~HostObjectRef() {
			reset();
		}

		SLAKE_FORCEINLINE T *get() const { return _value; }
		SLAKE_FORCEINLINE T *operator->() const { return _value; }

		SLAKE_FORCEINLINE HostObjectRef<T> &operator=(const HostObjectRef<T> &x) {
			reset();

			if ((_value = x._value)) {
				++_value->hostRefCount;
			}

			return *this;
		}
		SLAKE_FORCEINLINE HostObjectRef<T> &operator=(HostObjectRef<T> &&x) noexcept {
			reset();

			if ((_value = x._value)) {
				x._value = nullptr;
			}

			return *this;
		}

		SLAKE_FORCEINLINE HostObjectRef<T> &operator=(T *other) {
			reset();

			if ((_value = other)) {
				++_value->hostRefCount;
			}

			return *this;
		}

		SLAKE_FORCEINLINE bool operator<(const HostObjectRef<T> &rhs) const noexcept {
			return _value < rhs._value;
		}
		SLAKE_FORCEINLINE bool operator>(const HostObjectRef<T> &rhs) const noexcept {
			return _value > rhs._value;
		}
		SLAKE_FORCEINLINE bool operator==(const HostObjectRef<T> &rhs) const noexcept {
			return _value == rhs._value;
		}

		SLAKE_FORCEINLINE operator bool() const {
			return _value;
		}
	};
}

#endif
