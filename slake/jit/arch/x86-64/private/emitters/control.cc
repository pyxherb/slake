#include "control.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

static DEF_INS_BUFFER(_nearRetIns, 0xc3);

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitNearRetIns() {
	return emitRawIns(sizeof(_nearRetIns), _nearRetIns);
}

static DEF_INS_BUFFER(_farRetIns, 0xc3);

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitFarRetIns() {
	return emitRawIns(sizeof(_farRetIns), _farRetIns);
}
