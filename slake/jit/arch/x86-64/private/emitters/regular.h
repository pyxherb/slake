#ifndef _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_REGULAR_H_
#define _SLAKE_JIT_ARCH_X86_64_PRIVATE_EMITTERS_REGULAR_H_

#include "../emitter_base.h"

namespace slake {
	namespace jit {
		namespace x86_64 {
			DiscreteInstruction emitInsWithImm8AndAlReg(uint8_t opcode, uint8_t imm0[1]);
			DiscreteInstruction emitInsWithImm8AndReg8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[1]);
			DiscreteInstruction emitInsWithImm16AndAxReg(uint8_t opcode, uint8_t imm0[2]);
			DiscreteInstruction emitInsWithImm16AndReg16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[2]);
			DiscreteInstruction emitInsWithImm32AndEaxReg(uint8_t opcode, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithImm32AndReg32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithImm32AndRaxReg(uint8_t opcode, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithImm32AndReg64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithReg8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithReg16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithReg32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithReg64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithMem8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithMem16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithMem32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithMem64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithReg8AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId);
			DiscreteInstruction emitInsWithReg16AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId);
			DiscreteInstruction emitInsWithReg32AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId);
			DiscreteInstruction emitInsWithReg64AndModRMReg(uint8_t majorOpcode, RegisterId registerId, RegisterId srcRegisterId);
			DiscreteInstruction emitInsWithReg8AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithReg16AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithReg32AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithReg64AndMem(uint8_t majorOpcode, RegisterId srcRegisterId, const MemoryLocation &mem);
			DiscreteInstruction emitInsWithImm8AndMem8WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[1]);
			DiscreteInstruction emitInsWithImm16AndMem16WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[2]);
			DiscreteInstruction emitInsWithImm32AndMem32WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithImm32AndMem64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithImm64AndMem64WithMinorOpcode(uint8_t majorOpcode, uint8_t minorOpcode, const MemoryLocation &mem, uint8_t imm0[8]);
			DiscreteInstruction emitInsWithReg8Increment(uint8_t majorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithReg16Increment(uint8_t majorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithReg32Increment(uint8_t majorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithReg64Increment(uint8_t majorOpcode, RegisterId registerId);
			DiscreteInstruction emitInsWithImm8AndReg8Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[1]);
			DiscreteInstruction emitInsWithImm16AndReg16Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[2]);
			DiscreteInstruction emitInsWithImm32AndReg32Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[4]);
			DiscreteInstruction emitInsWithImm64AndReg64Increment(uint8_t majorOpcode, RegisterId registerId, uint8_t imm0[8]);
		}
	}
}

#endif
