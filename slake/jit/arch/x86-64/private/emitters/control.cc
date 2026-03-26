#include "control.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

static DEF_INS_BUFFER(_near_ret_ins, 0xc3);

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_near_ret_ins() {
	return emit_raw_ins(sizeof(_near_ret_ins), _near_ret_ins);
}

static DEF_INS_BUFFER(_far_ret_ins, 0xc3);

SLAKE_API DiscreteInstruction slake::jit::x86_64::emit_far_ret_ins() {
	return emit_raw_ins(sizeof(_far_ret_ins), _far_ret_ins);
}
