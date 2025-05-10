#include "common.h"
#include "../context.h"
#include <slake/runtime.h>

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

static const RegisterId _s_gpRegs[] = {
	REG_RAX,
	REG_RBX,
	REG_RCX,
	REG_RDX,
	REG_RSI,
	REG_RDI,
	REG_R8,
	REG_R9,
	REG_R10,
	// R11 will never be allocated since it is used for stack limit checking.
	REG_R12,
	REG_R13,
	REG_R14,
	REG_R15
};

static const RegisterId _s_xmmRegs[] = {
	REG_XMM0,
	REG_XMM1,
	REG_XMM2,
	REG_XMM3,
	REG_XMM4,
	REG_XMM5,
	REG_XMM6,
	REG_XMM7,
	REG_XMM8,
	REG_XMM9,
	REG_XMM10,
	REG_XMM11,
	REG_XMM12,
	REG_XMM13,
	REG_XMM14,
	REG_XMM15,
};

SLAKE_API JITCompileContext::JITCompileContext(Runtime *runtime, peff::Alloc *resourceAllocator)
	: nativeInstructions(resourceAllocator),
	  virtualRegStates(resourceAllocator),
	  localVarStates(resourceAllocator),
	  regRecycleBoundaries(resourceAllocator),
	  freeStackSpaces(resourceAllocator),
	  labelOffsets(resourceAllocator) {
	for (size_t i = 0; i < REG_MAX; ++i) {
		phyRegStates[i].savingInfo.moveFrom(peff::List<PhysicalRegSavingInfo>(resourceAllocator));
	}
}

SLAKE_API InternalExceptionPointer JITCompileContext::pushPrologStackOpIns() {
	SLAKE_RETURN_IF_EXCEPT(checkStackPointerOnProlog(sizeof(uint64_t) * 11));

	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_R8)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_RDX)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_RCX)));

	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_RBX)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_RDI)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_RSI)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_R12)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_R13)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_R14)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_R15)));

	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPushReg64Ins(REG_RBP)));

	SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovReg64ToReg64Ins(REG_RBP, REG_RSP)));

	addStackPtr(sizeof(uint64_t) * 11);

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::pushEpilogStackOpIns() {
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovReg64ToReg64Ins(REG_RSP, REG_RBP)));

	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_RBP)));

	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_R15)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_R14)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_R13)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_R12)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_RSI)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_RDI)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_RBX)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_RCX)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_RDX)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitPopReg64Ins(REG_R8)));

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::checkStackPointer(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitSubImm32ToReg64Ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitCmpReg64ToReg64Ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitLabelledJumpIns("_report_stack_overflow", DiscreteInstructionType::JumpIfLtLabelled)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitAddImm32ToReg64Ins(REG_RSP, (uint8_t *)&size)));

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::checkStackPointerOnProlog(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitSubImm32ToReg64Ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitCmpReg64ToReg64Ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitLabelledJumpIns("_report_stack_overflow_on_prolog", DiscreteInstructionType::JumpIfLtLabelled)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitAddImm32ToReg64Ins(REG_RSP, (uint8_t *)&size)));

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::checkAndPushStackPointer(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitSubImm32ToReg64Ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitCmpReg64ToReg64Ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitLabelledJumpIns("_report_stack_overflow", DiscreteInstructionType::JumpIfLtLabelled)));
	addStackPtr(size);

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::checkAndPushStackPointerOnProlog(uint32_t size) {
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitSubImm32ToReg64Ins(REG_RSP, (uint8_t *)&size)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitCmpReg64ToReg64Ins(REG_R11, REG_RSP)));
	SLAKE_RETURN_IF_EXCEPT(pushIns(emitLabelledJumpIns("_report_stack_overflow_on_prolog", DiscreteInstructionType::JumpIfLtLabelled)));
	addStackPtr(size);

	return {};
}

SLAKE_API RegisterId JITCompileContext::allocGpReg() noexcept {
realloc:
	for (auto i : _s_gpRegs) {
		if (!regAllocFlags.test(i)) {
			regAllocFlags.set(i);
			return i;
		}
	}
	for (auto i : _s_gpRegs) {
		regAllocFlags.reset(i);
	}
	goto realloc;
}

SLAKE_API RegisterId JITCompileContext::allocXmmReg() noexcept {
realloc:
	for (auto i : _s_xmmRegs) {
		if (!regAllocFlags.test(i)) {
			regAllocFlags.set(i);
			return i;
		}
	}
	for (auto i : _s_xmmRegs) {
		regAllocFlags.reset(i);
	}
	goto realloc;
}

SLAKE_API void JITCompileContext::setRegAllocated(RegisterId reg) {
	regAllocFlags.set(reg);
}

SLAKE_API void JITCompileContext::unallocReg(RegisterId reg) {
	assert(regAllocFlags.test(reg));
	regAllocFlags.reset(reg);
}

