#include "bitshift.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg8_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg8_with_minor_opcode(0xd0, 4, register_id);
	} else {
		return emit_ins_with_reg8_with_minor_opcode(0xc0, 4, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg16_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg16_with_minor_opcode(0xd1, 4, register_id);
	} else {
		return emit_ins_with_reg16_with_minor_opcode(0xc1, 4, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg32_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg32_with_minor_opcode(0xd1, 4, register_id);
	} else {
		return emit_ins_with_reg32_with_minor_opcode(0xc1, 4, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg64_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg64_with_minor_opcode(0xd1, 4, register_id);
	} else {
		return emit_ins_with_reg64_with_minor_opcode(0xc1, 4, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg8_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xd2, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg16_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xd3, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg32_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xd3, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shl_reg64_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xd3, 4, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg8_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg8_with_minor_opcode(0xd0, 5, register_id);
	} else {
		return emit_ins_with_reg8_with_minor_opcode(0xc0, 5, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg16_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg16_with_minor_opcode(0xd1, 5, register_id);
	} else {
		return emit_ins_with_reg16_with_minor_opcode(0xc1, 5, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg32_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg32_with_minor_opcode(0xd1, 5, register_id);
	} else {
		return emit_ins_with_reg32_with_minor_opcode(0xc1, 5, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg64_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg64_with_minor_opcode(0xd1, 5, register_id);
	} else {
		return emit_ins_with_reg64_with_minor_opcode(0xc1, 5, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg8_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xd2, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg16_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xd3, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg32_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xd3, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_shr_reg64_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xd3, 5, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg8_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg8_with_minor_opcode(0xd0, 7, register_id);
	} else {
		return emit_ins_with_reg8_with_minor_opcode(0xc0, 7, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg16_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg16_with_minor_opcode(0xd1, 7, register_id);
	} else {
		return emit_ins_with_reg16_with_minor_opcode(0xc1, 7, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg32_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg32_with_minor_opcode(0xd1, 7, register_id);
	} else {
		return emit_ins_with_reg32_with_minor_opcode(0xc1, 7, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg64_with_imm8_ins(RegisterId register_id, uint8_t times) {
	if (times == 1) {
		return emit_ins_with_reg64_with_minor_opcode(0xd1, 7, register_id);
	} else {
		return emit_ins_with_reg64_with_minor_opcode(0xc1, 7, register_id);
	}
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg8_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg8_with_minor_opcode(0xd2, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg16_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg16_with_minor_opcode(0xd3, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg32_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg32_with_minor_opcode(0xd3, 7, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_sar_reg64_with_cl_ins(RegisterId register_id) {
	return emit_ins_with_reg64_with_minor_opcode(0xd3, 7, register_id);
}
