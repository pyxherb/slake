#include "../runtime.h"
#include <slake/flib/math/fmod.h>
#include <slake/flib/bitop.h>
#include <slake/flib/cmp.h>
#include <peff/base/scope_guard.h>
#include <cmath>

#undef new

using namespace slake;

template <bool hasOutput, size_t nOperands>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandCount(
	Runtime *runtime,
	size_t output,
	size_t nOperandsIn) noexcept {
	if constexpr (hasOutput) {
		if ((output == UINT32_MAX) || (nOperandsIn != nOperandsIn))
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	} else {
		if (nOperands != nOperandsIn) {
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
	}

	return {};
}

template <ValueType valueType>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkOperandType(
	Runtime *runtime,
	const Value &operand) noexcept {
	if (operand.valueType != valueType) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

template <ReferenceKind kind>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectRefOperandType(
	Runtime *runtime,
	const Reference &operand) noexcept {
	if (operand.kind != kind) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

template <ObjectKind typeId>
[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _checkObjectOperandType(
	Runtime *runtime,
	Object *object) noexcept {
	if (object && object->getObjectKind() != typeId) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE bool _isRegisterValid(
	size_t stackSize,
	const MajorFrame *curMajorFrame,
	uint32_t index) noexcept {
	return index < curMajorFrame->resumable->nRegs;
}

static SLAKE_FORCEINLINE Value *_calcRegPtr(
	char *stackData,
	size_t stackSize,
	const MajorFrame *curMajorFrame,
	uint32_t index) noexcept {
	return (Value *)calcStackAddr(stackData, stackSize, curMajorFrame->offRegs + sizeof(Value) * (index + 1));
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _setRegisterValue(
	Runtime *runtime,
	char *stackData,
	const size_t stackSize,
	const MajorFrame *curMajorFrame,
	uint32_t index,
	const Value &value) noexcept {
	if (!_isRegisterValid(stackSize, curMajorFrame, index)) {
		// The register does not present.
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
	}
	*_calcRegPtr(stackData, stackSize, curMajorFrame, index) = value;
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _unwrapRegOperand(
	Runtime *runtime,
	const char *stackData,
	size_t stackSize,
	const MajorFrame *curMajorFrame,
	const Value &value,
	Value &valueOut) noexcept {
	if (value.valueType == ValueType::RegIndex) {
		uint32_t index = value.getRegIndex();
		if (index >= curMajorFrame->resumable->nRegs) {
			// The register does not present.
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
		valueOut = *(Value *)calcStackAddr(stackData, stackSize, curMajorFrame->offRegs + sizeof(Value) * (index + 1));
		return {};
	}
	valueOut = value;
	return {};
}

[[nodiscard]] static SLAKE_FORCEINLINE InternalExceptionPointer _unwrapRegOperandIntoPtr(
	Runtime *runtime,
	const char *stackData,
	size_t stackSize,
	const MajorFrame *curMajorFrame,
	const Value &value,
	const Value *&valueOut) noexcept {
	if (value.valueType == ValueType::RegIndex) {
		uint32_t index = value.getRegIndex();
		if (index >= curMajorFrame->resumable->nRegs) {
			// The register does not present.
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(runtime->getFixedAlloc()));
		}
		valueOut = (Value *)calcStackAddr(stackData, stackSize, curMajorFrame->offRegs + sizeof(Value) * (index + 1));
		return {};
	}
	valueOut = &value;
	return {};
}

template <typename LT>
static void _castToLiteralValue(const Value &x, Value &valueOut) noexcept {
	switch (x.valueType) {
		case ValueType::I8:
			valueOut = ((LT)(x.getI8()));
			break;
		case ValueType::I16:
			valueOut = ((LT)(x.getI16()));
			break;
		case ValueType::I32:
			valueOut = ((LT)(x.getI32()));
			break;
		case ValueType::I64:
			valueOut = ((LT)(x.getI64()));
			break;
		case ValueType::ISize:
			valueOut = ((LT)(x.getISize()));
			break;
		case ValueType::U8:
			valueOut = ((LT)(x.getU8()));
			break;
		case ValueType::U16:
			valueOut = ((LT)(x.getU16()));
			break;
		case ValueType::U32:
			valueOut = ((LT)(x.getU32()));
			break;
		case ValueType::U64:
			valueOut = ((LT)(x.getU64()));
			break;
		case ValueType::USize:
			valueOut = ((LT)(x.getUSize()));
			break;
		case ValueType::F32:
			valueOut = ((LT)(x.getF32()));
			break;
		case ValueType::F64:
			valueOut = ((LT)(x.getF64()));
			break;
		case ValueType::Bool:
			valueOut = ((LT)(x.getBool()));
			break;
		default:
			std::terminate();
	}
}

SLAKE_API InternalExceptionPointer Runtime::_fillArgs(
	MajorFrame *newMajorFrame,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	HostRefHolder &holder) {
	if (nArgs < fn->paramTypes.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidArgumentNumberError::alloc(getFixedAlloc(), nArgs));
	}

	if (!newMajorFrame->resumable->argStack.resize(fn->paramTypes.size()))
		return OutOfMemoryError::alloc();
	for (size_t i = 0; i < fn->paramTypes.size(); ++i) {
		newMajorFrame->resumable->argStack.at(i) = { Value(), fn->paramTypes.at(i) };
		SLAKE_RETURN_IF_EXCEPT(writeVar(Reference::makeArgRef(newMajorFrame, i), args[i]));
	}

	if (fn->isWithVarArgs()) {
		auto elementHeapTypeObject = HeapTypeObject::alloc(this);

		if (!elementHeapTypeObject)
			return OutOfMemoryError::alloc();

		elementHeapTypeObject->typeRef = TypeRef(TypeId::Any);

		if (!holder.addObject(elementHeapTypeObject.get()))
			return OutOfMemoryError::alloc();

		auto arrayTypeDefObject = ArrayTypeDefObject::alloc(this);

		if (!arrayTypeDefObject)
			return OutOfMemoryError::alloc();

		arrayTypeDefObject->elementType = elementHeapTypeObject.get();

		if (!holder.addObject(arrayTypeDefObject.get()))
			return OutOfMemoryError::alloc();

		size_t szVarArgArray = nArgs - fn->paramTypes.size();
		auto varArgArrayObject = newArrayInstance(this, TypeRef(TypeId::Any), szVarArgArray);
		if (!holder.addObject(varArgArrayObject.get()))
			return OutOfMemoryError::alloc();

		for (size_t i = 0; i < szVarArgArray; ++i) {
			((Value *)varArgArrayObject->data)[i] = args[fn->paramTypes.size() + i];
		}

		if (!newMajorFrame->resumable->argStack.pushBack({ Reference::makeObjectRef(varArgArrayObject.get()), TypeRef(TypeId::Array, arrayTypeDefObject.get()) }))
			return OutOfMemoryError::alloc();
	}

	return {};
}

SLAKE_API InternalExceptionPointer Runtime::_createNewCoroutineMajorFrame(
	Context *context,
	CoroutineObject *coroutine,
	uint32_t returnValueOut) noexcept {
	HostRefHolder holder(context->runtime->getFixedAlloc());

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop, coroutine]() noexcept {
		context->stackTop = prevStackTop;
		coroutine->offStackTop = 0;
	});
	MajorFramePtr newMajorFrame(MajorFrame::alloc(this, context));
	if (!newMajorFrame) {
		return OutOfMemoryError::alloc();
	}

	if (!(newMajorFrame->resumable = coroutine->resumable))
		return OutOfMemoryError::alloc();

	newMajorFrame->curFn = coroutine->overloading;
	newMajorFrame->curCoroutine = coroutine;

	coroutine->bindToContext(context, newMajorFrame.get());

	if (coroutine->stackData) {
		size_t align = alignof(std::max_align_t);
		if (size_t diff = align - (uintptr_t)(calcStackAddr(context->dataStack, context->stackSize, context->stackTop)) % align; (diff != align) && diff) {
			if (!context->stackAlloc(align - diff))
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
		newMajorFrame->offRegs = context->stackTop;
		size_t stackOffset = context->stackTop;
		char *initialData = context->stackAlloc(coroutine->lenStackData);
		if (!initialData) {
			return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
		coroutine->offStackTop = stackOffset;
		memcpy(initialData, coroutine->stackData, coroutine->lenStackData);
		coroutine->releaseStackData();
	} else {
		switch (coroutine->overloading->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)coroutine->overloading;
				coroutine->resumable->nRegs = (newMajorFrame->resumable->nRegs = ol->nRegisters);
				newMajorFrame->offRegs = context->stackTop;
				Value *regs = (Value *)context->stackAlloc(sizeof(Value) * ol->nRegisters);
				if (!regs)
					return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
				for (size_t i = 0; i < ol->nRegisters; ++i)
					regs[i] = Value(ValueType::Undefined);
				break;
			}
			default:;
		}
		coroutine->offStackTop = prevStackTop;
	}

	newMajorFrame->resumable = coroutine->resumable;

	newMajorFrame->returnValueOutReg = returnValueOut;

	newMajorFrame->stackBase = prevStackTop;
	if (!context->majorFrames.pushBack(std::move(newMajorFrame))) {
		return OutOfMemoryError::alloc();
	}

	restoreStackTopGuard.release();
	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_createNewMajorFrame(
	Context *context,
	Object *thisObject,
	const FnOverloadingObject *fn,
	const Value *args,
	uint32_t nArgs,
	uint32_t returnValueOut) noexcept {
	HostRefHolder holder(getFixedAlloc());

	size_t prevStackTop = context->stackTop;
	peff::ScopeGuard restoreStackTopGuard([context, prevStackTop]() noexcept {
		context->stackTop = prevStackTop;
	});
	MajorFramePtr newMajorFrame(MajorFrame::alloc(this, context));
	if (!newMajorFrame) {
		return OutOfMemoryError::alloc();
	}

	if (!(newMajorFrame->resumable = ResumableObject::alloc(this)))
		return OutOfMemoryError::alloc();

	if (!newMajorFrame->resumable->minorFrames.pushBack(MinorFrame(this, context->selfAllocator.get(), 0)))
		return OutOfMemoryError::alloc();

	if (!fn) {
		// Used in the creation of top major frame.
		newMajorFrame->curFn = nullptr;
		newMajorFrame->resumable->nRegs = 1;
		newMajorFrame->offRegs = context->stackTop;
		Value *regs = (Value *)context->stackAlloc(sizeof(Value) * 1);
		*regs = Value(ValueType::Undefined);
	} else {
		newMajorFrame->curFn = fn;
		newMajorFrame->resumable->thisObject = thisObject;

		SLAKE_RETURN_IF_EXCEPT(_fillArgs(newMajorFrame.get(), fn, args, nArgs, holder));

		switch (fn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)fn;
				newMajorFrame->resumable->nRegs = ol->nRegisters;
				newMajorFrame->offRegs = context->stackTop;
				Value *regs = (Value *)context->stackAlloc(sizeof(Value) * ol->nRegisters);
				for (size_t i = 0; i < ol->nRegisters; ++i)
					regs[i] = Value(ValueType::Undefined);
				break;
			}
			default:;
		}
	}

	newMajorFrame->returnValueOutReg = returnValueOut;
	newMajorFrame->stackBase = prevStackTop;
	if (!context->majorFrames.pushBack(std::move(newMajorFrame))) {
		return OutOfMemoryError::alloc();
	}

	restoreStackTopGuard.release();

	return {};
}

SLAKE_API InternalExceptionPointer slake::Runtime::_addLocalVar(Context *context, const MajorFrame *frame, TypeRef type, Reference &objectRefOut) noexcept {
	size_t stackOffset;

	switch (type.typeId) {
		case TypeId::StructInstance: {
			assert(type.getCustomTypeDef()->typeObject->getObjectKind() == ObjectKind::Struct);
			SLAKE_RETURN_IF_EXCEPT(prepareStructForInstantiation((StructObject *)type.getCustomTypeDef()->typeObject));
			break;
		}
		default:
			break;
	}

	size_t size = sizeofType(type), align = alignofType(type);

	if (align > 1) {
		if (size_t diff = align - (uintptr_t)(calcStackAddr(context->dataStack, context->stackSize, context->stackTop)) % align; (diff != align) && diff) {
			if (!context->stackAlloc(align - diff))
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
		}
	}

	if (!context->stackAlloc(size))
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));

	switch (type.typeId) {
		case TypeId::I8:
		case TypeId::I16:
		case TypeId::I32:
		case TypeId::I64:
		case TypeId::U8:
		case TypeId::U16:
		case TypeId::U32:
		case TypeId::U64:
		case TypeId::F32:
		case TypeId::F64:
		case TypeId::Bool:
		case TypeId::String:
		case TypeId::Any:
			break;
		case TypeId::Instance:
		case TypeId::GenericArg:
		case TypeId::Array:
		case TypeId::Ref: {
			// The data is already aligned, just directly assign to them.
			Object **typeInfo = (Object **)context->stackAlloc(sizeof(void *));
			if (!typeInfo)
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
#ifndef _NDEBUG
			const size_t diff = alignof(void *) - ((uintptr_t)(calcStackAddr(context->dataStack, context->stackSize, context->stackTop)) % alignof(void *));
			assert((diff == alignof(void *) || (!diff)));
#endif
			*typeInfo = type.typeDef;
			break;
		}
		case TypeId::StructInstance: {
			TypeDefObject **typeInfo = (TypeDefObject **)context->stackAlloc(sizeof(void *));
			if (!typeInfo)
				return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
			memcpy(typeInfo, &type.typeDef, sizeof(void *));
			break;
		}
		default:
			std::terminate();
	}

	TypeModifier *typeModifier = (TypeModifier *)context->stackAlloc(sizeof(TypeModifier));
	if (!typeModifier)
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	*typeModifier = type.typeModifier;

	TypeId *typeId = (TypeId *)context->stackAlloc(sizeof(TypeId));
	if (!typeId)
		return allocOutOfMemoryErrorIfAllocFailed(StackOverflowError::alloc(getFixedAlloc()));
	*typeId = type.typeId;

	stackOffset = context->stackTop;

	if (frame->curCoroutine) {
		objectRefOut = Reference::makeCoroutineLocalVarRef(frame->curCoroutine, stackOffset - frame->curCoroutine->offStackTop);
	} else {
		objectRefOut = Reference::makeLocalVarRef(context, stackOffset);
	}
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer larg(MajorFrame *majorFrame, Runtime *rt, uint32_t off, Reference &objectRefOut) {
	if (off >= majorFrame->resumable->argStack.size()) {
		return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(rt->getFixedAlloc()));
	}

	if (majorFrame->curCoroutine) {
		objectRefOut = Reference::makeCoroutineArgRef(majorFrame->curCoroutine, off);
	} else {
		objectRefOut = Reference::makeArgRef(majorFrame, off);
	}
	return {};
}

SLAKE_FORCEINLINE InternalExceptionPointer Runtime::_execIns(ContextObject *const context, MajorFrame *const curMajorFrame, const Opcode opcode, const size_t output, const size_t nOperands, const Value *const operands, bool &isContextChangedOut) noexcept {
	InternalExceptionPointer exceptPtr;
	char *const dataStack = context->_context.dataStack;
	const size_t stackSize = context->_context.stackSize;

	switch (opcode) {
		case Opcode::LOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, operands[0]));
			auto refPtr = operands[0].getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, refPtr));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::IdRef>(this, refPtr.asObject));

			Reference entityRef;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef((IdRefObject *)refPtr.asObject, entityRef));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(entityRef)));
			break;
		}
		case Opcode::RLOAD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::RegIndex>(this, operands[0]));

			Value lhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], lhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, lhs));

			Value rhs;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], rhs));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, rhs));

			auto &lhsEntityRef = lhs.getReference();

			auto &idRefEntityRef = rhs.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, idRefEntityRef));

			if (!lhsEntityRef) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (!idRefEntityRef) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			if (idRefEntityRef.asObject->getObjectKind() != ObjectKind::IdRef) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			IdRefObject *idRef = (IdRefObject *)idRefEntityRef.asObject;

			Reference entityRef;

			switch (lhsEntityRef.kind) {
				case ReferenceKind::StructRef: {
					StructObject *structObject = (StructObject *)(((CustomTypeDefObject *)typeofVar(lhsEntityRef).typeDef)->typeObject);
					IdRefEntry &curName = idRef->entries.at(0);

					if (auto it = structObject->cachedObjectLayout->fieldNameMap.find(curName.name); it != structObject->cachedObjectLayout->fieldNameMap.end()) {
						entityRef = Reference::makeStructFieldRef(lhsEntityRef.asStruct, it.value());
					} else {
						entityRef = structObject->getMember(curName.name);

						if (curName.genericArgs.size()) {
							peff::NullAlloc nullAlloc;
							GenericInstantiationContext genericInstantiationContext(&nullAlloc, getFixedAlloc());

							genericInstantiationContext.genericArgs = &curName.genericArgs;
							MemberObject *m;
							SLAKE_RETURN_IF_EXCEPT(instantiateGenericObject((MemberObject *)entityRef.asObject, m, &genericInstantiationContext));
							entityRef = Reference::makeObjectRef(m);
						}
					}

					break;
				}
				case ReferenceKind::ObjectRef:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, resolveIdRef(idRef, entityRef, lhsEntityRef.asObject));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (!entityRef) {
				std::terminate();
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(entityRef)));
			break;
		}
		case Opcode::STORE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

			Value destValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], destValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, destValue));

			Value data;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], data));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, writeVar(destValue.getReference(), data));
			break;
		}
		case Opcode::MOV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			const Value *value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], value));

			if (output != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, *value));
			}
			break;
		}
		case Opcode::LARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[0]));

			Reference entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, larg(curMajorFrame, this, operands[0].getU32(), entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(entityRef)));
			break;
		}
		case Opcode::LAPARG:
			return InvalidOpcodeError::alloc(getFixedAlloc(), opcode);
		case Opcode::LVAR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));

			TypeRef type = operands[0].getTypeName();

			Reference entityRef;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _addLocalVar(&context->_context, curMajorFrame, type, entityRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, entityRef));
			break;
		}
		case Opcode::ALLOCA:
			return InvalidOpcodeError::alloc(getFixedAlloc(), opcode);
		case Opcode::LVALUE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			const Value *dest;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], dest));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, *dest));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				// The register does not present.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			readVar(dest->getReference(), *_calcRegPtr(dataStack, stackSize, curMajorFrame, output));

			break;
		}
		case Opcode::ENTER: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 0>(this, output, nOperands));
			MinorFrame frame(
				this,
				context->selfAllocator.get(),
				context->_context.stackTop - curMajorFrame->stackBase);

			if (!curMajorFrame->resumable->minorFrames.pushBack(std::move(frame)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::LEAVE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[0]));
			uint32_t level = operands[0].getU32();
			if (curMajorFrame->resumable->minorFrames.size() < level + 1) {
				return allocOutOfMemoryErrorIfAllocFailed(FrameBoundaryExceededError::alloc(getFixedAlloc()));
			}
			for (uint32_t i = 0; i < level; ++i) {
				size_t stackTop = curMajorFrame->resumable->minorFrames.back().stackBase;
				if (!curMajorFrame->resumable->minorFrames.popBack())
					return OutOfMemoryError::alloc();
				context->_context.stackTop = curMajorFrame->stackBase + stackTop;
			}
			break;
		}
		case Opcode::ADD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() + y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() + y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() + y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() + y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() + y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() + y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() + y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() + y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() + y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() + y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			break;
		}
		case Opcode::SUB: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() - y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() - y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() - y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() - y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() - y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() - y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() - y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() - y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() - y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() - y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::MUL: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() * y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() * y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() * y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() * y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() * y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() * y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() * y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() * y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() * y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() * y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::DIV: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() / y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() / y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() / y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() / y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() / y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() / y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() / y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() / y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(x->getF32() / y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(x->getF64() / y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::MOD: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() % y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() % y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() % y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() % y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() % y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() % y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() % y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() % y->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)flib::fmodf(x->getF32(), y->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)flib::fmod(x->getF64(), y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::AND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() & y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() & y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() & y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() & y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() & y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() & y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() & y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() & y->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::OR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() | y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() | y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() | y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() | y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() | y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() | y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() | y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() | y->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::XOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(x->getI8() ^ y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(x->getI16() ^ y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(x->getI32() ^ y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(x->getI64() ^ y->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8() ^ y->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16() ^ y->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32() ^ y->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64() ^ y->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LAND: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x->getBool() && y->getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LOR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::Bool:
					valueOut = Value((int16_t)(x->getBool() || y->getBool()));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::EQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() == y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() == y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() == y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() == y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() == y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() == y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() == y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() == y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() == y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() == y->getF64());
					break;
				case ValueType::Bool:
					valueOut = (bool)(x->getBool() == y->getBool());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::NEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() != y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() != y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() != y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() != y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() != y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() != y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() != y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() != y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() != y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() != y->getF64());
					break;
				case ValueType::Bool:
					valueOut = (bool)(x->getBool() != y->getBool());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() < y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() < y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() < y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() < y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() < y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() < y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() < y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() < y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() < y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() < y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::GT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() > y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() > y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() > y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() > y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() > y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() > y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() > y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() > y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() > y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() > y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() <= y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() <= y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() <= y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() <= y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() <= y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() <= y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() <= y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() <= y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() <= y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() <= y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::GTEQ: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(x->getI8() >= y->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(x->getI16() >= y->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(x->getI32() >= y->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(x->getI64() >= y->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(x->getU8() >= y->getU8());
					break;
				case ValueType::U16:
					valueOut = (bool)(x->getU16() >= y->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(x->getU32() >= y->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(x->getU64() >= y->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(x->getF32() >= y->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(x->getF64() >= y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			uint64_t lhs = x->getU64(), rhs = y->getU64();
			if (lhs > rhs) {
				valueOut = Value((int32_t)1);
			} else if (lhs < rhs) {
				valueOut = Value((int32_t)-1);
			} else
				valueOut = Value((int32_t)0);
			break;
		}
		case Opcode::CMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));
			if (x->valueType != y->valueType) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int32_t)flib::compareI8(x->getI8(), y->getI8());
					break;
				case ValueType::I16:
					valueOut = (int32_t)flib::compareI16(x->getI16(), y->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)flib::compareI32(x->getI32(), y->getI32());
					break;
				case ValueType::I64:
					valueOut = (int32_t)flib::compareI64(x->getI64(), y->getI64());
					break;
				case ValueType::U8:
					valueOut = (int32_t)flib::compareU8(x->getU8(), y->getU8());
					break;
				case ValueType::U16:
					valueOut = (int32_t)flib::compareU16(x->getU16(), y->getU16());
					break;
				case ValueType::U32:
					valueOut = (int32_t)flib::compareU32(x->getU32(), y->getU32());
					break;
				case ValueType::U64:
					valueOut = (int32_t)flib::compareU64(x->getU64(), y->getU64());
					break;
				case ValueType::F32:
					valueOut = (int32_t)flib::compareF32(x->getF32(), y->getF32());
					break;
				case ValueType::F64:
					valueOut = (int32_t)flib::compareF64(x->getF64(), y->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			break;
		}
		case Opcode::LSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, y));

			uint32_t rhs = y->getU32();

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = flib::shlSigned8(x->getI8(), rhs);
					break;
				case ValueType::I16:
					valueOut = flib::shlSigned16(x->getI16(), rhs);
					break;
				case ValueType::I32:
					valueOut = flib::shlSigned32(x->getI32(), rhs);
					break;
				case ValueType::I64:
					valueOut = flib::shlSigned64(x->getI64(), rhs);
					break;
				case ValueType::U8:
					valueOut = flib::shlUnsigned8(x->getU8(), rhs);
					break;
				case ValueType::U16:
					valueOut = flib::shlUnsigned16(x->getU16(), rhs);
					break;
				case ValueType::U32:
					valueOut = flib::shlUnsigned32(x->getU32(), rhs);
					break;
				case ValueType::U64:
					valueOut = flib::shlUnsigned64(x->getU64(), rhs);
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::RSH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x, *y;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], x));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], y));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, y));

			uint32_t rhs = y->getU32();

			switch (x->valueType) {
				case ValueType::I8:
					valueOut = flib::shrSigned8(x->getI8(), rhs);
					break;
				case ValueType::I16:
					valueOut = flib::shrSigned16(x->getI16(), rhs);
					break;
				case ValueType::I32:
					valueOut = flib::shrSigned32(x->getI32(), rhs);
					break;
				case ValueType::I64:
					valueOut = flib::shrSigned64(x->getI64(), rhs);
					break;
				case ValueType::U8:
					valueOut = flib::shrUnsigned8(x->getU8(), rhs);
					break;
				case ValueType::U16:
					valueOut = flib::shrUnsigned16(x->getU16(), rhs);
					break;
				case ValueType::U32:
					valueOut = flib::shrUnsigned32(x->getU32(), rhs);
					break;
				case ValueType::U64:
					valueOut = flib::shrUnsigned64(x->getU64(), rhs);
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::NOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], x));
			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(~x->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(~x->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(~x->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(~x->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(~x->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(~x->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(~x->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(~x->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::LNOT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], x));
			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (bool)(!x->getI8());
					break;
				case ValueType::I16:
					valueOut = (bool)(!x->getI16());
					break;
				case ValueType::I32:
					valueOut = (bool)(!x->getI32());
					break;
				case ValueType::I64:
					valueOut = (bool)(!x->getI64());
					break;
				case ValueType::U8:
					valueOut = (bool)(!x->getI8());
					break;
				case ValueType::U16:
					valueOut = (bool)(!x->getU16());
					break;
				case ValueType::U32:
					valueOut = (bool)(!x->getU32());
					break;
				case ValueType::U64:
					valueOut = (bool)(!x->getU64());
					break;
				case ValueType::F32:
					valueOut = (bool)(!x->getF32());
					break;
				case ValueType::F64:
					valueOut = (bool)(!x->getF64());
					break;
				case ValueType::Bool:
					valueOut = (bool)(!x->getU64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::NEG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			const Value *x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[1], x));
			switch (x->valueType) {
				case ValueType::I8:
					valueOut = (int8_t)(-x->getI8());
					break;
				case ValueType::I16:
					valueOut = (int16_t)(-x->getI16());
					break;
				case ValueType::I32:
					valueOut = (int32_t)(-x->getI32());
					break;
				case ValueType::I64:
					valueOut = (int64_t)(-x->getI64());
					break;
				case ValueType::U8:
					valueOut = (uint8_t)(x->getU8());
					break;
				case ValueType::U16:
					valueOut = (uint16_t)(x->getU16());
					break;
				case ValueType::U32:
					valueOut = (uint32_t)(x->getU32());
					break;
				case ValueType::U64:
					valueOut = (uint64_t)(x->getU64());
					break;
				case ValueType::F32:
					valueOut = (float)(-x->getF32());
					break;
				case ValueType::F64:
					valueOut = (double)(-x->getF64());
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::AT: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			Value arrayValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], arrayValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, arrayValue));

			Value index;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], index));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, index));

			auto arrayIn = arrayValue.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, arrayIn));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::Array>(this, arrayIn.asObject));
			ArrayObject *arrayObject = (ArrayObject *)arrayIn.asObject;

			uint32_t indexIn = index.getU32();

			if (indexIn > arrayObject->length) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidArrayIndexError::alloc(getFixedAlloc(), indexIn));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Value(Reference::makeArrayElementRef(arrayObject, indexIn))));

			break;
		}
		case Opcode::JMP: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[0]));

			curMajorFrame->resumable->lastJumpSrc = curMajorFrame->resumable->curIns;
			curMajorFrame->resumable->curIns = operands[0].getU32();
			return {};
		}
		case Opcode::BR: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 3>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[1]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[2]));
			const Value *condition;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperandIntoPtr(this, dataStack, stackSize, curMajorFrame, operands[0], condition));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Bool>(this, condition));

			curMajorFrame->resumable->lastJumpSrc = curMajorFrame->resumable->curIns;
			curMajorFrame->resumable->curIns = operands[condition->getBool() ? 1 : 2].getU32();
			return {};
		}
		case Opcode::PUSHARG: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

			Value value;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], value));
			if (!curMajorFrame->resumable->nextArgStack.pushBack(std::move(value)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::PUSHAP:
			std::terminate();
		case Opcode::CTORCALL:
		case Opcode::MCALL:
		case Opcode::CALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;
			uint32_t returnValueOutputReg = UINT32_MAX;

			if (output != UINT32_MAX) {
				returnValueOutputReg = output;
			}

			switch (opcode) {
				case Opcode::CTORCALL:
				case Opcode::MCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], thisObjectValue));
					const Reference &thisObjectRef = thisObjectValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, thisObjectValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, thisObjectRef));
					thisObject = thisObjectRef.asObject;
					break;
				}
				case Opcode::CALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _createNewMajorFrame(
															&context->_context,
															thisObject,
															fn,
															curMajorFrame->resumable->nextArgStack.data(),
															curMajorFrame->resumable->nextArgStack.size(),
															returnValueOutputReg));
			curMajorFrame->resumable->nextArgStack.clear();

			isContextChangedOut = true;
			break;
		}
		case Opcode::RET: {
			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;

			Value returnValue = Value(ValueType::Invalid);

			switch (nOperands) {
				case 0:
					break;
				case 1:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], returnValue));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (CoroutineObject *co = curMajorFrame->curCoroutine; co) {
				co->resumable = curMajorFrame->resumable;
				if (returnValue != ValueType::Invalid)
					co->finalResult = returnValue;
				co->setDone();
			}

			if (returnValue == ValueType::Invalid) {
				if (returnValueOutReg != UINT32_MAX)
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
				context->_context.leaveMajor();
			} else {
				context->_context.leaveMajor();
				if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, context->_context.majorFrames.back().get(), returnValueOutReg, returnValue));
				}
			}

			isContextChangedOut = true;
			return {};
		}
		case Opcode::COCALL:
		case Opcode::COMCALL: {
			FnOverloadingObject *fn;
			Object *thisObject = nullptr;
			uint32_t returnValueOutputReg = UINT32_MAX;

			if (output != UINT32_MAX) {
				returnValueOutputReg = output;
			}

			switch (opcode) {
				case Opcode::COMCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;

					Value thisObjectValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], thisObjectValue));
					const Reference &thisObjectRef = thisObjectValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, thisObjectValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, thisObjectRef));
					thisObject = thisObjectRef.asObject;
					break;
				}
				case Opcode::COCALL: {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

					Value fnValue;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
					const Reference &fnObjectRef = fnValue.getReference();
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
					fn = (FnOverloadingObject *)fnObjectRef.asObject;
					break;
				}
				default:
					std::terminate();
			}

			if (!fn) {
				return allocOutOfMemoryErrorIfAllocFailed(NullRefError::alloc(getFixedAlloc()));
			}

			HostObjectRef<CoroutineObject> co;

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, createCoroutineInstance(
															fn,
															thisObject,
															curMajorFrame->resumable->nextArgStack.data(),
															curMajorFrame->resumable->nextArgStack.size(),
															co));
			curMajorFrame->resumable->nextArgStack.clear();

			if (returnValueOutputReg != UINT32_MAX) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, returnValueOutputReg, Reference::makeObjectRef(co.get())));
			}
			break;
		}
		case Opcode::YIELD: {
			if (!curMajorFrame->curCoroutine) {
				// TODO: Return an exception,
				std::terminate();
			}

			uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
			Value returnValue = Value(ValueType::Invalid);

			switch (nOperands) {
				case 0:
					break;
				case 1:
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], returnValue));
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if (size_t szFrame = context->_context.stackTop - curMajorFrame->curCoroutine->offStackTop; szFrame) {
				char *p = curMajorFrame->curCoroutine->allocStackData(szFrame);
				if (!p) {
					return OutOfMemoryError::alloc();
				}
				memcpy(p, calcStackAddr(context->_context.dataStack, context->_context.stackSize, context->_context.stackTop), szFrame);
			}

			curMajorFrame->curCoroutine->unbindContext();
			++curMajorFrame->resumable->curIns;

			curMajorFrame->curCoroutine->resumable = curMajorFrame->resumable;

			if (returnValue == ValueType::Invalid) {
				if (returnValueOutReg != UINT32_MAX) {
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
				}
				context->_context.leaveMajor();
			} else {
				context->_context.leaveMajor();
				MajorFrame *prevFrame = context->_context.majorFrames.back().get();
				if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, prevFrame, returnValueOutReg, returnValue));
				}
			}

			isContextChangedOut = true;
			return {};
		}
		case Opcode::RESUME: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			HostObjectRef<CoroutineObject> co;

			Value fnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
			const Reference &fnObjectRef = fnValue.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
			co = (CoroutineObject *)fnObjectRef.asObject;

			if (co->isDone()) {
				if (output != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, co->finalResult));
				}
			} else {
				SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, co.get(), output));
			}

			isContextChangedOut = true;
			break;
		}
		case Opcode::CODONE: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));

			HostObjectRef<CoroutineObject> co;

			Value fnValue;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], fnValue));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::Reference>(this, fnValue));
			const Reference &fnObjectRef = fnValue.getReference();
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectRefOperandType<ReferenceKind::ObjectRef>(this, fnObjectRef));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkObjectOperandType<ObjectKind::FnOverloading>(this, fnObjectRef.asObject));
			co = (CoroutineObject *)fnObjectRef.asObject;

			SLAKE_RETURN_IF_EXCEPT(_setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, co->isDone()));
			break;
		}
		case Opcode::LTHIS: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 0>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Reference::makeObjectRef(curMajorFrame->resumable->thisObject)));
			break;
		}
		case Opcode::NEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 1>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));

			TypeRef type = operands[0].getTypeName();

			switch (type.typeId) {
				case TypeId::Instance: {
					ClassObject *cls = (ClassObject *)(type.getCustomTypeDef())->typeObject;
					HostObjectRef<InstanceObject> instance = newClassInstance(cls, 0);
					if (!instance)
						// TODO: Return more detail exceptions.
						return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Reference::makeObjectRef(instance.get())));
					break;
				}
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}
			break;
		}
		case Opcode::ARRNEW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[1]));

			TypeRef type = operands[0].getTypeName();
			uint32_t size = operands[1].getU32();

			auto instance = newArrayInstance(this, type, size);
			if (!instance)
				// TODO: Return more detailed exceptions.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, Reference::makeObjectRef(instance.get())));

			break;
		}
		case Opcode::THROW: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 1>(this, output, nOperands));

			Value x;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[0], x));

			size_t nUnwindedFrames = 0;
			for (auto it = context->_context.majorFrames.beginReversed(); it != context->_context.majorFrames.endReversed(); ++it) {
				MajorFrame *majorFrame = it->get();

				for (size_t j = majorFrame->resumable->minorFrames.size(); j; --j) {
					auto &minorFrame = majorFrame->resumable->minorFrames.at(j - 1);

					uint32_t off;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _findAndDispatchExceptHandler(majorFrame->curExcept, minorFrame, off));
					if (off != UINT32_MAX) {
						if (!majorFrame->resumable->minorFrames.resizeUninitialized(j))
							return OutOfMemoryError::alloc();
						// Do not increase the current instruction offset,
						// the offset has been set to offset to first instruction
						// of the exception handler.
						majorFrame->resumable->curIns = off;
						for (size_t j = 0; j < nUnwindedFrames; ++j) {
							context->_context.leaveMajor();
						}
						return {};
					}
				}
				++nUnwindedFrames;
			}

			curMajorFrame->curExcept = x;
			return allocOutOfMemoryErrorIfAllocFailed(UncaughtExceptionError::alloc(getFixedAlloc(), x));
		}
		case Opcode::PUSHEH: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<false, 2>(this, output, nOperands));

			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[1]));

			ExceptionHandler xh;

			TypeRef type = operands[0].getTypeName();

			xh.type = operands[0].getTypeName();
			xh.off = operands[1].getU32();

			auto &minorFrame = curMajorFrame->resumable->minorFrames.back();

			if (!minorFrame.exceptHandlers.pushBack(std::move(xh)))
				return OutOfMemoryError::alloc();
			break;
		}
		case Opcode::CAST: {
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandCount<true, 2>(this, output, nOperands));
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::TypeName>(this, operands[0]));
			if (!_isRegisterValid(stackSize, curMajorFrame, output)) {
				// The register does not present.
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			Value v;
			SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[1], v));

			auto t = operands[0].getTypeName();
			Value &valueOut = *_calcRegPtr(dataStack, stackSize, curMajorFrame, output);

			switch (t.typeId) {
				case TypeId::I8:
					_castToLiteralValue<int8_t>(v, valueOut);
					break;
				case TypeId::I16:
					_castToLiteralValue<int16_t>(v, valueOut);
					break;
				case TypeId::I32:
					_castToLiteralValue<int32_t>(v, valueOut);
					break;
				case TypeId::I64:
					_castToLiteralValue<int64_t>(v, valueOut);
					break;
				case TypeId::U8:
					_castToLiteralValue<uint8_t>(v, valueOut);
					break;
				case TypeId::U16:
					_castToLiteralValue<uint16_t>(v, valueOut);
					break;
				case TypeId::U32:
					_castToLiteralValue<uint32_t>(v, valueOut);
					break;
				case TypeId::U64:
					_castToLiteralValue<uint64_t>(v, valueOut);
					break;
				case TypeId::Bool:
					_castToLiteralValue<bool>(v, valueOut);
					break;
				case TypeId::F32:
					_castToLiteralValue<float>(v, valueOut);
					break;
				case TypeId::F64:
					_castToLiteralValue<double>(v, valueOut);
					break;
				case TypeId::Instance:
					valueOut = v;
					break;
				default:
					return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			break;
		}
		case Opcode::PHI: {
			if (output == UINT32_MAX) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			if ((nOperands < 2) || ((nOperands & 1))) {
				return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));
			}

			Value v;

			for (size_t i = 0; i < nOperands; i += 2) {
				SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _checkOperandType<ValueType::U32>(this, operands[i]));

				uint32_t off = operands[i].getU32();

				if (off == curMajorFrame->resumable->lastJumpSrc) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _unwrapRegOperand(this, dataStack, stackSize, curMajorFrame, operands[i + 1], v));

					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, dataStack, stackSize, curMajorFrame, output, v));

					goto succeeded;
				}
			}

			return allocOutOfMemoryErrorIfAllocFailed(InvalidOperandsError::alloc(getFixedAlloc()));

		succeeded:
			break;
		}
		default:
			return allocOutOfMemoryErrorIfAllocFailed(InvalidOpcodeError::alloc(getFixedAlloc(), opcode));
	}
	++curMajorFrame->resumable->curIns;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::execContext(ContextObject *context) noexcept {
	size_t initialMajorFrameDepth = context->_context.majorFrames.size();
	InternalExceptionPointer exceptPtr;
	ExecutionRunnable *const managedThread = managedThreadRunnables.at(currentThreadHandle());

	while (context->_context.majorFrames.size() >= initialMajorFrameDepth) {
		MajorFrame *const curMajorFrame = context->_context.majorFrames.back().get();
		const FnOverloadingObject *const curFn = curMajorFrame->curFn;

		if (!curFn) {
			break;
		}

		// Pause if the runtime is in GC
		/*while (_flags & _RT_INGC)
			yieldCurrentThread();*/

		switch (curFn->overloadingKind) {
			case FnOverloadingKind::Regular: {
				RegularFnOverloadingObject *ol = (RegularFnOverloadingObject *)curFn;

				const size_t nIns = ol->instructions.size();

				bool isContextChanged = false;
				while (!isContextChanged) {
					const size_t idxCurIns = curMajorFrame->resumable->curIns;
					if (idxCurIns >=
						nIns) {
						// Raise out of fn body error.
					}

					// Interrupt execution if the thread is explicitly specified to be killed.
					if (managedThread->status == ThreadStatus::Dead) {
						return {};
					}

					if ((fixedAlloc.szAllocated > _szComputedGcLimit)) {
						gc();
					}

					const Instruction *const SLAKE_RESTRICT instruction = ol->instructions.data() + idxCurIns;
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _execIns(context, curMajorFrame, instruction->opcode, instruction->output, instruction->nOperands, instruction->operands, isContextChanged));
				}

				break;
			}
			case FnOverloadingKind::Native: {
				NativeFnOverloadingObject *ol = (NativeFnOverloadingObject *)curFn;

				Value returnValue = ol->callback(
					&context->getContext(),
					curMajorFrame);
				uint32_t returnValueOutReg = curMajorFrame->returnValueOutReg;
				context->_context.leaveMajor();
				if (returnValueOutReg != UINT32_MAX) {
					SLAKE_RETURN_IF_EXCEPT_WITH_LVAR(exceptPtr, _setRegisterValue(this, context->_context.dataStack, context->_context.stackSize, curMajorFrame, returnValueOutReg, returnValue));
				}

				break;
			}
			default:
				std::terminate();
		}
	}

	context->_context.flags |= CTX_DONE;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::execFn(
	const FnOverloadingObject *overloading,
	ContextObject *prevContext,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	Value &valueOut,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	HostObjectRef<ContextObject> context(prevContext);

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, UINT32_MAX));
	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));

	ExecutionRunnable runnable;

	runnable.context = context;

	if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
		return OutOfMemoryError::alloc();
	}

	NativeThreadHandle threadHandle = currentThreadHandle();

	peff::ScopeGuard removeManagedThreadRunnablesGuard([this, threadHandle]() noexcept {
		managedThreadRunnables.remove(threadHandle);
	});

	runnable.run();

	InternalExceptionPointer exceptPtr = std::move(runnable.exceptPtr);

	return exceptPtr;
}

