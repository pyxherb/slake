#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMMON_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_COMMON_H_

#include "emitters/add.h"
#include "emitters/sub.h"
#include "emitters/mov.h"

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
				std::list<PhysicalRegSavingInfo> savingInfo;
			};

			struct VirtualRegState {
				RegisterId phyReg;
				int32_t saveOffset = INT32_MIN;
				size_t size;
			};

			struct JITCompileContext {
				std::deque<DiscreteInstruction> nativeInstructions;
				std::map<size_t, JITRelAddrReplacingPoint32Storage> relativeAddrReplacingPoints;
				PhysicalRegState phyRegStates[REG_MAX];
				size_t curStackSize;
				JITCompilerOptions options;
				std::bitset<REG_MAX> regAllocFlags;
				std::map<uint32_t, VirtualRegState> virtualRegStates;
				std::map<size_t, std::list<uint32_t>> regRecycleBoundaries;
				std::map<int32_t, size_t> freeStackSpaces;

				SLAKE_FORCEINLINE void pushIns(const DiscreteInstruction &ins) {
					nativeInstructions.push_back(ins);
				}
				SLAKE_FORCEINLINE void pushRelativeAddrReplacingPoint32(const JITRelAddrReplacingPoint32 &replacingPoint) {
					relativeAddrReplacingPoints[nativeInstructions.size() - 1].pushReplacingPoint(replacingPoint);
				}

				SLAKE_FORCEINLINE void addStackPtr(size_t size) {
					curStackSize += size;
				}

				SLAKE_FORCEINLINE void subStackPtr(size_t size) {
					curStackSize -= size;
				}

				SLAKE_API RegisterId allocGpReg();
				SLAKE_API RegisterId allocXmmReg();

				SLAKE_FORCEINLINE void pushReg8(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					int32_t lhsRegStackSaveOff = stackAllocAligned(sizeof(uint8_t), sizeof(uint8_t));

					pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint8_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					phyRegStates[reg].savingInfo.push_back({ phyRegStates[reg].lastVregId });
				}

				SLAKE_FORCEINLINE void pushReg16(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					int32_t lhsRegStackSaveOff = stackAllocAligned(sizeof(uint16_t), sizeof(uint16_t));

					pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint16_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					phyRegStates[reg].savingInfo.push_back({ phyRegStates[reg].lastVregId });
				}

				SLAKE_FORCEINLINE void pushReg32(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					int32_t lhsRegStackSaveOff = stackAllocAligned(sizeof(uint32_t), sizeof(uint32_t));

					pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint32_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					phyRegStates[reg].savingInfo.push_back({ phyRegStates[reg].lastVregId });
				}

				SLAKE_FORCEINLINE void pushReg64(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					int32_t lhsRegStackSaveOff = stackAllocAligned(sizeof(uint64_t), sizeof(uint64_t));

					pushIns(emitMovReg8ToMemIns(MemoryLocation{ REG_RBP, lhsRegStackSaveOff, REG_MAX, 0 }, reg));

					offOut = lhsRegStackSaveOff;
					sizeOut = sizeof(uint64_t);

					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = lhsRegStackSaveOff;
					phyRegStates[reg].savingInfo.push_back({ phyRegStates[reg].lastVregId });
				}

				SLAKE_FORCEINLINE void pushReg(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					switch (virtualRegStates.at(phyRegStates[reg].lastVregId).size) {
						case sizeof(uint8_t):
							pushReg8(reg, offOut, sizeOut);
							break;
						case sizeof(uint16_t):
							pushReg16(reg, offOut, sizeOut);
							break;
						case sizeof(uint32_t):
							pushReg32(reg, offOut, sizeOut);
							break;
						case sizeof(uint64_t):
							pushReg64(reg, offOut, sizeOut);
							break;
						default:
							assert(("Invalid register size", false));
					}
				}

				SLAKE_FORCEINLINE void pushRegXmm32(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					pushReg(gpReg, tmpOff, tmpSize);

					pushIns(emitMovdRegXmmToReg32Ins(gpReg, reg));
					pushReg32(gpReg, offOut, sizeOut);

					popReg(gpReg, tmpOff, tmpSize);
				}

				SLAKE_FORCEINLINE void pushRegXmm64(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					pushReg(gpReg, tmpOff, tmpSize);

					pushIns(emitMovqRegXmmToReg64Ins(gpReg, reg));
					pushReg64(gpReg, offOut, sizeOut);

					popReg(gpReg, tmpOff, tmpSize);
				}

				SLAKE_FORCEINLINE void pushRegXmm(RegisterId reg, int32_t &offOut, size_t &sizeOut) {
					switch (virtualRegStates.at(phyRegStates[reg].lastVregId).size) {
						case sizeof(float):
							pushRegXmm32(reg, offOut, sizeOut);
							break;
						case sizeof(double):
							pushRegXmm64(reg, offOut, sizeOut);
							break;
						default:
							assert(("Invalid register size", false));
					}
				}

				SLAKE_FORCEINLINE void popReg8(RegisterId reg, int32_t off) {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					pushIns(emitMovMemToReg8Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
					stackFree(off, sizeof(uint8_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo.back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo.pop_back();
				}

				SLAKE_FORCEINLINE void popReg16(RegisterId reg, int32_t off) {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					pushIns(emitMovMemToReg16Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
					stackFree(off, sizeof(uint16_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo.back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo.pop_back();
				}

				SLAKE_FORCEINLINE void popReg32(RegisterId reg, int32_t off) {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					pushIns(emitMovMemToReg32Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
					stackFree(off, sizeof(uint32_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo.back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo.pop_back();
				}

				SLAKE_FORCEINLINE void popReg64(RegisterId reg, int32_t off) {
					virtualRegStates.at(phyRegStates[reg].lastVregId).saveOffset = INT32_MIN;
					pushIns(emitMovMemToReg64Ins(reg, MemoryLocation{ REG_RBP, off, REG_MAX, 0 }));
					stackFree(off, sizeof(uint64_t));

					virtualRegStates.at(phyRegStates[reg].savingInfo.back().vregId).saveOffset = INT32_MIN;
					phyRegStates[reg].savingInfo.pop_back();
				}

				SLAKE_FORCEINLINE void popReg(RegisterId reg, int32_t off, size_t size) {
					switch (size) {
						case sizeof(uint8_t):
							popReg8(reg, off);
							break;
						case sizeof(uint16_t):
							popReg16(reg, off);
							break;
						case sizeof(uint32_t):
							popReg32(reg, off);
							break;
						case sizeof(uint64_t):
							popReg64(reg, off);
							break;
						default:
							assert(("Invalid register size", false));
					}
				}

				SLAKE_FORCEINLINE void popRegXmm32(RegisterId reg, int32_t off) {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					pushReg(gpReg, tmpOff, tmpSize);

					popReg32(gpReg, off);
					pushIns(emitMovdReg32ToRegXmmIns(gpReg, reg));

					popReg(gpReg, tmpOff, tmpSize);
				}

				SLAKE_FORCEINLINE void popRegXmm64(RegisterId reg, int32_t off) {
					int32_t tmpOff;
					size_t tmpSize;
					RegisterId gpReg = allocGpReg();

					pushReg(gpReg, tmpOff, tmpSize);

					popReg64(gpReg, off);
					pushIns(emitMovqReg64ToRegXmmIns(gpReg, reg));

					popReg(gpReg, tmpOff, tmpSize);
				}

				SLAKE_FORCEINLINE void popRegXmm(RegisterId reg, int32_t off, size_t size) {
					switch (size) {
						case sizeof(float):
							popRegXmm32(reg, off);
							break;
						case sizeof(double):
							popRegXmm64(reg, off);
							break;
						default:
							assert(("Invalid register size", false));
					}
				}

				SLAKE_FORCEINLINE bool isRegInUse(RegisterId reg) {
					return phyRegStates[reg].lastVregId == UINT32_MAX;
				}

				SLAKE_FORCEINLINE VirtualRegState &defVirtualReg(uint32_t vreg, RegisterId phyReg, size_t size) {
					VirtualRegState &vregState = virtualRegStates[vreg];
					vregState.phyReg = phyReg;
					vregState.size = size;
					vregState.saveOffset = INT32_MIN;

					phyRegStates[phyReg].lastVregId = vreg;

					return vregState;
				}

				SLAKE_API int32_t stackAllocAligned(uint32_t size, uint32_t alignment);

				SLAKE_API void stackFree(int32_t saveOffset, size_t size);
			};

			struct JITExecContext;

			typedef void (*JITCompiledFnPtr)(JITExecContext *execContext);

			void loadInsWrapper(
				JITExecContext *rt,
				IdRefObject *idRefObject,
				Value *regOut,
				InternalException **internalExceptionOut);
		}
	}
}

#endif