SLAKE_API InternalExceptionPointer JITCompileContext::pushReg(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
	switch (virtualRegStates.at(phyRegStates[reg].lastVregId).size) {
		case sizeof(uint8_t):
			SLAKE_RETURN_IF_EXCEPT(pushReg8(reg, offOut, sizeOut));
			break;
		case sizeof(uint16_t):
			SLAKE_RETURN_IF_EXCEPT(pushReg16(reg, offOut, sizeOut));
			break;
		case sizeof(uint32_t):
			SLAKE_RETURN_IF_EXCEPT(pushReg32(reg, offOut, sizeOut));
			break;
		case sizeof(uint64_t):
			SLAKE_RETURN_IF_EXCEPT(pushReg64(reg, offOut, sizeOut));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::pushRegXmm(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
	switch (virtualRegStates.at(phyRegStates[reg].lastVregId).size) {
		case sizeof(float):
			SLAKE_RETURN_IF_EXCEPT(pushRegXmm32(reg, offOut, sizeOut));
			break;
		case sizeof(double):
			SLAKE_RETURN_IF_EXCEPT(pushRegXmm64(reg, offOut, sizeOut));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::popReg(RegisterId reg, int32_t off, size_t size) noexcept {
	switch (size) {
		case sizeof(uint8_t):
			SLAKE_RETURN_IF_EXCEPT(popReg8(reg, off));
			break;
		case sizeof(uint16_t):
			SLAKE_RETURN_IF_EXCEPT(popReg16(reg, off));
			break;
		case sizeof(uint32_t):
			SLAKE_RETURN_IF_EXCEPT(popReg32(reg, off));
			break;
		case sizeof(uint64_t):
			SLAKE_RETURN_IF_EXCEPT(popReg64(reg, off));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::popRegXmm(RegisterId reg, int32_t off, size_t size) noexcept {
	switch (size) {
		case sizeof(float):
			SLAKE_RETURN_IF_EXCEPT(popRegXmm32(reg, off));
			break;
		case sizeof(double):
			SLAKE_RETURN_IF_EXCEPT(popRegXmm64(reg, off));
			break;
		default:
			// Invalid register size
			std::terminate();
	}

	return {};
}

SLAKE_API InternalExceptionPointer JITCompileContext::stackAllocAligned(uint32_t size, uint32_t alignment, int32_t &offOut) noexcept {
	for (auto i = freeStackSpaces.begin(); i != freeStackSpaces.end(); ++i) {
		int32_t allocBase = i.key();
		size_t diff = allocBase % alignment;
		if (diff) {
			allocBase += alignment - diff;
		}

		if (i.value() > size) {
			int32_t newBase = (int32_t)i.key() + size;

			// We don't merge here, just shrink the space.
			freeStackSpaces.insert(std::move(newBase), i.value() - size);
			freeStackSpaces.remove(i.key());

			offOut = (int32_t)-allocBase;
			return {};
		} else if (i.value() == size) {
			int32_t off = i.key();
			freeStackSpaces.remove(i.key());
			offOut = -off;
			return {};
		}
	}

	int32_t allocBase = (int32_t)curStackSize;
	size_t diff = curStackSize % alignment;
	if (diff) {
		allocBase += alignment - diff;
	}
	size += alignment - diff;

	SLAKE_RETURN_IF_EXCEPT(checkAndPushStackPointer(size));

	offOut = -allocBase;
	return {};
}

SLAKE_API void JITCompileContext::stackFree(int32_t saveOffset, size_t size) {
	freeStackSpaces.insert(+saveOffset, +size);
	auto selfIt = freeStackSpaces.find(saveOffset);

	// Try to merge with the right neighbor.
	{
		auto rightNeighbor = selfIt;
		++rightNeighbor;

		if (rightNeighbor != freeStackSpaces.end()) {
			assert(rightNeighbor.value() >= saveOffset + sizeof(uint64_t));
			if (rightNeighbor.key() == saveOffset + sizeof(uint64_t)) {
				selfIt.value() += rightNeighbor.value();
				freeStackSpaces.remove(rightNeighbor);
			}
		}
	}

	// Try to merge the new slot with the left neighbor.
	if (selfIt != freeStackSpaces.begin()) {
		auto leftNeighbor = selfIt;
		--leftNeighbor;

		assert(leftNeighbor.key() + leftNeighbor.value() <= saveOffset);
		if (leftNeighbor.key() + leftNeighbor.value() == saveOffset) {
			leftNeighbor.value() += selfIt.value();
			freeStackSpaces.remove(selfIt);
		}
	}
}

void slake::jit::x86_64::loadInsWrapper(
	JITExecContext *context,
	IdRefObject *idRefObject) {
	EntityRef entityRef;
	InternalExceptionPointer e = context->runtime->resolveIdRef(idRefObject, entityRef, nullptr);
	if (e) {
		context->exception = e.get();
		e.reset();
		return;
	}
	context->returnValue = Value(entityRef);
}
void slake::jit::x86_64::rloadInsWrapper(
	JITExecContext *context,
	Object *baseObject,
	IdRefObject *idRefObject) {
	EntityRef entityRef;
	InternalExceptionPointer e = context->runtime->resolveIdRef(idRefObject, entityRef, baseObject);
	if (e) {
		context->exception = e.get();
		e.reset();
		return;
	}
	context->returnValue = Value(entityRef);
}

void slake::jit::x86_64::memcpyWrapper(
	void *dest,
	const void *src,
	uint64_t size) {
	memmove(dest, src, size);
}