[[nodiscard]] SLAKE_API InternalExceptionPointer Runtime::execFnInAotFn(
	const FnOverloadingObject *overloading,
	ContextObject *context,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	HostObjectRef<ContextObject> contextPtr(context);

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));

	InternalExceptionPointer exceptPtr = execContext(context);

	return exceptPtr;
}

SLAKE_API InternalExceptionPointer Runtime::execFnWithSeparatedExecutionThread(
	const FnOverloadingObject *overloading,
	ContextObject *prevContext,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	HostObjectRef<ContextObject> &contextOut) {
	HostObjectRef<ContextObject> context(prevContext);

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, 0));
	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, thisObject, overloading, args, nArgs, 0));

	ExecutionRunnable runnable;

	runnable.context = context;

	if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
		return OutOfMemoryError::alloc();
	}

	NativeThreadHandle threadHandle = currentThreadHandle();

	peff::ScopeGuard removeManagedThreadRunnablesGuard([this, threadHandle]() noexcept {
		managedThreadRunnables.remove(threadHandle);
	});

	std::unique_ptr<Thread, peff::DeallocableDeleter<Thread>> t(Thread::alloc(getFixedAlloc(), &runnable, SLAKE_NATIVE_STACK_SIZE_MAX));

	t->join();

	InternalExceptionPointer exceptPtr = std::move(runnable.exceptPtr);

	return exceptPtr;
}

