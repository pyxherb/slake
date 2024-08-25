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
		inline ContextObject(const ContextObject &x) : Object(x) {
			_context = x._context;
		}
		virtual ~ContextObject();

		virtual inline ObjectKind getKind() const override { return ObjectKind::Context; }

		static HostObjectRef<ContextObject> alloc(Runtime *rt, std::shared_ptr<Context> context);
		static HostObjectRef<ContextObject> alloc(const ContextObject *other);
		virtual void dealloc() override;

		inline std::shared_ptr<Context> getContext() { return _context; }

		Value resume();
		Value getResult();
		bool isDone();
	};
}

#endif
