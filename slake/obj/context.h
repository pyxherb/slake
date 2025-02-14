#ifndef _SLAKE_OBJ_CONTEXT_H_
#define _SLAKE_OBJ_CONTEXT_H_

#include "object.h"
#include "var.h"
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
		peff::DynArray<ExceptionHandler> exceptHandlers;  // Exception handlers

		uint32_t nLocalVars = 0;
		size_t stackBase = 0;

		SLAKE_API MinorFrame(
			Runtime *rt,
			uint32_t nLocalVars,
			size_t stackBase);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MinorFrame() {
			abort();
		}
	};

	struct ArgRecord {
		Value value;
		Type type;
	};

	/// @brief A major frame represents a single calling frame.
	struct MajorFrame final {
		Context *context = nullptr;	 // Context

		const FnOverloadingObject *curFn = nullptr;	 // Current function overloading.
		uint32_t curIns = 0;						 // Offset of current instruction in function body.

		peff::DynArray<ArgRecord> argStack;	 // Argument stack.

		peff::DynArray<Value> nextArgStack;	 // Argument stack for next call.

		peff::DynArray<LocalVarRecord> localVarRecords;			// Local variable records.
		LocalVarAccessorVarObject *localVarAccessor = nullptr;	// Local variable accessor.

		peff::DynArray<Value> regs;	 // Local registers.
		size_t nRegs = 0;

		Object *thisObject = nullptr;  // `this' object.

		uint32_t returnValueOutReg = UINT32_MAX;

		peff::DynArray<MinorFrame> minorFrames;	 // Minor frames.

		Value curExcept = Value();	// Current exception.

		SLAKE_API MajorFrame(Runtime *rt, Context *context);
		// Default constructor is required by resize() methods from the
		// containers.
		SLAKE_FORCEINLINE MajorFrame() {
			abort();
		}

		/// @brief Leave current minor frame.
		SLAKE_API [[nodiscard]] bool leave();

		SLAKE_FORCEINLINE void resizeRegs(size_t nRegs) {
			this->nRegs = nRegs;
			regs.resizeWith(nRegs, Value(ValueType::Undefined));
		}
	};

	using ContextFlags = uint8_t;
	constexpr static ContextFlags
		// Done
		CTX_DONE = 0x01,
		// Yielded
		CTX_YIELDED = 0x02;

	struct Context {
		Runtime *runtime;
		std::vector<std::unique_ptr<MajorFrame>> majorFrames;  // Major frame
		ContextFlags flags = 0;								   // Flags
		char *dataStack = nullptr;							   // Data stack
		size_t stackTop = 0;								   // Stack top

		SLAKE_API char *stackAlloc(size_t size);

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
		SLAKE_API Value getResult();
		SLAKE_API bool isDone();
	};
}

#endif
