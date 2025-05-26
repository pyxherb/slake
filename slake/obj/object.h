#ifndef _SLAKE_OBJ_OBJECT_H_
#define _SLAKE_OBJ_OBJECT_H_

#include "../value.h"
#include "scope.h"
#include <atomic>
#include <string>
#include <memory_resource>
#include <peff/containers/set.h>
#include <slake/plat/thread.h>

namespace slake {
	class Runtime;
	class MemberObject;
	class Object;

	using ObjectFlags = uint8_t;
	constexpr static ObjectFlags
		VF_WALKED = 0x01,		   // The value has been walked by the garbage collector.
		VF_ALIAS = 0x02,		   // The value is an alias thus the scope should not be deleted.
		VF_UNOWNED_VTABLE = 0x04,  // The value does not own the vtable.
		VF_DESTRUCTED = 0x08,	   // Destructor of the value has been executed.
		VF_GCREADY = 0x80		   // The object is ready to be GC., for objects created during GC.
		;

	struct Type;
	class Scope;

	enum class ObjectKind {
		String,		// String
		TypeDef,	// Type definition
		FnTypeDef,	// Function type definition

		Fn,				// Function
		FnOverloading,	// Function overloading
		Module,			// Module
		Array,			// Array
		Ref,			// Reference

		Class,		// Class
		Interface,	// Interface
		Struct,		// Structure
		Instance,	// Object instance

		Any,  // Any

		Alias,	// Alias

		IdRef,		 // Reference
		GenericArg,	 // Generic argument
		Context,	 // Context
		Resumable,	 // Resumable
		Coroutine,	 // Coroutine
	};

	enum class ObjectGCStatus : uint8_t {
		Unwalked = 0,
		ReadyToWalk,
		Walked
	};

	struct GCWalkContext;

	enum class ObjectGeneration {
		Young = 0,
		Persistent
	};

	class Object {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		// The object will never be freed if its host reference count is not 0.
		mutable std::atomic_uint32_t hostRefCount = 0;

		/// @brief The basic constructor.
		/// @param rt Runtime that the value belongs to.
		SLAKE_API Object(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API Object(const Object &x, peff::Alloc *allocator);
		SLAKE_API virtual ~Object();

		ObjectFlags _flags = 0;

		ObjectGeneration objectGeneration = ObjectGeneration::Young;
		Object *nextSameGenObject = nullptr;
		Object *prevSameGenObject = nullptr;
		Mutex gcMutex;
		GCWalkContext *gcWalkContext = nullptr;

		union {
			struct {
				ObjectGCStatus gcStatus;
				Object *nextWalkable;  // New reachable objects
				InstanceObject *prevDestructible;
				InstanceObject *nextDestructible;

				Object *nextHostRef;

				Object *nextUnwalked;
				Object *prevUnwalked;
			} heapless;
		} gcInfo;

		Runtime *associatedRuntime;

		/// @brief Get type of the value.
		/// @return Type of the value.
		SLAKE_API virtual ObjectKind getKind() const = 0;

		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		SLAKE_API virtual Object *duplicate() const;

		virtual void dealloc() = 0;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept;

		SLAKE_FORCEINLINE Runtime *getRuntime() const noexcept { return associatedRuntime; }

		SLAKE_API virtual EntityRef getMember(const std::string_view &name) const;
	};

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

		SLAKE_FORCEINLINE const T *get() const { return _value; }
		SLAKE_FORCEINLINE T *get() { return _value; }
		SLAKE_FORCEINLINE const T *operator->() const { return _value; }
		SLAKE_FORCEINLINE T *operator->() { return _value; }

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

	class HostRefHolder final {
	public:
		peff::Set<Object *> holdedObjects;

		SLAKE_API HostRefHolder(
			peff::Alloc *selfAllocator);
		SLAKE_API ~HostRefHolder();

		[[nodiscard]] SLAKE_API bool addObject(Object *object);
		SLAKE_API void removeObject(Object *object) noexcept;
	};
}

#include <slake/type.h>

#endif
