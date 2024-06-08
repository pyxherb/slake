#ifndef _SLAKE_VALDEF_STRING_H_
#define _SLAKE_VALDEF_STRING_H_

#include "object.h"
#include "generic.h"
#include <deque>

namespace slake {
	class StringObject final : public Object {
	private:
		void _setData(const char* str, size_t size);

	public:
		std::string data;

		StringObject(Runtime *rt, const char *str, size_t size);
		StringObject(Runtime *rt, std::string &&s);
		virtual ~StringObject();

		virtual inline Type getType() const override { return TypeId::String; }

		virtual Object *duplicate() const override;

		inline StringObject &operator=(const StringObject &x) {
			((Object &)*this) = (Object &)x;

			_setData(x.data.c_str(), x.data.size());

			return *this;
		}
		StringObject &operator=(StringObject &&) = delete;
	};
}

#endif
