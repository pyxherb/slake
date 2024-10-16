#ifndef _SLAKE_OBJ_STRING_H_
#define _SLAKE_OBJ_STRING_H_

#include "object.h"
#include "generic.h"
#include <deque>

namespace slake {
	class StringObject final : public Object {
	private:
		SLAKE_API void _setData(const char *str, size_t size);

	public:
		SLAKE_API StringObject(Runtime *rt, const char *str, size_t size);
		SLAKE_API StringObject(Runtime *rt, std::string &&s);
		SLAKE_API StringObject(const StringObject &x);
		SLAKE_API virtual ~StringObject();

		std::pmr::string data;

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API virtual Object *duplicate() const override;

		SLAKE_API static HostObjectRef<StringObject> alloc(Runtime *rt, const char *str, size_t size);
		SLAKE_API static HostObjectRef<StringObject> alloc(Runtime *rt, std::string &&s);
		SLAKE_API static HostObjectRef<StringObject> alloc(const StringObject *other);
		SLAKE_API virtual void dealloc() override;

		StringObject &operator=(StringObject &&) = delete;
	};
}

#endif
