#include <cstdint>
#include <cstring>

#include "compiler.h"

using namespace slake;
using namespace slake::jit;
using namespace slake::jit::x86_64;

#define DEF_INS_BUFFER(name, ...) uint8_t name[] = { __VA_ARGS__ }

DEF_INS_BUFFER(_pushRaxRegIns, 0x50);
DEF_INS_BUFFER(_pushRbxRegIns, 0x53);
DEF_INS_BUFFER(_pushRcxRegIns, 0x51);
DEF_INS_BUFFER(_pushRdxRegIns, 0x52);
DEF_INS_BUFFER(_pushRsiRegIns, 0x56);
DEF_INS_BUFFER(_pushRdiRegIns, 0x57);
DEF_INS_BUFFER(_pushRbpRegIns, 0x55);
DEF_INS_BUFFER(_pushRspRegIns, 0x54);
DEF_INS_BUFFER(_pushR8RegIns, 0x41, 0x50);
DEF_INS_BUFFER(_pushR9RegIns, 0x41, 0x51);
DEF_INS_BUFFER(_pushR10RegIns, 0x41, 0x52);
DEF_INS_BUFFER(_pushR11RegIns, 0x41, 0x53);
DEF_INS_BUFFER(_pushR12RegIns, 0x41, 0x54);
DEF_INS_BUFFER(_pushR13RegIns, 0x41, 0x55);
DEF_INS_BUFFER(_pushR14RegIns, 0x41, 0x56);
DEF_INS_BUFFER(_pushR15RegIns, 0x41, 0x57);

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPushRegIns(JITCompilerRegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
			return emitIns(sizeof(_pushRaxRegIns), _pushRaxRegIns);
		case REG_RBX:
			return emitIns(sizeof(_pushRbxRegIns), _pushRbxRegIns);
		case REG_RCX:
			return emitIns(sizeof(_pushRcxRegIns), _pushRcxRegIns);
		case REG_RDX:
			return emitIns(sizeof(_pushRdxRegIns), _pushRdxRegIns);
		case REG_RSI:
			return emitIns(sizeof(_pushRsiRegIns), _pushRsiRegIns);
		case REG_RDI:
			return emitIns(sizeof(_pushRdiRegIns), _pushRdiRegIns);
		case REG_RBP:
			return emitIns(sizeof(_pushRbpRegIns), _pushRbpRegIns);
		case REG_RSP:
			return emitIns(sizeof(_pushRspRegIns), _pushRspRegIns);
		case REG_R8:
			return emitIns(sizeof(_pushR8RegIns), _pushR8RegIns);
		case REG_R9:
			return emitIns(sizeof(_pushR9RegIns), _pushR9RegIns);
		case REG_R10:
			return emitIns(sizeof(_pushR10RegIns), _pushR10RegIns);
		case REG_R11:
			return emitIns(sizeof(_pushR11RegIns), _pushR11RegIns);
		case REG_R12:
			return emitIns(sizeof(_pushR12RegIns), _pushR12RegIns);
		case REG_R13:
			return emitIns(sizeof(_pushR13RegIns), _pushR13RegIns);
		case REG_R14:
			return emitIns(sizeof(_pushR14RegIns), _pushR14RegIns);
		case REG_R15:
			return emitIns(sizeof(_pushR15RegIns), _pushR15RegIns);
		default:;
	}

	throw std::logic_error("Invalid register ID");
}

DEF_INS_BUFFER(_popRaxRegIns, 0x58);
DEF_INS_BUFFER(_popRbxRegIns, 0x5b);
DEF_INS_BUFFER(_popRcxRegIns, 0x59);
DEF_INS_BUFFER(_popRdxRegIns, 0x5a);
DEF_INS_BUFFER(_popRsiRegIns, 0x5e);
DEF_INS_BUFFER(_popRdiRegIns, 0x5f);
DEF_INS_BUFFER(_popRbpRegIns, 0x5d);
DEF_INS_BUFFER(_popRspRegIns, 0x5c);
DEF_INS_BUFFER(_popR8RegIns, 0x41, 0x58);
DEF_INS_BUFFER(_popR9RegIns, 0x41, 0x59);
DEF_INS_BUFFER(_popR10RegIns, 0x41, 0x5a);
DEF_INS_BUFFER(_popR11RegIns, 0x41, 0x5b);
DEF_INS_BUFFER(_popR12RegIns, 0x41, 0x5c);
DEF_INS_BUFFER(_popR13RegIns, 0x41, 0x5d);
DEF_INS_BUFFER(_popR14RegIns, 0x41, 0x5e);
DEF_INS_BUFFER(_popR15RegIns, 0x41, 0x5f);

