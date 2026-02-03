#ifndef _SLAKE_OBJ_COROUTINE_H_
#define _SLAKE_OBJ_COROUTINE_H_

#include "fn.h"
#include "var.h"
#include "context.h"

namespace slake {
	class CoroutineObject : public Object {
	public:
		Context *curContext;
		MajorFrame *boundMajorFrame;

		char *stackData;
		size_t lenStackData;
		size_t offStackTop;
		size_t offRegs;
		const FnOverloadingObject *overloading;
		peff::Option<ResumableContextData> resumable;

		Value finalResult;

		SLAKE_API CoroutineObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API virtual ~CoroutineObject();

		SLAKE_API static HostObjectRef<CoroutineObject> alloc(Runtime *rt);
		SLAKE_API virtual void dealloc() override;

		SLAKE_API char *allocStackData(size_t size);
		SLAKE_API void releaseStackData();

		SLAKE_FORCEINLINE void bindToContext(Context *curContext, MajorFrame *curMajorFrame) noexcept {
			this->curContext = curContext;
			this->boundMajorFrame = curMajorFrame;
		}
		SLAKE_FORCEINLINE void unbindContext() noexcept {
			this->curContext = nullptr;
			this->boundMajorFrame = nullptr;
		}

		SLAKE_FORCEINLINE bool isActive() const noexcept {
			return this->curContext;
		}

		SLAKE_FORCEINLINE bool isDone() const {
			if (resumable) {
				return resumable->curIns == UINT32_MAX;
			}
			return false;
		}
		SLAKE_FORCEINLINE void setDone() {
			assert(resumable);
			resumable->curIns = UINT32_MAX;
		}

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};
}

#endif
