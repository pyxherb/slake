#ifndef _SLAKE_VALDEF_SCOPE_H_
#define _SLAKE_VALDEF_SCOPE_H_

#include <unordered_map>
#include <deque>
#include <stdexcept>
#include <memory>
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

		Scope(std::pmr::memory_resource *memoryResource,
			Object *owner);

		MemberObject *getMember(const std::pmr::string &name);

		void putMember(const std::pmr::string &name, MemberObject *value);

		void addMember(const std::pmr::string &name, MemberObject *value);

		void removeMember(const std::pmr::string &name);

		Scope *duplicate();

		static Scope *alloc(std::pmr::memory_resource *memoryResource, Object *owner);
		void dealloc();
	};

	class MethodTable {
	public:
		std::pmr::memory_resource *memoryResource;
		std::pmr::unordered_map<std::pmr::string, FnObject *> methods;
		std::pmr::deque<FnOverloadingObject *> destructors;

		MethodTable(std::pmr::memory_resource *memoryResource);

		FnObject *getMethod(const std::pmr::string &name);

		MethodTable *duplicate();

		static MethodTable *alloc(std::pmr::memory_resource *memoryResource);
		void dealloc();
	};
}

#endif
