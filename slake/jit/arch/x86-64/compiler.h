#ifndef _SLAKE_JIT_ARCH_X86_64_COMPILER_H_
#define _SLAKE_JIT_ARCH_X86_64_COMPILER_H_

#include <slake/jit/base.h>
#include <map>

namespace slake {
	namespace jit {
		namespace x86_64 {
			struct DiscreteInstruction {
				size_t szIns;
				uint8_t buffer[16];
			};

			enum JITCompilerRegisterId {
				REG_RAX = 0,
				REG_RBX,
				REG_RCX,
				REG_RDX,
				REG_RSI,
				REG_RDI,
				REG_RSP,
				REG_RBP,
				REG_R8,
				REG_R9,
				REG_R10,
				REG_R11,
				REG_R12,
				REG_R13,
				REG_R14,
				REG_R15,
				REG_MAX
			};

			SLAKE_FORCEINLINE DiscreteInstruction emitIns(size_t szIns, const uint8_t *buffer) {
				DiscreteInstruction ins;

				ins.szIns = szIns;
				memcpy(ins.buffer, buffer, szIns);

				return ins;
			}

			SLAKE_API DiscreteInstruction emitPushRegIns(JITCompilerRegisterId registerId);
			SLAKE_API DiscreteInstruction emitPopRegIns(JITCompilerRegisterId registerId);

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

			struct RegLifetime {
				size_t offBeginIns;
				size_t offEndIns;
			};

			struct RegInfo {
				RegLifetime lifeTime = { 0, 0 };
				Type type;
			};

			struct JITCompileContext {
				std::deque<DiscreteInstruction> nativeInstructions;
				std::map<uint32_t, RegInfo> regInfo;
				std::map<size_t, JITRelAddrReplacingPoint32Storage> relativeAddrReplacingPoints;
				size_t regUseLevels[REG_MAX] = { 0 };
				size_t curRegUseLevel = 0;
				size_t curStackSize;

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

				SLAKE_FORCEINLINE void stackAlloc8(uint8_t size) {
					uint8_t ins[] = {
						0x48, 0x83, 0xec,
						size
					};

					pushIns(emitIns(sizeof(ins), ins));
					curStackSize += size;
				}

				SLAKE_FORCEINLINE void stackAlloc32(uint32_t size) {
					uint8_t ins[] = {
						0x48, 0x81, 0xec,
						0xff, 0xff, 0xff, 0xff
					};
					memcpy(ins + 3, &size, sizeof(size));

					pushIns(emitIns(sizeof(ins), ins));
					curStackSize += size;
				}

				SLAKE_FORCEINLINE void stackFree8(uint8_t size) {
					uint8_t ins[] = {
						0x48, 0x83, 0xc4,
						size
					};

					pushIns(emitIns(sizeof(ins), ins));
					curStackSize -= size;
				}

				SLAKE_FORCEINLINE void stackFree32(uint32_t size) {
					uint8_t ins[] = {
						0x48, 0x81, 0xc4,
						0xff, 0xff, 0xff, 0xff
					};
					memcpy(ins + 3, &size, sizeof(size));

					pushIns(emitIns(sizeof(ins), ins));
					curStackSize -= size;
				}
			};

			void loadInsWrapper(
				Runtime *rt,
				IdRefObject *idRefObject,
				Value *regOut,
				InternalException **internalExceptionOut);
		}
	}
}

#endif
