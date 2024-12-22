#ifndef _SLAKE_OBJ_CONTEXT_H_
#define _SLAKE_OBJ_CONTEXT_H_

#include "object.h"
#include <slake/except.h>
#include <memory>

namespace slake {
	struct ExceptionHandler final {
		Type type;
		uint32_t off;
	};

	/// @brief Minor frames are created by ENTER instructions and destroyed by
	/// LEAVE instructions.
	struct MinorFrame final {
		std::pmr::vector<ExceptionHandler> exceptHandlers;	// Exception handlers

		uint32_t nLocalVars = 0, nRegs = 0;
		size_t stackBase = 0;

		SLAKE_API MinorFrame(
			Runtime *rt,
			uint32_t nLocalVars,
			uint32_t nRegs,
			size_t stackBase);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MinorFrame() {
			abort();
		}
	};

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		Context *context = nullptr;		// Context

		const FnOverloadingObject *curFn = nullptr;	 // Current function overloading.
		uint32_t curIns = 0;						 // Offset of current instruction in function body.

		std::pmr::vector<RegularVarObject *> argStack;	// Argument stack.

		std::pmr::vector<Value> nextArgStack;  // Argument stack for next call.

		std::pmr::vector<LocalVarRecord> localVarRecords;  // Local variable records.
		LocalVarAccessorVarObject *localVarAccessor;	   // Local variable accessor.

		std::pmr::vector<Value> regs;  // Local registers.

		Object *thisObject = nullptr;  // `this' object.

		uint32_t returnValueOutReg = UINT32_MAX;

		std::pmr::vector<MinorFrame> minorFrames;  // Minor frames.

		Value curExcept = nullptr;	// Current exception.

		SLAKE_API MajorFrame(Runtime *rt, Context *context);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MajorFrame() {
			abort();
		}

		/// @brief Leave current minor frame.
		SLAKE_API void leave();
	};

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Done
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context {
		std::vector<std::unique_ptr<MajorFrame>> majorFrames;  // Major frame
		ContextFlags flags = 0;								   // Flags
		char *dataStack = nullptr;							   // Data stack
		size_t stackTop = 0;								   // Stack top

		SLAKE_API char *stackAlloc(size_t size);

		SLAKE_API Context(Runtime *runtime);

		SLAKE_API ~Context();
	};

	class ContextObject final : public Object {
	private:
		Context _context;

		friend class Runtime;

	public:
		SLAKE_API ContextObject(Runtime *rt);
		SLAKE_API virtual ~ContextObject();

		SLAKE_API virtual ObjectKind getKind() const override;

		SLAKE_API static HostObjectRef<ContextObject> alloc(Runtime *rt);
		SLAKE_API virtual void dealloc() override;

		SLAKE_FORCEINLINE Context &getContext() { return _context; }

		SLAKE_API InternalExceptionPointer resume(HostRefHolder *hostRefHolder);
		SLAKE_API Value getResult();
		SLAKE_API bool isDone();
	};
}

#endif
