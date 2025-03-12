#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMMON_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMMON_H_

#include "emitters.h"
#include "../context.h"
#include <peff/containers/map.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			struct JITRelAddrReplacingPoint32 {
				uint8_t offset;
				const void *dest;
			};

			struct JITRelAddrReplacingPoint32Storage {
				uint8_t nReplacingPoints = 0;
				JITRelAddrReplacingPoint32 replacingPoints[3];

				SLAKE_FORCEINLINE void pushReplacingPoint(
					const JITRelAddrReplacingPoint32 &replacingPoint) {
					assert(nReplacingPoints < std::size(replacingPoints));
					replacingPoints[nReplacingPoints++] = replacingPoint;
				}
			};

			struct PhysicalRegSavingInfo {
				uint32_t vregId;
			};

			struct PhysicalRegState {
				uint32_t lastVregId;
				peff::Uninitialized<peff::List<PhysicalRegSavingInfo>> savingInfo;
			};

			struct VirtualRegState {
				RegisterId phyReg;
				int32_t saveOffset = INT32_MIN;
				size_t size;
			};

			struct CallingRegSavingInfo {
				int32_t offSavedRax = INT32_MIN,
						offSavedR10 = INT32_MIN,
						offSavedR11 = INT32_MIN;
				size_t szSavedRax,
					szSavedR10,
					szSavedR11;
				int32_t offSavedRcx, offSavedRdx, offSavedR8, offSavedR9;
				size_t szSavedRcx, szSavedRdx, szSavedR8, szSavedR9;
			};

			struct LocalVarState {
				Type type;
				int32_t stackOff;
				size_t size;
			};

			struct JITCompileContext {
				Runtime *runtime;
				peff::List<DiscreteInstruction> nativeInstructions;
				PhysicalRegState phyRegStates[REG_MAX];
				size_t curStackSize;
				JITCompilerOptions options;
				std::bitset<REG_MAX> regAllocFlags;
				peff::Map<uint32_t, VirtualRegState> virtualRegStates;
				peff::Map<uint32_t, LocalVarState> localVarStates;
				peff::Map<size_t, peff::List<uint32_t>> regRecycleBoundaries;
				peff::Map<int32_t, size_t> freeStackSpaces;
				int32_t jitContextOff;
				peff::HashMap<peff::String, size_t> labelOffsets;

				SLAKE_API JITCompileContext(Runtime *runtime);
				[[nodiscard]] SLAKE_API InternalExceptionPointer pushPrologStackOpIns();
				[[nodiscard]] SLAKE_API InternalExceptionPointer pushEpilogStackOpIns();

				[[nodiscard]] SLAKE_API InternalExceptionPointer checkStackPointer(uint32_t size);
				[[nodiscard]] SLAKE_API InternalExceptionPointer checkStackPointerOnProlog(uint32_t size);
				[[nodiscard]] SLAKE_API InternalExceptionPointer checkAndPushStackPointer(uint32_t size);
				[[nodiscard]] SLAKE_API InternalExceptionPointer checkAndPushStackPointerOnProlog(uint32_t size);

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer initJITContextStorage() noexcept {
					SLAKE_RETURN_IF_EXCEPT(stackAllocAligned(sizeof(JITExecContext *), sizeof(JITExecContext *), jitContextOff));

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushIns(DiscreteInstruction &&ins) noexcept {
					if (!nativeInstructions.pushBack(std::move(ins)))
						return OutOfMemoryError::alloc();
					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushLabel(peff::String &&label) noexcept {
					if (!labelOffsets.insert(std::move(label), nativeInstructions.size()))
						return OutOfMemoryError::alloc();
					return {};
				}

				SLAKE_FORCEINLINE void addStackPtr(size_t size) noexcept {
					curStackSize += size;
				}

				SLAKE_FORCEINLINE void subStackPtr(size_t size) noexcept {
					curStackSize -= size;
				}

				SLAKE_API RegisterId allocGpReg() noexcept;
				SLAKE_API RegisterId allocXmmReg() noexcept;

				SLAKE_API void setRegAllocated(RegisterId reg);
				SLAKE_API void unallocReg(RegisterId reg);

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushReg8(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
					int32_t lhsRegStackSaveOff;
					SLAKE_RETURN_IF_EXCEPT(stackAllocAligned(sizeof(uint8_t), sizeof(uint8_t), lhsRegStackSaveOff));

					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg)));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint8_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					if (!phyRegStates[reg].savingInfo->pushBack({ phyRegStates[reg].lastVregId }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushReg16(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
					int32_t lhsRegStackSaveOff;
					SLAKE_RETURN_IF_EXCEPT(stackAllocAligned(sizeof(uint16_t), sizeof(uint16_t), lhsRegStackSaveOff));

					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg)));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint16_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					if (!phyRegStates[reg].savingInfo->pushBack({ phyRegStates[reg].lastVregId }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushReg32(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
					int32_t lhsRegStackSaveOff;
					SLAKE_RETURN_IF_EXCEPT(stackAllocAligned(sizeof(uint32_t), sizeof(uint32_t), lhsRegStackSaveOff));

					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg)));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint32_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					if (!phyRegStates[reg].savingInfo->pushBack({ phyRegStates[reg].lastVregId }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushReg64(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
					int32_t lhsRegStackSaveOff;
					SLAKE_RETURN_IF_EXCEPT(stackAllocAligned(sizeof(uint64_t), sizeof(uint64_t), lhsRegStackSaveOff));

					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg)));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint64_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					if (!phyRegStates[reg].savingInfo->pushBack({ phyRegStates[reg].lastVregId }))
						return OutOfMemoryError::alloc();

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer pushReg(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept;

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushRegXmm32(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					SLAKE_RETURN_IF_EXCEPT(pushReg(gpReg, tmpOff, tmpSize));

					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovdRegXmmToReg32Ins(gpReg, reg)));
					SLAKE_RETURN_IF_EXCEPT(pushReg32(gpReg, offOut, sizeOut));

					SLAKE_RETURN_IF_EXCEPT(popReg(gpReg, tmpOff, tmpSize));

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer pushRegXmm64(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					SLAKE_RETURN_IF_EXCEPT(pushReg(gpReg, tmpOff, tmpSize));

					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovqRegXmmToReg64Ins(gpReg, reg)));
					SLAKE_RETURN_IF_EXCEPT(pushReg64(gpReg, offOut, sizeOut));

					SLAKE_RETURN_IF_EXCEPT(popReg(gpReg, tmpOff, tmpSize));

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer pushRegXmm(RegisterId reg, int32_t &offOut, size_t &sizeOut) noexcept;

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer popReg8(RegisterId reg, int32_t off) noexcept {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovMemToReg8Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stackFree(off, sizeof(uint8_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo->back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo->popBack();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer popReg16(RegisterId reg, int32_t off) noexcept {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovMemToReg16Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stackFree(off, sizeof(uint16_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo->back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo->popBack();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer popReg32(RegisterId reg, int32_t off) noexcept {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovMemToReg32Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stackFree(off, sizeof(uint32_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo->back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo->popBack();

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer popReg64(RegisterId reg, int32_t off) noexcept {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovMemToReg64Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 })));
					stackFree(off, sizeof(uint64_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo->back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo->popBack();

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer popReg(RegisterId reg, int32_t off, size_t size) noexcept;

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer popRegXmm32(RegisterId reg, int32_t off) noexcept {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					SLAKE_RETURN_IF_EXCEPT(pushReg(gpReg, tmpOff, tmpSize));

					SLAKE_RETURN_IF_EXCEPT(popReg32(gpReg, off));
					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovdReg32ToRegXmmIns(gpReg, reg)));

					SLAKE_RETURN_IF_EXCEPT(popReg(gpReg, tmpOff, tmpSize));

					return {};
				}

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer popRegXmm64(RegisterId reg, int32_t off) noexcept {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					SLAKE_RETURN_IF_EXCEPT(pushReg(gpReg, tmpOff, tmpSize));

					SLAKE_RETURN_IF_EXCEPT(popReg64(gpReg, off));
					SLAKE_RETURN_IF_EXCEPT(pushIns(emitMovqReg64ToRegXmmIns(gpReg, reg)));

					SLAKE_RETURN_IF_EXCEPT(popReg(gpReg, tmpOff, tmpSize));

					return {};
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer popRegXmm(RegisterId reg, int32_t off, size_t size) noexcept;

				SLAKE_FORCEINLINE bool isRegInUse(RegisterId reg) noexcept {
					return phyRegStates[reg].lastVregId == UINT32_MAX;
				}

				[[nodiscard]] SLAKE_FORCEINLINE VirtualRegState *defVirtualReg(uint32_t vreg, RegisterId phyReg, size_t size) noexcept {
					if (!virtualRegStates.insert(+vreg, {}))
						return nullptr;
					VirtualRegState &vregState = virtualRegStates.at(vreg);
					vregState.phyReg = phyReg;
					vregState.size = size;
					vregState.saveOffset = INT32_MIN;

					phyRegStates[phyReg].lastVregId = vreg;

					return &vregState;
				}
				[[nodiscard]] SLAKE_FORCEINLINE VirtualRegState *defVirtualReg(uint32_t vreg, int32_t saveOffset, size_t size) noexcept {
					if (!virtualRegStates.insert(+vreg, {}))
						return nullptr;
					VirtualRegState &vregState = virtualRegStates.at(vreg);
					vregState.phyReg = REG_MAX;
					vregState.size = size;
					vregState.saveOffset = saveOffset;

					return &vregState;
				}
				[[nodiscard]] SLAKE_FORCEINLINE VirtualRegState *defDummyVirtualReg(uint32_t vreg) noexcept {
					if (!virtualRegStates.insert(+vreg, {}))
						return nullptr;
					VirtualRegState &vregState = virtualRegStates.at(vreg);
					vregState.phyReg = REG_MAX;
					vregState.size = 0;
					vregState.saveOffset = 0;

					return &vregState;
				}

				[[nodiscard]] SLAKE_FORCEINLINE LocalVarState *defLocalVar(uint32_t index, int32_t stackOff, size_t size) noexcept {
					if (!localVarStates.insert(+index, {}))
						return nullptr;
					LocalVarState &localVarState = localVarStates.at(index);
					localVarState.stackOff = stackOff;
					localVarState.size = size;

					return &localVarState;
				}

				[[nodiscard]] SLAKE_API InternalExceptionPointer stackAllocAligned(uint32_t size, uint32_t alignment, int32_t &offOut) noexcept;

				SLAKE_API void stackFree(int32_t saveOffset, size_t size);

				[[nodiscard]] SLAKE_FORCEINLINE InternalExceptionPointer saveCallingRegs(CallingRegSavingInfo &infoOut) noexcept {
					// Save parameter registers.
					SLAKE_RETURN_IF_EXCEPT(pushReg(REG_RCX, infoOut.offSavedRcx, infoOut.szSavedRcx));
					SLAKE_RETURN_IF_EXCEPT(pushReg(REG_RDX, infoOut.offSavedRdx, infoOut.szSavedRdx));
					SLAKE_RETURN_IF_EXCEPT(pushReg(REG_R8, infoOut.offSavedR8, infoOut.szSavedR8));
					SLAKE_RETURN_IF_EXCEPT(pushReg(REG_R9, infoOut.offSavedR9, infoOut.szSavedR9));
					// Save scratch registers.
					if (isRegInUse(REG_RAX)) {
						SLAKE_RETURN_IF_EXCEPT(pushReg(REG_RAX, infoOut.offSavedRax, infoOut.szSavedRax));
					}
					if (isRegInUse(REG_R10)) {
						SLAKE_RETURN_IF_EXCEPT(pushReg(REG_R10, infoOut.offSavedR10, infoOut.szSavedR10));
					}
					if (isRegInUse(REG_R11)) {
						SLAKE_RETURN_IF_EXCEPT(pushReg(REG_R11, infoOut.offSavedR11, infoOut.szSavedR11));
					}
					return {};
				}

				SLAKE_FORCEINLINE InternalExceptionPointer restoreCallingRegs(const CallingRegSavingInfo &info) noexcept {
					// Restore scratch registers.
					if (info.offSavedR11 != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT(popReg(REG_R11, info.offSavedR11, info.szSavedR11));
					}
					if (info.offSavedR10 != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT(popReg(REG_R10, info.offSavedR10, info.szSavedR10));
					}
					if (info.offSavedRax != INT32_MIN) {
						SLAKE_RETURN_IF_EXCEPT(popReg(REG_RAX, info.offSavedRax, info.szSavedRax));
					}

					// Restore parameter registers.
					SLAKE_RETURN_IF_EXCEPT(popReg(REG_R9, info.offSavedR9, info.szSavedR9));
					SLAKE_RETURN_IF_EXCEPT(popReg(REG_R8, info.offSavedR8, info.szSavedR8));
					SLAKE_RETURN_IF_EXCEPT(popReg(REG_RDX, info.offSavedRdx, info.szSavedRdx));
					SLAKE_RETURN_IF_EXCEPT(popReg(REG_RCX, info.offSavedRcx, info.szSavedRcx));

					return {};
				}
			};

			struct JITExecContext;

			typedef void (*JITCompiledFnPtr)(JITExecContext *execContext);

			void loadInsWrapper(
				JITExecContext *context,
				IdRefObject *idRefObject);
			void rloadInsWrapper(
				JITExecContext *context,
				Object *baseObject,
				IdRefObject *idRefObject);
			void memcpyWrapper(
				void *dest,
				const void *src,
				uint64_t size);
		}
	}
}

#endif
