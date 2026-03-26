#include "stackop.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_push_reg16_ins(RegisterId register_id) {
	return emit_ins_with_reg16_increment(0x50, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_push_reg64_ins(RegisterId register_id) {
	return emit_ins_with_reg64_increment(0x50, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_pop_reg16_ins(RegisterId register_id) {
	return emit_ins_with_reg16_increment(0x58, register_id);
}

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_pop_reg64_ins(RegisterId register_id) {
	return emit_ins_with_reg64_increment(0x58, register_id);
}
