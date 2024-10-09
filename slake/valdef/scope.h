#ifndef _SLAKE_VALDEF_SCOPE_H_
#define _SLAKE_VALDEF_SCOPE_H_

#include <unordered_map>
#include <deque>
#include <stdexcept>
#include <memory>
#include <memory_resource>
#include <string>
#include <slake/util/memory.h>
#include <slake/basedefs.h>

namespace slake {
	class Object;
	class MemberObject;
	class FnObject;
	class FnOverloadingObject;

	class Scope {
	public:
		std::pmr::memory_resource *memoryResource;
		Object *owner;
		std::pmr::unordered_map<std::pmr::string, MemberObject *> members;

		SLAKE_API Scope(std::pmr::memory_resource *memoryResource,
			Object *owner);

		SLAKE_API MemberObject *getMember(const std::pmr::string &name);

		SLAKE_API void putMember(const std::pmr::string &name, MemberObject *value);

		SLAKE_API void addMember(const std::pmr::string &name, MemberObject *value);

		SLAKE_API void removeMember(const std::pmr::string &name);

		SLAKE_API Scope *duplicate();

		SLAKE_API static Scope *alloc(std::pmr::memory_resource *memoryResource, Object *owner);
		SLAKE_API void dealloc();
	};

	class MethodTable {
	public:
		std::pmr::memory_resource *memoryResource;
		std::pmr::unordered_map<std::pmr::string, FnObject *> methods;
		std::pmr::deque<FnOverloadingObject *> destructors;

		SLAKE_API MethodTable(std::pmr::memory_resource *memoryResource);

		SLAKE_API FnObject *getMethod(const std::pmr::string &name);

		SLAKE_API MethodTable *duplicate();

		SLAKE_API static MethodTable *alloc(std::pmr::memory_resource *memoryResource);
		SLAKE_API void dealloc();
	};
}

#endif
