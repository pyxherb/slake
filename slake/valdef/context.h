#ifndef _SLAKE_VALDEF_CONTEXT_H_
#define _SLAKE_VALDEF_CONTEXT_H_

#include "base.h"
#include <memory>

namespace slake {
	struct Context;

	class ContextValue final : public Value {
	private:
		std::shared_ptr<Context> _context;

		friend class Runtime;

	public:
		ContextValue(Runtime *rt, std::shared_ptr<Context> context);
		virtual ~ContextValue();

		virtual inline Type getType() const override { return TypeId::Context; }

		inline std::shared_ptr<Context> getContext() { return _context; }

		ValueRef<> resume();
		ValueRef<> getResult();
		bool isDone();

		inline ContextValue &operator=(const ContextValue &x) {
			((Value &)*this) = (Value &)x;

			_context = x._context;

			return *this;
		}
	};
}

#endif
