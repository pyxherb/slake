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
	REG_R11,
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

SLAKE_API RegisterId JITCompileContext::allocGpReg() {
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

SLAKE_API RegisterId JITCompileContext::allocXmmReg() {
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

SLAKE_API void JITCompileContext::unallocReg(RegisterId reg) {
	assert(regAllocFlags.test(reg));
	regAllocFlags.reset(reg);
}

SLAKE_API int32_t JITCompileContext::stackAllocAligned(uint32_t size, uint32_t alignment) {
	for (auto &i : freeStackSpaces) {
		int32_t allocBase = i.first;
		size_t diff = allocBase % alignment;
		if (diff) {
			allocBase += alignment - diff;
		}

		if (i.second > size) {
			int32_t newBase = (int32_t)i.first + size;

			// We don't merge here, just shrink the space.
			freeStackSpaces[newBase] = i.second - size;
			freeStackSpaces.erase(i.first);

			return (int32_t)-allocBase;
		} else if (i.second == size) {
			int32_t off = i.first;
			freeStackSpaces.erase(i.first);
			return -off;
		}
	}

	int32_t allocBase = (int32_t)curStackSize;
	size_t diff = curStackSize % alignment;
	if (diff) {
		allocBase += alignment - diff;
	}
	addStackPtr(size);
	pushIns(emitSubImm32ToReg32Ins(REG_RSP, (uint8_t *)&size));
	return -allocBase;
}

SLAKE_API void JITCompileContext::stackFree(int32_t saveOffset, size_t size) {
	freeStackSpaces[saveOffset] = size;
	auto selfIt = freeStackSpaces.find(saveOffset);

	// Try to merge the right neighbor.
	{
		auto rightNeighbor = selfIt;
		++rightNeighbor;

		if (rightNeighbor != freeStackSpaces.end()) {
			assert(rightNeighbor->second >= saveOffset + sizeof(uint64_t));
			if (rightNeighbor->first == saveOffset + sizeof(uint64_t)) {
				selfIt->second += rightNeighbor->second;
				freeStackSpaces.erase(rightNeighbor);
			}
		}
	}

	// Try to merge the new slot with the left neighbor.
	if (selfIt != freeStackSpaces.begin()) {
		auto leftNeighbor = selfIt;
		--leftNeighbor;

		assert(leftNeighbor->first + leftNeighbor->second <= saveOffset);
		if (leftNeighbor->first + leftNeighbor->second == saveOffset) {
			leftNeighbor->second += selfIt->second;
			freeStackSpaces.erase(selfIt);
		}
	}
}

void slake::jit::x86_64::loadInsWrapper(
	JITExecContext* context,
	IdRefObject* idRefObject,
	Value* regOut,
	InternalException** internalExceptionOut) {
	VarRefContext varRefContext;
	Object *object;
	InternalExceptionPointer e = context->runtime->resolveIdRef(idRefObject, &varRefContext, object, nullptr);
	if (e) {
		*internalExceptionOut = e.get();
		e.reset();
		return;
	}
	switch (object->getKind()) {
		case ObjectKind::Var:
			*regOut = Value(VarRef((VarObject *)object, varRefContext));
			break;
		default:
			*regOut = Value(object);
	}
}
