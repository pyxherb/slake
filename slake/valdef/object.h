#ifndef _SLAKE_VALDEF_OBJECT_H_
#define _SLAKE_VALDEF_OBJECT_H_

#include "scope.h"
#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>
#include <map>

namespace slake {
	class Runtime;
	class MemberObject;
	class Object;

	using ObjectFlags = uint8_t;
	constexpr static ObjectFlags
		VF_WALKED = 0x01,  // The value has been walked by the garbage collector.
		VF_ALIAS = 0x02,   // The value is an alias thus the scope should not be deleted.
		VF_GCREADY = 0x04  // The object is ready to be GC., for objects created during GC.
		;

	struct Type;
	class Scope;

	class Object {
	public:
		/// @brief The basic constructor.
		/// @param rt Runtime that the value belongs to.
		Object(Runtime *rt);
		virtual ~Object();

		// The object will never be freed if its host reference count is not 0.
		mutable std::atomic_uint32_t hostRefCount = 0;

		ObjectFlags _flags = 0;

		Runtime *_rt;

		Scope *scope = nullptr;

		/// @brief Get type of the value.
		/// @return Type of the value.
		virtual Type getType() const = 0;

		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		virtual Object *duplicate() const;

		virtual void dealloc() = 0;

		inline Runtime *getRuntime() const noexcept { return _rt; }

		MemberObject *getMember(const std::string &name);
		std::deque<std::pair<Scope *, MemberObject *>> getMemberChain(const std::string &name);

		Object &operator=(const Object &x);
		Object &operator=(Object &&) = delete;
	};

	template <typename T = Object>
	class HostObjectRef final {
	public:
		T *_value = nullptr;

		inline void reset() {
			if (_value) {
				--_value->hostRefCount;
				_value = nullptr;
			}
		}

		inline T *release() {
			T *v = _value;
			--_value->hostRefCount;
			_value = nullptr;
			return v;
		}

		inline void discard() noexcept { _value = nullptr; }

		inline HostObjectRef(const HostObjectRef<T> &x) : _value(x._value) {
			if (x._value) {
				++_value->hostRefCount;
			}
		}
		inline HostObjectRef(HostObjectRef<T> &&x) noexcept : _value(x._value) {
			if (x._value) {
				x._value = nullptr;
			}
		}
		inline HostObjectRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				++_value->hostRefCount;
			}
		}
		inline ~HostObjectRef() {
			reset();
		}

		inline const T *get() const { return _value; }
		inline T *get() { return _value; }
		inline const T *operator->() const { return _value; }
		inline T *operator->() { return _value; }

		inline HostObjectRef<T> &operator=(const HostObjectRef<T> &x) {
			reset();

			if ((_value = x._value)) {
				++_value->hostRefCount;
			}

			return *this;
		}
		inline HostObjectRef<T> &operator=(HostObjectRef<T> &&x) noexcept {
			reset();

			if ((_value = x._value)) {
				x._value = nullptr;
			}

			return *this;
		}

		inline HostObjectRef<T> &operator=(T *other) {
			reset();

			if ((_value = other)) {
				++_value->hostRefCount;
			}

			return *this;
		}

		inline bool operator<(const HostObjectRef<T> &rhs) const noexcept {
			return _value < rhs._value;
		}
		inline bool operator>(const HostObjectRef<T> &rhs) const noexcept {
			return _value > rhs._value;
		}
		inline bool operator==(const HostObjectRef<T> &rhs) const noexcept {
			return _value == rhs._value;
		}

		inline operator bool() const {
			return _value;
		}
	};
}

#include <slake/type.h>

#endif
