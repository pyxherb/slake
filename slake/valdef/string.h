#ifndef _SLAKE_VALDEF_STRING_H_
#define _SLAKE_VALDEF_STRING_H_

#include "object.h"
#include "generic.h"
#include <deque>

namespace slake {
	class StringObject final : public Object {
	private:
		void _setData(const char *str, size_t size);

	public:
		StringObject(Runtime *rt, const char *str, size_t size);
		StringObject(Runtime *rt, std::string &&s);
		inline StringObject(const StringObject &x) : Object(x) {
			_setData(x.data.c_str(), x.data.size());
		}
		virtual ~StringObject();

		std::pmr::string data;

		virtual inline ObjectKind getKind() const override { return ObjectKind::String; }

		virtual Object *duplicate() const override;

		static HostObjectRef<StringObject> alloc(Runtime *rt, const char *str, size_t size);
		static HostObjectRef<StringObject> alloc(Runtime *rt, std::string &&s);
		static HostObjectRef<StringObject> alloc(const StringObject *other);
		virtual void dealloc() override;

		StringObject &operator=(StringObject &&) = delete;
	};
}

#endif
