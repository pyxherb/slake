#include "../runtime.h"
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API bool Runtime::execContext(ContextObject *context) {
	const FnOverloadingObject *curFn;
	MajorFrame *curMajorFrame;
	bool interruptExecution = false;

	while (!interruptExecution) {
		bool isDestructing = destructingThreads.count(std::this_thread::get_id());

		curMajorFrame = context->getContext().majorFrames.back().get();
		curFn = curMajorFrame->curFn;

		// TODO: Check if the yield request is from the top level.
		if (context->getContext().flags & CTX_YIELDED)
			return true;

		// Pause if the runtime is in GC
		while ((_flags & _RT_INGC) && !isDestructing)
			std::this_thread::yield();

		switch (curFn->getOverloadingKind()) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

				if (curMajorFrame->curIns == UINT32_MAX)
					interruptExecution = true;
				else {
					if (curMajorFrame->curIns >=
						ol->instructions.size()) {
						// Raise out of fn body error.
					}
					if (!_execIns(
							context,
							ol->instructions[curMajorFrame->curIns]))
						return false;
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)curFn;

				Value returnValue = ol->callback(
					ol,
					curMajorFrame->thisObject,
					curMajorFrame->argStack.data(),
					curMajorFrame->argStack.size());
				context->_context.majorFrames.pop_back();
				context->_context.majorFrames.back()->returnValue = returnValue;

				break;
			}
			default:
				throw std::logic_error("Unhandled function overloading type");
		}
	}

	context->_context.flags |= CTX_DONE;
	return true;
}

SLAKE_API bool Runtime::execFn(
	const FnOverloadingObject *overloading,
	ContextObject *prevContext,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	HostObjectRef<ContextObject> &contextOut) {
	HostObjectRef<ContextObject> context(prevContext);

	if (!context) {
		context = ContextObject::alloc(this);

		contextOut = context;

		{
			auto frame = std::make_unique<MajorFrame>(this, &context->getContext());
			frame->curFn = overloading;
			frame->curIns = UINT32_MAX;
			context->getContext().majorFrames.push_back(std::move(frame));
		}

		if (!_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs)) {
			// Error creating new major frame - return anyway.
			return false;
		}
	} else {
		contextOut = context;
	}

	if (!execContext(context.get()))
		return false;

	return true;
}
