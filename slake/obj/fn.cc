#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API bool Instruction::operator==(const Instruction &rhs) const {
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

SLAKE_API bool Instruction::operator<(const Instruction &rhs) const {
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

SLAKE_API FnOverloadingObject::FnOverloadingObject(
	FnOverloadingKind overloadingKind,
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType,
	OverloadingFlags flags)
	: Object(fnObject->associatedRuntime),
	  overloadingKind(overloadingKind),
	  fnObject(fnObject),
	  access(access),
	  paramTypes(paramTypes),
	  returnType(returnType),
	  overloadingFlags(flags) {
}

SLAKE_API FnOverloadingObject::FnOverloadingObject(const FnOverloadingObject &other) : Object(other) {
	fnObject = other.fnObject;

	access = other.access;

	genericParams = other.genericParams;
	mappedGenericArgs = other.mappedGenericArgs;

	paramTypes = other.paramTypes;
	returnType = other.returnType;

	overloadingFlags = other.overloadingFlags;
	overloadingKind = other.overloadingKind;
}

SLAKE_API FnOverloadingObject::~FnOverloadingObject() {
}

SLAKE_API ObjectKind FnOverloadingObject::getKind() const { return ObjectKind::FnOverloading; }

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType,
	uint32_t nRegisters,
	OverloadingFlags flags)
	: FnOverloadingObject(
		  FnOverloadingKind::Regular,
		  fnObject,
		  access,
		  std::move(paramTypes),
		  returnType,
		  flags),
	  nRegisters(nRegisters) {}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(const RegularFnOverloadingObject &other) : FnOverloadingObject(other) {
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

	nRegisters = other.nRegisters;
}

SLAKE_API RegularFnOverloadingObject::~RegularFnOverloadingObject() {
}

SLAKE_API const slxfmt::SourceLocDesc *RegularFnOverloadingObject::getSourceLocationDesc(uint32_t offIns) const {
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

SLAKE_API FnOverloadingObject *slake::RegularFnOverloadingObject::duplicate() const {
	return (FnOverloadingObject *)alloc(this).get();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType,
	uint32_t nRegisters,
	OverloadingFlags flags) {
	using Alloc = std::pmr::polymorphic_allocator<RegularFnOverloadingObject>;
	Alloc allocator(&fnObject->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<RegularFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), fnObject, access, std::move(paramTypes), returnType, nRegisters, flags);

	fnObject->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(const RegularFnOverloadingObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<RegularFnOverloadingObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<RegularFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::RegularFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<RegularFnOverloadingObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	std::pmr::vector<Type> &&paramTypes,
	const Type &returnType,
	OverloadingFlags flags,
	NativeFnCallback callback)
	: FnOverloadingObject(
		  FnOverloadingKind::Native,
		  fnObject,
		  access,
		  std::move(paramTypes),
		  returnType,
		  flags),
	  callback(callback) {}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(const NativeFnOverloadingObject &other) : FnOverloadingObject(other) {
	callback = other.callback;
}

SLAKE_API NativeFnOverloadingObject::~NativeFnOverloadingObject() {
}

SLAKE_API FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate() const {
	return (FnOverloadingObject *)alloc(this).get();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(
	FnObject *fnObject,
	AccessModifier access,
	const std::vector<Type> &paramTypes,
	const Type &returnType,
	OverloadingFlags flags,
	NativeFnCallback callback) {
	using Alloc = std::pmr::polymorphic_allocator<NativeFnOverloadingObject>;
	Alloc allocator(&fnObject->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<NativeFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));

	std::pmr::vector<Type> pmrParamTypes(&fnObject->associatedRuntime->globalHeapPoolResource);
	allocator.construct(ptr.get(), fnObject, access, std::move(pmrParamTypes), returnType, flags, callback);

	fnObject->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(const NativeFnOverloadingObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<NativeFnOverloadingObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<NativeFnOverloadingObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::NativeFnOverloadingObject::dealloc() {
	std::pmr::polymorphic_allocator<NativeFnOverloadingObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API FnObject::FnObject(Runtime *rt) : MemberObject(rt) {
}

SLAKE_API FnObject::FnObject(const FnObject &x) : MemberObject(x) {
	for (auto &i : x.overloadings) {
		FnOverloadingObject *ol = i->duplicate();

		ol->fnObject = this;

		overloadings.insert(ol);
	}

	parent = x.parent;
}

SLAKE_API FnObject::~FnObject() {
}

SLAKE_API ObjectKind FnObject::getKind() const { return ObjectKind::Fn; }

SLAKE_API const char *FnObject::getName() const { return name.c_str(); }

SLAKE_API void FnObject::setName(const char *name) { this->name = name; }

SLAKE_API Object *FnObject::getParent() const { return parent; }

SLAKE_API void FnObject::setParent(Object *parent) { this->parent = parent; }

SLAKE_API FnOverloadingObject *FnObject::getOverloading(const std::pmr::vector<Type> &argTypes) const {
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

			if (auto e = j->paramTypes[k].loadDeferredType(associatedRuntime);
				e) {
				e.reset();
				goto mismatched;
			}
			if (argTypes[k] != j->paramTypes[k])
				goto mismatched;
		}

		return j;

	mismatched:;
	}

	return nullptr;
}

SLAKE_API Object *FnObject::duplicate() const {
	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	using Alloc = std::pmr::polymorphic_allocator<FnObject>;
	Alloc allocator(&rt->globalHeapPoolResource);

	std::unique_ptr<FnObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), rt);

	rt->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(const FnObject *other) {
	using Alloc = std::pmr::polymorphic_allocator<FnObject>;
	Alloc allocator(&other->associatedRuntime->globalHeapPoolResource);

	std::unique_ptr<FnObject, util::StatefulDeleter<Alloc>> ptr(
		allocator.allocate(1),
		util::StatefulDeleter<Alloc>(allocator));
	allocator.construct(ptr.get(), *other);

	other->associatedRuntime->createdObjects.push_back(ptr.get());

	return ptr.release();
}

SLAKE_API void slake::FnObject::dealloc() {
	std::pmr::polymorphic_allocator<FnObject> allocator(&associatedRuntime->globalHeapPoolResource);

	std::destroy_at(this);
	allocator.deallocate(this, 1);
}

SLAKE_API bool slake::isDuplicatedOverloading(
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
