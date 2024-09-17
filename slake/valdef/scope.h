#ifndef _SLAKE_VALDEF_SCOPE_H_
#define _SLAKE_VALDEF_SCOPE_H_

#include <unordered_map>
#include <deque>
#include <stdexcept>
#include <memory>
#include <string>
#include <slake/util/memory.h>

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

		inline Scope(std::pmr::memory_resource *memoryResource,
			Object *owner) : memoryResource(memoryResource),
							 owner(owner),
							 members(memoryResource) {}

		inline MemberObject *getMember(const std::pmr::string &name) {
			if (auto it = members.find(name); it != members.end())
				return it->second;
			return nullptr;
		}

		void putMember(const std::pmr::string &name, MemberObject *value);

		inline void addMember(const std::pmr::string &name, MemberObject *value) {
			if (members.find(name) != members.end())
				throw std::logic_error("The member is already exists");

			putMember(name, value);
		}

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

		inline MethodTable(std::pmr::memory_resource *memoryResource)
			: memoryResource(memoryResource),
			  methods(memoryResource),
			  destructors(memoryResource) {
		}

		inline FnObject *getMethod(const std::pmr::string &name) {
			if (auto it = methods.find(name); it != methods.end())
				return it->second;
			return nullptr;
		}

		MethodTable *duplicate();

		static MethodTable *alloc(std::pmr::memory_resource *memoryResource);
		void dealloc();
	};
}

#endif
