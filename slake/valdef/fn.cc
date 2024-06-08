#include <slake/runtime.h>

using namespace slake;

Type FnObject::getType() const { return TypeId::Fn; }

FnOverloadingObject::FnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	const std::deque<Type> &paramTypes,
	const Type &returnType)
	: Object(fnObject->_rt),
	  fnObject(fnObject),
	  access(access),
	  paramTypes(paramTypes),
	  returnType(returnType) {
}

FnOverloadingObject::~FnOverloadingObject() {
}

FnOverloadingKind slake::RegularFnOverloadingObject::getOverloadingKind() const {
	return FnOverloadingKind::Regular;
}

FnOverloadingObject *slake::RegularFnOverloadingObject::duplicate() const {
	HostObjectRef<RegularFnOverloadingObject> v = RegularFnOverloadingObject::alloc(fnObject, access, {}, returnType);

	*(v.get()) = *this;

	return (FnOverloadingObject *)v.release();
}

HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(FnObject *fnObject, AccessModifier access, const std::deque<Type> &paramTypes, const Type &returnType) {
	std::pmr::polymorphic_allocator<RegularFnOverloadingObject> allocator(&fnObject->_rt->globalHeapPoolResource);

	RegularFnOverloadingObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, fnObject, access, paramTypes, returnType);

	return ptr;
}

void slake::RegularFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<RegularFnOverloadingObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

FnOverloadingKind slake::NativeFnOverloadingObject::getOverloadingKind() const {
	return FnOverloadingKind::Native;
}

Value slake::NativeFnOverloadingObject::call(Object *thisObject, std::deque<Value> args) const {
	return callback(fnObject->_rt, thisObject, args, mappedGenericArgs);
}

FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate() const {
	HostObjectRef<NativeFnOverloadingObject> v = NativeFnOverloadingObject::alloc(fnObject, access, {}, returnType, {});

	*(v.get()) = *this;

	return (FnOverloadingObject *)v.release();
}

HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(
	FnObject *fnObject,
	AccessModifier access,
	const std::deque<Type> &paramTypes,
	const Type &returnType,
	NativeFnCallback callback) {
	std::pmr::polymorphic_allocator<NativeFnOverloadingObject> allocator(&fnObject->_rt->globalHeapPoolResource);

	NativeFnOverloadingObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, fnObject, access, paramTypes, returnType, callback);

	return ptr;
}

void slake::NativeFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<NativeFnOverloadingObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
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
			auto var = VarObject::alloc(rt, 0, TypeId::Any);
			var->setData(args[i]);
			frame.argStack[i] = var.release();
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
		}
	} catch (...) {
		context->flags |= CTX_DONE;
		std::rethrow_exception(std::current_exception());
	}

	if (context->flags & CTX_YIELDED)
		return Value(ContextObject::alloc(rt, context).release(), true);

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
	HostObjectRef<FnObject> v = FnObject::alloc(_rt);

	*(v.get()) = *this;

	return (Object *)v.release();
}

HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	std::pmr::polymorphic_allocator<FnObject> allocator(&rt->globalHeapPoolResource);

	FnObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt);

	return ptr;
}

void slake::FnObject::dealloc() {
	std::pmr::polymorphic_allocator<FnObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