SLAKE_API DiscreteInstruction slake::jit::x86_64::emitPopRegIns(JITCompilerRegisterId registerId) {
	switch (registerId) {
		case REG_RAX:
			return emitIns(sizeof(_pushRaxRegIns), _pushRaxRegIns);
		case REG_RBX:
			return emitIns(sizeof(_pushRbxRegIns), _pushRbxRegIns);
		case REG_RCX:
			return emitIns(sizeof(_pushRcxRegIns), _pushRcxRegIns);
		case REG_RDX:
			return emitIns(sizeof(_pushRdxRegIns), _pushRdxRegIns);
		case REG_RSI:
			return emitIns(sizeof(_pushRsiRegIns), _pushRsiRegIns);
		case REG_RDI:
			return emitIns(sizeof(_pushRdiRegIns), _pushRdiRegIns);
		case REG_RBP:
			return emitIns(sizeof(_pushRbpRegIns), _pushRbpRegIns);
		case REG_RSP:
			return emitIns(sizeof(_pushRspRegIns), _pushRspRegIns);
		case REG_R8:
			return emitIns(sizeof(_pushR8RegIns), _pushR8RegIns);
		case REG_R9:
			return emitIns(sizeof(_pushR9RegIns), _pushR9RegIns);
		case REG_R10:
			return emitIns(sizeof(_pushR10RegIns), _pushR10RegIns);
		case REG_R11:
			return emitIns(sizeof(_pushR11RegIns), _pushR11RegIns);
		case REG_R12:
			return emitIns(sizeof(_pushR12RegIns), _pushR12RegIns);
		case REG_R13:
			return emitIns(sizeof(_pushR13RegIns), _pushR13RegIns);
		case REG_R14:
			return emitIns(sizeof(_pushR14RegIns), _pushR14RegIns);
		case REG_R15:
			return emitIns(sizeof(_pushR15RegIns), _pushR15RegIns);
		default:;
	}

	throw std::logic_error("Invalid register ID");
}

slake::CodePage *slake::compileRegularFn(RegularFnOverloadingObject *fn) {
	slake::CodePage *codePage;
	size_t size;

	JITCompileContext compileContext;
	size_t nIns = fn->instructions.size();

	// Analyze lifetime of virtual registers.
	for (size_t i = 0; i < nIns; ++i) {
		const Instruction &curIns = fn->instructions[i];

		uint32_t regIndex = UINT32_MAX;

		if (curIns.output.valueType == ValueType::RegRef) {
			regIndex = curIns.output.getRegIndex();

			if (compileContext.regInfo.count(regIndex)) {
				// Malformed program, return.
				return nullptr;
			}

			compileContext.regInfo[regIndex].lifeTime = { i, i };
		}

		for (auto &j : curIns.operands) {
			if (j.valueType == ValueType::RegRef) {
				uint32_t index = curIns.output.getRegIndex();

				if (!compileContext.regInfo.count(index)) {
					// Malformed program, return.
					return nullptr;
				}

				compileContext.regInfo.at(index).lifeTime.offEndIns = i;
			}
		}

		switch (curIns.opcode) {
			case Opcode::NOP:
				if (regIndex != UINT32_MAX) {
					return nullptr;
				}
		}
	}

	for (size_t i = 0; i < nIns; ++i) {
		const Instruction &curIns = fn->instructions[i];

		switch (curIns.opcode) {
			case Opcode::NOP:
				break;
			case Opcode::LOAD: {
				// Save parameter registers.
				compileContext.pushIns(emitPopRegIns(REG_RCX));
				compileContext.addStackPtr(sizeof(uint64_t));
				compileContext.pushIns(emitPopRegIns(REG_RDX));
				compileContext.addStackPtr(sizeof(uint64_t));
				compileContext.pushIns(emitPopRegIns(REG_R8));
				compileContext.addStackPtr(sizeof(uint64_t));
				compileContext.pushIns(emitPopRegIns(REG_R9));
				compileContext.addStackPtr(sizeof(uint64_t));

				// Save scratch registers.
				if (compileContext.regUseLevels[REG_RAX] > 0) {
					compileContext.pushIns(emitPushRegIns(REG_RAX));
					compileContext.addStackPtr(sizeof(uint64_t));
				}
				if (compileContext.regUseLevels[REG_R10] > 0) {
					compileContext.pushIns(emitPushRegIns(REG_R10));
					compileContext.addStackPtr(sizeof(uint64_t));
				}
				if (compileContext.regUseLevels[REG_R11] > 0) {
					compileContext.pushIns(emitPushRegIns(REG_R11));
					compileContext.addStackPtr(sizeof(uint64_t));
				}

				// Restore scratch registers.
				if (compileContext.regUseLevels[REG_R11] > 0) {
					compileContext.pushIns(emitPopRegIns(REG_R11));
					compileContext.subStackPtr(sizeof(uint64_t));
				}
				if (compileContext.regUseLevels[REG_R10] > 0) {
					compileContext.pushIns(emitPopRegIns(REG_R10));
					compileContext.subStackPtr(sizeof(uint64_t));
				}
				if (compileContext.regUseLevels[REG_RAX] > 0) {
					compileContext.pushIns(emitPopRegIns(REG_RAX));
					compileContext.subStackPtr(sizeof(uint64_t));
				}

				// Restore parameter registers.
				compileContext.pushIns(emitPopRegIns(REG_R9));
				compileContext.subStackPtr(sizeof(uint64_t));
				compileContext.pushIns(emitPopRegIns(REG_R8));
				compileContext.subStackPtr(sizeof(uint64_t));
				compileContext.pushIns(emitPopRegIns(REG_RDX));
				compileContext.subStackPtr(sizeof(uint64_t));
				compileContext.pushIns(emitPopRegIns(REG_RCX));
				compileContext.subStackPtr(sizeof(uint64_t));
				break;
			}
		}
	}

	return codePage;
}
