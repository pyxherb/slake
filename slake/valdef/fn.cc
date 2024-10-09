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
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType)
	: Object(fnObject->_rt),
	  fnObject(fnObject),
	  access(access),
	  paramTypes(paramTypes),
	  returnType(returnType) {
}

FnOverloadingObject::FnOverloadingObject(const FnOverloadingObject &other) : Object(other) {
	fnObject = other.fnObject;

	access = other.access;

	genericParams = other.genericParams;
	mappedGenericArgs = other.mappedGenericArgs;

	paramTypes = other.paramTypes;
	returnType = other.returnType;

	overloadingFlags = other.overloadingFlags;
}

FnOverloadingObject::~FnOverloadingObject() {
}

ObjectKind FnOverloadingObject::getKind() const { return ObjectKind::FnOverloading; }

RegularFnOverloadingObject::RegularFnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType)
	: FnOverloadingObject(
		  fnObject,
		  access,
		  std::move(paramTypes),
		  returnType) {}

RegularFnOverloadingObject::RegularFnOverloadingObject(const RegularFnOverloadingObject &other) : FnOverloadingObject(other) {
	sourceLocDescs = other.sourceLocDescs;

	instructions.resize(other.instructions.size());
	for (size_t i = 0; i < instructions.size(); ++i) {
		instructions[i].opcode = other.instructions[i].opcode;

		if (auto &output = other.instructions[i].output; output.valueType == ValueType::ObjectRef) {
			if (auto ptr = output.getObjectRef(); ptr)
				instructions[i].output = ptr->duplicate();
			else
				instructions[i].output = nullptr;
		} else
			instructions[i].output = output;

		// Duplicate each of the operands.
		instructions[i].operands.resize(other.instructions[i].operands.size());
		for (size_t j = 0; j < other.instructions[i].operands.size(); ++j) {
			auto &operand = other.instructions[i].operands[j];

			if (operand.valueType == ValueType::ObjectRef) {
				if (auto ptr = operand.getObjectRef(); ptr)
					instructions[i].operands[j] =
						ptr->duplicate();
				else
					instructions[i].operands[j] = nullptr;
			} else
				instructions[i].operands[j] = operand;
		}
	}
}

RegularFnOverloadingObject::~RegularFnOverloadingObject() {
}

const slxfmt::SourceLocDesc *RegularFnOverloadingObject::getSourceLocationDesc(uint32_t offIns) const {
	const slxfmt::SourceLocDesc *curDesc = nullptr;

	for (auto &i : sourceLocDescs) {
		if ((offIns >= i.offIns) &&
			(offIns < i.offIns + i.nIns)) {
			if (curDesc) {
				if ((i.offIns >= curDesc->offIns) &&
					(i.nIns < curDesc->nIns))
					curDesc = &i;
			} else
				curDesc = &i;
		}
	}

	return curDesc;
}

FnOverloadingKind slake::RegularFnOverloadingObject::getOverloadingKind() const {
	return FnOverloadingKind::Regular;
}

FnOverloadingObject *slake::RegularFnOverloadingObject::duplicate() const {
	return (FnOverloadingObject *)alloc(this).get();
}

HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType) {
	using Alloc = std::pmr::polymorphic_allocator<RegularFnOverloadingObject>;
	Alloc allocator(&fnObject->_rt->globalHeapPoolResource);

	std::unique_ptr<RegularFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), fnObject, access, std::move(paramTypes), returnType);

	fnObject->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(const RegularFnOverloadingObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<RegularFnOverloadingObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<RegularFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::RegularFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<RegularFnOverloadingObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

NativeFnOverloadingObject::NativeFnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType,
	NativeFnCallback callback)
	: FnOverloadingObject(
		  fnObject,
		  access,
		  std::move(paramTypes),
		  returnType),
	  callback(callback) {}

NativeFnOverloadingObject::NativeFnOverloadingObject(const NativeFnOverloadingObject &other) : FnOverloadingObject(other) {
	callback = other.callback;
}

NativeFnOverloadingObject::~NativeFnOverloadingObject() {

}

FnOverloadingKind slake::NativeFnOverloadingObject::getOverloadingKind() const {
	return FnOverloadingKind::Native;
}

Value slake::NativeFnOverloadingObject::call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const {
	return callback(fnObject->_rt, thisObject, args, mappedGenericArgs);
}

FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate() const {
	return (FnOverloadingObject *)alloc(this).get();
}

HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(
	FnObject *fnObject,
	AccessModifier access,
	const std::vector<Type> &paramTypes,
	const Type &returnType,
	NativeFnCallback callback) {
	using Alloc = std::pmr::polymorphic_allocator<NativeFnOverloadingObject>;
	Alloc allocator(&fnObject->_rt->globalHeapPoolResource);

	std::unique_ptr<NativeFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));

	std::pmr::vector<Type> pmrParamTypes(&fnObject->_rt->globalHeapPoolResource);
	allocator.construct(ptr.get(), fnObject, access, std::move(pmrParamTypes), returnType, callback);

	fnObject->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(const NativeFnOverloadingObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<NativeFnOverloadingObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<NativeFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::NativeFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<NativeFnOverloadingObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

Value RegularFnOverloadingObject::call(Object *thisObject, std::pmr::vector<Value> args, HostRefHolder *hostRefHolder) const {
	// trimFnInstructions((std::vector<Instruction> &)instructions);
	Runtime *rt = fnObject->_rt;

	// Save previous context
	std::shared_ptr<Context> context;
	if (auto it = rt->activeContexts.find(std::this_thread::get_id());
		it != rt->activeContexts.end()) {
		context = it->second;
	} else {
		context = std::make_shared<Context>();

		auto frame = std::make_unique<MajorFrame>(rt, context.get());
		frame->curFn = this;
		frame->curIns = UINT32_MAX;
		context->majorFrames.push_back(std::move(frame));

		rt->activeContexts[std::this_thread::get_id()] = context;
	}

	if (!(context->flags & CTX_YIELDED)) {
		auto frame = std::make_unique<MajorFrame>(rt, context.get());
		frame->curFn = this;
		frame->curIns = 0;
		frame->scopeObject = fnObject->getParent();
		frame->thisObject = thisObject;
		frame->argStack.resize(args.size());
		for (size_t i = 0; i < args.size(); ++i) {
			auto var = RegularVarObject::alloc(rt, 0, i < paramTypes.size() ? paramTypes[i] : TypeId::Any);
			var->setData(VarRefContext(), args[i]);
			frame->argStack[i] = var.release();
		}
		context->majorFrames.push_back(std::move(frame));
	} else
		context->flags &= ~CTX_YIELDED;

	bool isDestructing = rt->destructingThreads.count(std::this_thread::get_id());

	MajorFrame *curMajorFrame;
	try {
		for (;;) {
			curMajorFrame = context->majorFrames.back().get();

			if (context->majorFrames.back()->curFn != this)
				break;

			if (curMajorFrame->curIns == UINT32_MAX)
				break;

			if (context->majorFrames.back()->curIns >=
				context->majorFrames.back()->curFn->instructions.size())
				throw OutOfFnBodyError("Out of function body");

			if (context->flags & CTX_YIELDED)
				break;

			// Pause if the runtime is in GC
			while ((rt->_flags & _RT_INGC) && !isDestructing)
				std::this_thread::yield();

			rt->_execIns(
				context.get(),
				curMajorFrame->curFn->instructions[curMajorFrame->curIns]);
		}
	} catch (...) {
		context->flags |= CTX_DONE;
		std::rethrow_exception(std::current_exception());
	}

	if (context->flags & CTX_YIELDED) {
		auto returnValue = ContextObject::alloc(rt, context);

		hostRefHolder->addObject(returnValue.get());

		return Value(returnValue.get());
	}

	if (context->majorFrames.back()->curIns == UINT32_MAX) {
		rt->activeContexts.erase(std::this_thread::get_id());
		context->flags |= CTX_DONE;
	}

	return context->majorFrames.back()->returnValue;
}

FnObject::FnObject(Runtime *rt) : MemberObject(rt) {
}

FnObject::FnObject(const FnObject &x) : MemberObject(x) {
	for (auto &i : x.overloadings) {
		FnOverloadingObject *ol = i->duplicate();

		ol->fnObject = this;

		overloadings.insert(ol);
	}

	parent = x.parent;
}

FnObject::~FnObject() {
}

ObjectKind FnObject::getKind() const { return ObjectKind::Fn; }

const char *FnObject::getName() const { return name.c_str(); }

void FnObject::setName(const char *name) { this->name = name; }

Object *FnObject::getParent() const { return parent; }

void FnObject::setParent(Object *parent) { this->parent = parent; }

FnOverloadingObject *FnObject::getOverloading(const std::pmr::vector<Type> &argTypes) const {
	const FnObject *i = this;

	for (auto &j : overloadings) {
		if (j->overloadingFlags & OL_VARG) {
			if (argTypes.size() < j->paramTypes.size())
				continue;
		} else {
			if (argTypes.size() != j->paramTypes.size())
				continue;
		}

		for (size_t k = 0; k < argTypes.size(); ++k) {
			assert(!argTypes[k].isLoadingDeferred());

			j->paramTypes[k].loadDeferredType(_rt);
			if (argTypes[k] != j->paramTypes[k])
				goto mismatched;
		}

		return j;

	mismatched:;
	}

	return nullptr;
}

Value FnObject::call(Object *thisObject, std::pmr::vector<Value> args, std::pmr::vector<Type> argTypes, HostRefHolder *hostRefHolder) const {
	FnOverloadingObject *overloading = getOverloading(argTypes);

	if (overloading)
		return overloading->call(thisObject, args, hostRefHolder);

	throw NoOverloadingError("No matching overloading was found");
}

Object *FnObject::duplicate() const {
	return (Object *)alloc(this).get();
}

HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<FnObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<FnObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

HostObjectRef<FnObject> slake::FnObject::alloc(const FnObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<FnObject>;
	Alloc allocator(&other->_rt->globalHeapPoolResource);

	std::unique_ptr<FnObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->_rt->createdObjects.insert(ptr.get());

	return ptr.release();
}

void slake::FnObject::dealloc() {
	std::pmr::polymorphic_allocator<FnObject> allocator(&_rt->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

bool slake::isDuplicatedOverloading(
	const FnOverloadingObject *overloading,
	const std::pmr::vector<Type> &paramTypes,
	const GenericParamList &genericParams,
	bool hasVarArg) {
	if ((overloading->overloadingFlags & OL_VARG) != (hasVarArg ? OL_VARG : 0))
		return false;

	if (overloading->paramTypes.size() != paramTypes.size())
		return false;

	if (overloading->genericParams.size() != genericParams.size())
		return false;

	for (size_t j = 0; j < paramTypes.size(); ++j) {
		if (overloading->paramTypes[j] != paramTypes[j])
			return false;
	}

	return true;
}
