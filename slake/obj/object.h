#ifndef _SLAKE_OBJ_OBJECT_H_
#define _SLAKE_OBJ_OBJECT_H_

#include "../value.h"
#include <atomic>
#include <string>
#include <memory_resource>
#include <peff/containers/set.h>
#include <peff/containers/list.h>
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

		Class,			// Class
		Interface,		// Interface
		Struct,			// Structure
		ScopedEnum,		// Scoped enumeration
		UnionEnumItem,	// Union enumeration item
		UnionEnum,		// Union enumeration

		Instance,  // Object instance

		Any,  // Any

		Alias,	// Alias

		IdRef,		 // Reference
		GenericArg,	 // Generic argument
		Context,	 // Context
		Resumable,	 // Resumable
		Coroutine,	 // Coroutine
	};

	SLAKE_FORCEINLINE bool verify_object_kind(const Object *object);

	SLAKE_FORCEINLINE bool verify_object_kind(const Object *object, ObjectKind object_kind);

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
		BasicModuleObject *mod;
		MemberObject *src;
	};

	struct TypeDuplicationTask {
		TypeRef *type;
		TypeRef src;
	};

	struct DuplicationTask {
		union {
			NormalDuplicationTask as_normal;
			ModuleMemberDuplicationTask as_module_member;
			TypeDuplicationTask as_type;
		};

		DuplicationTaskType task_type;

		SLAKE_API static DuplicationTask make_normal(Object **dest, Object *src);
		SLAKE_API static DuplicationTask make_module_member(BasicModuleObject *mod, MemberObject *src);
		SLAKE_API static DuplicationTask make_type(TypeRef *type, const TypeRef &src);
	};

	class Duplicator {
	public:
		Runtime *runtime;
		peff::List<DuplicationTask> tasks;

		SLAKE_API Duplicator(Runtime *runtime, peff::Alloc *allocator);

		[[nodiscard]] SLAKE_API bool insert_task(DuplicationTask &&task);

		[[nodiscard]] SLAKE_API bool exec();
	};

	class Object {
	public:
		// The object will never be freed if its host reference count is not 0.
		mutable std::atomic_size_t host_ref_count = 0;

		/// @brief The basic constructor.
		/// @param rt Runtime that the value belongs to.
		SLAKE_API Object(Runtime *rt, peff::Alloc *self_allocator, ObjectKind object_kind);
		SLAKE_API Object(const Object &x, peff::Alloc *allocator);
		SLAKE_API virtual ~Object();

		Object *next_same_gen_object = nullptr;
		Object *prev_same_gen_object = nullptr;

		Object *prev_same_gcset;
		Object *next_same_gcset;

		Runtime *associated_runtime;

		ObjectFlags object_flags = 0;
		ObjectGeneration object_generation = ObjectGeneration::Young;
		Spinlock gc_spinlock;
		ObjectGCStatus gc_status;

	private:
		ObjectKind _object_kind;

	public:
		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const;

		virtual void dealloc() = 0;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept;

		SLAKE_FORCEINLINE Runtime *get_runtime() const noexcept { return associated_runtime; }

		SLAKE_API peff::Alloc *get_allocator() const noexcept;

		SLAKE_FORCEINLINE ObjectKind get_object_kind_unchecked() const noexcept {
			return _object_kind;
		}

		SLAKE_FORCEINLINE ObjectKind get_object_kind() const noexcept {
			assert(verify_object_kind(this));
			return _object_kind;
		}

		SLAKE_FORCEINLINE void inc_host_ref() const noexcept {
			++host_ref_count;
		}

		SLAKE_FORCEINLINE void dec_host_ref() const noexcept {
			assert(host_ref_count > 0);
			--host_ref_count;
		}

		SLAKE_API virtual Reference get_member(const std::string_view &name) const;
	};

	template <typename T = Object>
	class HostObjectRef final {
	public:
		T *_value = nullptr;

		SLAKE_FORCEINLINE void reset() noexcept {
			if (_value) {
				--_value->host_ref_count;
				_value = nullptr;
			}
		}

		SLAKE_FORCEINLINE T *release() noexcept {
			T *v = _value;
			--_value->host_ref_count;
			_value = nullptr;
			return v;
		}

		SLAKE_FORCEINLINE void discard() noexcept { _value = nullptr; }

		SLAKE_FORCEINLINE HostObjectRef(const HostObjectRef<T> &x) noexcept : _value(x._value) {
			if (x._value) {
				++_value->host_ref_count;
			}
		}
		SLAKE_FORCEINLINE HostObjectRef(HostObjectRef<T> &&x) noexcept : _value(x._value) {
			if (x._value) {
				x._value = nullptr;
			}
		}
		SLAKE_FORCEINLINE HostObjectRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				++_value->host_ref_count;
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
				++_value->host_ref_count;
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
				++_value->host_ref_count;
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
		peff::Set<Object *> holded_objects;

		SLAKE_API HostRefHolder(
			peff::Alloc *self_allocator);
		SLAKE_API ~HostRefHolder();

		[[nodiscard]] SLAKE_API bool add_object(Object *object);
		SLAKE_API void remove_object(Object *object) noexcept;
	};

	SLAKE_FORCEINLINE bool verify_object_kind(const Object *object) {
		return (object->get_object_kind_unchecked() >= ObjectKind::String) && (object->get_object_kind_unchecked() <= ObjectKind::Coroutine);
	}

	SLAKE_FORCEINLINE bool verify_object_kind(const Object *object, ObjectKind object_kind) {
		return object->get_object_kind_unchecked() == object_kind;
	}
}

#include <slake/type.h>

#endif