SLAKE_API InternalExceptionPointer Runtime::createCoroutineInstance(
	const FnOverloadingObject *fn,
	Object *thisObject,
	const Value *args,
	uint32_t nArgs,
	HostObjectRef<CoroutineObject> &coroutineOut) {
	HostRefHolder holder(getFixedAlloc());
	HostObjectRef<CoroutineObject> co = CoroutineObject::alloc(this);

	if (!co) {
		return OutOfMemoryError::alloc();
	}

	co->overloading = fn;

	MajorFrame newMajorFrame(this, co->selfAllocator.get());

	if (!(newMajorFrame.resumable = ResumableObject::alloc(this)))
		return OutOfMemoryError::alloc();

	SLAKE_RETURN_IF_EXCEPT(_fillArgs(&newMajorFrame, fn, args, nArgs, holder));
	newMajorFrame.resumable->thisObject = thisObject;

	if (!newMajorFrame.resumable->minorFrames.pushBack(MinorFrame(this, co->selfAllocator.get(), 0)))
		return OutOfMemoryError::alloc();

	co->resumable = newMajorFrame.resumable;

	coroutineOut = co;
	return {};
}

SLAKE_API InternalExceptionPointer Runtime::resumeCoroutine(
	ContextObject *context,
	CoroutineObject *coroutine,
	Value &resultOut,
	void *nativeStackBaseCurrentPtr,
	size_t nativeStackSize) {
	if (coroutine->isDone()) {
		resultOut = coroutine->finalResult;
		return {};
	}

	HostObjectRef<ContextObject> contextRef(context);

	SLAKE_RETURN_IF_EXCEPT(_createNewMajorFrame(&context->_context, nullptr, nullptr, nullptr, 0, UINT32_MAX));
	MajorFrame *topMajorFrame = context->_context.majorFrames.back().get();
	SLAKE_RETURN_IF_EXCEPT(_createNewCoroutineMajorFrame(&context->_context, coroutine, 0));

	{
		ExecutionRunnable runnable;

		runnable.context = context;

		{
			if (!managedThreadRunnables.insert(currentThreadHandle(), &runnable)) {
				contextRef->_context.leaveMajor();

				return OutOfMemoryError::alloc();
			}

			NativeThreadHandle threadHandle = currentThreadHandle();

			peff::ScopeGuard removeManagedThreadRunnablesGuard([this, threadHandle]() noexcept {
				managedThreadRunnables.remove(threadHandle);
			});

			runnable.run();
		}

		InternalExceptionPointer exceptPtr = std::move(runnable.exceptPtr);

		if (exceptPtr) {
			return exceptPtr;
		}

		resultOut = ((const Value *)calcStackAddr(context->_context.dataStack, context->_context.stackSize, topMajorFrame->offRegs + sizeof(Value)))[0];

		contextRef->_context.leaveMajor();
	}

	return {};
}
