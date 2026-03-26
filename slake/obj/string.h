#ifndef _SLAKE_OBJ_STRING_H_
#define _SLAKE_OBJ_STRING_H_

#include "object.h"
#include "generic.h"

namespace slake {
	class StringObject final : public Object {
	private:
		[[nodiscard]] SLAKE_API bool _set_data(const char *str, size_t size);

	public:
		SLAKE_API StringObject(Runtime *rt, peff::Alloc *self_allocator);
		SLAKE_API StringObject(const StringObject &x, peff::Alloc *allocator, bool &succeeded_out);
		SLAKE_API virtual ~StringObject();

		peff::String data;

		SLAKE_API virtual Object *duplicate(Duplicator *duplicator) const override;

		SLAKE_API static HostObjectRef<StringObject> alloc(Runtime *rt);
		SLAKE_API static HostObjectRef<StringObject> alloc(const StringObject *other);
		SLAKE_API virtual void dealloc() override;

		StringObject &operator=(StringObject &&) = delete;

		SLAKE_API virtual void replace_allocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
