#include <slake/runtime.h>
#include <slake/opti/optimizer.h>

using namespace slake;

bool Instruction::operator==(const Instruction &rhs) const {
	if (opcode != rhs.opcode)
		return false;
	if (output != rhs.output)
		return false;
	if (operands.size() != rhs.operands.size())
		return false;
	for (size_t i = 0; i < operands.size(); ++i) {
		if (operands[i] != rhs.operands[i])
			return false;
	}
	return true;
}

bool Instruction::operator<(const Instruction &rhs) const {
	if (opcode < rhs.opcode)
		return true;
	if (opcode > rhs.opcode)
		return false;
	if (output < rhs.output)
		return true;
	if (operands.size() < rhs.operands.size())
		return true;
	if (operands.size() > rhs.operands.size())
		return false;
	for (size_t i = 0; i < operands.size(); ++i) {
		if (operands[i] < rhs.operands[i])
			return true;
		if (operands[i] != rhs.operands[i])
			return false;
	}
	return false;
}

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
	return (FnOverloadingObject *)alloc(this).get();
}

HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(FnObject *fnObject, AccessModifier access, const std::deque<Type> &paramTypes, const Type &returnType) {
	std::pmr::polymorphic_allocator<RegularFnOverloadingObject> allocator(&fnObject->_rt->globalHeapPoolResource);

	RegularFnOverloadingObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, fnObject, access, paramTypes, returnType);

	fnObject->_rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(const RegularFnOverloadingObject *other) {
	std::pmr::polymorphic_allocator<RegularFnOverloadingObject> allocator(&other->_rt->globalHeapPoolResource);

	RegularFnOverloadingObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

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
	return (FnOverloadingObject *)alloc(this).get();
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

	fnObject->_rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(const NativeFnOverloadingObject *other) {
	std::pmr::polymorphic_allocator<NativeFnOverloadingObject> allocator(&other->_rt->globalHeapPoolResource);

	NativeFnOverloadingObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::NativeFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<NativeFnOverloadingObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

Value RegularFnOverloadingObject::call(Object *thisObject, std::deque<Value> args) const {
	trimFnInstructions((std::vector<Instruction>&)instructions);
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
			auto var = VarObject::alloc(rt, 0, i < paramTypes.size() ? paramTypes[i] : TypeId::Any);
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

			MajorFrame &curMajorFrame = context->majorFrames.back();
			Instruction curIns;
			curIns.opcode = Opcode::NOP;
			curIns.operands = {};
			curIns = curMajorFrame.curFn->instructions[curMajorFrame.curIns];

			rt->_execIns(
				context.get(),
				curIns);
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
	const FnObject *i = this;

	for (; i->descentFn; i = i->descentFn)
		;

	while (i) {
		for (auto &j : overloadings) {
			if (j->overloadingFlags & OL_VARG) {
				if (argTypes.size() < j->paramTypes.size())
					continue;
			} else {
				if (argTypes.size() != j->paramTypes.size())
					continue;
			}

			for (size_t k = 0; k < argTypes.size(); ++k) {
				argTypes[k].loadDeferredType(_rt);
				j->paramTypes[k].loadDeferredType(_rt);

				if (argTypes[k] != j->paramTypes[k])
					goto mismatched;
			}

			return j;

		mismatched:;
		}

		i = i->parentFn;
	}

	return nullptr;
}

Value FnObject::call(Object *thisObject, std::deque<Value> args, std::deque<Type> argTypes) const {
	FnOverloadingObject *overloading = getOverloading(argTypes);

	if (overloading)
		return overloading->call(thisObject, args);

	throw NoOverloadingError("No matching overloading was found");
}

Object *FnObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	std::pmr::polymorphic_allocator<FnObject> allocator(&rt->globalHeapPoolResource);

	FnObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, rt);

	rt->createdObjects.insert(ptr);

	return ptr;
}

HostObjectRef<FnObject> slake::FnObject::alloc(const FnObject *other) {
	std::pmr::polymorphic_allocator<FnObject> allocator(&other->_rt->globalHeapPoolResource);

	FnObject *ptr = allocator.allocate(1);
	allocator.construct(ptr, *other);

	other->_rt->createdObjects.insert(ptr);

	return ptr;
}

void slake::FnObject::dealloc() {
	std::pmr::polymorphic_allocator<FnObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}
