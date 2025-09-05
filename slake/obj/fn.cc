#include <slake/runtime.h>
#include <slake/util/scope_guard.h>

using namespace slake;

SLAKE_API Instruction::Instruction()
	: opcode((Opcode)0xff),
	  nOperands(0),
	  output(UINT32_MAX),
	  operands(nullptr),
	  operandsAllocator(nullptr) {
}

SLAKE_API Instruction::Instruction(Instruction &&rhs)
	: opcode(rhs.opcode),
	  nOperands(rhs.nOperands),
	  output(rhs.output),
	  operands(rhs.operands),
	  operandsAllocator(rhs.operandsAllocator) {
	rhs.opcode = (Opcode)0xff;
	rhs.nOperands = 0;
	rhs.output = UINT32_MAX;
	rhs.operands = nullptr;
	rhs.operandsAllocator = nullptr;
}

SLAKE_API Instruction::~Instruction() {
	clearOperands();
}

SLAKE_API void Instruction::clearOperands() {
	if (nOperands) {
		operandsAllocator->release(operands, sizeof(Value) * nOperands, alignof(Value));
		operands = nullptr;
		nOperands = 0;
	} else {
		assert(!operands);
	}
	operandsAllocator = nullptr;
}

[[nodiscard]] SLAKE_API bool Instruction::reserveOperands(peff::Alloc *allocator, uint32_t nOperands) {
	clearOperands();
	if (nOperands) {
		if (!(operands = (Value *)allocator->alloc(sizeof(Value) * nOperands, alignof(Value)))) {
			return false;
		}
		operandsAllocator = allocator;
	}
	this->nOperands = nOperands;
	return true;
}

SLAKE_API void Instruction::replaceAllocator(peff::Alloc *allocator) noexcept {
	peff::verifyReplaceable(operandsAllocator.get(), allocator);

	operandsAllocator = allocator;
}

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

SLAKE_API Instruction &Instruction::operator=(Instruction &&rhs) {
	peff::constructAt<Instruction>(this, std::move(rhs));
	return *this;
}

SLAKE_API FnOverloadingObject::FnOverloadingObject(
	FnOverloadingKind overloadingKind,
	FnObject *fnObject,
	peff::Alloc *selfAllocator)
	: Object(fnObject->associatedRuntime, selfAllocator, ObjectKind::FnOverloading),
	  overloadingKind(overloadingKind),
	  fnObject(fnObject),
	  genericParams(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  paramTypes(selfAllocator),
	  returnType({ TypeId::Void }) {
}

SLAKE_API FnOverloadingObject::FnOverloadingObject(const FnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut)
	: Object(other, allocator),
	  genericParams(allocator),
	  mappedGenericArgs(allocator),
	  paramTypes(allocator) {
	fnObject = other.fnObject;

	access = other.access;

	if (!peff::copyAssign(genericParams, other.genericParams)) {
		succeededOut = false;
		return;
	}
	for (auto [k, v] : other.mappedGenericArgs) {
		peff::String name(allocator);

		if (!name.build(k)) {
			succeededOut = false;
			return;
		}

		if (!(mappedGenericArgs.insert(std::move(name), TypeRef(v)))) {
			succeededOut = false;
			return;
		}
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

SLAKE_API void FnOverloadingObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	genericParams.replaceAllocator(allocator);

	for (auto& i : genericParams) {
		i.replaceAllocator(allocator);
	}

	mappedGenericArgs.replaceAllocator(allocator);

	for (auto i : mappedGenericArgs) {
		i.first.replaceAllocator(allocator);
	}

	paramTypes.replaceAllocator(allocator);
}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(
	FnObject *fnObject,
	peff::Alloc *selfAllocator)
	: FnOverloadingObject(
		  FnOverloadingKind::Regular,
		  fnObject,
		  selfAllocator),
	  nRegisters(0),
	  sourceLocDescs(selfAllocator),
	  instructions(selfAllocator) {}

SLAKE_API RegularFnOverloadingObject::RegularFnOverloadingObject(const RegularFnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut) : FnOverloadingObject(other, allocator, succeededOut), sourceLocDescs(allocator), instructions(allocator) {
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
			Instruction &curIns = instructions.at(i);
			const Instruction &otherCurIns = other.instructions.at(i);
			curIns.opcode = otherCurIns.opcode;

			curIns.output = otherCurIns.output;

			if (!curIns.reserveOperands(allocator, otherCurIns.nOperands)) {
				succeededOut = false;
				return;
			}
			for (size_t j = 0; j < otherCurIns.nOperands; ++j) {
				curIns.operands[j] = Value(ValueType::Undefined);
			}

			// Duplicate each of the operands.
			for (size_t j = 0; j < otherCurIns.nOperands; ++j) {
				auto &operand = otherCurIns.operands[j];

				if (operand.valueType == ValueType::EntityRef) {
					const EntityRef &entityRef = operand.getEntityRef();
					switch (entityRef.kind) {
						case ObjectRefKind::ObjectRef:
							if (entityRef.asObject.instanceObject)
								curIns.operands[j] = EntityRef::makeObjectRef(entityRef.asObject.instanceObject->duplicate(nullptr));
							else
								curIns.operands[j] = operand;
							break;
						default:
							curIns.operands[j] = operand;
					}
				} else
					curIns.operands[j] = operand;
			}
		}

		nRegisters = other.nRegisters;
	}
}

