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

	enum class ObjectKind : uint8_t {
		Invalid = 0,  // Invalid

		String,	 // String

		HeapType,  // On-heap type

		TypeDef,  // Type definition

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

	SLAKE_FORCEINLINE bool verifyObjectKind(const Object *object);

	SLAKE_FORCEINLINE bool verifyObjectKind(const Object *object, ObjectKind objectKind);

	using ObjectFlags = uint8_t;
	constexpr static ObjectFlags
		VF_WALKED = 0x01,		   // The value has been walked by the garbage collector.
		VF_ALIAS = 0x02,		   // The value is an alias thus the scope should not be deleted.
		VF_UNOWNED_VTABLE = 0x04,  // The value does not own the vtable.
		VF_DESTRUCTED = 0x08,	   // Destructor of the value has been executed.
		VF_GCREADY = 0x80		   // The object is ready to be GC., for objects created during GC.
		;

	class Scope;

	enum class ObjectGCStatus : uint8_t {
		Unwalked = 0,
		ReadyToWalk,
		Walked
	};

	struct GCWalkContext;

	enum class ObjectGeneration : uint8_t {
		Young = 0,
		Persistent
	};

	enum class DuplicationTaskType : uint8_t {
		Normal = 0,
		ModuleMember,
		Type,
	};

	struct NormalDuplicationTask {
		Object **dest;
		Object *src;
	};

	class ModuleObject;

	struct ModuleMemberDuplicationTask {
		ModuleObject *mod;
		MemberObject *src;
	};

	struct TypeDuplicationTask {
		TypeRef *type;
		TypeRef src;
	};

	struct DuplicationTask {
		union {
			NormalDuplicationTask asNormal;
			ModuleMemberDuplicationTask asModuleMember;
			TypeDuplicationTask asType;
		};

		DuplicationTaskType taskType;

		SLAKE_API static DuplicationTask makeNormal(Object **dest, Object *src);
		SLAKE_API static DuplicationTask makeModuleMember(ModuleObject *mod, MemberObject *src);
		SLAKE_API static DuplicationTask makeType(TypeRef *type, const TypeRef &src);
	};

	class Duplicator {
	public:
		Runtime *runtime;
		peff::List<DuplicationTask> tasks;

		SLAKE_API Duplicator(Runtime *runtime, peff::Alloc *allocator);

		[[nodiscard]] SLAKE_API bool insertTask(DuplicationTask &&task);

		[[nodiscard]] SLAKE_API bool exec();
	};

	class Object {
	private:
		ObjectKind _objectKind;

	public:
		ObjectFlags _flags = 0;
		ObjectGeneration objectGeneration = ObjectGeneration::Young;
		Spinlock gcSpinlock;
		ObjectGCStatus gcStatus;

	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		// The object will never be freed if its host reference count is not 0.
		mutable std::atomic_uint32_t hostRefCount = 0;

		/// @brief The basic constructor.
		/// @param rt Runtime that the value belongs to.
		SLAKE_API Object(Runtime *rt, peff::Alloc *selfAllocator, ObjectKind objectKind);
		SLAKE_API Object(const Object &x, peff::Alloc *allocator);
		SLAKE_API virtual ~Object();

		Object *nextSameGenObject = nullptr;
		Object *prevSameGenObject = nullptr;
		GCWalkContext *gcWalkContext = nullptr;

		Object *nextWalkable;		   // New reachable objects
		Object *nextWalked = nullptr;  // Next reached objects
		Object **sameKindObjectList = NULL;
		Object *prevSameKindObject = NULL;
		Object *nextSameKindObject = NULL;

		Object *nextHostRef;

		Object *nextUnwalked;
		Object *prevUnwalked;

		Runtime *associatedRuntime;

		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const;

		virtual void dealloc() = 0;

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept;

		SLAKE_FORCEINLINE Runtime *getRuntime() const noexcept { return associatedRuntime; }

		SLAKE_FORCEINLINE ObjectKind getObjectKindUnchecked() const noexcept {
			return _objectKind;
		}

		SLAKE_FORCEINLINE ObjectKind getObjectKind() const noexcept {
			assert(verifyObjectKind(this));
			return _objectKind;
		}

		SLAKE_API virtual EntityRef getMember(const std::string_view &name) const;
	};

	template <typename T = Object>
	class HostObjectRef final {
	public:
		T *_value = nullptr;

		SLAKE_FORCEINLINE void reset() noexcept {
			if (_value) {
				--_value->hostRefCount;
				_value = nullptr;
			}
		}

		SLAKE_FORCEINLINE T *release() noexcept {
			T *v = _value;
			--_value->hostRefCount;
			_value = nullptr;
			return v;
		}

		SLAKE_FORCEINLINE void discard() noexcept { _value = nullptr; }

		SLAKE_FORCEINLINE HostObjectRef(const HostObjectRef<T> &x) noexcept : _value(x._value) {
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

		SLAKE_FORCEINLINE const T *get() const noexcept { return _value; }
		SLAKE_FORCEINLINE T *get() noexcept { return _value; }
		SLAKE_FORCEINLINE const T *operator->() const noexcept { return _value; }
		SLAKE_FORCEINLINE T *operator->() noexcept { return _value; }

		SLAKE_FORCEINLINE HostObjectRef<T> &operator=(const HostObjectRef<T> &x) noexcept {
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

		SLAKE_FORCEINLINE HostObjectRef<T> &operator=(T *other) noexcept {
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

	SLAKE_FORCEINLINE bool verifyObjectKind(const Object *object) {
		return (object->getObjectKindUnchecked() >= ObjectKind::String) && (object->getObjectKindUnchecked() <= ObjectKind::Coroutine);
	}

	SLAKE_FORCEINLINE bool verifyObjectKind(const Object *object, ObjectKind objectKind) {
		return object->getObjectKindUnchecked() == objectKind;
	}
}

#include <slake/type.h>

#endif
