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
		VF_ALIAS = 0x02	   // The value is an alias thus the scope should not be deleted.
		;

	struct Type;
	class Scope;

	class Object {
	protected:
		void reportSizeAllocatedToRuntime(size_t size);
		void reportSizeFreedToRuntime(size_t size);

		friend class Runtime;

	public:
		// The object will never be freed if its host reference count is not 0.
		mutable std::atomic_uint32_t hostRefCount = 0;

		ObjectFlags _flags = 0;

		Runtime *_rt;

		Scope *scope = nullptr;

		/// @brief The basic constructor.
		/// @param rt Runtime which the value belongs to.
		Object(Runtime *rt);
		virtual ~Object();

		/// @brief Get type of the value.
		/// @return Type of the value.
		virtual Type getType() const = 0;

		/// @brief Dulplicate the value if supported.
		/// @return Duplicate of the value.
		virtual Object *duplicate() const;

		inline Runtime *getRuntime() const noexcept { return _rt; }

		MemberObject *getMember(const std::string &name);
		std::deque<std::pair<Scope *, MemberObject *>> getMemberChain(const std::string &name);

		Object &operator=(const Object &x);
		Object &operator=(Object &&) = delete;
	};
}

#include <slake/type.h>

#endif
