#ifndef _SLAKE_VALDEF_CONTEXT_H_
#define _SLAKE_VALDEF_CONTEXT_H_

#include "object.h"
#include <memory>

namespace slake {
	struct Context;

	class ContextObject final : public Object {
	private:
		std::shared_ptr<Context> _context;

		friend class Runtime;

	public:
		ContextObject(Runtime *rt, std::shared_ptr<Context> context);
		virtual ~ContextObject();

		virtual inline Type getType() const override { return TypeId::Context; }

		inline std::shared_ptr<Context> getContext() { return _context; }

		Value resume();
		Value getResult();
		bool isDone();

		inline ContextObject &operator=(const ContextObject &x) {
			((Object &)*this) = (Object &)x;

			_context = x._context;

			return *this;
		}
	};
}

#endif
