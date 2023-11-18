#ifndef _SLAKE_VALDEF_BASE_H_
#define _SLAKE_VALDEF_BASE_H_

#include <atomic>
#include <stdexcept>
#include <string>
#include <deque>

namespace slake {
	class Runtime;
	class MemberValue;
	class Value;

	bool _isRuntimeInDestruction(Runtime *runtime);

	template <typename T = Value, bool isHostRef = true>
	class ValueRef final {
	public:
		T *_value;
		Runtime *_rt;

		inline void release() {
			if (_value) {
				if (!_isRuntimeInDestruction(_rt)) {
					if constexpr (isHostRef) {
						_value->decHostRefCount();
					} else {
						_value->decRefCount();
					}
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

				_rt = x->_rt;
			}
		}
		inline ValueRef(const ValueRef<T, isHostRef> &&x) noexcept : _value(x._value) {
			if (x._value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}

				_rt = x->_rt;
			}
		}
		inline ValueRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}

				_rt = value->_rt;
			}
		}
		inline ~ValueRef() {
			release();
		}
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
				_rt = nullptr;
			}
			if ((_value = x._value)) {
				if constexpr (isHostRef) {
					_value->incHostRefCount();
				} else {
					_value->incRefCount();
				}
				_rt = _value->_rt;
			}
			return *this;
		}

		inline bool operator<(const ValueRef<> rhs) const noexcept {
			return _value < rhs._value;
		}

		inline operator bool() const {
			return _value;
		}
	};

	using ValueFlags = uint8_t;
	constexpr static ValueFlags VF_WALKED = 0x01;

	struct Type;
	class Value {
	protected:
		mutable std::atomic_uint32_t refCount = 0;

		// The value will never be freed if its host reference count is not 0.
		mutable std::atomic_uint32_t hostRefCount = 0;

		void reportSizeToRuntime(long size);

		/// @brief This method will be executed when the reference count becomes zero, the default action is delete the value.
		virtual void onRefZero();

		friend class Runtime;

	public:
		Runtime *_rt;
		ValueFlags _flags = 0;

		/// @brief The basic constructor.
		/// @param rt Runtime which the value belongs to.
		Value(Runtime *rt);
		virtual ~Value();

		/// @brief Get type of the value.
		/// @return Type of the value.
		virtual Type getType() const = 0;

		/// @brief Get a single member of the value.
		/// @param name Name of the member to get.
		/// @return Pointer to the member, nullptr if not found.
		virtual MemberValue *getMember(std::string name);

		/// @brief Get a single member of the value.
		/// @param name Name of the member to get.
		/// @return Pointer to the member, nullptr if not found.
		virtual const MemberValue *getMember(std::string name) const;

		inline MemberValue *operator[](std::string name) {
			return getMember(name);
		}
		inline const MemberValue *operator[](std::string name) const {
			return getMember(name);
		}

		/// @brief Call the value as callable.
		/// @param nArgs Number of arguments.
		/// @param args Pointer to linear-arranged arguments.
		/// @return Result of the calling.
		virtual ValueRef<> call(std::deque<ValueRef<>> args) const;

		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		virtual Value *duplicate() const;

		inline void incRefCount() const { ++refCount; }
		inline void decRefCount() const {
			if ((!--refCount) && (!hostRefCount))
				const_cast<Value *>(this)->onRefZero();
		}
		inline void incHostRefCount() const { ++hostRefCount; }
		inline void decHostRefCount() const {
			if ((!--hostRefCount) && (!refCount))
				const_cast<Value *>(this)->onRefZero();
		}
		inline uint32_t getRefCount() const { return refCount; }
		inline uint32_t getHostRefCount() const { return hostRefCount; }
		inline Runtime *getRuntime() const noexcept { return _rt; }

		inline Value &operator=(const Value &x) {
			_rt = x._rt;
			_flags = x._flags & ~VF_WALKED;

			return *this;
		}
		Value &operator=(Value &&) = delete;
	};
}

#include <slake/type.h>

#endif
