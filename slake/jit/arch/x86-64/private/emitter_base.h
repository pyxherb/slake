#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTER_BASE_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTER_BASE_H_

#include <slake/jit/arch/x86-64/basedefs.h>

namespace slake {
	namespace jit {
		namespace x86_64 {
			SLAKE_FORCEINLINE DiscreteInstruction emit_raw_ins(size_t sz_ins, const uint8_t *buffer) {
				DiscreteInstruction ins;

				ins.as_raw_ins_data.sz_ins = sz_ins;
				memcpy(ins.as_raw_ins_data.buffer, buffer, sz_ins);
				ins.ins_type = DiscreteInstructionType::Raw;

				return ins;
			}

			SLAKE_FORCEINLINE DiscreteInstruction emit_jump_ins(void *dest, DiscreteInstructionType type = DiscreteInstructionType::Jump) {
				DiscreteInstruction ins;
				ins.ins_type = type;
				ins.as_jump_ins_data.dest = dest;
				return ins;
			}

			SLAKE_FORCEINLINE DiscreteInstruction emit_labelled_jump_ins(const char *dest, DiscreteInstructionType type = DiscreteInstructionType::JumpLabelled) {
				DiscreteInstruction ins;
				ins.ins_type = type;
				ins.as_labelled_jump_ins_data.dest = dest;
				return ins;
			}

			SLAKE_FORCEINLINE DiscreteInstruction emit_call_ins(void *dest) {
				DiscreteInstruction ins;
				ins.ins_type = DiscreteInstructionType::Call;
				ins.as_jump_ins_data.dest = dest;
				return ins;
			}
		}
	}
}

#endif
