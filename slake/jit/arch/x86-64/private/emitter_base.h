#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTER_BASE_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTER_BASE_H_

#include <slake/jit/arch/x86-64/basedefs.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_FORCEINLINE DiscreteInstruction emitRawIns(size_t szIns, const uint8_t *buffer) {
				DiscreteInstruction ins;

				ins.szIns = szIns;
				memcpy(ins.buffer, buffer, szIns);

				return ins;
			}

			using EmitRegularInsFlags = uint8_t;
			constexpr static EmitRegularInsFlags
				EMREGINS_MODRM = 0x01,
				EMREGINS_SIB = 0x02;
			SLAKE_FORCEINLINE DiscreteInstruction emitRegularIns(
				uint8_t nLegacyPrefixes,
				uint8_t legacyPrefixes[],
				uint8_t rexPrefix,
				uint8_t lenOpcode,
				uint8_t opcode[],
				EmitRegularInsFlags flags,
				uint8_t modRm,
				uint8_t sib,
				uint8_t lenDisp,
				uint8_t disp[],
				uint8_t lenImm,
				uint8_t imm[]) {
				DiscreteInstruction ins;
				size_t offIns = 0;

				if (nLegacyPrefixes) {
					memcpy(ins.buffer + offIns, legacyPrefixes, nLegacyPrefixes);
					offIns += nLegacyPrefixes;
				}

				if (rexPrefix & 0b01000000) {
					ins.buffer[offIns++] = rexPrefix;
				}

				memcpy(ins.buffer + offIns, opcode, lenOpcode);
				offIns += lenOpcode;

				if (flags & EMREGINS_MODRM) {
					ins.buffer[offIns++] = modRm;
				}

				if (flags & EMREGINS_SIB) {
					ins.buffer[offIns++] = sib;
				}

				if (lenDisp) {
					memcpy(ins.buffer + offIns, disp, lenDisp);
					offIns += lenDisp;
				}

				if (lenImm) {
					memcpy(ins.buffer + offIns, imm, lenImm);
					offIns += lenImm;
				}
			}
		}
	}
}

#endif
