#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API bool Instruction::operator==(const Instruction &rhs) const {
	if (opcode != rhs.opcode)
		return false;
	if (output != rhs.output)
		return false;
	if (nOperands != rhs.nOperands)
		return false;
	for (size_t i = 0; i < nOperands; ++i) {
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
	if (nOperands < rhs.nOperands)
		return true;
	if (nOperands > rhs.nOperands)
		return false;
	for (size_t i = 0; i < nOperands; ++i) {
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
	peff::DynArray<Type> &&paramTypes,
	const Type &returnType,
	OverloadingFlags flags)
	: Object(fnObject->associatedRuntime),
	  overloadingKind(overloadingKind),
	  fnObject(fnObject),
	  access(access),
	  genericParams(&fnObject->associatedRuntime->globalHeapPoolAlloc),
	  mappedGenericArgs(&fnObject->associatedRuntime->globalHeapPoolAlloc),
	  specializationArgs(&fnObject->associatedRuntime->globalHeapPoolAlloc),
	  paramTypes(std::move(paramTypes)),
	  returnType(returnType),
	  overloadingFlags(flags) {
}

SLAKE_API FnOverloadingObject::FnOverloadingObject(const FnOverloadingObject &other, bool &succeededOut)
	: Object(other),
	  genericParams(&other.associatedRuntime->globalHeapPoolAlloc),
	  mappedGenericArgs(&other.associatedRuntime->globalHeapPoolAlloc),
	  paramTypes(&other.associatedRuntime->globalHeapPoolAlloc),
	  specializationArgs(&other.associatedRuntime->globalHeapPoolAlloc) {
	fnObject = other.fnObject;

	access = other.access;

	if (!peff::copyAssign(genericParams, other.genericParams)) {
		succeededOut = false;
		return;
	}
	if (!peff::copyAssign(mappedGenericArgs, other.mappedGenericArgs)) {
		succeededOut = false;
		return;
	}

	if (!peff::copyAssign(paramTypes, other.paramTypes)) {
		succeededOut = false;
		return;
	}
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
	peff::DynArray<Type> &&paramTypes,
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
	  nRegisters(nRegisters),
	  sourceLocDescs(&fnObject->associatedRuntime->globalHeapPoolAlloc),
	  instructions(&fnObject->associatedRuntime->globalHeapPoolAlloc) {}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(const RegularFnOverloadingObject &other, bool &succeededOut) : FnOverloadingObject(other, succeededOut), sourceLocDescs(&other.associatedRuntime->globalHeapPoolAlloc), instructions(&other.associatedRuntime->globalHeapPoolAlloc) {
	if (succeededOut) {
		if (!peff::copyAssign(sourceLocDescs, other.sourceLocDescs)) {
			succeededOut = false;
			return;
		}

		if (!instructions.resize(other.instructions.size())) {
			succeededOut = false;
			return;
		}
		for (size_t i = 0; i < instructions.size(); ++i) {
			Instruction &curIns = instructions.at(i), otherCurIns = other.instructions.at(i);
			curIns.opcode = otherCurIns.opcode;

			if (auto &output = otherCurIns.output; output.valueType == ValueType::EntityRef) {
				const EntityRef &entityRef = output.getEntityRef();
				switch (entityRef.kind) {
					case ObjectRefKind::ObjectRef:
						if (entityRef.asObject.instanceObject)
							curIns.output = EntityRef::makeObjectRef(entityRef.asObject.instanceObject->duplicate());
						break;
					default:
						curIns.output = output;
				}
			} else
				curIns.output = output;

			// Duplicate each of the operands.
			for (size_t j = 0; j < otherCurIns.nOperands; ++j) {
				auto &operand = otherCurIns.operands[j];

				if (operand.valueType == ValueType::EntityRef) {
					const EntityRef &entityRef = operand.getEntityRef();
					switch (entityRef.kind) {
						case ObjectRefKind::ObjectRef:
							if (entityRef.asObject.instanceObject)
								curIns.operands[j] = EntityRef::makeObjectRef(entityRef.asObject.instanceObject->duplicate());
							else
								curIns.operands[j] = operand;
							break;
						default:
							curIns.operands[j] = operand;
					}
				} else
					curIns.operands[j] = operand;
			}
			for (size_t j = otherCurIns.nOperands; j < 3; ++j) {
				curIns.operands[j] = Value(ValueType::Undefined);
			}
			curIns.nOperands = otherCurIns.nOperands;
		}

		nRegisters = other.nRegisters;
	}
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
	peff::DynArray<Type> &&paramTypes,
	const Type &returnType,
	uint32_t nRegisters,
	OverloadingFlags flags) {
	std::unique_ptr<RegularFnOverloadingObject, util::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::allocAndConstruct<RegularFnOverloadingObject>(
			&fnObject->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			fnObject, access, std::move(paramTypes), returnType, nRegisters, flags));
	if (!ptr)
		return nullptr;

	if (!fnObject->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(const RegularFnOverloadingObject *other) {
	bool succeeded = true;

	std::unique_ptr<RegularFnOverloadingObject, util::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::allocAndConstruct<RegularFnOverloadingObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::RegularFnOverloadingObject::dealloc() {
	peff::destroyAndRelease<RegularFnOverloadingObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(
	FnObject *fnObject,
	AccessModifier access,
	peff::DynArray<Type> &&paramTypes,
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

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(const NativeFnOverloadingObject &other, bool &succeededOut) : FnOverloadingObject(other, succeededOut) {
	if (succeededOut) {
		callback = other.callback;
	}
}

SLAKE_API NativeFnOverloadingObject::~NativeFnOverloadingObject() {
}

SLAKE_API FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate() const {
	return (FnOverloadingObject *)alloc(this).get();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(
	FnObject *fnObject,
	AccessModifier access,
	peff::DynArray<Type> &&paramTypes,
	const Type &returnType,
	OverloadingFlags flags,
	NativeFnCallback callback) {
	std::unique_ptr<NativeFnOverloadingObject, util::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
		peff::allocAndConstruct<NativeFnOverloadingObject>(
			&fnObject->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			fnObject, access, std::move(paramTypes), returnType, flags, callback));
	if (!ptr)
		return nullptr;

	if (!fnObject->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(const NativeFnOverloadingObject *other) {
	bool succeeded = true;

	std::unique_ptr<NativeFnOverloadingObject, util::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
		peff::allocAndConstruct<NativeFnOverloadingObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::NativeFnOverloadingObject::dealloc() {
	peff::destroyAndRelease<NativeFnOverloadingObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API FnObject::FnObject(Runtime *rt) : MemberObject(rt), overloadings(&rt->globalHeapPoolAlloc) {
}

SLAKE_API FnObject::FnObject(const FnObject &x, bool &succeededOut) : MemberObject(x, succeededOut), overloadings(&x.associatedRuntime->globalHeapPoolAlloc) {
	if (succeededOut) {
		for (auto i : x.overloadings) {
			FnOverloadingObject *ol = i->duplicate();

			if (!ol) {
				succeededOut = false;
				return;
			}

			ol->fnObject = this;

			if (!overloadings.insert(std::move(ol))) {
				succeededOut = false;
				return;
			}
		}

		parent = x.parent;
	}
}

SLAKE_API FnObject::~FnObject() {
}

SLAKE_API ObjectKind FnObject::getKind() const { return ObjectKind::Fn; }

SLAKE_API Object *FnObject::getParent() const { return parent; }

SLAKE_API void FnObject::setParent(Object *parent) { this->parent = parent; }

SLAKE_API FnOverloadingObject *FnObject::getOverloading(const peff::DynArray<Type> &argTypes) const {
	const FnObject *i = this;

	for (auto j : overloadings) {
		if (j->overloadingFlags & OL_VARG) {
			if (argTypes.size() < j->paramTypes.size())
				continue;
		} else {
			if (argTypes.size() != j->paramTypes.size())
				continue;
		}

		for (size_t k = 0; k < argTypes.size(); ++k) {
			assert(!argTypes.at(k).isLoadingDeferred());

			if (auto e = j->paramTypes.at(k).loadDeferredType(associatedRuntime);
				e) {
				e.reset();
				goto mismatched;
			}
			if (argTypes.at(k) != j->paramTypes.at(k))
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
	std::unique_ptr<FnObject, util::DeallocableDeleter<FnObject>> ptr(
		peff::allocAndConstruct<FnObject>(
			&rt->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			rt));

	if (!rt->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(const FnObject *other) {
	bool succeeded = true;

	std::unique_ptr<FnObject, util::DeallocableDeleter<FnObject>> ptr(
		peff::allocAndConstruct<FnObject>(
			&other->associatedRuntime->globalHeapPoolAlloc,
			sizeof(std::max_align_t),
			*other, succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->createdObjects.insert(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::FnObject::dealloc() {
	peff::destroyAndRelease<FnObject>(&associatedRuntime->globalHeapPoolAlloc, this, sizeof(std::max_align_t));
}

SLAKE_API FnOverloadingObject *slake::findOverloading(
	FnObject *fnObject,
	const peff::DynArray<Type> &paramTypes,
	const GenericParamList &genericParams,
	bool hasVarArg) {
	for (auto i : fnObject->overloadings) {
		if (isDuplicatedOverloading(i, paramTypes, genericParams, hasVarArg)) {
			return i;
		}
	}

	return nullptr;
}

SLAKE_API bool slake::isDuplicatedOverloading(
	const FnOverloadingObject *overloading,
	const peff::DynArray<Type> &paramTypes,
	const GenericParamList &genericParams,
	bool hasVarArg) {
	if ((overloading->overloadingFlags & OL_VARG) != (hasVarArg ? OL_VARG : 0))
		return false;

	if (overloading->paramTypes.size() != paramTypes.size())
		return false;

	if (overloading->genericParams.size() != genericParams.size())
		return false;

	for (size_t j = 0; j < paramTypes.size(); ++j) {
		if (overloading->paramTypes.at(j) != paramTypes.at(j))
			return false;
	}

	return true;
}
