#ifndef _SLAKE_OBJ_CONTEXT_H_
#define _SLAKE_OBJ_CONTEXT_H_

#include "object.h"
#include "var.h"
#include <slake/except.h>
#include <memory>
#include <peff/base/deallocable.h>

namespace slake {
	struct ExceptionHandler final {
		TypeRef type;
		uint32_t off;
	};

	class CoroutineObject;

	struct ExceptHandler {
		size_t offNext;
		TypeRef type;
		uint32_t offHandler;
	};

	struct MinorFrame {
		size_t offLastMinorFrame = SIZE_MAX;
		size_t offExceptHandler = SIZE_MAX;
		size_t offAllocaRecords = SIZE_MAX;
		size_t stackBase = 0;
	};

	/// @brief Alloca record, used for invalidating references in the registers, etc.
	/// @note Do not forget to add GC scan codes when you adding anything new into this structure!
	struct AllocaRecord {
		size_t offNext;
		uint32_t defReg;
	};

	struct ResumableContextData {
		uint32_t curIns = 0;
		uint32_t lastJumpSrc = UINT32_MAX;
		peff::DynArray<Value> argStack;
		peff::DynArray<Value> nextArgStack;
		/// @brief Offset of current minor frame, note that it is context-dependent offset.
		size_t offCurMinorFrame = SIZE_MAX;
		size_t nRegs = 0;
		Object *thisObject = nullptr;

		SLAKE_API ResumableContextData(peff::Alloc *allocator) noexcept;
		ResumableContextData(ResumableContextData &&) noexcept = default;
		SLAKE_API ~ResumableContextData();

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
	};

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		size_t offPrevFrame = SIZE_MAX, offNextFrame = SIZE_MAX;

		Runtime *associatedRuntime;

		const FnOverloadingObject *curFn = nullptr;	 // Current function overloading.
		CoroutineObject *curCoroutine = nullptr;

		peff::Option<ResumableContextData> resumableContextData;

		uint32_t returnValueOutReg = UINT32_MAX;
		Reference returnStructRef;

		size_t stackBase = 0;
		size_t offRegs = UINT32_MAX;

		Value curExcept = Value();	// Current exception.

		SLAKE_API MajorFrame(Runtime *rt) noexcept;
		MajorFrame(MajorFrame &&) noexcept = default;
		SLAKE_API ~MajorFrame();

		SLAKE_API void dealloc() noexcept;

		SLAKE_API static MajorFrame *alloc(Runtime *rt, Context *context);

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;
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
		peff::RcObjectPtr<peff::Alloc> selfAllocator;
		size_t nMajorFrames = 0;
		size_t offCurMajorFrame = SIZE_MAX;	 // Offset of current major frame
		ContextFlags flags = 0;				 // Flags
		char *dataStack = nullptr;			 // Data stack
		char *dataStackTopPtr = nullptr;	 // Data stack top pointer
		size_t stackTop = 0;				 // Stack top
		size_t stackSize;

		SLAKE_API char *stackAlloc(size_t size);

		SLAKE_API Context(Runtime *runtime, peff::Alloc *allocator);

		SLAKE_API ~Context();

		SLAKE_API void replaceAllocator(peff::Alloc *allocator) noexcept;

		typedef bool (*MajorFrameWalker)(MajorFrame *majorFrame, void *userData);
		SLAKE_API void forEachMajorFrame(MajorFrameWalker walker, void *userData);
	};

	class ContextObject final : public Object {
	public:
		Context _context;

		SLAKE_API ContextObject(Runtime *rt, peff::Alloc *selfAllocator);
		SLAKE_API virtual ~ContextObject();

		SLAKE_API static HostObjectRef<ContextObject> alloc(Runtime *rt, size_t stackSize);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE Context &getContext() { return _context; }

		SLAKE_API InternalExceptionPointer resume(HostRefHolder *hostRefHolder);
		SLAKE_API bool isDone();

		SLAKE_API virtual void replaceAllocator(peff::Alloc *allocator) noexcept override;
	};

	SLAKE_FORCEINLINE constexpr char *calcStackAddr(char *data, size_t szStack, size_t offset) {
		return data + (szStack - offset);
	}

	SLAKE_FORCEINLINE constexpr const char *calcStackAddr(const char *data, size_t szStack, size_t offset) {
		return data + (szStack - offset);
	}

	class ExecutionRunnable : public Runnable {
	public:
		Thread *thread = nullptr;
		HostObjectRef<ContextObject> context;
		void *nativeStackBase = nullptr;
		size_t nativeStackSize = 0;
		InternalExceptionPointer exceptPtr;
		ThreadStatus status = ThreadStatus::Ready;

		SLAKE_API ExecutionRunnable();

		SLAKE_API virtual void run() override;
	};
}

#endif
