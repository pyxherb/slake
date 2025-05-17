#ifndef _SLAKE_OBJ_CONTEXT_H_
#define _SLAKE_OBJ_CONTEXT_H_

#include "object.h"
#include "var.h"
#include <slake/except.h>
#include <memory>
#include <peff/base/deallocable.h>

namespace slake {
	struct ExceptionHandler final {
		Type type;
		uint32_t off;
	};

	/// @brief Minor frames are created by ENTER instructions and destroyed by
	/// LEAVE instructions.
	struct MinorFrame final {
		peff::DynArray<ExceptionHandler> exceptHandlers;  // Exception handlers

		size_t stackBase = 0;

		SLAKE_API MinorFrame(
			Runtime *rt,
			size_t stackBase);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MinorFrame() : exceptHandlers(nullptr) {
			abort();
		}
	};

	struct ArgRecord {
		Value value;
		Type type;
	};

	class CoroutineObject;

	struct ResumableContext {
		uint32_t curIns = 0;
		uint32_t lastJumpSrc = UINT32_MAX;
		peff::DynArray<ArgRecord> argStack;
		peff::DynArray<size_t> lvarRecordOffsets;
		peff::DynArray<Value> nextArgStack;
		size_t nRegs = 0;
		Object *thisObject = nullptr;
		peff::DynArray<MinorFrame> minorFrames;

		SLAKE_API ResumableContext(peff::Alloc *allocator);
		SLAKE_API ~ResumableContext();

		SLAKE_API ResumableContext &operator=(ResumableContext &&rhs);
	};

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		Runtime *associatedRuntime;

		const FnOverloadingObject *curFn = nullptr;	 // Current function overloading.
		CoroutineObject *curCoroutine = nullptr;

		ResumableContext resumable;

		uint32_t returnValueOutReg = UINT32_MAX;

		size_t stackBase = 0;
		size_t offRegs = UINT32_MAX;

		Value curExcept = Value();	// Current exception.

		SLAKE_API MajorFrame(Runtime *rt);
		SLAKE_API ~MajorFrame();

		SLAKE_API void dealloc() noexcept;

		SLAKE_API static MajorFrame *alloc(Runtime *rt, Context *context);
	};

	using MajorFramePtr = std::unique_ptr<MajorFrame, peff::DeallocableDeleter<MajorFrame>>;

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Done
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context {
		Runtime *runtime;
		peff::DynArray<MajorFramePtr> majorFrames;	// Major frame list
		ContextFlags flags = 0;						// Flags
		char *dataStack = nullptr;					// Data stack
		size_t stackTop = 0;						// Stack top

		SLAKE_API char *stackAlloc(size_t size);
		SLAKE_API void leaveMajor();

		SLAKE_API Context(Runtime *runtime);

		SLAKE_API ~Context();
	};

	class ContextObject final : public Object {
	public:
		Context _context;

		SLAKE_API ContextObject(Runtime *rt);
		SLAKE_API virtual ~ContextObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API static HostObjectRef<ContextObject> alloc(Runtime *rt);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE Context &getContext() { return _context; }

		SLAKE_API InternalExceptionPointer resume(HostRefHolder *hostRefHolder);
		SLAKE_API bool isDone();
	};

	SLAKE_FORCEINLINE char *calcStackAddr(char *data, size_t szStack, size_t offset) {
		return data + szStack - offset;
	}
}

#endif
