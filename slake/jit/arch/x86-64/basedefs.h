#ifndef _SLAKE_JIT_ARCH_X86_64_BASEDEFS_H_
#define _SLAKE_JIT_ARCH_X86_64_BASEDEFS_H_

#include <slake/jit/base.h>
#include <map>
#include <bitset>
#include "private/emitter_base.h"

#define DEF_INS_BUFFER(name, ...) uint8_t name[] = { __VA_ARGS__ }
#define REX_PREFIX(w, r, x, b) ((uint8_t)(0b01000000 | ((w) << 3) | ((r) << 2) | ((x) << 1) | (b)))
#define MODRM_BYTE(mode, regOrOpcode, value) ((uint8_t)(((mode) << 6) | ((regOrOpcode) << 3) | (value)))

namespace slake {
	namespace jit {
		namespace x86_64 {
			struct DiscreteInstruction {
				size_t szIns;
				uint8_t buffer[16];
			};

			enum RegisterId : uint8_t {
				REG_RAX = 0,
				REG_RCX,
				REG_RDX,
				REG_RBX,
				REG_RSP,
				REG_RBP,
				REG_RSI,
				REG_RDI,

				REG_R8,
				REG_R9,
				REG_R10,
				REG_R11,
				REG_R12,
				REG_R13,
				REG_R14,
				REG_R15,

				REG_MM0,
				REG_MM1,
				REG_MM2,
				REG_MM3,
				REG_MM4,
				REG_MM5,
				REG_MM6,
				REG_MM7,

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
				REG_XMM16,
				REG_XMM17,
				REG_XMM18,
				REG_XMM19,
				REG_XMM20,
				REG_XMM21,
				REG_XMM22,
				REG_XMM23,
				REG_XMM24,
				REG_XMM25,
				REG_XMM26,
				REG_XMM27,
				REG_XMM28,
				REG_XMM29,
				REG_XMM30,
				REG_XMM31,

				REG_MAX,
			};

			struct MemoryLocation {
				RegisterId base;
				int32_t disp;
				RegisterId scaleIndex;
				uint8_t scale;
			};

			SLAKE_FORCEINLINE bool memoryToModRmAndSib(const MemoryLocation &mem, uint8_t &modRm, uint8_t &sib, uint8_t &rexPrefix) {
				if (!mem.disp) {
					modRm &= 0b00111000;
				} else if (mem.disp <= UINT8_MAX) {
					modRm &= 0b01111000;
				} else {
					modRm &= 0b10111000;
				}

				if (mem.scale == REG_MAX) {
					switch (mem.base) {
						case REG_RAX:
							modRm |= 0b00000000;
							break;
						case REG_RCX:
							modRm |= 0b00000001;
							break;
						case REG_RDX:
							modRm |= 0b00000010;
							break;
						case REG_RBX:
							modRm |= 0b00000011;
							break;
						case REG_RBP:
							if (!mem.disp) {
								goto fallback;
							}
							modRm |= 0b00000101;
							break;
						case REG_RSI:
							modRm |= 0b00000110;
							break;
						case REG_RDI:
							modRm |= 0b00000111;
							break;
						default:
							goto fallback;
					}

					return false;
				}

			fallback:
				modRm |= 0b00000100;

				sib = 0;

				switch (mem.scaleIndex) {
					case 1:
						sib |= 0b00000000;
						break;
					case 2:
						sib |= 0b01000000;
						break;
					case 4:
						sib |= 0b10000000;
						break;
					case 8:
						sib |= 0b11000000;
						break;
				}

				switch (mem.base) {
					case REG_RAX:
						sib |= 0b00000000;
						break;
					case REG_RCX:
						sib |= 0b00000001;
						break;
					case REG_RDX:
						sib |= 0b00000010;
						break;
					case REG_RBX:
						sib |= 0b00000011;
						break;
					case REG_RSP:
						sib |= 0b00000100;
						break;
					case REG_RBP:
						sib |= 0b00000101;
						break;
					case REG_RSI:
						sib |= 0b00000110;
						break;
					case REG_RDI:
						sib |= 0b00000111;
						break;
					case REG_R8:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000000;
						break;
					case REG_R9:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000001;
						break;
					case REG_R10:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000010;
						break;
					case REG_R11:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000011;
						break;
					case REG_R12:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000100;
						break;
					case REG_R13:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000101;
						break;
					case REG_R14:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000110;
						break;
					case REG_R15:
						rexPrefix |= REX_PREFIX(0, 0, 0, 1);
						sib |= 0b00000111;
						break;
					default:
						assert(("Invalid base register", false));
				}

				switch (mem.scaleIndex) {
					case REG_RAX:
						sib |= 0b00000000;
						break;
					case REG_RCX:
						sib |= 0b00001000;
						break;
					case REG_RDX:
						sib |= 0b00010000;
						break;
					case REG_RBX:
						sib |= 0b00011000;
						break;
					case REG_RBP:
						sib |= 0b00101000;
						break;
					case REG_RSI:
						sib |= 0b00110000;
						break;
					case REG_RDI:
						sib |= 0b00111000;
						break;
					case REG_MAX:
						sib |= 0b00100000;
						break;
					case REG_R8:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00000000;
						break;
					case REG_R9:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00001000;
						break;
					case REG_R10:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00010000;
						break;
					case REG_R11:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00011000;
						break;
					case REG_R12:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00100000;
						break;
					case REG_R13:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00101000;
						break;
					case REG_R14:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00110000;
						break;
					case REG_R15:
						rexPrefix |= REX_PREFIX(0, 0, 1, 0);
						sib |= 0b00111000;
						break;
					default:
						assert(("Invalid scale index register", false));
				}

				return true;
			}
		}
	}
}

#endif
