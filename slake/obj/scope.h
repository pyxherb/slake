#ifndef _SLAKE_OBJ_SCOPE_H_
#define _SLAKE_OBJ_SCOPE_H_

#include <peff/containers/hashmap.h>
#include <peff/containers/string.h>
#include <peff/containers/list.h>
#include <memory>
#include <string>
#include <string_view>
#include <slake/util/memory.h>
#include <slake/basedefs.h>

namespace slake {
	class Object;
	class MemberObject;
	class FnObject;
	class FnOverloadingObject;

	class Scope final {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		Object *owner;
		peff::HashMap<std::string_view, MemberObject *> members;

		SLAKE_API Scope(
			peff::Alloc *selfAllocator,
			Object *owner);

		SLAKE_API MemberObject *getMember(const std::string_view &name);

		[[nodiscard]] SLAKE_API bool putMember(MemberObject *value);

		/// @brief Remove a member with specified name.
		/// @param name Name of the member object to be removed.
		SLAKE_API void removeMember(const std::string_view &name);

		/// @brief Remove a member but fail if the resizing operation of buckets of member map fails.
		/// @param name Name of the member object to be removed.
		/// @return true if all operations are succeeded, false otherwise.
		SLAKE_API bool removeMemberConservative(const std::string_view &name);

		SLAKE_API Scope *duplicate();

		SLAKE_API static Scope *alloc(
			peff::Alloc *selfAllocator,
			Object *owner);
		SLAKE_API void dealloc();
	};

	class InstanceObject;

	using ScopeUniquePtr = std::unique_ptr<Scope, util::DeallocableDeleter<Scope>>;
	typedef void (*ClassNativeDestructor)(InstanceObject *instanceObject);

	class MethodTable {
	public:
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		peff::HashMap<std::string_view, FnObject *> methods;
		peff::List<ClassNativeDestructor> nativeDestructors;

		SLAKE_API MethodTable(peff::Alloc *selfAllocator);

		SLAKE_API FnObject *getMethod(const std::string_view &name);

		SLAKE_API MethodTable *duplicate();

		SLAKE_API static MethodTable *alloc(peff::Alloc *selfAllocator);
		SLAKE_API void dealloc();
	};
}

#endif
