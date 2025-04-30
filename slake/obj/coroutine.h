#ifndef _SLAKE_OBJ_COROUTINE_H_
#define _SLAKE_OBJ_COROUTINE_H_

#include "fn.h"
#include "var.h"

namespace slake {
	class CoroutineObject : public Object {
	public:
		Context *curContext;
		MajorFrame *curMajorFrame;
		Object *thisObject;
		const FnOverloadingObject *overloading;
		peff::DynArray<ArgRecord> args;
		uint32_t offIns;
		char *stackData;
		size_t lenStackData;
		size_t nRegs;
		size_t offRegs;
		Value finalResult;

		SLAKE_API CoroutineObject(Runtime *rt);
		SLAKE_API virtual ~CoroutineObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API static HostObjectRef<CoroutineObject> alloc(Runtime *rt);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API char *allocStackData(size_t size);
		SLAKE_API void releaseStackData();

		SLAKE_FORCEINLINE void bindToContext(Context *curContext, MajorFrame *curMajorFrame) noexcept {
			assert(!curContext);
			assert(!curMajorFrame);
			this->curContext = curContext;
			this->curMajorFrame = curMajorFrame;
		}
		SLAKE_FORCEINLINE void unbindContext() noexcept {
			this->curContext = nullptr;
			this->curMajorFrame = nullptr;
		}

		SLAKE_FORCEINLINE bool isDone() const {
			return offIns == UINT32_MAX;
		}
		SLAKE_FORCEINLINE void setDone() {
			offIns = UINT32_MAX;
		}
	};
}

#endif
