#ifndef _SLAKE_OBJ_CONTEXT_H_
#define _SLAKE_OBJ_CONTEXT_H_

#include "object.h"
#include <memory>

namespace slake {
	struct Context;

	class ContextObject final : public Object {
	private:
		std::shared_ptr<Context> _context;

		friend class Runtime;

	public:
		SLAKE_API ContextObject(Runtime *rt, std::shared_ptr<Context> context);
		SLAKE_API ContextObject(const ContextObject &x);
		SLAKE_API virtual ~ContextObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API static HostObjectRef<ContextObject> alloc(Runtime *rt, std::shared_ptr<Context> context);
		SLAKE_API static HostObjectRef<ContextObject> alloc(const ContextObject *other);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE std::shared_ptr<Context> getContext() { return _context; }

		SLAKE_API Value resume(HostRefHolder *hostRefHolder);
		SLAKE_API Value getResult();
		SLAKE_API bool isDone();
	};
}

#endif
