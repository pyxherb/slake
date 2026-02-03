#include <slake/runtime.h>
#include <peff/base/scope_guard.h>

using namespace slake;

SLAKE_API Instruction::Instruction()
	: opcode((Opcode)0xff),
	  nOperands(0),
	  output(UINT32_MAX),
	  operands(nullptr),
	  operandsAllocator(nullptr) {
}

SLAKE_API Instruction::Instruction(Instruction &&rhs)
	: offSourceLocDesc(rhs.offSourceLocDesc),
	  opcode(rhs.opcode),
	  nOperands(rhs.nOperands),
	  output(rhs.output),
	  operands(rhs.operands),
	  operandsAllocator(rhs.operandsAllocator) {
	rhs.offSourceLocDesc = SIZE_MAX;
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
	  mappedGenericParams(selfAllocator),
	  mappedGenericArgs(selfAllocator),
	  paramTypes(selfAllocator),
	  returnType({ TypeId::Void }) {
}

SLAKE_API FnOverloadingObject::FnOverloadingObject(const FnOverloadingObject &other, peff::Alloc *allocator, bool &succeededOut)
	: Object(other, allocator),
	  genericParams(allocator),
	  mappedGenericParams(allocator),  // No need to copy
	  mappedGenericArgs(allocator),	   // No need to copy
	  paramTypes(allocator) {
	fnObject = other.fnObject;

	access = other.access;

	if (!genericParams.resizeUninitialized(other.genericParams.size())) {
		succeededOut = false;
		return;
	}
	for (size_t i = 0; i < other.genericParams.size(); ++i) {
		if (!other.genericParams.at(i).copy(genericParams.at(i))) {
			for (size_t j = i; j; --j) {
				peff::destroyAt<GenericParam>(&genericParams.at(j - 1));
			}
			succeededOut = false;
			return;
		}
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

	if (!paramTypes.resize(other.paramTypes.size())) {
		succeededOut = false;
		return;
	}
	memcpy(paramTypes.data(), other.paramTypes.data(), paramTypes.size() * sizeof(TypeRef));
	returnType = other.returnType;

	overloadingFlags = other.overloadingFlags;
	overloadingKind = other.overloadingKind;
	overridenType = other.overridenType;
}

SLAKE_API FnOverloadingObject::~FnOverloadingObject() {
}

SLAKE_API void FnOverloadingObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->Object::replaceAllocator(allocator);

	genericParams.replaceAllocator(allocator);

	for (auto &i : genericParams) {
		i.replaceAllocator(allocator);
	}

	mappedGenericParams.replaceAllocator(allocator);
	mappedGenericArgs.replaceAllocator(allocator);

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
		if (!sourceLocDescs.resize(other.sourceLocDescs.size())) {
			succeededOut = false;
			return;
		}
		memcpy(sourceLocDescs.data(), other.sourceLocDescs.data(), sourceLocDescs.size() * sizeof(slxfmt::SourceLocDesc));

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

				if (operand.valueType == ValueType::Reference) {
					const Reference &entityRef = operand.getReference();
					switch (entityRef.kind) {
						case ReferenceKind::ObjectRef:
							if (entityRef.asObject)
								curIns.operands[j] = Reference(entityRef.asObject->duplicate(nullptr));
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

SLAKE_API Object *slake::RegularFnOverloadingObject::duplicate(Duplicator *duplicator) const {
	return alloc(this).get();
}

SLAKE_API HostObjectRef<RegularFnOverloadingObject> slake::RegularFnOverloadingObject::alloc(
	FnObject *fnObject) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = fnObject->associatedRuntime->getCurGenAlloc();

	std::unique_ptr<RegularFnOverloadingObject, peff::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
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

	std::unique_ptr<RegularFnOverloadingObject, peff::DeallocableDeleter<RegularFnOverloadingObject>> ptr(
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

	for (auto &i : instructions) {
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

	std::unique_ptr<NativeFnOverloadingObject, peff::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
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

	std::unique_ptr<NativeFnOverloadingObject, peff::DeallocableDeleter<NativeFnOverloadingObject>> ptr(
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

SLAKE_API int FnSignatureComparator::operator()(const FnSignature &lhs, const FnSignature &rhs) const noexcept {
	int result = innerComparator(lhs.paramTypes, rhs.paramTypes);
	if (result)
		return result;

	if (((int)lhs.hasVarArg) < ((int)rhs.hasVarArg))
		return -1;
	if (((int)rhs.hasVarArg) > ((int)rhs.hasVarArg))
		return 1;

	if (lhs.nGenericParams < rhs.nGenericParams) {
		return -1;
	}
	if (lhs.nGenericParams > rhs.nGenericParams) {
		return 1;
	}

	if (lhs.overridenType < rhs.overridenType) {
		return -1;
	}
	if (lhs.overridenType > rhs.overridenType) {
		return 1;
	}

	return 0;
}

SLAKE_API FnObject::FnObject(Runtime *rt, peff::Alloc *selfAllocator) : MemberObject(rt, selfAllocator, ObjectKind::Fn), overloadings(selfAllocator) {
}

SLAKE_API FnObject::FnObject(const FnObject &x, peff::Alloc *allocator, bool &succeededOut) : MemberObject(x, allocator, succeededOut), overloadings(allocator) {
	if (succeededOut) {
		for (auto [k, v] : x.overloadings) {
			FnOverloadingObject *ol = (FnOverloadingObject *)v->duplicate(nullptr);

			if (!ol) {
				succeededOut = false;
				return;
			}

			ol->fnObject = this;

			if (!overloadings.insert({ ol->paramTypes, ol->isWithVarArgs(), ol->genericParams.size(), ol->overridenType }, +ol)) {
				succeededOut = false;
				return;
			}
		}
	}
}

SLAKE_API FnObject::~FnObject() {
}

SLAKE_API Object *FnObject::duplicate(Duplicator *duplicator) const {
	SLAKE_REFERENCED_PARAM(duplicator);

	return (Object *)alloc(this).get();
}

SLAKE_API HostObjectRef<FnObject> slake::FnObject::alloc(Runtime *rt) {
	peff::RcObjectPtr<peff::Alloc> curGenerationAllocator = rt->getCurGenAlloc();

	std::unique_ptr<FnObject, peff::DeallocableDeleter<FnObject>> ptr(
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

	std::unique_ptr<FnObject, peff::DeallocableDeleter<FnObject>> ptr(
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

SLAKE_API void FnObject::replaceAllocator(peff::Alloc *allocator) noexcept {
	this->MemberObject::replaceAllocator(allocator);

	overloadings.replaceAllocator(allocator);
}

SLAKE_API InternalExceptionPointer FnObject::resortOverloadings() noexcept {
	// Resort the overloading map.
	// TODO: Can we check if any one of the overloadings is changed to
	// implement on-demand resorting?
	auto oldOverloadings = std::move(overloadings);

	overloadings = peff::Map<FnSignature, FnOverloadingObject *, FnSignatureComparator, true>(selfAllocator.get());

	for (auto [k, v] : oldOverloadings) {
		if (!overloadings.insert(FnSignature(k), +v))
			return OutOfMemoryError::alloc();
	}

	return {};
}
