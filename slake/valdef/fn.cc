#include <slake/runtime.h>

using namespace slake;

Type FnObject::getType() const { return TypeId::Fn; }

FnOverloadingObject::FnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	std::deque<Type> paramTypes,
	Type returnType)
	: Object(fnObject->_rt),
	  fnObject(fnObject),
	  access(access),
	  paramTypes(paramTypes),
	  returnType(returnType) {
	reportSizeAllocatedToRuntime(sizeof(*this) - sizeof(Object));
}

FnOverloadingObject::~FnOverloadingObject() {
	reportSizeFreedToRuntime(sizeof(*this) - sizeof(Object));
}

FnOverloadingKind slake::RegularFnOverloadingObject::getOverloadingKind() const {
	return FnOverloadingKind::Regular;
}

FnOverloadingObject *slake::RegularFnOverloadingObject::duplicate() const {
	RegularFnOverloadingObject *v = new RegularFnOverloadingObject(fnObject, access, {}, returnType);

	*v = *this;

	return (FnOverloadingObject *)v;
}

FnOverloadingKind slake::NativeFnOverloadingObject::getOverloadingKind() const {
	return FnOverloadingKind::Native;
}

Value slake::NativeFnOverloadingObject::call(Object *thisObject, std::deque<Value> args) const {
	return callback(fnObject->_rt, thisObject, args, mappedGenericArgs);
}

FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate() const {
	NativeFnOverloadingObject *v = new NativeFnOverloadingObject(fnObject, access, {}, returnType, {});

	*v = *this;

	return (FnOverloadingObject *)v;
}

Value RegularFnOverloadingObject::call(Object *thisObject, std::deque<Value> args) const {
	Runtime *rt = fnObject->_rt;

	// Save previous context
	std::shared_ptr<Context> context;
	if (auto it = rt->activeContexts.find(std::this_thread::get_id());
		it != rt->activeContexts.end()) {
		context = it->second;
	} else {
		context = std::make_shared<Context>();

		auto frame = MajorFrame(rt);
		frame.curFn = this;
		frame.curIns = UINT32_MAX;
		context->majorFrames.push_back(frame);

		rt->activeContexts[std::this_thread::get_id()] = context;
	}

	if (!(context->flags & CTX_YIELDED)) {
		auto frame = MajorFrame(rt);
		frame.curFn = this;
		frame.curIns = 0;
		frame.scopeObject = fnObject->_parent;
		frame.thisObject = thisObject;
		frame.argStack.resize(args.size());
		for (size_t i = 0; i < args.size(); ++i) {
			auto var = new VarObject(rt, 0, TypeId::Any);
			var->setData(args[i]);
			frame.argStack[i] = var;
		}
		context->majorFrames.push_back(frame);
	} else
		context->flags &= ~CTX_YIELDED;

	bool isDestructing = rt->destructingThreads.count(std::this_thread::get_id());

	try {
		while ((context->majorFrames.back().curIns != UINT32_MAX) &&
			   (context->majorFrames.back().curFn == this)) {
			if (context->majorFrames.back().curIns >= context->majorFrames.back().curFn->instructions.size())
				throw OutOfFnBodyError("Out of function body");

			if (context->flags & CTX_YIELDED)
				break;

			// Pause if the runtime is in GC
			while ((rt->_flags & _RT_INGC) && !isDestructing)
				std::this_thread::yield();

			rt->_execIns(
				context.get(),
				context->majorFrames.back().curFn->instructions[context->majorFrames.back().curIns]);

			if ((rt->_szMemInUse > (rt->_szMemUsedAfterLastGc << 1)) && !isDestructing)
				rt->gc();
		}
	} catch (...) {
		context->flags |= CTX_DONE;
		std::rethrow_exception(std::current_exception());
	}

	// Do a GC cycle if size of memory in use is greater than double the size used after last cycle.
	if (((rt->_szMemInUse >> 1) > rt->_szMemUsedAfterLastGc) && !isDestructing)
		rt->gc();

	if (context->flags & CTX_YIELDED)
		return new ContextObject(rt, context);

	if (context->majorFrames.back().curIns == UINT32_MAX) {
		rt->activeContexts.erase(std::this_thread::get_id());
		context->flags |= CTX_DONE;
	}

	return context->majorFrames.back().returnValue;
}

FnOverloadingObject *FnObject::getOverloading(std::deque<Type> argTypes) const {
	for (auto &i : overloadings) {
		if (i->overloadingFlags & OL_VARG) {
			if (argTypes.size() < i->paramTypes.size())
				continue;
		} else {
			if (argTypes.size() != i->paramTypes.size())
				continue;
		}

		for (size_t j = 0; j < argTypes.size(); ++j) {
			argTypes[j].loadDeferredType(_rt);
			i->paramTypes[j].loadDeferredType(_rt);

			if (argTypes[j] != i->paramTypes[j])
				goto mismatched;
		}

		return i;

	mismatched:;
	}

	if (parentFn)
		return parentFn->getOverloading(argTypes);

	return nullptr;
}

Value FnObject::call(Object *thisObject, std::deque<Value> args, std::deque<Type> argTypes) const {
	FnOverloadingObject *overloading = getOverloading(argTypes);

	if (overloading)
		return overloading->call(thisObject, args);

	throw NoOverloadingError("No matching overloading was found");
}

Object *FnObject::duplicate() const {
	FnObject *v = new FnObject(_rt);

	*v = *this;

	return (Object *)v;
}