SLAKE_API RegularFnOverloadingObject::~RegularFnOverloadingObject() {
	instructions.clear();
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

SLAKE_API Object *slake::RegularFnOverloadingObject::duplicate(Duplicator *duplicator) const {
	return alloc(this).get();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(
	FnObject *fnObject) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = fnObject->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<RegularFnOverloadingObject, util::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::allocAndConstruct<RegularFnOverloadingObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			fnObject,
			curGenerationAllocator.get()));
	if (!ptr)
		return nullptr;

	if (!fnObject->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(const RegularFnOverloadingObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->fnObject->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<RegularFnOverloadingObject, util::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
		peff::allocAndConstruct<RegularFnOverloadingObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::RegularFnOverloadingObject::dealloc() {
	peff::destroyAndRelease<RegularFnOverloadingObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void RegularFnOverloadingObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->FnOverloadingObject::replaceAllocator(allocator);

	sourceLocDescs.replaceAllocator(allocator);

	instructions.replaceAllocator(allocator);

	for (auto& i : instructions) {
		i.replaceAllocator(allocator);
	}
}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(
	FnObject *fnObject,
	peff::Alloc *selfAllocator,
	NativeFnCallback callback)
	: FnOverloadingObject(
		  FnOverloadingKind::Native,
		  fnObject,
		  selfAllocator),
	  callback(callback) {}

SLAKE_API NativeFnOverloadingObject::NativeFnOverloadingObject(const NativeFnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut) : FnOverloadingObject(other, allocator, succeededOut) {
	if (succeededOut) {
		callback = other.callback;
	}
}

SLAKE_API NativeFnOverloadingObject::~NativeFnOverloadingObject() {
}

SLAKE_API FnOverloadingObject *slake::NativeFnOverloadingObject::duplicate(Duplicator *duplicator) const {
	return (FnOverloadingObject *)alloc(this).get();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(
	FnObject *fnObject,
	NativeFnCallback callback) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = fnObject->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<NativeFnOverloadingObject, util::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
		peff::allocAndConstruct<NativeFnOverloadingObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			fnObject, curGenerationAllocator.get(), callback));
	if (!ptr)
		return nullptr;

	if (!fnObject->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<NativeFnOverloadingObject> slake::NativeFnOverloadingObject::alloc(const NativeFnOverloadingObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<NativeFnOverloadingObject, util::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
		peff::allocAndConstruct<NativeFnOverloadingObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::NativeFnOverloadingObject::dealloc() {
	peff::destroyAndRelease<NativeFnOverloadingObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API FnObject::FnObject(Runtime *rt, peff::Alloc *selfAllocator) : MemberObject(rt, selfAllocator, ObjectKind::Fn), overloadings(selfAllocator) {
}

SLAKE_API FnObject::FnObject(const FnObject &x, peff::Alloc *allocator, bool &succeededOut) : MemberObject(x, allocator, succeededOut), overloadings(allocator) {
	if (succeededOut) {
		for (auto i : x.overloadings) {
			FnOverloadingObject *ol = (FnOverloadingObject *)i->duplicate(nullptr);

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

SLAKE_API InternalExceptionPointer FnObject::getOverloading(peff::Alloc *allocator, const peff::DynArray<TypeRef> &argTypes, FnOverloadingObject *&overloadingOut) const {
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
			if (argTypes.at(k) != j->paramTypes.at(k))
				goto mismatched;
		}

		overloadingOut = j;
		return {};

	mismatched:;
	}

	overloadingOut = nullptr;
	return {};
}

SLAKE_API Object *FnObject::duplicate(Duplicator *duplicator) const {
	SLAKE_REFERENCED_PARAM(duplicator);

	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<FnObject, util::DeallocableDeleter<FnObject>> ptr(
		peff::allocAndConstruct<FnObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			rt, curGenerationAllocator.get()));

	if (!rt->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(const FnObject *other) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = other->associatedRuntime->getCurGenAlloc();

	bool succeeded = true;

	std::unique_ptr<FnObject, util::DeallocableDeleter<FnObject>> ptr(
		peff::allocAndConstruct<FnObject>(
			curGenerationAllocator.get(),
			sizeof(std::max_align_t),
			*other, curGenerationAllocator.get(), succeeded));
	if (!ptr)
		return nullptr;

	if (!succeeded)
		return nullptr;

	if (!other->associatedRuntime->addObject(ptr.get()))
		return nullptr;

	return ptr.release();
}

SLAKE_API void slake::FnObject::dealloc() {
	peff::destroyAndRelease<FnObject>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API void FnObject::replaceAllocator(peff::Alloc* allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	overloadings.replaceAllocator(allocator);
}

SLAKE_API InternalExceptionPointer slake::findOverloading(
	peff::Alloc *allocator,
	FnObject *fnObject,
	const peff::DynArray<TypeRef> &paramTypes,
	const GenericParamList &genericParams,
	bool hasVarArg,
	FnOverloadingObject *&overloadingOut) {
	for (auto i : fnObject->overloadings) {
		bool result;

		SLAKE_RETURN_IF_EXCEPT(isDuplicatedOverloading(allocator, i, paramTypes, genericParams, hasVarArg, result));

		if (result) {
			overloadingOut = i;
			return {};
		}
	}

	overloadingOut = nullptr;
	return {};
}

SLAKE_API InternalExceptionPointer slake::isDuplicatedOverloading(
	peff::Alloc *allocator,
	const FnOverloadingObject *overloading,
	const peff::DynArray<TypeRef> &paramTypes,
	const GenericParamList &genericParams,
	bool hasVarArg,
	bool &resultOut) {
	if ((overloading->overloadingFlags & OL_VARG) != (hasVarArg ? OL_VARG : 0)) {
		resultOut = false;
		return {};
	}

	if (overloading->paramTypes.size() != paramTypes.size()) {
		resultOut = false;
		return {};
	}

	if (overloading->genericParams.size() != genericParams.size()) {
		resultOut = false;
		return {};
	}

	for (size_t j = 0; j < paramTypes.size(); ++j) {
		if (overloading->paramTypes.at(j) != paramTypes.at(j)) {
			resultOut = false;
			return {};
		}
	}

	resultOut = true;

	return {};
}
