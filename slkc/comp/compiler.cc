#include "compiler.h"

using namespace slkc;

SLKC_API CompileContext::~CompileContext() {
}

SLKC_API void CompileContext::onRefZero() noexcept {
	peff::destroyAndRelease<CompileContext>(selfAllocator.get(), this, sizeof(std::max_align_t));
}

SLAKE_API std::optional<CompilationError> CompileContext::emitIns(slake::Opcode opcode, uint32_t outputRegIndex, const std::initializer_list<slake::Value> &operands) {
	if (flags & COMPCTXT_NOCOMPILE) {
		return {};
	}

	slake::Instruction insOut;

	insOut.opcode = opcode;
	insOut.output = outputRegIndex;
	if (!(insOut.operands = (slake::Value *)allocator->alloc(sizeof(slake::Value) * operands.size(), sizeof(std::max_align_t)))) {
		return genOutOfMemoryCompError();
	}
	insOut.nOperands = operands.size();
	insOut.operandsAllocator = allocator;

	auto it = operands.begin();
	for (size_t i = 0; i < operands.size(); ++i) {
		insOut.operands[i] = *it++;
	}

	if (!fnCompileContext.instructionsOut.pushBack(std::move(insOut))) {
		return genOutOfMemoryCompError();
	}

	return {};
}
